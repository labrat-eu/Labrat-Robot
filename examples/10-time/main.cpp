#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/plugins/gazebo-time/time.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

// Time
//
// Plugins allow you to globally access messages.
// This is useful if you want to record messages for later analysis or if you want to debug you program while it is running.
//
// In this example we will use some existing plugins that might help you in your projects.

class ExampleNode : public lbot::Node {
public:
  ExampleNode() {
    thread = lbot::TimerThread(&ExampleNode::loopFunction, std::chrono::milliseconds(100), "sender_thread", 1, this);
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

  // Register external time source plugin.
  manager->addPlugin<lbot::plugins::GazeboTimeSource>("gazebo");

  manager->addPlugin<lbot::plugins::McapRecorder>("mcap");
  manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");

  manager->addNode<ExampleNode>("example");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = lbot::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
