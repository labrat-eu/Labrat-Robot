/**
 * @file plugin.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>

#include <atomic>
#include <string>
#include <vector>

inline namespace labrat {
namespace lbot {

class Plugin {
public:
  virtual ~Plugin() = default;

protected:
  Plugin(const PluginEnvironment &environment) : environment(environment) {}
  Plugin(PluginEnvironment &&environment) : environment(std::forward<PluginEnvironment>(environment)) {}

  const std::string &getName() const {
    return environment.name;
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
  std::shared_ptr<T> addNode(Args &&...args) requires std::is_base_of_v<UniqueNode, T> || std::is_base_of_v<SharedNode, T> {
    std::shared_ptr<T> result = Manager::get()->addNode<T>(std::forward<Args>(args)...);
    nodes.emplace_back(result);

    return result;
  }

private:
  std::vector<std::shared_ptr<Node>> nodes;
  const PluginEnvironment environment;

  friend class Manager;
  friend class Node;
};

class UniquePlugin : public Plugin {
protected:
  explicit UniquePlugin(PluginEnvironment environment, std::string name) : Plugin(PluginEnvironment(std::move(environment), name)) {}
};

class SharedPlugin : public Plugin {
protected:
  explicit SharedPlugin(PluginEnvironment environment) : Plugin(std::move(environment)) {}
};

}  // namespace lbot
}  // namespace labrat
