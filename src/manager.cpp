/**
 * @file manager.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugin.hpp>
#include <labrat/lbot/utils/atomic.hpp>

inline namespace labrat {
namespace lbot {

std::weak_ptr<Manager> Manager::instance;

thread_local std::optional<Manager::NodeEnvironment> Manager::node_environment;
thread_local std::optional<Manager::PluginEnvironment> Manager::plugin_environment;

Manager::Manager() = default;

Manager::~Manager() {
  topic_map.forceFlush();

  Logger::deinitialize();

  {
    FlagGuard guard(plugin_update_flag);
    waitUntil(plugin_use_count, 0U);
    plugin_list.clear();
  }

  node_map.clear();
}

Manager::Ptr Manager::get() {
  if (instance.use_count()) {
    return instance.lock();
  }

  std::shared_ptr<Manager> result(new Manager());
  instance = result;

  Logger::initialize();

  return result;
}

void Manager::removeNodeInternal(const std::string &name) {
  const NodeRegistration::Map::iterator iterator = node_map.find(name);

  if (iterator == node_map.end()) {
    throw ManagementException("Node not found.");
  }

  if (iterator->second.type_hash.has_value()) {
    node_set.erase(iterator->second.type_hash.value());
  }

  node_map.erase(iterator);
}

void Manager::removePluginInternal(const std::string &name) {
  const std::list<PluginRegistration>::iterator iterator =
    std::find_if(plugin_list.begin(), plugin_list.end(), [&name](const PluginRegistration &plugin) {
    return name == plugin.name;
  });

  if (iterator == plugin_list.end()) {
    throw ManagementException("Plugin not found.");
  }

  std::vector<std::shared_ptr<Node>> plugin_nodes;

  {
    FlagGuard guard(plugin_update_flag);
    waitUntil(plugin_use_count, 0U);

    FinalPtr<Plugin> &plugin = iterator->plugin;

    plugin_nodes = plugin->nodes;

    if (iterator->type_hash.has_value()) {
      plugin_set.erase(iterator->type_hash.value());
    }

    plugin_list.erase(iterator);
  }

  while (!plugin_nodes.empty()) {
    const std::string &name = plugin_nodes.back()->getName();
    plugin_nodes.pop_back();

    removeNode(name);
  }
}

void Manager::createNodeEnvironment(const std::string &name) {
  node_environment.emplace(NodeEnvironment{.name = name, .plugin_list = plugin_list, .topic_map = topic_map, .service_map = service_map, .plugin_update_flag = plugin_update_flag, .plugin_use_count = plugin_use_count});
}

void Manager::createPluginEnvironment(const std::string &name) {
  plugin_environment.emplace(PluginEnvironment{.name = name});
}

Manager::NodeEnvironment Manager::getNodeEnvironment() {
  if (!node_environment.has_value()) {
    throw ManagementException("A node must be created through the central manager.");
  }

  Manager::NodeEnvironment result = std::move(node_environment.value());
  node_environment.reset();

  return result;
}

Manager::PluginEnvironment Manager::getPluginEnvironment() {
  if (!plugin_environment.has_value()) {
    throw ManagementException("A plugin must be created through the central manager.");
  }

  Manager::PluginEnvironment result = std::move(plugin_environment.value());
  plugin_environment.reset();

  return result;
}

}  // namespace lbot
}  // namespace labrat
