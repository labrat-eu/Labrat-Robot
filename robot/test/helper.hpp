#include <labrat/robot/message.hpp>
#include <labrat/robot/msg/test.pb.h>
#include <labrat/robot/node.hpp>

#include <atomic>

#include <gtest/gtest.h>

namespace labrat::robot::test {

using TestMessage = labrat::robot::Message<Test>;

class TestContainer {
public:
  using MessageType = TestMessage;

  u64 integral_field = 0;
  double float_field = 0;

  static inline void toMessage(const TestContainer &source, TestMessage &destination) {
    destination().set_integral_field(source.integral_field);
    destination().set_float_field(source.float_field);
  }

  static inline void fromMessage(const TestMessage &source, TestContainer &destination) {
    destination.integral_field = source().integral_field();
    destination.float_field = source().float_field();
  }

  bool operator==(const TestContainer &rhs) const {
    return (integral_field == rhs.integral_field) && (float_field == rhs.float_field);
  }
};

class TestNode : public labrat::robot::Node {
public:
  TestNode(const std::string &name, TopicMap &topic_map, const std::string &sender_topic, const std::string &receiver_topic) :
    labrat::robot::Node(name, topic_map) {
    sender = addSender<TestContainer>(sender_topic);
    receiver = addReceiver<TestContainer>(receiver_topic, 10);
  }

  std::unique_ptr<ContainerSender<TestContainer>> sender;
  std::unique_ptr<ContainerReceiver<TestContainer>> receiver;
};

}  // namespace labrat::robot::test
