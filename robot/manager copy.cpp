#include <labrat/robot/manager.hpp>

namespace labrat::robot {

std::unique_ptr<Manager> Manager::instance;

Manager::Manager() {}

Manager::~Manager() {
  node_map.clear();
}

Manager &Manager::get() {
  if (!instance) {
    instance = std::make_unique<Manager>();
  }

  return *instance;
}

}  // namespace labrat::robot
