/**
 * @file recorder.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/config.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <mcap/internal.hpp>
#include <mcap/types.inl>
#include <mcap/writer.hpp>
#include <mcap/writer.inl>

inline namespace labrat {
namespace lbot::plugins {

class McapRecorderPrivate {
public:
  McapRecorderPrivate() : logger("mcap") {
    Config::Ptr config = Config::get();
    const std::string filename =
      config
        ->getParameterFallback("/lbot/plugins/mcap/tracefile",
          "trace_"
            + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
            + ".mcap")
        .get<std::string>();

    const mcap::McapWriterOptions options("");
    const mcap::Status result = writer.open(filename, options);

    if (!result.ok()) {
      throw IoException("Failed to open '" + filename + "'.", logger);
    }

    enable_callbacks.test_and_set();
  }

  ~McapRecorderPrivate() {
    writer.close();
  }

  struct ChannelInfo {
    mcap::Channel channel;
    u64 index = 0;

    ChannelInfo(const std::string_view topic, const std::string_view encoding, mcap::SchemaId schema) : channel(topic, encoding, schema) {}
  };

  using ChannelMap = std::unordered_map<std::size_t, ChannelInfo>;
  using SchemaMap = std::unordered_map<std::size_t, mcap::Schema>;

  ChannelMap::iterator handleTopic(const TopicInfo &info);
  inline ChannelMap::iterator handleMessage(const MessageInfo &info);

  std::atomic_flag enable_callbacks;

private:
  SchemaMap schema_map;
  ChannelMap channel_map;

  mcap::McapWriter writer;
  std::mutex mutex;

  Logger logger;
};

McapRecorder::McapRecorder() : UniquePlugin("mcap") {
  priv = new McapRecorderPrivate();
}

McapRecorder::~McapRecorder() {
  delete priv;
}

void McapRecorder::topicCallback(const TopicInfo &info) {
  if (priv->enable_callbacks.test()) {
    (void)priv->handleTopic(info);
  }
}

void McapRecorder::messageCallback(const MessageInfo &info) {
  if (priv->enable_callbacks.test()) {
    (void)priv->handleMessage(info);
  }
}

McapRecorderPrivate::ChannelMap::iterator McapRecorderPrivate::handleTopic(const TopicInfo &info) {
  SchemaMap::iterator schema_iterator = schema_map.find(info.type_hash);
  if (schema_iterator == schema_map.end()) {
    schema_iterator = schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(info.type_hash),
      std::forward_as_tuple(info.type_name, "flatbuffer", info.type_reflection));

    std::lock_guard guard(mutex);
    writer.addSchema(schema_iterator->second);
  }

  const std::pair<ChannelMap::iterator, bool> result = channel_map.emplace(std::piecewise_construct, std::forward_as_tuple(info.topic_hash),
    std::forward_as_tuple(info.topic_name, "flatbuffer", schema_iterator->second.id));

  if (result.second) {
    std::lock_guard guard(mutex);
    writer.addChannel(result.first->second.channel);
  }

  return result.first;
}

inline McapRecorderPrivate::ChannelMap::iterator McapRecorderPrivate::handleMessage(const MessageInfo &info) {
  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    channel_iterator = handleTopic(info.topic_info);
  }

  mcap::Message message;
  message.channelId = channel_iterator->second.channel.id;
  message.sequence = channel_iterator->second.index++;
  message.publishTime = std::chrono::duration_cast<std::chrono::nanoseconds>(info.timestamp.time_since_epoch()).count();
  message.logTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  message.data = reinterpret_cast<const std::byte *>(info.serialized_message.data());
  message.dataSize = info.serialized_message.size();

  std::lock_guard guard(mutex);
  const mcap::Status result = writer.write(message);
  if (!result.ok()) {
    writer.terminate();
    writer.close();

    throw IoException("Failed to write message.");
  }

  return channel_iterator;
}

}  // namespace lbot::plugins
}  // namespace labrat
