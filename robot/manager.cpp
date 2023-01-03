#include <labrat/robot/manager.hpp>
#include <labrat/robot/utils/atomic.hpp>

#include <google/protobuf/stubs/common.h>

namespace labrat::robot {

std::unique_ptr<Manager> Manager::instance;

Manager::Manager() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Manager::~Manager() {
  node_map.clear();

  google::protobuf::ShutdownProtobufLibrary();
}

Manager &Manager::get() {
  if (!instance) {
    instance = std::unique_ptr<Manager>(new Manager());
  }

  return *instance;
}

Plugin::List::iterator Manager::addPlugin(const Plugin &plugin) {
  return plugin_list.emplace(plugin_list.begin(), plugin);
}

void Manager::removePlugin(Plugin::List::iterator iterator) {
  iterator->delete_flag.test_and_set();
  utils::WaitUntil(iterator->use_count, 0U);

  plugin_list.erase(iterator);
}

}  // namespace labrat::robot
