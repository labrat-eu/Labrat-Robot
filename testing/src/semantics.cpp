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
  Node::Sender<TestContainer>::Ptr sender_c = node->addSender<TestContainer>("topic_c");

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
  Node::Receiver<TestContainer>::Ptr receiver_c = node->addReceiver<TestContainer>("topic_c");
  
  TestMessage message_raw;
  message_raw = receiver_a->latest();
  message_raw = receiver_b->latest();

  TestContainer message_container;
  message_container = receiver_c->latest();
}

}  // namespace lbot::test
}  // namespace labrat
