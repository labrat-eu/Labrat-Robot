/**
 * @file manager.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/filter.hpp>
#include <labrat/lbot/info.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/service.hpp>
#include <labrat/lbot/topic.hpp>
#include <labrat/lbot/utils/final_ptr.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <atomic>
#include <concepts>
#include <list>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <variant>

inline namespace labrat {
namespace lbot {

class Manager;

class Node;
class UniqueNode;

class Plugin;
class UniquePlugin;

template <typename T>
concept has_topic_callback = requires(T *self, const TopicInfo &info) {
  {
    self->topicCallback(info)
  };
};
template <typename T>
concept has_service_callback = requires(T *self, const ServiceInfo &info) {
  {
    self->serviceCallback(info)
  };
};
template <typename T>
concept has_message_callback = requires(T *self, const MessageInfo &info) {
  {
    self->messageCallback(info)
  };
};

/**
 * @brief Central class to manage nodes, plugins, topics and services.
 *
 */
class Manager {
private:
  /**
   * @brief Class containing data required for registering a node.
   *
   */
  struct NodeRegistration {
    using Map = std::unordered_map<std::string, NodeRegistration>;

    std::string name;
    std::optional<std::size_t> type_hash;
    FinalPtr<Node> node;

    NodeRegistration(FinalPtr<Node> &&node) : node(std::forward<FinalPtr<Node>>(node)){}
    NodeRegistration(NodeRegistration &&rhs) : name(std::move(rhs.name)), type_hash(rhs.type_hash), node(std::move(rhs.node)) {}
  };

  /**
   * @brief Class containing data required for registering a plugin.
   *
   */
  struct PluginRegistration {
    using List = std::list<PluginRegistration>;

    std::string name;
    std::optional<std::size_t> type_hash;
    FinalPtr<Plugin> plugin;
    Filter filter;

    void *user_ptr;
    void (*topic_callback)(void *plugin, const TopicInfo &info);
    void (*service_callback)(void *plugin, const ServiceInfo &info);
    void (*message_callback)(void *plugin, const MessageInfo &info);

    PluginRegistration(FinalPtr<Plugin> &&plugin) : plugin(std::forward<FinalPtr<Plugin>>(plugin)){}
  };

  /**
   * @brief Information on the environment in which the node is created.
   * This data will be copied by a node upon construction.
   *
   */
  struct NodeEnvironment final {
    std::string name;

    PluginRegistration::List &plugin_list;
    TopicMap &topic_map;
    ServiceMap &service_map;

    std::atomic_flag &plugin_update_flag;
    std::atomic<u32> &plugin_use_count;
  };

  /**
   * @brief Information on the environment in which the plugin is created.
   * This data will be copied by a plugin upon construction.
   *
   */
  struct PluginEnvironment final {
    std::string name;
  };

  /**
   * @brief Construct a new Manager object.
   *
   */
  Manager();

  template <typename T>
  static void callPluginTopicCallback(void *plugin, const TopicInfo &info) {
    {
      T *self = reinterpret_cast<T *>(plugin);
      self->topicCallback(info);
    };
  }

  template <typename T>
  static void callPluginServiceCallback(void *plugin, const ServiceInfo &info) {
    {
      T *self = reinterpret_cast<T *>(plugin);
      self->serviceCallback(info);
    };
  }

  template <typename T>
  static void callPluginMessageCallback(void *plugin, const MessageInfo &info) {
    {
      T *self = reinterpret_cast<T *>(plugin);
      self->messageCallback(info);
    };
  }

  static std::weak_ptr<Manager> instance;

  NodeRegistration::Map node_map;
  std::unordered_set<std::size_t> node_set;
  PluginRegistration::List plugin_list;
  std::unordered_set<std::size_t> plugin_set;

  TopicMap topic_map;
  ServiceMap service_map;

  std::atomic_flag plugin_update_flag;
  std::atomic<u32> plugin_use_count;

  static thread_local std::optional<NodeEnvironment> node_environment;
  static thread_local std::optional<PluginEnvironment> plugin_environment;

public:
  using Ptr = std::shared_ptr<Manager>;

  /**
   * @brief Destroy the Manager object.
   *
   */
  ~Manager();

  /**
   * @brief Get the static instance.
   *
   * @return Manager::Ptr Pointer to the static instance.
   */
  static Manager::Ptr get();

