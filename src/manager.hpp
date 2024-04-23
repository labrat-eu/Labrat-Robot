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
#include <variant>

inline namespace labrat {
namespace lbot {

class Manager;

class Node;
class UniqueNode;
class SharedNode;

class Plugin;
class UniquePlugin;
class SharedPlugin;

using ManagerHandle = std::variant<std::size_t, std::string>;

/**
 * @brief Class containing data required for registering a plugin.
 *
 */
struct PluginRegistration {
  using List = std::list<PluginRegistration>;

  ManagerHandle handle;
  std::atomic_flag delete_flag;
  std::atomic<u32> use_count;
  FinalPtr<Plugin> plugin;
  Filter filter;

  void *user_ptr;
  void (*topic_callback)(void *plugin, const TopicInfo &info);
  void (*service_callback)(void *plugin, const ServiceInfo &info);
  void (*message_callback)(void *plugin, const MessageInfo &info);

  PluginRegistration(FinalPtr<Plugin> &&plugin) : plugin(std::forward<FinalPtr<Plugin>>(plugin)){};

  PluginRegistration(PluginRegistration &&rhs) :
    handle(rhs.handle), use_count(rhs.use_count.load()), plugin(std::forward<FinalPtr<Plugin>>(rhs.plugin)), filter(rhs.filter),
    user_ptr(rhs.user_ptr), topic_callback(rhs.topic_callback), service_callback(rhs.service_callback),
    message_callback(rhs.message_callback) {
    if (rhs.delete_flag.test()) {
      delete_flag.test_and_set();
    }
  }
};

/**
 * @brief Information on the environment in which the node is created.
 * This data will be copied by a node upon construction.
 *
 */
class NodeEnvironment final {
public:
  NodeEnvironment(const NodeEnvironment &rhs) :
    name(rhs.name), topic_map(rhs.topic_map), service_map(rhs.service_map), plugin_list(rhs.plugin_list) {}
  NodeEnvironment(NodeEnvironment &&rhs) :
    name(std::move(rhs.name)), topic_map(rhs.topic_map), service_map(rhs.service_map), plugin_list(rhs.plugin_list) {}

  const std::string name;

  TopicMap &topic_map;
  ServiceMap &service_map;
  PluginRegistration::List &plugin_list;

private:
  NodeEnvironment(NodeEnvironment &&rhs, std::string name) :
    name(std::move(name)), topic_map(rhs.topic_map), service_map(rhs.service_map), plugin_list(rhs.plugin_list) {}
  NodeEnvironment(std::string name, TopicMap &topic_map, ServiceMap &service_map, PluginRegistration::List &plugin_list) :
    name(std::move(name)), topic_map(topic_map), service_map(service_map), plugin_list(plugin_list) {}

  friend Manager;
  friend UniqueNode;
};

/**
 * @brief Information on the environment in which the plugin is created.
 * This data will be copied by a plugin upon construction.
 *
 */
class PluginEnvironment final {
public:
  PluginEnvironment(const PluginEnvironment &rhs) : name(rhs.name) {}
  PluginEnvironment(PluginEnvironment &&rhs) : name(std::move(rhs.name)) {}

  const std::string name;

private:
  PluginEnvironment(PluginEnvironment &&rhs, std::string name) : name(std::move(name)) {}
  PluginEnvironment(std::string name) : name(std::move(name)) {}

  friend Manager;
  friend UniquePlugin;
};

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

  std::unordered_map<ManagerHandle, FinalPtr<Node>> node_map;
  PluginRegistration::List plugin_list;
  TopicMap topic_map;
  ServiceMap service_map;

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
   * @param args Arguments to be forwarded to the node specific constructor.
   * @return std::shared_ptr<T> Pointer to the created node.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addNode(Args &&...args) requires std::is_base_of_v<UniqueNode, T> {
    return addNodeInternal<T>(typeid(T).hash_code(), std::forward<Args>(args)...);
  }

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
  std::shared_ptr<T> addNode(const std::string &name, Args &&...args) requires std::is_base_of_v<SharedNode, T> {
    return addNodeInternal<T>(name, std::forward<Args>(args)...);
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
  std::shared_ptr<T> addPlugin(Args &&...args) requires std::is_base_of_v<UniquePlugin, T> {
    return addPlugin<T>(Filter(), std::forward<Args>(args)...);
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
  std::shared_ptr<T> addPlugin(Filter filter, Args &&...args) requires std::is_base_of_v<UniquePlugin, T> {
    return addPluginInternal<T>(typeid(T).hash_code(), filter, std::forward<Args>(args)...);
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
  std::shared_ptr<T> addPlugin(std::string name, Args &&...args) requires std::is_base_of_v<SharedPlugin, T> {
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
  std::shared_ptr<T> addPlugin(std::string name, Filter filter, Args &&...args) requires std::is_base_of_v<SharedPlugin, T> {
    return addPluginInternal<T>(name, filter, std::forward<Args>(args)...);
  }

  /**
   * @brief Remove a plugin by iterator.
   *
   * @tparam T Type of the plugin to be removed.
   */
  template <typename T>
  inline void removeNode() {
    removeNodeInternal(typeid(T).hash_code());
  }

  /**
   * @brief Remove a node by name from the internal network.
   *
   * @param name
   */
  void removeNode(const std::string &name) {
    removeNodeInternal(name);
  }

  /**
   * @brief Remove a plugin by iterator.
   *
   * @tparam T Type of the plugin to be removed.
   */
  template <typename T>
  inline void removePlugin() {
    removePluginInternal(typeid(T).hash_code());
  }

  /**
   * @brief Remove a plugin by iterator.
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
  template <typename T, typename... Args>
  std::shared_ptr<T> addNodeInternal(ManagerHandle handle, Args &&...args) {
    NodeEnvironment environment = getNodeEnvironment(handle);

    if (std::holds_alternative<std::string>(handle)) {
      if (std::get<std::string>(handle).empty()) {
        throw ManagementException("Node name must be non-empty.");
      }
    }

    const std::pair<std::unordered_map<ManagerHandle, FinalPtr<Node>>::iterator, bool> result =
      node_map.emplace(handle, std::make_shared<T>(std::move(environment), std::forward<Args>(args)...));

    if (!result.second) {
      throw ManagementException("Node not added.");
    }

    return std::shared_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  template <typename T, typename... Args>
  std::shared_ptr<T> addPluginInternal(ManagerHandle handle, Filter filter, Args &&...args) requires std::is_base_of_v<Plugin, T> {
    PluginEnvironment environment = getPluginEnvironment(handle);

    if (std::holds_alternative<std::string>(handle)) {
      if (std::get<std::string>(handle).empty()) {
        throw ManagementException("Plugin name must be non-empty.");
      }
    }

    const std::list<PluginRegistration>::iterator iterator =
      std::find_if(plugin_list.begin(), plugin_list.end(), [&handle](const PluginRegistration &plugin) {
      return handle == plugin.handle;
    });

    if (iterator != plugin_list.end()) {
      throw ManagementException("Plugin not added.");
    }

    std::shared_ptr<T> result = std::make_shared<T>(std::move(environment), std::forward<Args>(args)...);

    PluginRegistration registration(std::dynamic_pointer_cast<Plugin>(result));
    registration.handle = handle;
    registration.filter = std::move(filter);
    registration.user_ptr = result.get();

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

  void removeNodeInternal(const ManagerHandle &handle);
  void removePluginInternal(const ManagerHandle &handle);

  inline NodeEnvironment getNodeEnvironment(const ManagerHandle &handle) {
    if (std::holds_alternative<std::string>(handle)) {
      return NodeEnvironment(std::get<std::string>(handle), topic_map, service_map, plugin_list);
    }

    return NodeEnvironment("", topic_map, service_map, plugin_list);
  }

  inline PluginEnvironment getPluginEnvironment(const ManagerHandle &handle) {
    if (std::holds_alternative<std::string>(handle)) {
      return PluginEnvironment(std::get<std::string>(handle));
    }

    return PluginEnvironment("");
  }
};

}  // namespace lbot
}  // namespace labrat
