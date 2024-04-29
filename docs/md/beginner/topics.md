@page topics Topics

@note
You may also want to take a look at the [code example](@ref example_topics).

# Basics
Topics provide a way of exchanging data between nodes. You can think of a topic as a channel over which messages are being sent. In order to read from a topic you need to use a receiver. For writing you need to use a sender.

For each topic you will need exactly one sender that writes data onto the topic. You can create many receivers that all read from the same topic. You are not required to create any receivers at all. This is useful, if you only want to trace information but do not wish to process it further.

![topics.drawio.svg](uploads/509cd16c0960ae26c0b2031c1b13114a/topics.drawio.svg)

Topics are distinguished by their name and message type. The convention is to name topics like a file path (e. g. `/path/to/topic`). This helps to order your topics. The message type of a topic has to be known by every sender and receiver at compile time. If there is a type mismatch, a [lbot::ManagementException](@ref labrat::lbot::ManagementException) exception will be raised.

# Messages
Before you can exchange messages over topics you need to know how to define message schemas. Labrat-robot does not define its own message standard but instead makes use of [FlatBuffers](https://flatbuffers.dev/). In order to define a message schema you have to create a `.fbs` FlatBuffer schema file. It is recommended to create a subdirectory in your source folder for your message definitions.
```
src/
|- CMakeLists.txt           # define and configure targets
|- main.cpp                 # source file
|- msg/
|  |- example_message.fbs   # message definition
```

## Message schema file
@note
For a detailed guide on how to write FlatBuffer schemas, please refer to the official [documentation](https://flatbuffers.dev/flatbuffers_guide_writing_schema.html).

The filename of your `.fbs` file should match the name of the C++ type you want to create. In this example we will use `example_message.fbs`, as we want to create the message type `ExampleMessage`. A `.fbs` will look somewhat like this:
```
namespace examples.msg;

table ExampleMessage {
  field_a:long;
  field_b:float;
}

root_type ExampleMessage;
```
The first should contain the namespace of the message. The namespace you specify here will translate into C++. In this example, the full name of the created message will be `examples::msg::ExampleMessage`. The convention is to use a `msg` namespace inside whatever namespace you are currently working in.

Afterwards you define the fields of your message inside the `table` block. Here you have to specify the name of your type. At the end you specify the `root_type` of your message. This will usually be the same as the table you defined earlier. But if you define multiple tables in order to nest your message you have to specify the top-level table of your message.

## CMake configuration
Now that you created your message definition, you need to configure CMake to generate the required code. To make this process easier the function `lbot_generate_flatbuffer()` is provided as part of the lbot package.
```cmake
lbot_generate_flatbuffer(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS msg/example_message.fbs TARGET_PATH examples/msg)
```
The `TARGET` argument expects a string with name of the CMake target to be created. This target will then perform the code generation during the build step. The `SCHEMAS` argument consists of a list of message definition files you want to use. The `TARGET_PATH` determines the path of the C++ header file you need to include to access the message type. It is convention to mirror the namespace of a message in the `TARGET_PATH`. The correct include directive for this example would be:
```cpp
#include <examples/msg/example_message.fb.hpp>
```
Now you need to link your library or executable target to the message target you just created. This will correctly set the include path, so that you can include the generated header in your code.
```cmake
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME_MESSAGE})
```

## Using messages in C++
Inside of labrat-robot, messages appear wrapped inside of the [lbot::Message](@ref labrat::lbot::Message) class. If you want to declare a labrat-robot message you must therefore do it in the following way.
```cpp
lbot::Message<examples::msg::ExampleMessage> message;
```
If you then wish to access the fields you have declared previuosly, you can simply use the members of the class.
```cpp
message.field_a = 42;
float field_b = message.field_b;
```

# Sender
In order to send a message you need to create a [Node::Sender](@ref labrat::lbot::Node::Sender) object. You must specify the first template argument of [Node::Sender](@ref labrat::lbot::Node::Sender) as the message type you want to send. It is recommended to declare a sender pointer as a member of your node.
```cpp
Sender<examples::msg::ExampleMessage>::Ptr sender;
```
This pointer should then be initialized in the constructor of the node via the [addSender()](@ref labrat::lbot::Node::addSender()) function. The template arguments of the [addSender()](@ref labrat::lbot::Node::addSender()) function must match the [Node::Sender::Ptr](@ref labrat::lbot::Node::Sender::Ptr) object you declared earlier. The first argument of the function itself specifies the name of the topic onto which the data is sent. You don't need to declare the topic anywhere explicitly.
```cpp
sender = addSender<examples::msg::ExampleMessage>("/examples/test_topic");
```

Now you can construct messages and send them over the topic by using the [put()](@ref labrat::lbot::Node::Sender::put()) function.
```cpp
sender->put(message);
```

# Receiver
In order to receive a message you need to create a [Node::Receiver](@ref labrat::lbot::Node::Receiver) object. It is declared in a similar manner to [Node::Sender](@ref labrat::lbot::Node::Sender).
```cpp
Receiver<examples::msg::ExampleMessage>::Ptr receiver;
```
Once again this pointer should then be initialized in the constructor of the node. This can be achieved through the [addReceiver()](@ref labrat::lbot::Node::addReceiver())function which behaves similarly to [addSender()](@ref labrat::lbot::Node::addSender()).
```cpp
receiver = addReceiver<examples::msg::ExampleMessage>("/examples/test_topic");
```

Now you can receive and deconstruct messages from the topic. For this purpose you have two options.

## Latest message
One method to receive a message from a topic is to use the [latest()](@ref labrat::lbot::Node::Receiver::latest()) function. It will return the latest message that was sent by the corresponding sender over the topic. Successive calls to [latest()](@ref labrat::lbot::Node::Receiver::latest()) might return the same message (if no new messages have been sent in the meantime). If no message has been sent yet, a [lbot::TopicNoDataAvailableException](@ref labrat::lbot::TopicNoDataAvailableException) will be thrown. You should therefore always call [latest()](@ref labrat::lbot::Node::Receiver::latest()) inside a try-catch block.
```cpp
try {
  message = receiver->latest();
} catch (lbot::TopicNoDataAvailableException &) {}
```

## Next message
Another method to receive a message from a topic is to use the [next()](@ref labrat::lbot::Node::Receiver::next()) function. Successive calls to [next()](@ref labrat::lbot::Node::Receiver::next()) will yield different messages sent over the topic in order. If no new message has been sent, this call will block. As long as the internal buffer of the receiver has not been exceeded, you are guaranteed that no message will be skipped. Such an overflow can occur if the receiver thread is not able to keep pace with the sender thread. A [lbot::TopicNoDataAvailableException](@ref labrat::lbot::TopicNoDataAvailableException) will be thrown if the corresponding sender is being deleted. This is done to prevent deadlocks. You should therefore once again call [next()](@ref labrat::lbot::Node::Receiver::next()) only inside a try-catch block.
```cpp
try {
  message = receiver->next();
} catch (lbot::TopicNoDataAvailableException &) {}
```