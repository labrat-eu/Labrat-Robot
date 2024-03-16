/**
 * @file server->cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>

#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/server_interface.hpp>

inline namespace labrat {
namespace lbot::plugins {

class FoxgloveServerPrivate {
public:
  FoxgloveServerPrivate(const std::string &name, u16 port, const Plugin::Filter &filter) : logger("foxglove-ws") {
    auto log_handler = [this](foxglove::WebSocketLogLevel level, const char *message) {
      switch (level) {
        case (foxglove::WebSocketLogLevel::Debug): {
          logger.logDebug() << message;
          break;
        }
        case (foxglove::WebSocketLogLevel::Info): {
          logger.logInfo() << message;
          break;
        }
        case (foxglove::WebSocketLogLevel::Warn): {
          logger.logWarning() << message;
          break;
        }
        case (foxglove::WebSocketLogLevel::Error): {
          logger.logError() << message;
          break;
        }
        case (foxglove::WebSocketLogLevel::Critical): {
          logger.logCritical() << message;
          break;
        }
      }
    };

    const foxglove::ServerOptions options;
    server = foxglove::ServerFactory::createServer<websocketpp::connection_hdl>(name, log_handler, options);

    foxglove::ServerHandlers<websocketpp::connection_hdl> handlers;
    handlers.subscribeHandler = [&](foxglove::ChannelId channel_id, websocketpp::connection_hdl) {
      logger.logInfo() << "First client subscribed to " << channel_id;
    };
    handlers.unsubscribeHandler = [&](foxglove::ChannelId channel_id, websocketpp::connection_hdl) {
      logger.logInfo() << "Last client unsubscribed from " << channel_id;
    };

    server->setHandlers(std::move(handlers));

    Plugin plugin_info;
    plugin_info.user_ptr = reinterpret_cast<void *>(this);
    plugin_info.topic_callback = FoxgloveServerPrivate::topicCallback;
    plugin_info.message_callback = FoxgloveServerPrivate::messageCallback;
    plugin_info.filter = filter;

    self_reference = Manager::get()->addPlugin(plugin_info);

    server->start("0.0.0.0", port);
  }

  ~FoxgloveServerPrivate() {
    Manager::get()->removePlugin(self_reference);

    for (const std::pair<std::size_t, foxglove::ChannelId> channel : channel_id_map) {
      server->removeChannels({channel.second});
    }

    server->stop();
  }

  struct SchemaInfo {
    std::string name;
    std::string definition;

    SchemaInfo(const std::string &name, const std::string &definition) : name(name), definition(definition) {}
  };

private:
  using SchemaMap = std::unordered_map<std::size_t, SchemaInfo>;
  using ChannelIdMap = std::unordered_map<std::size_t, foxglove::ChannelId>;

  static void topicCallback(void *user_ptr, const Plugin::TopicInfo &info);
  static void messageCallback(void *user_ptr, const Plugin::MessageInfo &info);

  SchemaMap::iterator handleSchema(const std::string type_name, const std::size_t type_hash);
  ChannelIdMap::iterator handleTopic(const Plugin::TopicInfo &info);
  inline ChannelIdMap::iterator handleMessage(const Plugin::MessageInfo &info);

  SchemaMap schema_map;
  ChannelIdMap channel_id_map;

  std::unique_ptr<foxglove::ServerInterface<websocketpp::connection_hdl>> server;
  std::mutex mutex;

  Plugin::List::iterator self_reference;

  Logger logger;
};

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

FoxgloveServerPrivate::SchemaMap::iterator FoxgloveServerPrivate::handleSchema(const std::string type_name, const std::size_t type_hash) {
  std::lock_guard guard(mutex);
  
  SchemaMap::iterator schema_iterator = schema_map.find(type_hash);
  if (schema_iterator == schema_map.end()) {
    MessageReflection reflection(type_name);

    if (!reflection.isValid()) {
      throw SchemaUnknownException("Unknown message schema '" + type_name + "'.", logger);
    }

    schema_iterator = schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(type_hash),
      std::forward_as_tuple(type_name, foxglove::base64Encode(reflection.getBuffer())));
  }

  return schema_iterator;
}

FoxgloveServerPrivate::ChannelIdMap::iterator FoxgloveServerPrivate::handleTopic(const Plugin::TopicInfo &info) {
  SchemaMap::iterator schema_iterator = handleSchema(info.type_name, info.type_hash);

  std::lock_guard guard(mutex);

  ChannelIdMap::iterator channel_id_iterator = channel_id_map.find(info.topic_hash);
  if (channel_id_iterator == channel_id_map.end()) {
    std::vector<foxglove::ChannelId> channel_ids = server->addChannels({{
      .topic = info.topic_name,
      .encoding = "flatbuffer",
      .schemaName = schema_iterator->second.name,
      .schema = schema_iterator->second.definition,
    }});

    if (channel_ids.empty()) {
      throw RuntimeException("Failed to add channel.", logger);
    }

    channel_id_iterator = channel_id_map.emplace_hint(channel_id_iterator, std::make_pair(info.topic_hash, channel_ids.front()));
  }

  return channel_id_iterator;
}

inline FoxgloveServerPrivate::ChannelIdMap::iterator FoxgloveServerPrivate::handleMessage(const Plugin::MessageInfo &info) {
  ChannelIdMap::iterator channel_id_iterator = channel_id_map.find(info.topic_info.topic_hash);
  if (channel_id_iterator == channel_id_map.end()) {
    channel_id_iterator = handleTopic(info.topic_info);
  }

  std::lock_guard guard(mutex);
  server->broadcastMessage(channel_id_iterator->second, info.timestamp.count(), info.serialized_message.data(),
    info.serialized_message.size());

  return channel_id_iterator;
}

}  // namespace lbot::plugins
}  // namespace labrat