  /**
   * @brief Construct and add a node to the internal network.
   *
   * @tparam T Type of the node to be added.
   * @tparam Args Types of the arguments to be forwarded to the node specific constructor.
   * @param name Name of the node.
   * @param args Arguments to be forwarded to the node specific constructor.
   * @return std::shared_ptr<T> Pointer to the created node.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addNode(std::string name, Args &&...args) requires std::is_base_of_v<Node, T> {
    createNodeEnvironment(name);

    if (name.empty()) {
      throw ManagementException("Node name must be non-empty.");
    }

    if constexpr (std::is_base_of_v<UniqueNode, T>) {
      if (node_set.contains(typeid(T).hash_code())) {
        throw ManagementException("Plugin not added due to type conflict.");
      }
    }

    const NodeRegistration::Map::iterator iterator = node_map.find(name);

    if (iterator != node_map.end()) {
      throw ManagementException("Node not added due to name conflict.");
    }

    std::shared_ptr<T> result = std::make_shared<T>(std::forward<Args>(args)...);

    NodeRegistration registration(std::dynamic_pointer_cast<Node>(result));
    registration.name = name;

    if constexpr (std::is_base_of_v<UniqueNode, T>) {
      registration.type_hash = typeid(T).hash_code();
      node_set.emplace(typeid(T).hash_code());
    }

    if (!node_map.emplace(name, std::move(registration)).second) {
      throw ManagementException("Node not added.");
    }

    return std::shared_ptr<T>(result);
  }

  /**
   * @brief Construct and add a plugin to the internal network.
   *
   * @tparam T Type of the plugin to be added.
   * @tparam Args Types of the arguments to be forwarded to the plugin specific constructor.
   * @param args Arguments to be forwarded to the plugin specific constructor.
   * @return std::shared_ptr<T> Pointer to the created plugin.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addPlugin(std::string name, Args &&...args) requires std::is_base_of_v<Plugin, T> {
    return addPlugin<T>(std::move(name), Filter(), std::forward<Args>(args)...);
  }

  /**
   * @brief Construct and add a plugin to the internal network.
   *
   * @tparam T Type of the plugin to be added.
   * @tparam Args Types of the arguments to be forwarded to the plugin specific constructor.
   * @param filter Topic filter to be used on callbacks.
   * @param args Arguments to be forwarded to the plugin specific constructor.
   * @return std::shared_ptr<T> Pointer to the created plugin.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addPlugin(std::string name, Filter filter, Args &&...args) requires std::is_base_of_v<Plugin, T> {
    createPluginEnvironment(name);

    if (name.empty()) {
      throw ManagementException("Plugin name must be non-empty.");
    }

    if constexpr (std::is_base_of_v<UniquePlugin, T>) {
      if (plugin_set.contains(typeid(T).hash_code())) {
        throw ManagementException("Plugin not added due to type conflict.");
      }
    }

    const std::list<PluginRegistration>::iterator iterator =
      std::find_if(plugin_list.begin(), plugin_list.end(), [&name](const PluginRegistration &plugin) {
      return name == plugin.name;
    });

    if (iterator != plugin_list.end()) {
      throw ManagementException("Plugin not added due to name conflict.");
    }

    std::shared_ptr<T> result = std::make_shared<T>(std::forward<Args>(args)...);

    PluginRegistration registration(std::dynamic_pointer_cast<Plugin>(result));
    registration.filter = std::move(filter);
    registration.user_ptr = result.get();
    registration.name = name;

    if constexpr (std::is_base_of_v<UniquePlugin, T>) {
      registration.type_hash = typeid(T).hash_code();
      plugin_set.emplace(typeid(T).hash_code());
    }

    if constexpr (has_topic_callback<T>) {
      registration.topic_callback = &Manager::callPluginTopicCallback<T>;
    } else {
      registration.topic_callback = nullptr;
    }

    if constexpr (has_service_callback<T>) {
      registration.service_callback = &Manager::callPluginServiceCallback<T>;
    } else {
      registration.service_callback = nullptr;
    }

    if constexpr (has_message_callback<T>) {
      registration.message_callback = &Manager::callPluginMessageCallback<T>;
    } else {
      registration.message_callback = nullptr;
    }

    plugin_list.emplace_back(std::move(registration));
  
    return result;
  }

  /**
   * @brief Remove a node by name from the internal network.
   *
   * @param name Name of the node to be removed.
   */
  inline void removeNode(const std::string &name) {
    removeNodeInternal(name);
  }

  /**
   * @brief Remove a plugin  by name from the internal network.
   *
   * @param name Name of the plugin to be removed.
   */
  inline void removePlugin(const std::string &name) {
    removePluginInternal(name);
  }

  /**
   * @brief Flush all topics.
   *
   */
  void flushAllTopics();

private:
  void removeNodeInternal(const std::string &handle);
  void removePluginInternal(const std::string &handle);

  void createNodeEnvironment(const std::string &name);
  void createPluginEnvironment(const std::string &name);

  NodeEnvironment getNodeEnvironment();
  PluginEnvironment getPluginEnvironment();

  friend Node;
  friend Plugin;
};

}  // namespace lbot
}  // namespace labrat
