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
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugin.hpp>
#include <labrat/lbot/service.hpp>
#include <labrat/lbot/topic.hpp>
#include <labrat/lbot/utils/final_ptr.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

inline namespace labrat {
namespace lbot {

class Cluster;

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

  static std::weak_ptr<Manager> instance;

  std::unordered_map<std::string, utils::FinalPtr<Node>> node_map;
  std::unordered_map<std::string, utils::FinalPtr<Cluster>> cluster_map;
  TopicMap topic_map;
  ServiceMap service_map;
  Plugin::List plugin_list;

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
  std::shared_ptr<T> addNode(const std::string &name, Args &&...args)
  requires std::is_base_of_v<Node, T>
  {
    const Node::Environment environment = getEnvironment(name);

    const std::pair<std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator, bool> result =
      node_map.emplace(name, std::make_shared<T>(environment, std::forward<Args>(args)...));

    if (!result.second) {
      throw ManagementException("Node not added.");
    }

    return std::shared_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  /**
   * @brief Construct and add a node cluster to the internal network.
   *
   * @tparam T Type of the cluster to be added.
   * @tparam Args Types of the arguments to be forwarded to the cluster specific constructor.
   * @param name Name of the cluster.
   * @param args Arguments to be forwarded to the cluster specific constructor.
   * @return std::shared_ptr<T> Pointer to the created cluster.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addCluster(const std::string &name, Args &&...args)
  requires std::is_base_of_v<Cluster, T>
  {
    const std::pair<std::unordered_map<std::string, utils::FinalPtr<Cluster>>::iterator, bool> result =
      cluster_map.emplace(name, std::make_shared<T>(name, std::forward<Args>(args)...));

    if (!result.second) {
      throw ManagementException("Cluster not added.");
    }

    return std::shared_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  /**
   * @brief Remove a node by name from the internal network.
   *
   * @param name
   */
  void removeNode(const std::string &name);

  /**
   * @brief Remove a node cluster by name from the internal network.
   *
   * @param name
   */
  void removeCluster(const std::string &name);

  /**
   * @brief Add a plugin to the manager.
   *
   * @param Plugin Plugin to be added.
   * @return Plugin::List::iterator Iterator to the inserted plugin item in internal plugin list.
   */
  Plugin::List::iterator addPlugin(const Plugin &Plugin);

  /**
   * @brief Remove a plugin by iterator.
   *
   * @param iterator Iterator to an inserted plugin item in internal plugin list.
   */
  void removePlugin(Plugin::List::iterator iterator);

  /**
   * @brief Get the internal plugin list.
   *
   * @return const Plugin::List& Reference to the internal plugin list.
   */
  inline const Plugin::List &getPlugins() const {
    return plugin_list;
  }

  /**
   * @brief Flush all topics.
   *
   */
  void flushAllTopics();

private:
  inline Node::Environment getEnvironment(const std::string &name) {
    return Node::Environment{
      .name = name,
      .topic_map = topic_map,
      .service_map = service_map,
      .plugin_list = plugin_list,
    };
  }
};

}  // namespace lbot
}  // namespace labrat
