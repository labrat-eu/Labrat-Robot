#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(manager, node) {
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  ASSERT_THROW(manager->addNode<TestNode>("node_a", "main", "void"), lbot::ManagementException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(manager, unique_plugin) {
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestUniquePlugin> test_plugin(manager->addPlugin<TestUniquePlugin>());

  ASSERT_THROW(manager->addPlugin<TestUniquePlugin>(), lbot::ManagementException);

  test_plugin = std::shared_ptr<TestUniquePlugin>();
  ASSERT_NO_THROW(manager->removePlugin<TestUniquePlugin>());
}

TEST(manager, shared_plugin) {
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestSharedPlugin> plugin_a(manager->addPlugin<TestSharedPlugin>("plugin_a"));
  std::shared_ptr<TestSharedPlugin> plugin_b(manager->addPlugin<TestSharedPlugin>("plugin_b"));

  ASSERT_THROW(manager->addPlugin<TestSharedPlugin>("plugin_a"), lbot::ManagementException);

  plugin_a = std::shared_ptr<TestSharedPlugin>();
  ASSERT_NO_THROW(manager->removePlugin("plugin_a"));
  plugin_b = std::shared_ptr<TestSharedPlugin>();
  ASSERT_NO_THROW(manager->removePlugin("plugin_b"));
}

}  // namespace lbot::test
}  // namespace labrat
