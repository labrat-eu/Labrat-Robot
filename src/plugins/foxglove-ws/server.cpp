/**
 * @file server.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/exception.hpp>
#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/foxglove-ws/server.hpp>

#include <chrono>
#include <queue>

#include <libbase64.h>
#include <google/protobuf/descriptor.pb.h>

#include <foxglove/websocket/server.hpp>

namespace labrat::robot::plugins {

class FoxgloveServerPrivate {
public:
  FoxgloveServerPrivate(const std::string &name, u16 port, const Plugin::Filter &filter) : server(port, name) {
    server.setSubscribeHandler([](foxglove::websocket::ChannelId channel_id) {
      Logger("foxglove-ws")() << "First client subscribed to " << channel_id;
    });

    server.setUnsubscribeHandler([](foxglove::websocket::ChannelId channel_id) {
      Logger("foxglove-ws")() << "Last client unsubscribed from " << channel_id;
    });

    Plugin plugin_info;
    plugin_info.user_ptr = reinterpret_cast<void *>(this);
    plugin_info.topic_callback = FoxgloveServerPrivate::topicCallback;
    plugin_info.message_callback = FoxgloveServerPrivate::messageCallback;
    plugin_info.filter = filter;

    self_reference = Manager::get().addPlugin(plugin_info);

    run_thread = std::thread([this]() {
      server.run();
    });
  }

  ~FoxgloveServerPrivate() {
    Manager::get().removePlugin(self_reference);

    for (const std::pair<std::size_t, foxglove::websocket::ChannelId> channel : channel_map) {
      server.removeChannel(channel.second);
    }

    server.stop();
    run_thread.join();
  }

  struct SchemaInfo {
    std::string name;
    std::string definition;

    SchemaInfo(const std::string &name, const std::string &definition) : name(name), definition(definition) {}
  };

private:
  using SchemaMap = std::unordered_map<std::size_t, SchemaInfo>;
  using ChannelMap = std::unordered_map<std::size_t, foxglove::websocket::ChannelId>;

  static void topicCallback(void *user_ptr, const Plugin::TopicInfo &info);
  static void messageCallback(void *user_ptr, const Plugin::MessageInfo &info);

  ChannelMap::iterator handleTopic(const Plugin::TopicInfo &info);
  inline ChannelMap::iterator handleMessage(const Plugin::MessageInfo &info);

  SchemaMap schema_map;
  ChannelMap channel_map;

  foxglove::websocket::Server server;
  std::mutex mutex;

  std::thread run_thread;

  Plugin::List::iterator self_reference;
};

static google::protobuf::FileDescriptorSet buildFileDescriptorSet(const google::protobuf::Descriptor *top_descriptor);

FoxgloveServer::FoxgloveServer(const std::string &name, u16 port, const Plugin::Filter &filter) {
  priv = new FoxgloveServerPrivate(name, port, filter);
}

FoxgloveServer::~FoxgloveServer() {
  delete priv;
}

void FoxgloveServerPrivate::topicCallback(void *user_ptr, const Plugin::TopicInfo &info) {
  FoxgloveServerPrivate *self = reinterpret_cast<FoxgloveServerPrivate *>(user_ptr);

  (void)self->handleTopic(info);
}

void FoxgloveServerPrivate::messageCallback(void *user_ptr, const Plugin::MessageInfo &info) {
  FoxgloveServerPrivate *self = reinterpret_cast<FoxgloveServerPrivate *>(user_ptr);

  (void)self->handleMessage(info);
}

FoxgloveServerPrivate::ChannelMap::iterator FoxgloveServerPrivate::handleTopic(const Plugin::TopicInfo &info) {
  SchemaMap::iterator schema_iterator = schema_map.find(info.type_hash);
  if (schema_iterator == schema_map.end()) {
    const std::string description = buildFileDescriptorSet(info.type_descriptor).SerializeAsString();
    std::string encoded_description(description.size() * 4 / 3 + 1);
    base64_encode(description.c_str(), description.size(), encoded_description.data(), encoded_description.size(), 0);

    schema_iterator = schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(info.type_hash),
      std::forward_as_tuple(info.type_descriptor->full_name(), encoded_description));
  }

  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    foxglove::websocket::ChannelId channel_id = server.addChannel({
      .topic = info.topic_name,
      .encoding = "protobuf",
      .schemaName = schema_iterator->second.name,
      .schema = schema_iterator->second.definition,
    });

    channel_iterator = channel_map.emplace_hint(channel_iterator, std::make_pair(info.topic_hash, channel_id));
  }

  return channel_iterator;
}

inline FoxgloveServerPrivate::ChannelMap::iterator FoxgloveServerPrivate::handleMessage(const Plugin::MessageInfo &info) {
  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    channel_iterator = handleTopic(info.topic_info);
  }

  std::lock_guard guard(mutex);
  server.sendMessage(channel_iterator->second, info.timestamp.count(), info.serialized_message);

  return channel_iterator;
}

static google::protobuf::FileDescriptorSet buildFileDescriptorSet(const google::protobuf::Descriptor *top_descriptor) {
  google::protobuf::FileDescriptorSet result;
  std::queue<const google::protobuf::FileDescriptor *> queue;

  queue.push(top_descriptor->file());

  std::unordered_set<std::string> dependencies;

  while (!queue.empty()) {
    const google::protobuf::FileDescriptor *next = queue.front();
    queue.pop();
    next->CopyTo(result.add_file());

    for (int i = 0; i < next->dependency_count(); ++i) {
      const auto &dep = next->dependency(i);

      if (dependencies.find(dep->name()) == dependencies.end()) {
        dependencies.insert(dep->name());
        queue.push(dep);
      }
    }
  }

  return result;
}

}  // namespace labrat::robot::plugins
