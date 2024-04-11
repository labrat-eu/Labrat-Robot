#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>
#include <labrat/lbot/utils/performance.hpp>

#include <vector>

#include <examples/msg/array.fb.hpp>

// Moves
//
// When transferring large amounts of data between nodes the overhead due to copy operations can take up considerable resources.
// If just one receiver or plugin is receiving messages from a topic this overhead can be avoided.
// With the help of move semantics the messages can be efficiently moved between the sender and the single receiver or plugin.
// No copy operation is taking place in this scenario.
// Move semantics are very efficient when the bulk of the transferrable data is located in the heap.
// Within lbot this is true for any large vectors defined in flatbuffer messages.
// These can typically be found in images, videos, pointclouds etc.
// Careful use of move functions can improve the performance of your project considerably.
//
// In this example we will extend the 08-conversions example to work with move functions.

class ConversionMessage : public lbot::MessageBase<examples::msg::Array, std::vector<float>> {
public:
  // Convert an array message to a vector.
  static void convertTo(const Storage &source, std::vector<float> &destination) {
    destination = source.values;
  }

  // Convert a vector to an array message.
  static void convertFrom(const std::vector<float> &source, Storage &destination) {
    destination.values = source;
  }

  // Move an array message to a vector.
  static void moveTo(Storage &&source, std::vector<float> &destination) {
    destination = std::forward<std::vector<float>>(source.values);
  }

  // Move a vector to an array message.
  static void moveFrom(std::vector<float> &&source, Storage &destination) {
    destination.values = std::forward<std::vector<float>>(source);
  }
};

class SenderNode : public lbot::Node {
public:
  SenderNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a sender with the previously defined move function.
    sender = addSender<ConversionMessage>("/examples/vector");

    sender_thread = utils::TimerThread(&SenderNode::senderFunction, std::chrono::seconds(1), "sender_thread", 1, this);
  }

private:
  void senderFunction() {
    std::vector<float> data;
    data.resize(10000000L);

    {
      utils::TimerTrace<std::chrono::microseconds> trace("Sender::put()", getLogger());
      sender->put(data);
    }

    {
      utils::TimerTrace<std::chrono::microseconds> trace("Sender::move()", getLogger());
      sender->move(std::move(data));
    }

    // 'data' may no longer be used.
  }

  Sender<ConversionMessage>::Ptr sender;
  utils::TimerThread sender_thread;
};

class ReceiverNode : public lbot::Node {
public:
  ReceiverNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a receiver with the previously defined move function.
    receiver = addReceiver<ConversionMessage>("/examples/vector");

    receiver_thread = utils::TimerThread(&ReceiverNode::receiverFunction, std::chrono::seconds(1), "receiver_thread", 1, this);
  }

private:
  void receiverFunction() {
    // Receiver::next() might throw an exception if the topic has been flushed.
    try {
      std::vector<float> data;

      {
        utils::TimerTrace<std::chrono::microseconds> trace("Receiver::next()", getLogger());
        data = receiver->next();
      }

      getLogger().logInfo() << "Received vector of size: " << data.size();
    } catch (lbot::TopicNoDataAvailableException &) {
    }
  }

  Receiver<ConversionMessage>::Ptr receiver;
  utils::TimerThread receiver_thread;
};

int main(int argc, char **argv) {
  lbot::Logger::setLogLevel(lbot::Logger::Verbosity::debug);

  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  manager->addNode<SenderNode>("sender");
  manager->addNode<ReceiverNode>("receiver");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
