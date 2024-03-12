#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cmath>

#include <examples/msg/number.fb.hpp>

// Conversions
//
// Usually, the types used to pass messages (flatbuffers) are not used internally by a program to perform computations.
// For instance, you might use a separate linear-algebra library to work compute vector operations.
// Such libraries normally come with their own data types.
// You might find yourself manually converting between flatbuffer messages and other types all over your code.
// To make this easier, you can register custom conversion functions with your senders and receivers.
// Calls to put(), latest() and next() will then expect/return the type you've specified in the conversion function.
// This leads to an overall cleaner code style, as the focus in now on the actual computations you perform instead of the type conversions.
//
// In this example we will extend the 04-topics example to work with conversion functions.

// Convert a number message to a float.
void convertMessageToNumber(const lbot::Message<examples::msg::Number> &source, float &destination, const void *) {
  destination = source.value;
}

// Convert a float to a number message.
void convertNumberToMessage(const float &source, lbot::Message<examples::msg::Number> &destination, const void *) {
  destination.value = source;
}

class SenderNode : public lbot::Node {
public:
  SenderNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a sender with the previously defined conversion function.
    sender = addSender<lbot::Message<examples::msg::Number>, float>("/examples/number", &convertNumberToMessage);

    sender_thread = utils::TimerThread(&SenderNode::senderFunction, std::chrono::seconds(1), "sender_thread", 1, this);
  }

private:
  void senderFunction() {
    // Send the data directly without having to construct a message first.
    sender->put(std::sin(++i / 100.0));
  }

  Sender<lbot::Message<examples::msg::Number>, float>::Ptr sender;
  utils::TimerThread sender_thread;

  uint64_t i = 0;
};

class ReceiverNode : public lbot::Node {
public:
  ReceiverNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a receiver with the previously defined conversion function.
    receiver = addReceiver<lbot::Message<examples::msg::Number>, float>("/examples/number", &convertMessageToNumber);

    receiver_thread = utils::TimerThread(&ReceiverNode::receiverFunction, std::chrono::seconds(1), "receiver_thread", 1, this);
  }

private:
  void receiverFunction() {
    // Receiver::latest() might throw an exception if no data is available.
    try {
      // Process the data directly without having to deconstruct a message first.
      getLogger().logInfo() << "Received message: " << receiver->latest();
    } catch (lbot::TopicNoDataAvailableException &) {
    }
  }

  Receiver<lbot::Message<examples::msg::Number>, float>::Ptr receiver;
  utils::TimerThread receiver_thread;
};

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  manager->addNode<SenderNode>("sender");
  manager->addNode<ReceiverNode>("receiver");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
