#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(setup, latest) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

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
  ASSERT_THROW(node_b->receiver->latest(), labrat::lbot::TopicNoDataAvailableException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(setup, next) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

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
  ASSERT_THROW(node_b->receiver->next(), labrat::lbot::TopicNoDataAvailableException);

  TestContainer message_h;
  node_a->sender->put(message_h);

  TestContainer message_i = node_b->receiver->next();

  ASSERT_THROW(node_b->receiver->latest(), labrat::lbot::TopicNoDataAvailableException);

  TestContainer message_j;
  node_a->sender->put(message_j);

  ASSERT_NO_THROW(node_b->receiver->latest());

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(setup, move) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  static const std::size_t size = 10000000L;
  std::vector<u8> local_buffer;
  local_buffer.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    local_buffer.emplace_back(static_cast<u8>(i));
  }

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;
  message_a.buffer = local_buffer;

  node_a->sender->put(std::move<TestContainer &>(message_a));

  ASSERT_TRUE(message_a.buffer.empty());

  TestContainer message_b = node_b->receiver->next();

  ASSERT_NE(message_a, message_b);

  message_a.buffer = local_buffer;
  ASSERT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(setup, callback) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  TestContainer message_b;

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main", nullptr, &message_b));

  auto callback = [](TestNode::GenericReceiver<TestContainer> &receiver, TestContainer *message) -> void {
    *message = receiver.next();
  };

  void (*ptr)(TestNode::GenericReceiver<TestContainer> &, TestContainer *) = callback;

  node_b->receiver->setCallback(ptr);

  TestContainer message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;

  node_a->sender->put(message_a);

  ASSERT_EQ(message_a, message_b);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

TEST(setup, server) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "main", "void"));
  std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "void", "main"));

  TestContainer result;
  u64 counter = 0;

  auto handler = [](const TestContainer &request, u64 *user_ptr) -> TestContainer {
    ++(*user_ptr);

    TestContainer response;
    response.float_field = 10 * request.float_field;
    return response;
  };

  TestContainer (*ptr)(const TestContainer &, u64 *) = handler;

  Node::Server<TestMessageConv, TestMessageConv>::Ptr server =
    node_a->addServer<TestMessageConv, TestMessageConv>("test_service", ptr, &counter);
  Node::Client<TestMessageConv, TestMessageConv>::Ptr client = node_b->addClient<TestMessageConv, TestMessageConv>("test_service");

  TestContainer request;
  request.float_field = 10.5;
  result = client->callSync(request);

  ASSERT_EQ(result.float_field, 105);
  ASSERT_EQ(counter, 1);

  server.reset(nullptr);
  ASSERT_THROW(client->callSync(request), labrat::lbot::ServiceUnavailableException);

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(manager->removeNode("node_b"));
}

}  // namespace lbot::test
}  // namespace labrat
