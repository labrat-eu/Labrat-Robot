#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(DISABLED_semantics, sender) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Sender<TestFlatbuffer>::Ptr sender_a = node->addSender<TestFlatbuffer>("topic_a");
  Node::Sender<TestMessage>::Ptr sender_b = node->addSender<TestMessage>("topic_b");
  Node::Sender<TestMessageConv>::Ptr sender_c = node->addSender<TestMessageConv>("topic_c");

  TestMessage message_raw;
  sender_a->put(message_raw);
  sender_b->put(message_raw);

  TestContainer message_container;
  sender_c->put(message_container);
}

TEST(DISABLED_semantics, receiver) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node->addReceiver<TestFlatbuffer>("topic_a");
  Node::Receiver<TestMessage>::Ptr receiver_b = node->addReceiver<TestMessage>("topic_b");
  Node::Receiver<TestMessageConv>::Ptr receiver_c = node->addReceiver<TestMessageConv>("topic_c");

  TestMessage message_raw;
  message_raw = receiver_a->latest();
  message_raw = receiver_b->latest();

  TestContainer message_container;
  message_container = receiver_c->latest();
}

TEST(DISABLED_semantics, server) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  auto handler_a = [](const TestMessage &request, u64 *user_ptr) -> TestMessage {
    return TestMessage();
  };

  TestMessage (*ptr_a)(const TestMessage &, u64 *) = handler_a;

  //Node::Server<TestFlatbuffer, TestFlatbuffer>::Ptr server_a = node->addServer<TestFlatbuffer, TestFlatbuffer>("service_a", ptr_a);
  //Node::Server<TestMessage>::Ptr server_b = node->addReceiver<TestMessage, TestFlatbuffer>, ("service_b");
  //Node::Server<TestMessageConv>::Ptr server_c = node->addReceiver<TestMessageConv, TestFlatbuffer>("service_c");

  auto handler_d = [](const TestContainer &request, u64 *user_ptr) -> TestContainer {
    return TestContainer();
  };

  TestContainer (*ptr_d)(const TestContainer &, u64 *) = handler_d;

  Node::Server<TestMessageConv, TestMessageConv>::Ptr server_d = node->addServer<TestMessageConv, TestMessageConv>("service_d", ptr_d);
}

}  // namespace lbot::test
}  // namespace labrat
