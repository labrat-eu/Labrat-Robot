#include <helper.hpp>

#include <labrat/robot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(manager, node) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(manager, cluster) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestCluster> test_cluster(manager->addCluster<TestCluster>("test_cluster"));

  test_cluster = std::shared_ptr<TestCluster>();
  ASSERT_NO_THROW(manager->removeCluster("test_cluster"));
}

}  // namespace labrat::robot::test
