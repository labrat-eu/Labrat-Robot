#include <helper.hpp>

#include <labrat/robot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(manager, node) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

TEST(manager, cluster) {
  std::shared_ptr<TestCluster> test_cluster(labrat::robot::Manager::get().addCluster<TestCluster>("test_cluster"));

  test_cluster = std::shared_ptr<TestCluster>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeCluster("test_cluster"));
}

}  // namespace labrat::robot::test
