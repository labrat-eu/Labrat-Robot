#include <labrat/robot/node.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/utils/thread.hpp>
#include <labrat/robot/utils/signal.hpp>

#include <examples/msg/numbers.fb.hpp>

#include <cmath>

using namespace labrat;

// Topics
//
// Topics provide a mechanism to pass messages between nodes.
//
// In this example we will construct two nodes, one to send messages and one to receive them.
// Classes and code patterns are showcased.

class SenderNode : public robot::Node {
public:
  SenderNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    // Register a sender on the topic with the name "/examples/numbers".
    // There can only be one sender per topic.
    // The type of this sender must match any previously registered receiver on the same topic.
    sender = addSender<robot::Message<examples::msg::Numbers>>("/examples/numbers");

    sender_thread = utils::TimerThread(&SenderNode::senderFunction, std::chrono::seconds(1), "sender_thread", 1, this);
  }

private:
  void senderFunction() {
    // Construct a message.
    robot::Message<examples::msg::Numbers> message;
    message().iteration = ++i;
    message().value = std::sin(i / 100.0);

    // Send the message.
    sender->put(message);
  }

  Sender<robot::Message<examples::msg::Numbers>>::Ptr sender;
  utils::TimerThread sender_thread;

  uint64_t i = 0;
};

class ReceiverNode : public robot::Node {
public:
  ReceiverNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    // Register a receiver on the topic with the name "/examples/numbers".
    // The type of this receiver must match any previously registered sender and receiver on the same topic.
    receiver = addReceiver<robot::Message<examples::msg::Numbers>>("/examples/numbers");

    receiver_thread = utils::LoopThread(&ReceiverNode::receiverFunction, "receiver_thread", 1, this);
  }

private:
  void receiverFunction() {
    // Receiver::next() might throw an exception if no data is available.
    try {
      // Receive a message.
      // Receiver::next() will block until there is a new message to be received.
      // If an active sender has been deregistered it will also unblock and throw an exception.
      // Alternatively Receiver::latest() could also be used if a non-blocking call is required.
      // However, Receiver::latest() does not guarantee that the returned message has not already been processed.
      // The recommendation is to only use one call to Receiver::next() per thread.
      robot::Message<examples::msg::Numbers> message = receiver->next();

      getLogger().logInfo() << "Received message: " << message().value;
    } catch (robot::TopicNoDataAvailableException &) {}
  }

  Receiver<robot::Message<examples::msg::Numbers>>::Ptr receiver;
  utils::LoopThread receiver_thread;
};

int main(int argc, char **argv) {
  robot::Logger logger("main");
  robot::Manager::Ptr manager = robot::Manager::get();

  manager->addNode<SenderNode>("sender");
  manager->addNode<ReceiverNode>("receiver");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
