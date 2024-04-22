#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/plugins/mcap/recorder.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cmath>

#include <examples/msg/vector.fb.hpp>

// Plugins
//
// Plugins allow you to globally access messages.
// This is useful if you want to record messages for later analysis or if you want to debug you program while it is running.
//
// In this example we will use some existing plugins that might help you in your projects.

class SenderNode : public lbot::UniqueNode {
public:
  SenderNode(const lbot::NodeEnvironment &environment) : lbot::UniqueNode(environment, "sender") {
    sender = addSender<lbot::Message<examples::msg::Vector>>("/examples/number");

    sender_thread = utils::TimerThread(&SenderNode::senderFunction, std::chrono::milliseconds(50), "sender_thread", 1, this);
  }

private:
  void senderFunction() {
    ++i;

    lbot::Message<examples::msg::Vector> message;
    message.x = std::sin(i / 100.0);
    message.y = std::cos(i / 100.0);

    sender->put(message);
  }

  Sender<lbot::Message<examples::msg::Vector>>::Ptr sender;
  utils::TimerThread sender_thread;

  uint64_t i = 0;
};

class ServerNode : public lbot::UniqueNode {
public:
  ServerNode(const lbot::NodeEnvironment &environment) : lbot::UniqueNode(environment, "server") {
    // Register a server on the service with the name "/examples/power" and the handler ServerNode::handleRequest().
    // There can only be one server per service.
    // The type of this server must match any previously registered client on the same service.
    server = addServer<examples::msg::Vector, examples::msg::Vector>("/examples/mirror", &ServerNode::handleRequest);
  }

private:
  // When a request has been made, this function will be called to respond.
  // Handler functions must be static.
  static lbot::Message<examples::msg::Vector> handleRequest(const lbot::Message<examples::msg::Vector> &request, void *) {
    // Construct a response message.
    lbot::Message<examples::msg::Vector> response;
    response.x = request.y;
    response.y = request.x;

    return response;
  }

  // A service consists of two messages.
  // One is the request sent from the client to the server.
  // The other is the response sent from the server to the client.
  Server<examples::msg::Vector, examples::msg::Vector>::Ptr server;
};

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // Register some existing plugins.

  // The McapRecorder plugin allows you to trace messages to a mcap file.
  // If you want to see what this program has been doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a local file with the path trace.mcap
  manager->addPlugin<lbot::plugins::McapRecorder>();

  // The FoxgloveServer plugin allows you to trace messages live via Foxglove Studio.
  // If you want to see what this program is doing, you might:
  //  1. Install and open Foxglove Studio (https://foxglove.dev/)
  //  2. Open a connection via Foxglove WebSocket with the URL ws://localhost:8765
  manager->addPlugin<lbot::plugins::FoxgloveServer>();

  manager->addNode<SenderNode>();
  manager->addNode<ServerNode>();

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
