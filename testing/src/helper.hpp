#include <labrat/lbot/cluster.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/msg/test.fb.hpp>
#include <labrat/lbot/node.hpp>

#include <atomic>

#include <gtest/gtest.h>

inline namespace labrat {
namespace lbot::test {

using TestFlatbuffer = msg::Test;
using TestMessage = lbot::Message<TestFlatbuffer>;

class TestContainer {
public:
  u64 integral_field = 0;
  double float_field = 0;
  std::vector<u8> buffer;

  bool operator==(const TestContainer &rhs) const {
    return (integral_field == rhs.integral_field) && (float_field == rhs.float_field) && (buffer == rhs.buffer);
  }
};

class TestMessageConv : public lbot::UnsafeMessage<TestFlatbuffer, TestContainer> {
public:
  static void convertFrom(const TestContainer &source, TestMessageConv &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
  }

  static void moveFrom(TestContainer &&source, TestMessageConv &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
  }

  static void convertTo(const TestMessageConv &source, TestContainer &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = source.buffer;
  }

  static void moveTo(TestMessageConv &&source, TestContainer &destination) {
    destination.integral_field = source.integral_field;
    destination.float_field = source.float_field;
    destination.buffer = std::forward<std::vector<u8>>(source.buffer);
  }
};

class TestNode : public lbot::Node {
public:
  TestNode(const Node::Environment environment, const std::string &sender_topic = "", const std::string &receiver_topic = "") :
    lbot::Node(environment) {
    if (!sender_topic.empty()) {
      sender = addSender<TestMessageConv>(sender_topic);
    }
    if (!receiver_topic.empty()) {
      receiver = addReceiver<TestMessageConv>(receiver_topic, nullptr, 10);
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

class TestCluster : public lbot::Cluster {
public:
  TestCluster(const std::string &name) : lbot::Cluster(name) {
    addNode<TestNode>("node_a", "main", "void");
    addNode<TestNode>("node_b", "void", "main");
  }
};

}  // namespace lbot::test
}  // namespace labrat
