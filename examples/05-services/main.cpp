#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/utils/signal.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cmath>

// Services
//
// Services provide a mechanism to pass messages between nodes that require an immediate answer.
//
// In this example we will construct two nodes, one to send requests and one to answer them.
// Classes and code patterns are showcased.

struct Request {
  double base;
  double exponent;
};

using Response = double;

class ServerNode : public lbot::Node {
public:
  ServerNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a server on the service with the name "/examples/power" and the handler ServerNode::handleRequest().
    // There can only be one server per service.
    // The type of this server must match any previously registered client on the same service.
    server = addServer<Request, Response>("/examples/power", &ServerNode::handleRequest);
  }

private:
  // When a request has been made, this function will be called to respond.
  // Handler functions must be static.
  static Response handleRequest(const Request &request, void *) {
    // Construct a response message.
    return std::pow(request.base, request.exponent);
  }

  // A service consists of two messages.
  // One is the request sent from the client to the server.
  // The other is the response sent from the server to the client.
  Server<Request, Response>::Ptr server;
};

class ClientNode : public lbot::Node {
public:
  ClientNode(const lbot::Node::Environment &environment) : lbot::Node(environment) {
    // Register a client on the service with the name "/examples/power".
    // The type of this client must match any previously registered server or client on the same service.
    client = addClient<Request, Response>("/examples/power");

    client_thread = utils::TimerThread(&ClientNode::clientFunction, std::chrono::seconds(1), "receiver_thread", 1, this);
  }

private:
  void clientFunction() {
    // Construct a request message.
    Request request = {.base = 2, .exponent = ++e};

    // Client::callSync() might throw an exception.
    try {
      // Make a blocking call to the service with a timeout of 1 second.
      // The function returns once a response has been received.
      // If no response has been received after the specified timeout, an exception will be thrown.
      // Alternatively a non-blocking call to Client::callAsync() could be made.
      Response response = client->callSync(request, std::chrono::seconds(1));

      getLogger().logInfo() << "Received response: " << response;
    } catch (lbot::ServiceUnavailableException &) {
    } catch (lbot::ServiceTimeoutException &) {
      getLogger().logWarning() << "Failed to reach service, trying again.";
    }
  }

  Client<Request, Response>::Ptr client;
  utils::TimerThread client_thread;

  float e = 0;
};

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  manager->addNode<ServerNode>("server");
  manager->addNode<ClientNode>("client");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
