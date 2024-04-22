#include <labrat/lbot/manager.hpp>

#include <atomic>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(deadlock, next) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestSharedNode> node_a(manager->addNode<TestSharedNode>("node_a", "main", "void"));
  std::shared_ptr<TestSharedNode> node_b(manager->addNode<TestSharedNode>("node_b", "void", "main", 4));

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  node_a->sender->put(message_a);

  TestContainer message_b = node_b->receiver->next();

  ASSERT_EQ(message_a, message_b);

  for (int i = 0; i < 4; ++i) {
    TestContainer message_c;
    message_c.integral_field = 5;
    message_c.float_field = 10.0;

    node_a->sender->put(message_c);
  }

  std::atomic_flag exit_flag;

  std::thread([&]() {
    TestContainer message_d = node_b->receiver->next();
    exit_flag.test_and_set();
  }).detach();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  ASSERT_TRUE(exit_flag.test());
}

}  // namespace lbot::test
}  // namespace labrat
