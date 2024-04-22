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

Manager::Manager() = default;

Manager::~Manager() {
  topic_map.forceFlush();

  Logger::deinitialize();

  for (PluginRegistration &item : plugin_list) {
    item.delete_flag.test_and_set();
    utils::waitUntil(item.use_count, 0U);
  }

  plugin_list.clear();
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

void Manager::removeNode(const std::string &name) {
  const std::unordered_map<ManagerHandle, utils::FinalPtr<Node>>::iterator iterator = node_map.find(name);

  if (iterator == node_map.end()) {
    throw ManagementException("Node not found.");
  }

  node_map.erase(iterator);
}

void Manager::removePluginInternal(const ManagerHandle &handle) {
  const std::list<PluginRegistration>::iterator iterator =
    std::find_if(plugin_list.begin(), plugin_list.end(), [&handle](const PluginRegistration &plugin) {
    return handle == plugin.handle;
  });

  if (iterator == plugin_list.end()) {
    throw ManagementException("Plugin not found.");
  }

  iterator->delete_flag.test_and_set();
  utils::waitUntil(iterator->use_count, 0U);

  utils::FinalPtr<Plugin> &plugin = iterator->plugin;

  std::vector<std::shared_ptr<Node>> plugin_nodes = plugin->nodes;

  plugin_list.erase(iterator);

  while (!plugin_nodes.empty()) {
    const std::string &name = plugin_nodes.back()->getName();
    plugin_nodes.pop_back();

    removeNode(name);
  }
}

}  // namespace lbot
}  // namespace labrat
