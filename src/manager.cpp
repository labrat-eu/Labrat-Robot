/**
 * @file manager.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/manager.hpp>
#include <labrat/robot/logger.hpp>
#include <labrat/robot/utils/atomic.hpp>

namespace labrat::robot {

std::weak_ptr<Manager> Manager::instance;

Manager::Manager() = default;

Manager::~Manager() {
  topic_map.forceFlush();

  Logger::deinitialize();
  
  cluster_map.clear();
  node_map.clear();
}

Manager::Ptr Manager::get() {
  if (instance.use_count() != 0) {
    return instance.lock();
  }

  std::shared_ptr<Manager> result(new Manager());
  instance = result;

  Logger::initialize();

  return result;
}

void Manager::removeNode(const std::string &name) {
  const std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator iterator = node_map.find(name);

  if (iterator == node_map.end()) {
    throw ManagementException("Node not found.");
  }

  node_map.erase(iterator);
}

void Manager::removeCluster(const std::string &name) {
  const std::unordered_map<std::string, utils::FinalPtr<Cluster>>::iterator iterator = cluster_map.find(name);

  if (iterator == cluster_map.end()) {
    throw ManagementException("Cluster not found.");
  }

  cluster_map.erase(iterator);
}

Plugin::List::iterator Manager::addPlugin(const Plugin &plugin) {
  return plugin_list.emplace(plugin_list.begin(), plugin);
}

void Manager::removePlugin(Plugin::List::iterator iterator) {
  iterator->delete_flag.test_and_set();
  utils::waitUntil(iterator->use_count, 0U);

  plugin_list.erase(iterator);
}

}  // namespace labrat::robot
