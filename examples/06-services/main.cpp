#include <labrat/robot/node.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/utils/thread.hpp>
#include <labrat/robot/utils/signal.hpp>

#include <examples/msg/request.fb.hpp>
#include <examples/msg/response.fb.hpp>

#include <cmath>

using namespace labrat;

// Services
//
// Services provide a mechanism to pass messages between nodes that require an immediate answer.
//
// In this example we will construct two nodes, one to send requests and one to answer them.
// Classes and code patterns are showcased.

class ServerNode : public robot::Node {
public:
  ServerNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    // Register a server on the service with the name "/examples/power" and the handler ServerNode::handleRequest().
    // There can only be one server per service.
    // The type of this server must match any previously registered client on the same service.
    server = addServer<robot::Message<examples::msg::Request>, robot::Message<examples::msg::Response>>("/examples/power", &ServerNode::handleRequest);
  }

private:
  // When a request has been made, this function will be called to respond.
  // Handler functions must be static.
  static robot::Message<examples::msg::Response> handleRequest(const robot::Message<examples::msg::Request> &request, void *) {
    // Construct a response message.
    robot::Message<examples::msg::Response> result;
    result().answer = std::pow(request().base, request().exponent);

    return result;
  }

  // A service consists of two messages.
  // One is the request sent from the client to the server.
  // The other is the response sent from the server to the client.
  Server<robot::Message<examples::msg::Request>, robot::Message<examples::msg::Response>>::Ptr server;
};

class ClientNode : public robot::Node {
public:
  ClientNode(const robot::Node::Environment &environment) : robot::Node(environment) {
    // Register a client on the service with the name "/examples/power".
    // The type of this client must match any previously registered server or client on the same service.
    client = addClient<robot::Message<examples::msg::Request>, robot::Message<examples::msg::Response>>("/examples/power");

    client_thread = utils::TimerThread(&ClientNode::clientFunction, std::chrono::seconds(1), "receiver_thread", 1, this);
  }

private:
  void clientFunction() {
    // Construct a request message.
    robot::Message<examples::msg::Request> request;
    request().base = 2;
    request().exponent = ++e;

    // Client::callSync() might throw an exception.
    try {
      // Make a blocking call to the service with a timeout of 1 second.
      // The function returns once a response has been received.
      // If no response has been received after the specified timeout, an exception will be thrown.
      // Alternatively a non-blocking call to Client::callAsync() could be made.
      robot::Message<examples::msg::Response> response = client->callSync(request, std::chrono::seconds(1));

      getLogger().logInfo() << "Received response: " << response().answer;
    } catch (robot::ServiceUnavailableException &) {
    } catch (robot::ServiceTimeoutException &) {
      getLogger().logWarning() << "Failed to reach service, trying again.";
    }
  }

  Client<robot::Message<examples::msg::Request>, robot::Message<examples::msg::Response>>::Ptr client;
  utils::TimerThread client_thread;

  float e = 0;
};

int main(int argc, char **argv) {
  robot::Logger logger("main");
  robot::Manager::Ptr manager = robot::Manager::get();

  manager->addNode<ServerNode>("server");
  manager->addNode<ClientNode>("client");

  logger.logInfo() << "Press CTRL+C to exit the program.";

  int signal = utils::signalWait();
  logger.logInfo() << "Caught signal (" << signal << "), shutting down.";

  return 0;
}
