#pragma once

#include <labrat/robot/plugin.hpp>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <foxglove/websocket/server.hpp>

namespace labrat::robot::plugins {

class FoxgloveServer {
public:
  FoxgloveServer(const std::string &name, u16 port = 8765);
  ~FoxgloveServer();

private:
  struct SchemaInfo {
    std::string name;
    std::string definition;

    SchemaInfo(const std::string &name, const std::string &definition) : name(name), definition(definition) {}
  };

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

}  // namespace labrat::robot::plugins
