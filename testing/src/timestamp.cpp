#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

static void testCallback(const Message<TestFlatbuffer> &message, int *num) {
  ++(*num);
}

class TimestampTest : public LbotTest {};

TEST_F(TimestampTest, latest) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_sender(manager->addNode<TestNode>("node_sender"));
  std::shared_ptr<TestNode> node_receiver(manager->addNode<TestNode>("node_receiver"));

  Node::Sender<TestFlatbuffer>::Ptr sender = node_sender->addSender<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node_receiver->addReceiver<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_b = node_receiver->addReceiver<TestFlatbuffer>("/topic");

  Message<TestFlatbuffer> message_sender;
  sender->put(message_sender);

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_a = receiver_a->latest();

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_b = receiver_b->latest();

  EXPECT_EQ(message_sender.getTimestamp(), message_a.getTimestamp());
  EXPECT_EQ(message_sender.getTimestamp(), message_b.getTimestamp());
}

TEST_F(TimestampTest, next) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_sender(manager->addNode<TestNode>("node_sender"));
  std::shared_ptr<TestNode> node_receiver(manager->addNode<TestNode>("node_receiver"));

  Node::Sender<TestFlatbuffer>::Ptr sender = node_sender->addSender<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node_receiver->addReceiver<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_b = node_receiver->addReceiver<TestFlatbuffer>("/topic");

  Message<TestFlatbuffer> message_sender;
  sender->put(message_sender);

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_a = receiver_a->next();

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_b = receiver_b->next();

  EXPECT_EQ(message_sender.getTimestamp(), message_a.getTimestamp());
  EXPECT_EQ(message_sender.getTimestamp(), message_b.getTimestamp());
}

TEST_F(TimestampTest, move) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_sender(manager->addNode<TestNode>("node_sender"));
  std::shared_ptr<TestNode> node_receiver(manager->addNode<TestNode>("node_receiver"));

  Node::Sender<TestFlatbuffer>::Ptr sender = node_sender->addSender<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_a = node_receiver->addReceiver<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_b = node_receiver->addReceiver<TestFlatbuffer>("/topic");

  Message<TestFlatbuffer> message_sender;
  sender->put(std::move(message_sender));

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_a = receiver_a->next();

  lbot::Thread::sleepFor(std::chrono::milliseconds(10));

  Message<TestFlatbuffer> message_b = receiver_b->next();

  EXPECT_EQ(message_sender.getTimestamp(), message_a.getTimestamp());
  EXPECT_EQ(message_sender.getTimestamp(), message_b.getTimestamp());
}

}  // namespace lbot::test
}  // namespace labrat
