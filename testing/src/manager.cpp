#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class ManagerTest : public LbotTest
{};

TEST_F(ManagerTest, get)
{
  {
    lbot::Manager::Ptr manager = lbot::Manager::get();
    ASSERT_NO_THROW(lbot::Manager::get());
  }

  ASSERT_THROW(lbot::Manager::get(), lbot::ManagementException);
}

TEST_F(ManagerTest, unique_node)
{
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestUniqueNode> node_a(manager->addNode<TestUniqueNode>("test_node"));

  ASSERT_THROW(manager->addNode<TestNode>("test_node"), lbot::ManagementException);
  ASSERT_THROW(manager->addNode<TestUniqueNode>("test_node_again"), lbot::ManagementException);

  node_a = std::shared_ptr<TestUniqueNode>();
  ASSERT_NO_THROW(manager->removeNode("test_node"));
}

TEST_F(ManagerTest, shared_node)
{
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  ASSERT_THROW(manager->addNode<TestNode>("node_a", "main", "void"), lbot::ManagementException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_F(ManagerTest, unique_plugin)
{
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestUniquePlugin> test_plugin(manager->addPlugin<TestUniquePlugin>("test_plugin"));

  ASSERT_THROW(manager->addPlugin<TestPlugin>("test_plugin"), lbot::ManagementException);
  ASSERT_THROW(manager->addPlugin<TestUniquePlugin>("test_plugin_again"), lbot::ManagementException);

  test_plugin = std::shared_ptr<TestUniquePlugin>();
  ASSERT_NO_THROW(manager->removePlugin("test_plugin"));
}

TEST_F(ManagerTest, shared_plugin)
{
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TestPlugin> plugin_a(manager->addPlugin<TestPlugin>("plugin_a"));
  std::shared_ptr<TestPlugin> plugin_b(manager->addPlugin<TestPlugin>("plugin_b"));

  ASSERT_THROW(manager->addPlugin<TestPlugin>("plugin_a"), lbot::ManagementException);

  plugin_a = std::shared_ptr<TestPlugin>();
  ASSERT_NO_THROW(manager->removePlugin("plugin_a"));
  plugin_b = std::shared_ptr<TestPlugin>();
  ASSERT_NO_THROW(manager->removePlugin("plugin_b"));
}

}  // namespace lbot::test
}  // namespace labrat
