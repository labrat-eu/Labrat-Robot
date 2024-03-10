#include <helper.hpp>

#include <labrat/robot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(semantics, sender) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Sender<TestFlatbuffer>::Ptr sender_a = node->addSender<TestFlatbuffer>("topic_a");
  Node::Sender<TestMessage>::Ptr sender_b = node->addSender<TestMessage>("topic_b");
  Node::Sender<TestContainer>::Ptr sender_c = node->addSender<TestContainer>("topic_c");
}

TEST(semantics, receiver) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node->addReceiver<TestFlatbuffer>("topic_a");
  Node::Receiver<TestMessage>::Ptr receiver_b = node->addReceiver<TestMessage>("topic_b");
  Node::Receiver<TestContainer>::Ptr receiver_c = node->addReceiver<TestContainer>("topic_c");
}

TEST(semantics, server) {
  labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

  std::shared_ptr<TestNode> node(manager->addNode<TestNode>("node"));

  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node->addReceiver<TestFlatbuffer>("topic_a");
  Node::Receiver<TestMessage>::Ptr receiver_b = node->addReceiver<TestMessage>("topic_b");
  Node::Receiver<TestContainer>::Ptr receiver_c = node->addReceiver<TestContainer>("topic_c");
}

}  // namespace labrat::robot::test
