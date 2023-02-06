#include <labrat/robot/message.hpp>
#include <labrat/robot/msg/test_generated.h>
#include <labrat/robot/node.hpp>

#include <atomic>

#include <gtest/gtest.h>

namespace labrat::robot::test {

using TestMessage = labrat::robot::Message<msg::Test>;

class TestContainer {
public:
  using MessageType = TestMessage;

  u64 integral_field = 0;
  double float_field = 0;

  static inline void toMessage(const TestContainer &source, TestMessage &destination, const void *) {
    destination().integral_field = source.integral_field;
    destination().float_field = source.float_field;
  }

  static inline void fromMessage(const TestMessage &source, TestContainer &destination, const void *) {
    destination.integral_field = source().integral_field;
    destination.float_field = source().float_field;
  }

  bool operator==(const TestContainer &rhs) const {
    return (integral_field == rhs.integral_field) && (float_field == rhs.float_field);
  }
};

class TestNode : public labrat::robot::Node {
public:
  TestNode(const Node::Environment environment, const std::string &sender_topic = "", const std::string &receiver_topic = "") :
    labrat::robot::Node(environment) {
    if (!sender_topic.empty()) {
      sender = addSender<TestContainer>(sender_topic);
    }
    if (!receiver_topic.empty()) {
      receiver = addReceiver<TestContainer>(receiver_topic, nullptr, 10);
    }
  }

  template <typename RequestType, typename ResponseType, typename... Args>
  typename Server<RequestType, ResponseType>::Ptr addTestServer(Args &&...args) {
    return addServer<RequestType, ResponseType>(std::forward<Args>(args)...);
  }

  template <typename RequestType, typename ResponseType, typename... Args>
  typename Client<RequestType, ResponseType>::Ptr addTestClient(Args &&...args) {
    return addClient<RequestType, ResponseType>(std::forward<Args>(args)...);
  }

  inline Logger getLogger() const {
    return labrat::robot::Node::getLogger();
  }

  std::unique_ptr<ContainerSender<TestContainer>> sender;
  std::unique_ptr<ContainerReceiver<TestContainer>> receiver;
};

}  // namespace labrat::robot::test
