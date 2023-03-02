/**
 * @file cluster.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/cluster.hpp>

namespace labrat::robot {

Cluster::Cluster(const std::string &name) : name(name) {}

Cluster::~Cluster() {
  while (!nodes.empty()) {
    const std::string &name = nodes.back()->getName();
    nodes.pop_back();

    Manager::get().removeNode(name);
  }
}

}  // namespace labrat::robot
