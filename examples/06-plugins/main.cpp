#include <labrat/robot/node.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/utils/thread.hpp>
#include <labrat/robot/utils/signal.hpp>
#include <labrat/robot/plugins/foxglove-ws/server.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>

#include <examples/msg/vector.fb.hpp>

#include <cmath>

using namespace labrat;

// Plugins
//
// Plugins allow you to globally access messages.
// This is useful if you want to record messages for later analysis or if you want to debug you program while it is running.
// 
// In this example we will use some existing plugins that might help you in your projects.

class SenderNode : public robot::Node {
public:
  SenderNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    sender = addSender<robot::Message<examples::msg::Vector>>("/examples/number");

    sender_thread = utils::TimerThread(&SenderNode::senderFunction, std::chrono::milliseconds(50), "sender_thread", 1, this);
  }

private:
  void senderFunction() {
    ++i;

    robot::Message<examples::msg::Vector> message;
    message.x = std::sin(i / 100.0);
    message.y = std::cos(i / 100.0);

    sender->put(message);
  }

  Sender<robot::Message<examples::msg::Vector>>::Ptr sender;
  utils::TimerThread sender_thread;

  uint64_t i = 0;
};

int main(int argc, char **argv) {
  robot::Logger logger("main");
  robot::Manager::Ptr manager = robot::Manager::get();

  // Register some existing plugins.

  // The McapRecorder plugin allows you to trace messages to a mcap file.
  // If you want to see what this program has been doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a local file with the path trace.mcap
  std::unique_ptr<labrat::robot::plugins::McapRecorder> mcap_plugin = std::make_unique<labrat::robot::plugins::McapRecorder>("trace.mcap");

  // The FoxgloveServer plugin allows you to trace messages live via Foxglove Studio.
  // If you want to see what this program is doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a connection via Foxglove WebSocket with the URL ws://localhost:8765
  std::unique_ptr<labrat::robot::plugins::FoxgloveServer> foxglove_plugin = std::make_unique<labrat::robot::plugins::FoxgloveServer>("Example Server", 8765);

  manager->addNode<SenderNode>("sender");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
