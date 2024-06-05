#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/msg/timestamp.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <chrono>

// Time
//
// There exist multiple feasable time sources to synchronize a robotics program.
//
// In this example we will use the gazebo-time plugin to showcase the custom time mode.

class TimeNode : public lbot::Node {
public:
  TimeNode() {
    sender_time = addSender<lbot::Timestamp>("/time");
    thread = lbot::LoopThread(&TimeNode::loopFunction, "time_thread", 1, this);
  }

private:
  void loopFunction() {
    lbot::Message<lbot::Timestamp> message;
    message.value = std::make_unique<foxglove::Time>(++i, 0);
    sender_time->put(message);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  Sender<lbot::Timestamp>::Ptr sender_time;
  lbot::LoopThread thread;

  uint64_t i = 0;
};

class ExampleNode : public lbot::Node {
public:
  ExampleNode() {
    thread = lbot::TimerThread(&ExampleNode::loopFunction, std::chrono::seconds(1), "sender_thread", 1, this);
  }

private:
  void loopFunction() {
    getLogger().logInfo() << "Test message (" << i++ << ")";
  }

  lbot::TimerThread thread;

  uint64_t i = 0;
};

int main(int argc, char **argv) {
  // Specify that we use an external time source.
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", "custom");

  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // Register custom time source node.
  manager->addNode<TimeNode>("time");

  manager->addPlugin<lbot::plugins::McapRecorder>("mcap");
  manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");

  manager->addNode<ExampleNode>("example");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
