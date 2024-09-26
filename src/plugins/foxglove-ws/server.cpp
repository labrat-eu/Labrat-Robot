/**
 * @file server->cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>

#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/server_interface.hpp>

inline namespace labrat {
namespace lbot::plugins {

static foxglove::ParameterValue convertParameterValue(const ConfigValue &source)
{
  if (source.contains<bool>()) {
    return foxglove::ParameterValue(source.get<bool>());
  } else if (source.contains<i64>()) {
    return foxglove::ParameterValue(source.get<i64>());
  } else if (source.contains<double>()) {
    return foxglove::ParameterValue(source.get<double>());
  } else if (source.contains<std::string>()) {
    return foxglove::ParameterValue(source.get<std::string>());
  } else if (source.contains<ConfigValue::Sequence>()) {
    const ConfigValue::Sequence &source_sequence = source.get<ConfigValue::Sequence>();
    std::vector<foxglove::ParameterValue> destination_sequence;
    destination_sequence.reserve(source_sequence.size());

    for (const ConfigValue &value : source_sequence) {
      destination_sequence.emplace_back(convertParameterValue(value));
    }

    return foxglove::ParameterValue(destination_sequence);
  } else {
    return foxglove::ParameterValue();
  }
}

class FoxgloveServerPrivate
{
public:
  FoxgloveServerPrivate() :
    logger("foxglove-ws")
  {
    logger.disableTopic();

    Config::Ptr config = Config::get();
    const std::string name = config->getParameterFallback("/lbot/plugins/foxglove-ws/name", "lbot").get<std::string>();
    const u16 port = config->getParameterFallback("/lbot/plugins/foxglove-ws/port", 8765L).get<i64>();

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

    const foxglove::ServerOptions options = {
      .capabilities = {foxglove::CAPABILITY_PARAMETERS, foxglove::CAPABILITY_TIME},
    };

    server = foxglove::ServerFactory::createServer<websocketpp::connection_hdl>(name, log_handler, options);

    foxglove::ServerHandlers<websocketpp::connection_hdl> handlers;
    handlers.subscribeHandler = [&](foxglove::ChannelId channel_id, websocketpp::connection_hdl handle) -> void {
      logger.logInfo() << "Client subscribed to " << channel_id;

      handleSubscription(channel_id, handle);
    };
    handlers.unsubscribeHandler = [&](foxglove::ChannelId channel_id, websocketpp::connection_hdl) -> void {
      logger.logInfo() << "Client unsubscribed from " << channel_id;
    };
    handlers.parameterRequestHandler =
      [&](const std::vector<std::string> &names, const std::optional<std::string> &command, websocketpp::connection_hdl handle) -> void {
      logger.logInfo() << "Parameter request received";

      handleParameterRequest(names, command, handle);
    };

    server->setHandlers(std::move(handlers));

    enable_callbacks.test_and_set();

    server->start("0.0.0.0", port);

    exit_mutex.lock();

    time_thread = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
        const std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();

        {
          std::lock_guard guard(mutex);
          server->broadcastTime(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
        }

        (void)exit_mutex.try_lock_until(time + std::chrono::milliseconds(10));
      }
    });

    housekeeping_thread = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
        const std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();

        for (std::list<std::jthread>::iterator iter = cache_threads.begin(); iter != cache_threads.end();) {
          const std::list<std::jthread>::iterator current_iter = iter;
          ++iter;

          if (!current_iter->joinable()) {
            continue;
          }

          current_iter->join();

          std::lock_guard guard(mutex);
          cache_threads.erase(current_iter);
        }

        (void)exit_mutex.try_lock_until(time + std::chrono::seconds(1));
      }
    });
  }

  ~FoxgloveServerPrivate()
  {
    time_thread.request_stop();
    housekeeping_thread.request_stop();
    exit_mutex.unlock();

    for (std::pair<std::size_t, const ChannelInfo &> channel : channel_map) {
      server->removeChannels({channel.second.id});
    }

    time_thread.join();
    housekeeping_thread.join();

    server->stop();
  }

  struct SchemaInfo
  {
    std::string name;
    std::string definition;

    SchemaInfo(std::string name, std::string definition) :
      name(std::move(name)),
      definition(std::move(definition))
    {}
  };

  struct ChannelInfo
  {
    foxglove::ChannelId id;
    Clock::time_point last_message_timestamp;
    std::vector<u8> last_message_data;

    ChannelInfo(foxglove::ChannelId id) :
      id(id)
    {}
  };

  using SchemaMap = std::unordered_map<std::size_t, SchemaInfo>;
  using ChannelMap = std::unordered_map<std::size_t, ChannelInfo>;
  using ChannelIdMap = std::unordered_map<foxglove::ChannelId, ChannelInfo &>;

  ChannelMap::iterator handleTopic(const TopicInfo &info);
  ChannelMap::iterator handleMessage(const MessageInfo &info);

  std::atomic_flag enable_callbacks;

private:
  SchemaMap::iterator handleSchema(std::string_view type_name, std::size_t type_hash, std::string_view type_reflection);
  void handleSubscription(foxglove::ChannelId channel_id, websocketpp::connection_hdl handle);
  void handleParameterRequest(
    const std::vector<std::string> &names,
    const std::optional<std::string> &command,
    websocketpp::connection_hdl handle
  );

  SchemaMap schema_map;
  ChannelMap channel_map;
  ChannelIdMap channel_id_map;

  std::unique_ptr<foxglove::ServerInterface<websocketpp::connection_hdl>> server;
  std::mutex mutex;
  std::timed_mutex exit_mutex;

  Logger logger;

  std::list<std::jthread> cache_threads;
  std::jthread time_thread;
  std::jthread housekeeping_thread;
};

FoxgloveServer::FoxgloveServer() :
  UniquePlugin("foxglove-ws")
{
  priv = new FoxgloveServerPrivate();
}

FoxgloveServer::~FoxgloveServer()
{
  delete priv;
}

void FoxgloveServer::topicCallback(const TopicInfo &info)
{
  if (priv->enable_callbacks.test()) {
    (void)priv->handleTopic(info);
  }
}

void FoxgloveServer::messageCallback(const MessageInfo &info)
{
  if (priv->enable_callbacks.test()) {
    (void)priv->handleMessage(info);
  }
}

FoxgloveServerPrivate::SchemaMap::iterator
FoxgloveServerPrivate::handleSchema(std::string_view type_name, const std::size_t type_hash, std::string_view type_reflection)
{
  std::lock_guard guard(mutex);

  SchemaMap::iterator schema_iterator = schema_map.find(type_hash);
  if (schema_iterator == schema_map.end()) {
    schema_iterator = schema_map.emplace_hint(
      schema_iterator,
      std::piecewise_construct,
      std::forward_as_tuple(type_hash),
      std::forward_as_tuple(std::string(type_name), foxglove::base64Encode(type_reflection))
    );
  }

  return schema_iterator;
}

FoxgloveServerPrivate::ChannelMap::iterator FoxgloveServerPrivate::handleTopic(const TopicInfo &info)
{
  const SchemaMap::iterator schema_iterator = handleSchema(info.type_name, info.type_hash, info.type_reflection);

  std::lock_guard guard(mutex);

  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    const std::vector<foxglove::ChannelId> channel_ids = server->addChannels({{
      .topic = info.topic_name,
      .encoding = "flatbuffer",
      .schemaName = schema_iterator->second.name,
      .schema = schema_iterator->second.definition,
    }});

    if (channel_ids.empty()) {
      throw RuntimeException("Failed to add channel.", logger);
    }

    const foxglove::ChannelId channel_id = channel_ids.front();

    channel_iterator = channel_map.emplace_hint(channel_iterator, std::make_pair(info.topic_hash, channel_id));
    channel_id_map.emplace(channel_id, channel_iterator->second);
  }

  return channel_iterator;
}

FoxgloveServerPrivate::ChannelMap::iterator FoxgloveServerPrivate::handleMessage(const MessageInfo &info)
{
  ChannelMap::iterator channel_iterator = channel_map.find(info.topic_info.topic_hash);
  if (channel_iterator == channel_map.end()) {
    channel_iterator = handleTopic(info.topic_info);
  }

  std::lock_guard guard(mutex);
  server->broadcastMessage(
    channel_iterator->second.id,
    std::chrono::duration_cast<std::chrono::nanoseconds>(info.timestamp.time_since_epoch()).count(),
    info.serialized_message.data(),
    info.serialized_message.size()
  );

  // Cache infrequently sent messages.
  if (info.timestamp - channel_iterator->second.last_message_timestamp > std::chrono::seconds(1)
      || channel_iterator->second.last_message_timestamp == Clock::time_point()) {
    channel_iterator->second.last_message_data.assign(info.serialized_message.begin(), info.serialized_message.end());
  }

  channel_iterator->second.last_message_timestamp = info.timestamp;

  return channel_iterator;
}

void FoxgloveServerPrivate::handleSubscription(foxglove::ChannelId channel_id, websocketpp::connection_hdl handle)
{
  const ChannelIdMap::iterator channel_id_iterator = channel_id_map.find(channel_id);
  if (channel_id_iterator == channel_id_map.end()) {
    throw RuntimeException("Failed to find channel.", logger);
  }

  const Clock::time_point cache_message_timestamp = channel_id_iterator->second.last_message_timestamp;

  std::lock_guard guard(mutex);

  cache_threads.emplace_back(std::jthread([this, channel_id, handle, channel_id_iterator, cache_message_timestamp]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::lock_guard guard(mutex);

    // Send out infrequently published messages.
    if (cache_message_timestamp != channel_id_iterator->second.last_message_timestamp
        && channel_id_iterator->second.last_message_timestamp != Clock::time_point()) {
      return;
    }

    if (channel_id_iterator->second.last_message_data.empty()) {
      return;
    }

    server->sendMessage(
      handle,
      channel_id,
      std::chrono::duration_cast<std::chrono::nanoseconds>(channel_id_iterator->second.last_message_timestamp.time_since_epoch()).count(),
      channel_id_iterator->second.last_message_data.data(),
      channel_id_iterator->second.last_message_data.size()
    );
  }));
}

void FoxgloveServerPrivate::handleParameterRequest(
  const std::vector<std::string> &names,
  const std::optional<std::string> &command,
  websocketpp::connection_hdl handle
)
{
  std::vector<foxglove::Parameter> parameters;

  if (command == "get-all-params") {
    for (std::pair<const std::string &, ConfigValue> parameter : *Config::get()) {
      parameters.emplace_back(parameter.first, convertParameterValue(parameter.second));
    }
  } else {
    Config::Ptr config = Config::get();
    parameters.reserve(names.size());

    for (const std::string &name : names) {
      try {
        parameters.emplace_back(name, convertParameterValue(config->getParameter(name)));
      } catch (ConfigAccessException &) {
        parameters.emplace_back(name);
      }
    }
  }

  std::lock_guard guard(mutex);
  server->publishParameterValues(handle, parameters, command);
}

}  // namespace lbot::plugins
}  // namespace labrat
