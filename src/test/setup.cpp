#include <labrat/robot/manager.hpp>
#include <labrat/robot/test/helper.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(setup, latest) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  node_a->sender->put(message_a);

  TestContainer message_b = node_b->receiver->latest();

  ASSERT_EQ(message_a, message_b);

  TestContainer message_c;
  message_c.integral_field = 5;
  message_c.float_field = 10.0;

  TestContainer message_d = node_b->receiver->latest();

  node_a->sender->put(message_c);

  ASSERT_EQ(message_a, message_d);
  ASSERT_EQ(message_b, message_d);
  ASSERT_NE(message_c, message_d);

  TestContainer message_e = node_b->receiver->latest();

  ASSERT_NE(message_a, message_e);
  ASSERT_NE(message_b, message_e);
  ASSERT_EQ(message_c, message_e);

  node_a->sender->flush();
  ASSERT_THROW(node_b->receiver->latest(), labrat::robot::TopicNoDataAvailableException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

TEST(setup, next) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  node_a->sender->put(message_a);

  TestContainer message_b = node_b->receiver->next();

  ASSERT_EQ(message_a, message_b);

  TestContainer message_c;
  message_c.integral_field = 5;
  message_c.float_field = 10.0;

  TestContainer message_d;
  auto receiver_lambda = [&message_d, &node_b]() {
    message_d = node_b->receiver->next();
  };

  std::thread receiver_thread(receiver_lambda);

  node_a->sender->put(message_c);

  receiver_thread.join();

  ASSERT_NE(message_a, message_d);
  ASSERT_NE(message_b, message_d);
  ASSERT_EQ(message_c, message_d);

  TestContainer message_e;
  message_e.integral_field = 100;
  node_a->sender->put(message_e);

  TestContainer message_f;
  message_f.integral_field = 101;
  node_a->sender->put(message_f);

  TestContainer message_g = node_b->receiver->next();

  ASSERT_NE(message_e, message_g);
  ASSERT_EQ(message_f, message_g);

  node_a->sender->flush();
  ASSERT_THROW(node_b->receiver->next(), labrat::robot::TopicNoDataAvailableException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

TEST(setup, callback) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  TestContainer message_b;

  auto callback = [](TestNode::ContainerReceiver<TestContainer> &receiver, TestContainer *message) -> void {
    *message = receiver.next();
  };

  void (*ptr)(TestNode::ContainerReceiver<TestContainer> &, TestContainer *) = callback;

  node_b->receiver->setCallback(ptr, &message_b);

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  node_a->sender->put(message_a);

  ASSERT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

TEST(setup, server) {
  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "void", "main"));

  u64 result = 0;
  u64 counter = 0;

  auto handler = [](const double &request, u64 *user_ptr) -> u64 {
    ++(*user_ptr);

    return 10 * request;
  };

  u64 (*ptr)(const double &, u64 *) = handler;

  Node::Server<double, u64>::Ptr server = node_a->addTestServer<double, u64>("test_service", ptr, &counter);
  Node::Client<double, u64>::Ptr client = node_b->addTestClient<double, u64>("test_service");

  result = client->callSync(10.5);

  ASSERT_EQ(result, 105);
  ASSERT_EQ(counter, 1);

  server.reset(nullptr);
  ASSERT_THROW(client->callSync(10.5), labrat::robot::ServiceUnavailableException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

}  // namespace labrat::robot::test