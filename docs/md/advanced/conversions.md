@page conversions Conversions

@note
You may also want to take a look at the [code example](@ref example_conversions).

# Basics
Usually, the types used to pass messages (flatbuffers) are not used internally by a program to perform computations. For instance, you might use a separate linear-algebra library to work compute vector operations. Such libraries normally come with their own data types. You might find yourself manually converting between flatbuffer messages and other types all over your code. To make this easier, you can register custom conversion functions with your senders, receivers, servers and clients. Calls to access functions like `put()`, `latest()` or `next()` will then expect/return the type you've specified in the conversion function. This leads to an overall cleaner code style, as the focus in now on the actual computations you perform instead of the type conversions.

# Conversion Types
In order to define a conversion function you need to declare an encapsulating message type. This type must inherit from [lbot::MessageBase<FlatbufferType, ConvertedType>](@ref labrat::lbot::MessageBase) with `FlatbufferType` being the flatbuffer type used internally and `ConvertedType` being the type you actually want to access. So if you want to use a flatbuffer schema called `examples::msg::Number` but want to convert it from/to a `float` you need to define a conversion class in the following way.
```cpp
class ConversionMessage : public lbot::MessageBase<examples::msg::Number, float> {
...
};
```
Inside your conversion class you then need to specify your actual coversion functions. There are two types of conversion function. One is the `convertTo()` function which takes a constant reference to the relevant flatbuffer message and converts it to a `ConvertedType` instance. Note that you can make use of the type `Storage` for references to the flatbuffer message type. It is defined in [lbot::MessageBase](@ref labrat::lbot::MessageBase).
```cpp
// Convert a number message to a float.
static void convertTo(const Storage &source, float &destination) {
  destination = source.value;
}
```
Then there is `convertFrom()` which does the opposite, as it takes a constant reference to a `ConvertedType` object and converts it to a flatbuffer message.
```cpp
// Convert a float to a number message.
static void convertFrom(const float &source, Storage &destination) {
  destination.value = source;
}
```
Which ones you need to define depends on where you want to use your conversion class. For use in receivers, only `convertTo()` must be defined. Senders only require `convertFrom()`, while servers and clients require both functions.

# Usage
You can now use your conversion class by using it as a template argument when declaring and constructing a sender, receiver, server or client.

## Sender
First you need to declare and construct your sender with the conversion class as the template parameter.
```cpp
Sender<ConversionMessage>::Ptr sender;
...
sender = addSender<ConversionMessage>(...);
```
Calls to [Sender::put()](@ref labrat::lbot::Node::Sender::put()) will now expect a `float` as an argument.
```cpp
sender->put(1.0f);
```

## Receiver
First you need to declare and construct your receiver with the conversion class as the template parameter.
```cpp
Receiver<ConversionMessage>::Ptr receiver;
...
receiver = addReceiver<ConversionMessage>(...);
```
Calls to [Receiver::next()](@ref labrat::lbot::Node::Receiver::next()) and [Receiver::latest()](@ref labrat::lbot::Node::Receiver::latest()) will now return a `float`.
```cpp
float value = receiver->latest();
```
The signature of a callback function must now accept a `float` as the message argument.
```cpp
static void callback(const float &message, void *);
```

## Server
First you need to declare and construct your server with the conversion classes as the template parameters. You can specify a conversion class for only the request or response type or both.
```cpp
Server<ConversionMessage, ConversionMessage>::Ptr server;
...
server = addServer<ConversionMessage, ConversionMessage>(...);
```
The signature of the handler function must now accept a `float` as the request argument and must return a `float` as the response.
```cpp
static float handleRequest(const float &request, void *);
```

## Client
First you need to declare and construct your client with the conversion classes as the template parameters. You can specify a conversion class for only the request or response type or both.
```cpp
Client<ConversionMessage, ConversionMessage>::Ptr client;
...
client = addClient<ConversionMessage, ConversionMessage>(...);
```
Calls to [Client::callSync()](@ref labrat::lbot::Node::Client::callSync()) and [Client::callAsync()](@ref labrat::lbot::Node::Client::callAsync()) will now expect a `float` as an argument. [Client::callSync()](@ref labrat::lbot::Node::Client::callSync()) will now return a `float`. The future returned by [Client::callAsync()](@ref labrat::lbot::Node::Client::callAsync()) will now also contain a `float`.
```cpp
float response = client->callSync(1.0f, std::chrono::seconds(1));
```