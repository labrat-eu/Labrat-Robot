@page moves Move Operations

@note
You may also want to take a look at the [code example](@ref example_moves).

# Basics
When transferring large amounts of data between nodes the overhead due to copy operations can take up considerable resources. If just one receiver or plugin is receiving messages from a topic this overhead can be avoided. With the help of move semantics the messages can be efficiently moved between the sender and the single receiver or plugin. No copy operation is taking place in this scenario. Move semantics are very efficient when the bulk of the transferrable data is located in the heap. Within lbot this is true for any large vectors defined in flatbuffer messages. These can typically be found in images, videos, pointclouds etc. Careful use of move functions can improve the performance of your project considerably.

# Availability
Certain criteria must be met for the usage of move operations. You need to use a message type that supports move operations **and** you need to use the relevant methods correctly.

## Message types
Move operations are available on any specializations of [lbot::Message](@ref lbot::Message) as well as any custom conversion message types that define the `moveTo()` and/or and `moveFrom()` methods. `moveTo()` takes an rvalue reference of the relevant flatbuffer message and converts it to a `ConvertedType` instance.
```cpp
// Move an array message to a vector.
static void moveTo(Storage &&source, std::vector<float> &destination) {
  destination = std::forward<std::vector<float>>(source.values);
}
```
On the other hand, `moveFrom()` takes an rvalue reference of `ConvertedType` and converts it to a flatbuffer message.
```cpp
// Move a vector to an array message.
static void moveFrom(std::vector<float> &&source, Storage &destination) {
  destination.values = std::forward<std::vector<float>>(source);
}
```
Note that you need to use the `std::forward()` function to make move operations work efficiently.
## Methods
The [Sender::put()](@ref lbot::Node::Sender::put()) method will use move operations when called with an rvalue reference as the argument. [Receiver::latest()](@ref lbot::Node::Receiver::latest()) will never make use of move operations while [Receiver::next()](@ref lbot::Node::Receiver::next()) will always use them if they are available. Callback functions are unaffected my move operations. When calling up a service, move operations can be used to improve performance. However, some copy operations cannot be avoided.

# Usage
When moving messages onto a sender, you need to call [Sender::put()](@ref lbot::Node::Sender::put()) with an rvalue reference as the argument. This can be achieved by using the `std::move()` function.
```cpp
sender->put(std::move(message));
```
Note that you cannot use the moved object (in this case `message`) afterwards, as this would lead to undefined behavior.

Calls to [Receiver::next()](@ref lbot::Node::Receiver::next()) will automatically use move operations when they are available. You don't need to change the call to [Receiver::next()](@ref lbot::Node::Receiver::next()) in order for this to work.
```cpp
message = receiver->next();
```