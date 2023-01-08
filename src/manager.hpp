/**
 * @file manager.hpp
 * @author Max Yvon Zimmermann
 * 
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 * 
 */

#pragma once

#include <labrat/robot/exception.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugin.hpp>
#include <labrat/robot/service.hpp>
#include <labrat/robot/topic.hpp>
#include <labrat/robot/utils/final_ptr.hpp>
#include <labrat/robot/utils/types.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace labrat::robot {

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

  static std::unique_ptr<Manager> instance;

  std::unordered_map<std::string, utils::FinalPtr<Node>> node_map;
  TopicMap topic_map;
  ServiceMap service_map;
  Plugin::List plugin_list;

public:
  /**
   * @brief Destroy the Manager object.
   *
   */
  ~Manager();

  /**
   * @brief Get the static instance.
   *
   * @return Manager& Reference to the static instance.
   */
  static Manager &get();

  /**
   * @brief Construct and add a node to the internal network.
   *
   * @tparam T Type of the node to be added.
   * @tparam Args Types of the arguments to be forwarded to the node specific constructor.
   * @param name Name of the node.
   * @param args Arguments to be forwarded to the node specific constructor.
   * @return std::weak_ptr<T> Pointer to the created node.
   */
  template <typename T, typename... Args>
  std::weak_ptr<T> addNode(const std::string &name, Args &&...args) requires std::is_base_of_v<Node, T> {
    const Node::Environment environment = {
      .name = name,
      .topic_map = topic_map,
      .service_map = service_map,
      .plugin_list = plugin_list,
    };

    const std::pair<std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator, bool> result =
      node_map.emplace(name, std::make_shared<T>(environment, std::forward<Args>(args)...));

    if (!result.second) {
      throw ManagementException("Node not added.");
    }

    return std::weak_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  /**
   * @brief Remove a node by name from the internal network.
   *
   * @param name
   */
  void removeNode(const std::string &name);

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
};

}  // namespace labrat::robot
