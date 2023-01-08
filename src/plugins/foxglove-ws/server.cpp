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

#include <base64/Base64.h>
#include <google/protobuf/descriptor.pb.h>

namespace labrat::robot::plugins {

static google::protobuf::FileDescriptorSet buildFileDescriptorSet(const google::protobuf::Descriptor *top_descriptor);

FoxgloveServer::FoxgloveServer(const std::string &name, u16 port) : server(port, name) {
  server.setSubscribeHandler([](foxglove::websocket::ChannelId channel_id) {
    Logger("foxglove-ws")() << "First client subscribed to " << channel_id;
  });

  server.setUnsubscribeHandler([](foxglove::websocket::ChannelId channel_id) {
    Logger("foxglove-ws")() << "Last client unsubscribed from " << channel_id;
  });

  Plugin plugin_info;
  plugin_info.user_ptr = reinterpret_cast<void *>(this);
  plugin_info.topic_callback = FoxgloveServer::topicCallback;
  plugin_info.message_callback = FoxgloveServer::messageCallback;

  self_reference = Manager::get().addPlugin(plugin_info);

  run_thread = std::thread([this]() {
    server.run();
  });
}

FoxgloveServer::~FoxgloveServer() {
  Manager::get().removePlugin(self_reference);

  for (const std::pair<std::size_t, foxglove::websocket::ChannelId> channel : channel_map) {
    server.removeChannel(channel.second);
  }

  server.stop();
  run_thread.join();
}

void FoxgloveServer::topicCallback(void *user_ptr, const Plugin::TopicInfo &info) {
  FoxgloveServer *self = reinterpret_cast<FoxgloveServer *>(user_ptr);

  (void)self->handleTopic(info);
}

void FoxgloveServer::messageCallback(void *user_ptr, const Plugin::MessageInfo &info) {
  FoxgloveServer *self = reinterpret_cast<FoxgloveServer *>(user_ptr);

  (void)self->handleMessage(info);
}

FoxgloveServer::ChannelMap::iterator FoxgloveServer::handleTopic(const Plugin::TopicInfo &info) {
  SchemaMap::iterator schema_iterator = schema_map.find(info.type_hash);
  if (schema_iterator == schema_map.end()) {
    schema_iterator = schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(info.type_hash),
      std::forward_as_tuple(info.type_descriptor->full_name(),
        macaron::Base64::Encode(buildFileDescriptorSet(info.type_descriptor).SerializeAsString())));
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

inline FoxgloveServer::ChannelMap::iterator FoxgloveServer::handleMessage(const Plugin::MessageInfo &info) {
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
