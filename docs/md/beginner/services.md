@page services Services

@note
You may also want to take a look at the [code example](@ref example_services).

# Basics
So far we have covered topics as a means of communication between nodes. Topics essentially only allow for uni-directional message passing. However there are many use cases where you want to receive some kind of answer on if a message has been received. Maybe you even want to know the result of some operation. In other words, you want to call up a procedure of one node from another. Within labrat-robot this behavior can be achieved through the use of services.

For each service you will need exactly one server that answers any incoming requests on the service. You can create many clients that all call up the same service.

@image html services.svg

Services are distinguished by their name and message types. The convention is to name services like a file path (e. g. `/path/to/topic`). This helps to order your services. Even though you could give a topic and a service the same name, you are discouraged from doing so. While topics only have one message type, services require two. One is the request message sent from the client to the server, the other is the response message sent the other way around. The message types of a service have to be known by every client and server at compile time. Once again, if there is a type mismatch, a [lbot::ManagementException](@ref labrat::lbot::ManagementException) exception will be raised.

# Server
In order to handle incoming requests you need to create a [Node::Server](@ref labrat::lbot::Node::Server) object. You must specify the both  template arguments of [Node::Server](@ref labrat::lbot::Node::Server) as the request and response message types respectively. It is recommended to declare a server pointer as a member of your node.
```cpp
Server<examples::msg::Request, examples::msg::Response>::Ptr server;
```
This pointer should then be initialized in the constructor of the node via the [addServer()](@ref labrat::lbot::Node::addServer()) function. The template arguments of the [addServer()](@ref labrat::lbot::Node::addServer()) function must match the [Node::Server::Ptr](@ref labrat::lbot::Node::Server::Ptr) object you declared earlier. The first argument of the function itself specifies the name of the service. After the creation of the server you need to specify a handler function that will be called to handle any incoming requests via the [Server::setHandler()](@ref labrat::lbot::Node::Server::setHandler()) method. You may also pass a user pointer of the type `void *` to access any non-static data inside of the handler function. You don't need to declare the service anywhere explicitly.
```cpp
server = addServer<examples::msg::Request, examples::msg::Response>("/examples/test_service", ...);
server->setHandler(&ServerNode::handleRequest, ...);
```
The handler function must be static and have the following signature:
```cpp
lbot::Message<examples::msg::Response> handleRequest(const lbot::Message<examples::msg::Request> &request, void *user_ptr);
```
The function is expected to construct a response and return it. There are no limits on how long this function may run. The handler function may be called simultaneously when more than one requests are made at the same time. You should therefore be careful when accessing any non-local data.

# Client
In order to send a request you need to create a [Node::Client](@ref labrat::lbot::Node::Client) object. It is declared in a similar manner to [Node::Server](@ref labrat::lbot::Node::Server).
```cpp
Client<examples::msg::Request, examples::msg::Response>::Ptr client;
```
Once again this pointer should then be initialized in the constructor of the node. This can be achieved through the [addClient()](@ref labrat::lbot::Node::addClient()) function. The template arguments of the [addClient()](@ref labrat::lbot::Node::addClient()) function must match the [Node::Client::Ptr](@ref labrat::lbot::Node::Client::Ptr) object you declared earlier. In the function arguments you need to specify the name of the service to connect to.
```cpp
client = addClient<examples::msg::Request, examples::msg::Response>("/examples/test_service");
```

There are two methods of calling up a service.

## Synchronous Call
The most simple way of making a request to a service is through the [Client::callSync()](@ref labrat::lbot::Node::Client::callSync()) function. It will block the current thread until a response has been received. You need to to pass a request message as the first argument. Afterwards you may set a timeout. Once the timeout expires a [lbot::ServiceTimeoutException](@ref labrat::lbot::ServiceTimeoutException) will be thrown. If there is no server attached to the server a [lbot::ServiceUnavailableException](@ref labrat::lbot::ServiceUnavailableException) will be thrown. You should therefore call [Client::callSync()](@ref labrat::lbot::Node::Client::callSync()) only inside a try-catch block.
```cpp
try {
  lbot::Message<examples::msg::Response> response = client->callSync(request, ...);
} catch (lbot::ServiceUnavailableException &) {}
```

## Asynchronous Call
Alternatively you could make a request through the [Client::callAsync()](@ref labrat::lbot::Node::Client::callAsync()) function. Instead of blocking the current thread, it returns a [Future](@ref labrat::lbot::Node::Client::Future) object. The thread will not block until a call to [Future::get()](@ref labrat::lbot::Node::Client::Future::get()) is made. This allows you to perform computations while waiting on a reponse. You need to to pass a request message as the first argument to [Client::callAsync()](@ref labrat::lbot::Node::Client::callAsync()). Once again a [lbot::ServiceUnavailableException](@ref labrat::lbot::ServiceUnavailableException) will be thrown if there is no server attached to the service. You should therefore call [Client::callAsync()](@ref labrat::lbot::Node::Client::callAsync()) only inside a try-catch block.
```cpp
try {
  auto future = client->callAsync(request);
  ...
  lbot::Message<examples::msg::Response> response = future.get();
} catch (lbot::ServiceUnavailableException &) {}
```
