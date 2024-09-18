#include <labrat/lbot/manager.hpp>

#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

static void testCallback(const Message<TestFlatbuffer> &message, int *num)
{
  ++(*num);
}

class MultisetupTest : public LbotTest
{};

TEST_F(MultisetupTest, const_move)
{
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::shared_ptr<TestNode> node_sender(manager->addNode<TestNode>("node_sender"));
  std::shared_ptr<TestNode> node_receiver(manager->addNode<TestNode>("node_receiver"));

  Node::Sender<TestFlatbuffer>::Ptr sender = node_sender->addSender<TestFlatbuffer>("/topic");
  Node::Receiver<TestFlatbuffer>::Ptr receiver_normal = node_receiver->addReceiver<TestFlatbuffer>("/topic");
  Node::Receiver<Message<const TestFlatbuffer>>::Ptr receiver_const1 = node_receiver->addReceiver<Message<const TestFlatbuffer>>("/topic");
  Node::Receiver<Message<const TestFlatbuffer>>::Ptr receiver_const2 = node_receiver->addReceiver<Message<const TestFlatbuffer>>("/topic");

  int c1 = 0;
  int c2 = 0;

  receiver_const1->setCallback(&testCallback, &c1);
  receiver_const2->setCallback(&testCallback, &c2);

  Message<TestFlatbuffer> message_a;
  message_a.integral_field = 10;
  message_a.float_field = 5.0;
  sender->put(message_a);

  EXPECT_EQ(1, c1);
  EXPECT_EQ(1, c2);

  Message<TestFlatbuffer> message_b;
  message_b.integral_field = 20;
  message_b.float_field = 25.0;
  sender->put(std::move(message_b));

  EXPECT_EQ(2, c1);
  EXPECT_EQ(2, c2);

  Message<TestFlatbuffer> message_c = receiver_normal->latest();
  EXPECT_EQ(20, message_c.integral_field);
  EXPECT_EQ(25.0, message_c.float_field);
}

}  // namespace lbot::test
}  // namespace labrat
