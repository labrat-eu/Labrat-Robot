/**
 * @file plugin.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */
#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/logger.hpp>

#include <atomic>
#include <string>
#include <vector>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {

class Plugin {
public:
  virtual ~Plugin() = default;

  /**
   * @brief Get the name of the plugin.
   * 
   * @return const std::string& name
   */
  const std::string &getName() const {
    return environment.name;
  }

protected:
  /**
   * @brief Construct a new Plugin object.
   *
   */
  explicit Plugin() : environment(Manager::get()->getPluginEnvironment()), logger(environment.name) {}

  /**
   * @brief Construct a new Plugin object.
   *
   * @param name Favored plugin name.
   */
  explicit Plugin(std::string name) : environment(Manager::get()->getPluginEnvironment()), logger(environment.name) {
    if (environment.name != name) {
      getLogger().logWarning() << "Plugin name differs from favored name '" << name << "'";
    }
  }

  /**
   * @brief Get a logger with the name of the plugin.
   *
   * @return Logger A logger with the name of the plugin.
   */
  [[nodiscard]] inline Logger &getLogger() {
    return logger;
  }

  /**
   * @brief Construct and add a node to the internal network.
   *
   * @tparam T Type of the node to be added.
   * @tparam Args Types of the arguments to be forwarded to the node specific constructor.
   * @param args Arguments to be forwarded to the node specific constructor.
   * @return std::shared_ptr<T> Pointer to the created node.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> addNode(Args &&...args) requires std::is_base_of_v<Node, T> {
    std::shared_ptr<T> result = Manager::get()->addNode<T>(std::forward<Args>(args)...);
    nodes.emplace_back(result);

    return result;
  }

private:
  Manager::PluginEnvironment environment;
  Logger logger;
  std::vector<std::shared_ptr<Node>> nodes;

  friend class Manager;
  friend class Node;
};

class UniquePlugin : public Plugin {
protected:
  explicit UniquePlugin() : Plugin() {}
  explicit UniquePlugin(std::string name) : Plugin(std::move(name)) {}
};

}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
