#include <labrat/robot/msg/network.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/test/helper.hpp>
#include <test.pb.h>

#include <gtest/gtest.h>

namespace labrat::robot::test {

class NetworkTest : public ::testing::Test {
public:
  NetworkTest() {
    node_a = labrat::robot::Manager::get().addNode<TestNode>("node_a").lock();
    node_b = labrat::robot::Manager::get().addNode<TestNode>("node_b").lock();

    sender = node_a->addTestSender<msg::Network<MessageA>>("/protobuf");
    receiver = node_b->addTestReceiver<msg::Network<MessageA>>("/protobuf");
  }

protected:
  std::shared_ptr<TestNode> node_a;
  std::shared_ptr<TestNode> node_b;

  TestNode::Sender<msg::Network<MessageA>>::Ptr sender;
  TestNode::Receiver<msg::Network<MessageA>>::Ptr receiver;
};

TEST_F(NetworkTest, build) {
  msg::Network<MessageA> message;

  static const u32 id = 42;
  message().set_id(id);

  EXPECT_EQ(message().id(), id);
}

TEST_F(NetworkTest, latest) {
  msg::Network<MessageA> message_a;

  static const u32 id = 42;
  message_a().set_id(id);

  sender->put(message_a);
  msg::Network<MessageA> message_b = receiver->latest();

  EXPECT_EQ(message_b().id(), id);
}

}  // namespace labrat::robot::test
