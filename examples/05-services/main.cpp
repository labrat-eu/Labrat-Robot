#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cmath>

#include <examples/msg/request.fb.hpp>
#include <examples/msg/response.fb.hpp>

// Services
//
// Services provide a mechanism to pass messages between nodes that require an immediate answer.
//
// In this example we will construct two nodes, one to send requests and one to answer them.
// Classes and code patterns are showcased.

class ServerNode : public lbot::UniqueNode {
public:
  ServerNode(const lbot::NodeEnvironment &environment) : lbot::UniqueNode(environment, "server") {
    // Register a server on the service with the name "/examples/power" and the handler ServerNode::handleRequest().
    // There can only be one server per service.
    // The type of this server must match any previously registered client on the same service.
    server = addServer<examples::msg::Request, examples::msg::Response>("/examples/power", &ServerNode::handleRequest);
  }

private:
  // When a request has been made, this function will be called to respond.
  // Handler functions must be static.
  static lbot::Message<examples::msg::Response> handleRequest(const lbot::Message<examples::msg::Request> &request, void *) {
    // Construct a response message.
    lbot::Message<examples::msg::Response> response;
    response.result = std::pow(request.base, request.exponent);

    return response;
  }

  // A service consists of two messages.
  // One is the request sent from the client to the server.
  // The other is the response sent from the server to the client.
  Server<examples::msg::Request, examples::msg::Response>::Ptr server;
};

class ClientNode : public lbot::UniqueNode {
public:
  ClientNode(const lbot::NodeEnvironment &environment) : lbot::UniqueNode(environment, "client") {
    // Register a client on the service with the name "/examples/power".
    // The type of this client must match any previously registered server or client on the same service.
    client = addClient<examples::msg::Request, examples::msg::Response>("/examples/power");

    client_thread = utils::TimerThread(&ClientNode::clientFunction, std::chrono::seconds(1), "receiver_thread", 1, this);
  }

private:
  void clientFunction() {
    // Construct a request message.
    lbot::Message<examples::msg::Request> request;
    request.base = 2;
    request.exponent = ++e;

    // Client::callSync() might throw an exception.
    try {
      // Make a blocking call to the service with a timeout of 1 second.
      // The function returns once a response has been received.
      // If no response has been received after the specified timeout, an exception will be thrown.
      // Alternatively a non-blocking call to Client::callAsync() could be made.
      lbot::Message<examples::msg::Response> response = client->callSync(request, std::chrono::seconds(1));

      getLogger().logInfo() << "Received response: " << response.result;
    } catch (lbot::ServiceUnavailableException &) {
    } catch (lbot::ServiceTimeoutException &) {
      getLogger().logWarning() << "Failed to reach service, trying again.";
    }
  }

  Client<examples::msg::Request, examples::msg::Response>::Ptr client;
  utils::TimerThread client_thread;

  float e = 0;
};

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  manager->addNode<ServerNode>();
  manager->addNode<ClientNode>();

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
