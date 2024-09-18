#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

struct MessageSize
{
  const u64 limit;
  const u64 size;
};

class PerformanceTest : public LbotTestWithParam<MessageSize>
{};

TEST_P(PerformanceTest, put)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.buffer.resize(GetParam().size);

  for (u64 i = 1; i <= GetParam().limit; ++i) {
    message_a.integral_field = i;

    node_a->sender->put(message_a);
  }

  TestContainer message_b = node_b->receiver->latest();

  EXPECT_EQ(message_b.integral_field, GetParam().limit);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_P(PerformanceTest, latest)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;
  message_a.buffer.resize(GetParam().size);

  node_a->sender->put(message_a);

  TestContainer message_b;

  for (u64 i = 0; i < GetParam().limit; ++i) {
    message_b = node_b->receiver->latest();
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_P(PerformanceTest, next)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;
  message_a.buffer.resize(GetParam().size);

  TestContainer message_b;

  for (u64 i = 0; i < GetParam().limit; ++i) {
    node_a->sender->put(message_a);
    message_b = node_b->receiver->next();
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_P(PerformanceTest, callback)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void1", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;
  message_a.buffer.resize(GetParam().size);

  TestContainer message_b;

  auto lambda = [](const TestContainer &message, TestContainer *destination) -> void {
    *destination = message;
  };
  void (*callback)(const TestContainer &message, TestContainer *destination) = lambda;

  node_b->receiver->setCallback(callback, &message_b);

  for (u64 i = 0; i < GetParam().limit; ++i) {
    node_a->sender->put(message_a);
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_P(PerformanceTest, callback_parallel)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void1", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;
  message_a.buffer.resize(GetParam().size);

  TestContainer message_b;

  auto lambda = [](const TestContainer &message, TestContainer *destination) -> void {
    *destination = message;
  };
  void (*callback)(const TestContainer &message, TestContainer *destination) = lambda;

  node_b->receiver->setCallback(callback, &message_b, lbot::ExecutionPolicy::parallel);

  for (u64 i = 0; i < GetParam().limit; ++i) {
    node_a->sender->put(message_a);
  }

  EXPECT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST_P(PerformanceTest, callback_multi)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void1", "main"));
  std::shared_ptr<TestNode> node_c(manager->addNode<TestNode>("node_c", "void2", "main"));
  std::shared_ptr<TestNode> node_d(manager->addNode<TestNode>("node_d", "void3", "main"));

  TestContainer message_a;
  message_a.integral_field = 42;
  message_a.buffer.resize(GetParam().size);

  TestContainer message_b;
  TestContainer message_c;
  TestContainer message_d;

  auto lambda = [](const TestContainer &message, TestContainer *destination) -> void {
    *destination = message;
  };
  void (*callback)(const TestContainer &message, TestContainer *destination) = lambda;

  node_b->receiver->setCallback(callback, &message_b, lbot::ExecutionPolicy::parallel);
  node_c->receiver->setCallback(callback, &message_c, lbot::ExecutionPolicy::parallel);
  node_d->receiver->setCallback(callback, &message_d, lbot::ExecutionPolicy::parallel);

  for (u64 i = 0; i < GetParam().limit; ++i) {
    node_a->sender->put(message_a);
  }

  EXPECT_EQ(message_a, message_b);
  EXPECT_EQ(message_a, message_c);
  EXPECT_EQ(message_a, message_d);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
  node_c = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_c"));
  node_d = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_d"));
}

INSTANTIATE_TEST_SUITE_P(
  performance,
  PerformanceTest,
  testing::Values(MessageSize{.limit = 100000, .size = 1}, MessageSize{.limit = 1000, .size = 10000000}),
  [](const testing::TestParamInfo<PerformanceTest::ParamType> &info) {
    if (info.param.size == 1) {
      return "small";
    } else {
      return "large";
    }
  }
);

}  // namespace lbot::test
}  // namespace labrat
