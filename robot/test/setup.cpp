#include <labrat/robot/manager.hpp>
#include <labrat/robot/test/helper.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(setup, latest) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b"));

  TestNode::Sender<TestMessage>::Ptr sender = node_a->addTestSender<TestMessage>("/testing");
  TestNode::Receiver<TestMessage>::Ptr receiver = node_b->addTestReceiver<TestMessage>("/testing", 10);

  TestMessage message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  sender->put(message_a);

  TestMessage message_b = receiver->latest();

  ASSERT_EQ(message_a, message_b);

  TestMessage message_c;
  message_c.integral_field = 5;
  message_c.float_field = 10.0;

  TestMessage message_d = receiver->latest();

  sender->put(message_c);

  ASSERT_EQ(message_a, message_d);
  ASSERT_EQ(message_b, message_d);
  ASSERT_NE(message_c, message_d);

  TestMessage message_e = receiver->latest();

  ASSERT_NE(message_a, message_e);
  ASSERT_NE(message_b, message_e);
  ASSERT_EQ(message_c, message_e);
}

TEST(setup, next) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b"));

  TestNode::Sender<TestMessage>::Ptr sender = node_a->addTestSender<TestMessage>("/testing");
  TestNode::Receiver<TestMessage>::Ptr receiver = node_b->addTestReceiver<TestMessage>("/testing", 10);

  TestMessage message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  sender->put(message_a);

  TestMessage message_b = receiver->next();

  ASSERT_EQ(message_a, message_b);

  TestMessage message_c;
  message_c.integral_field = 5;
  message_c.float_field = 10.0;

  TestMessage message_d;
  auto receiver_lambda = [&message_d, &receiver]() {
    message_d = receiver->next();
  };

  std::thread receiver_thread(receiver_lambda);

  sender->put(message_c);

  receiver_thread.join();

  ASSERT_NE(message_a, message_d);
  ASSERT_NE(message_b, message_d);
  ASSERT_EQ(message_c, message_d);

  TestMessage message_e;
  message_e.integral_field = 100;
  sender->put(message_e);

  TestMessage message_f;
  message_f.integral_field = 101;
  sender->put(message_f);

  TestMessage message_g = receiver->next();

  ASSERT_NE(message_e, message_g);
  ASSERT_EQ(message_f, message_g);
}

TEST(setup, buffer_size) {
  std::shared_ptr<TestNode> node(labrat::robot::Manager::get().addNode<TestNode>("node_a"));

  ASSERT_THROW(node->addTestReceiver<TestMessage>("/testing", 1), labrat::robot::Exception);
}

}  // namespace labrat::robot::test
