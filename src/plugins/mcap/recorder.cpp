/**
 * @file recorder.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/config.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>

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
  McapRecorderPrivate(const Plugin::Filter &filter) : logger("mcap") {
    Config::Ptr config = Config::get();
    const std::string filename = config->getParameterFallback("/lbot/plugins/mcap/tracefile", "trace_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())).get<std::string>();

    const mcap::McapWriterOptions options("");
    const mcap::Status result = writer.open(filename, options);

    if (!result.ok()) {
      throw IoException("Failed to open '" + filename + "'.", logger);
    }

    Plugin plugin_info;
    plugin_info.user_ptr = reinterpret_cast<void *>(this);
    plugin_info.topic_callback = McapRecorderPrivate::topicCallback;
    plugin_info.message_callback = McapRecorderPrivate::messageCallback;
    plugin_info.filter = filter;

    self_reference = Manager::get()->addPlugin(plugin_info);
  }

  ~McapRecorderPrivate() {
    Manager::get()->removePlugin(self_reference);
    writer.close();
  }

private:
  using SchemaMap = std::unordered_map<std::size_t, mcap::Schema>;

  struct ChannelInfo {
    mcap::Channel channel;
    u64 index = 0;

    ChannelInfo(const std::string_view topic, const std::string_view encoding, mcap::SchemaId schema) :
      channel(topic, encoding, schema) {}
  };

  using ChannelMap = std::unordered_map<std::size_t, ChannelInfo>;

  static void topicCallback(void *user_ptr, const Plugin::TopicInfo &info);
  static void messageCallback(void *user_ptr, const Plugin::MessageInfo &info);

  ChannelMap::iterator handleTopic(const Plugin::TopicInfo &info);
  inline ChannelMap::iterator handleMessage(const Plugin::MessageInfo &info);

  SchemaMap schema_map;
  ChannelMap channel_map;

  mcap::McapWriter writer;
  std::mutex mutex;

  Plugin::List::iterator self_reference;

  Logger logger;
};

McapRecorder::McapRecorder(const Plugin::Filter &filter) {
  priv = new McapRecorderPrivate(filter);
}

McapRecorder::~McapRecorder() {
  delete priv;
}

void McapRecorderPrivate::topicCallback(void *user_ptr, const Plugin::TopicInfo &info) {
  McapRecorderPrivate *self = reinterpret_cast<McapRecorderPrivate *>(user_ptr);

  (void)self->handleTopic(info);
}

void McapRecorderPrivate::messageCallback(void *user_ptr, const Plugin::MessageInfo &info) {
  McapRecorderPrivate *self = reinterpret_cast<McapRecorderPrivate *>(user_ptr);

  (void)self->handleMessage(info);
}

McapRecorderPrivate::ChannelMap::iterator McapRecorderPrivate::handleTopic(const Plugin::TopicInfo &info) {
  SchemaMap::iterator schema_iterator = schema_map.find(info.type_hash);
  if (schema_iterator == schema_map.end()) {
    MessageReflection reflection(info.type_name);

    if (!reflection.isValid()) {
      throw SchemaUnknownException("Unknown message schema '" + info.type_name + "'.", logger);
    }

    schema_iterator = schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(info.type_hash),
      std::forward_as_tuple(info.type_name, "flatbuffer", reflection.getBuffer()));

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

inline McapRecorderPrivate::ChannelMap::iterator McapRecorderPrivate::handleMessage(const Plugin::MessageInfo &info) {
  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    channel_iterator = handleTopic(info.topic_info);
  }

  mcap::Message message;
  message.channelId = channel_iterator->second.channel.id;
  message.sequence = channel_iterator->second.index++;
  message.publishTime = info.timestamp.count();
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
