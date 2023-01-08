/**
 * @file server.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/robot/plugin.hpp>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <foxglove/websocket/server.hpp>

namespace labrat::robot::plugins {

/**
 * @brief Class to register a plugin to the manager that will open a websocket server in order to forward massages to Foxglove Studio.
 *
 */
class FoxgloveServer {
public:
  /**
   * @brief Construct a new Foxglove Server object.
   *
   * @param name Name of the server.
   * @param port Local port on which the server should listen for new connections.
   */
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
