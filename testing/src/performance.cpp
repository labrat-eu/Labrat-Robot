#include <helper.hpp>

#include <labrat/robot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(performance, put) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  const u64 limit = 10000000;

  for (u64 i = 1; i <= limit; ++i) {
    TestContainer message;
    message.integral_field = i;

    node_a->sender->put(message);
  }

  TestContainer message = node_b->receiver->latest();

  EXPECT_EQ(message.integral_field, limit);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(performance, latest) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;

  node_a->sender->put(message_a);

  const u64 limit = 10000000;
  TestContainer message_b;

  for (u64 i = 0; i < limit; ++i) {
    message_b = node_b->receiver->latest();
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(performance, next) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;

  const u64 limit = 10000000;
  TestContainer message_b;

  for (u64 i = 0; i < limit; ++i) {
    node_a->sender->put(message_a);
    message_b = node_b->receiver->latest();
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

}  // namespace labrat::robot::test
