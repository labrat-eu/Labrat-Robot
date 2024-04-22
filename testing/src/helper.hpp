#include <labrat/lbot/message.hpp>
#include <labrat/lbot/msg/test.fb.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugin.hpp>

#include <atomic>

#include <gtest/gtest.h>

inline namespace labrat {
namespace lbot::test {

using TestFlatbuffer = lbot::Test;

static_assert(is_flatbuffer<TestFlatbuffer>);

using TestMessage = lbot::Message<TestFlatbuffer>;

static_assert(is_message<TestMessage>);
static_assert(can_convert_from<TestMessage>);
static_assert(can_move_from<TestMessage>);
static_assert(can_convert_to<TestMessage>);
static_assert(can_move_to<TestMessage>);

class TestContainer {
public:
  u64 integral_field = 0;
  double float_field = 0;
  std::vector<u8> buffer;

  bool operator==(const TestContainer &rhs) const {
    return (integral_field == rhs.integral_field) && (float_field == rhs.float_field) && (buffer == rhs.buffer);
  }
};

class TestMessageConv : public lbot::MessageBase<TestFlatbuffer, TestContainer> {
public:
  using Message = lbot::MessageBase<TestFlatbuffer, TestContainer>;

  static void convertFrom(const TestContainer &source, Message &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
  }

  static void moveFrom(TestContainer &&source, Message &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
  }

  static void convertTo(const Message &source, TestContainer &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
  }

  static void moveTo(Message &&source, TestContainer &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
  }
};

static_assert(is_message<TestMessageConv>);
static_assert(can_convert_from<TestMessageConv>);
static_assert(can_move_from<TestMessageConv>);
static_assert(can_convert_to<TestMessageConv>);
static_assert(can_move_to<TestMessageConv>);

class TestUniqueNode : public lbot::UniqueNode {
public:
  TestUniqueNode(const NodeEnvironment environment) : lbot::UniqueNode(environment, "test_node") {}
};

class TestSharedNode : public lbot::SharedNode {
public:
  TestSharedNode(const NodeEnvironment environment, const std::string &sender_topic = "", const std::string &receiver_topic = "",
    int buffer_size = 10) : lbot::SharedNode(environment) {
    if (!sender_topic.empty()) {
      sender = addSender<TestMessageConv>(sender_topic);
    }
    if (!receiver_topic.empty()) {
      receiver = addReceiver<TestMessageConv>(receiver_topic, nullptr, buffer_size);
    }
  }

  TestSharedNode(const NodeEnvironment environment, const std::string &sender_topic, const std::string &receiver_topic, const void *sender_ptr,
    const void *receiver_ptr, int buffer_size = 10) : lbot::SharedNode(environment) {
    if (!sender_topic.empty()) {
      sender = addSender<TestMessageConv>(sender_topic, sender_ptr);
    }
    if (!receiver_topic.empty()) {
      receiver = addReceiver<TestMessageConv>(receiver_topic, receiver_ptr, buffer_size);
    }
  }

  using lbot::Node::addClient;
  using lbot::Node::addReceiver;
  using lbot::Node::addSender;
  using lbot::Node::addServer;
  using lbot::Node::getLogger;

  Sender<TestMessageConv>::Ptr sender;
  Receiver<TestMessageConv>::Ptr receiver;
};

class TestUniquePlugin : public lbot::UniquePlugin {
public:
  TestUniquePlugin(const PluginEnvironment environment) : lbot::UniquePlugin(environment, "test_plugin") {
    addNode<TestSharedNode>("node_a", "main", "void");
    addNode<TestSharedNode>("node_b", "void", "main");
  }
};

class TestSharedPlugin : public lbot::SharedPlugin {
public:
  TestSharedPlugin(const PluginEnvironment environment) : lbot::SharedPlugin(environment) {}
};

}  // namespace lbot::test
}  // namespace labrat
