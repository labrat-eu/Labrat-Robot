#include <helper.hpp>

#include <labrat/robot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(stress, latest) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  const u64 limit = 1000000;

  auto sender_lambda = [&node_a]() {
    for (u64 i = 1; i <= limit; ++i) {
      TestContainer message;
      message.integral_field = i;

      node_a->sender->put(message);
    }
  };

  std::thread sender_thread(sender_lambda);

  u64 last_integral = 0;
  while (last_integral < limit) {
    try {
      TestContainer message = node_b->receiver->latest();

      EXPECT_GE(message.integral_field, last_integral);

      last_integral = message.integral_field;
    } catch (TopicNoDataAvailableException &e) {
      if (last_integral != 0) {
        throw e;
      }
    }
  }

  sender_thread.join();

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

TEST(stress, next) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  const u64 limit = 1000000;
  std::atomic_flag done;

  auto sender_lambda = [&node_a, &done]() {
    for (u64 i = 1; i <= limit; ++i) {
      TestContainer message;
      message.integral_field = i;

      node_a->sender->put(message);
    }

    while (!done.test()) {
      node_a->sender->flush();
    }
  };

  std::thread sender_thread(sender_lambda);

  u64 last_integral = 0;
  while (last_integral < limit) {
    try {
      TestContainer message = node_b->receiver->next();

      EXPECT_GT(message.integral_field, last_integral);

      last_integral = message.integral_field;
    } catch (TopicNoDataAvailableException &) {
      if (last_integral != 0) {
        break;
      }
    }
  }

  done.test_and_set();

  sender_thread.join();

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

}  // namespace labrat::robot::test
