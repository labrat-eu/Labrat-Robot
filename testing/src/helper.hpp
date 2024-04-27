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
static_assert(is_flatbuffer<const TestFlatbuffer>);

using TestMessage = lbot::Message<TestFlatbuffer>;

static_assert(is_message<TestMessage>);
static_assert(can_convert_from<TestMessage>);
static_assert(can_move_from<TestMessage>);
static_assert(can_convert_to<TestMessage>);
static_assert(can_move_to<TestMessage>);

using TestConstMessage = lbot::Message<const TestFlatbuffer>;

static_assert(is_message<TestConstMessage>);
static_assert(can_convert_from<TestConstMessage>);
static_assert(can_move_from<TestConstMessage>);
static_assert(can_convert_to<TestConstMessage>);
static_assert(can_move_to<TestConstMessage>);

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

class TestMessageConvPtr : public lbot::MessageBase<TestFlatbuffer, TestContainer> {
public:
  using Message = lbot::MessageBase<TestFlatbuffer, TestContainer>;

  static void convertFrom(const TestContainer &source, Message &destination, bool *called) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
    *called = true;
  }

  static void moveFrom(TestContainer &&source, Message &destination, bool *called) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
    *called = true;
  }

  static void convertTo(const Message &source, TestContainer &destination, bool *called) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
    *called = true;
  }

  static void moveTo(Message &&source, TestContainer &destination, bool *called) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
    *called = true;
  }
};

static_assert(is_message<TestMessageConvPtr>);
static_assert(can_convert_from<TestMessageConvPtr>);
static_assert(can_move_from<TestMessageConvPtr>);
static_assert(can_convert_to<TestMessageConvPtr>);
static_assert(can_move_to<TestMessageConvPtr>);

class TestUniqueNode : public lbot::UniqueNode {
public:
  TestUniqueNode() : lbot::UniqueNode("test_node") {}
};

class TestNode : public lbot::Node {
public:
  TestNode(const std::string &sender_topic = "", const std::string &receiver_topic = "",
    int buffer_size = 10) {
    if (!sender_topic.empty()) {
      sender = addSender<TestMessageConv>(sender_topic);
    }
    if (!receiver_topic.empty()) {
      receiver = addReceiver<TestMessageConv>(receiver_topic, nullptr, buffer_size);
    }
  }

  TestNode(const std::string &sender_topic, const std::string &receiver_topic,
    void *sender_ptr, void *receiver_ptr, int buffer_size = 10) {
    if (!sender_topic.empty()) {
      sender_p = addSender<TestMessageConvPtr>(sender_topic, sender_ptr);
    }
    if (!receiver_topic.empty()) {
      receiver_p = addReceiver<TestMessageConvPtr>(receiver_topic, receiver_ptr, buffer_size);
    }
  }

  using lbot::Node::addClient;
  using lbot::Node::addReceiver;
  using lbot::Node::addSender;
  using lbot::Node::addServer;
  using lbot::Node::getLogger;

  Sender<TestMessageConv>::Ptr sender;
  Receiver<TestMessageConv>::Ptr receiver;
  Sender<TestMessageConvPtr>::Ptr sender_p;
  Receiver<TestMessageConvPtr>::Ptr receiver_p;
};

class TestUniquePlugin : public lbot::UniquePlugin {
public:
  TestUniquePlugin() : lbot::UniquePlugin("test_plugin") {
    addNode<TestNode>("node_a", "main", "void");
    addNode<TestNode>("node_b", "void", "main");
  }
};

class TestPlugin : public lbot::Plugin {
public:
  TestPlugin() = default;
};

}  // namespace lbot::test
}  // namespace labrat
