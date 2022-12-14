#include <labrat/robot/manager.hpp>
#include <labrat/robot/test/helper.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(stress, latest) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b"));

  TestNode::Sender<TestMessage>::Ptr sender = node_a->addTestSender<TestMessage>("/testing");
  TestNode::Receiver<TestMessage>::Ptr receiver = node_b->addTestReceiver<TestMessage>("/testing");

  const u64 limit = 1000000;

  auto sender_lambda = [&sender]() {
    for (u64 i = 1; i <= limit; ++i) {
      TestMessage message;
      message.integral_field = i;

      sender->put(message);
    }
  };

  std::thread sender_thread(sender_lambda);

  u64 last_integral = 0;
  while (last_integral < limit) {
    TestMessage message = receiver->latest();

    EXPECT_GE(message.integral_field, last_integral);

    last_integral = message.integral_field;
  }

  sender_thread.join();
}

TEST(stress, next) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b"));

  TestNode::Sender<TestMessage>::Ptr sender = node_a->addTestSender<TestMessage>("/testing");
  TestNode::Receiver<TestMessage>::Ptr receiver = node_b->addTestReceiver<TestMessage>("/testing");

  const u64 limit = 1000000;
  std::atomic_flag done;

  auto sender_lambda = [&sender, &done]() {
    for (u64 i = 1; i <= limit; ++i) {
      TestMessage message;
      message.integral_field = i;

      sender->put(message);
    }

    while (!done.test()) {
      sender->flush();
    }
  };

  std::thread sender_thread(sender_lambda);

  u64 last_integral = 0;
  while (last_integral < limit) {
    TestMessage message = receiver->next();

    EXPECT_GT(message.integral_field, last_integral);

    last_integral = message.integral_field;
  }

  done.test_and_set();

  sender_thread.join();
}

}  // namespace labrat::robot::test
