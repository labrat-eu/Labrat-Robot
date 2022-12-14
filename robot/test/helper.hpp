#include <labrat/robot/node.hpp>
#include <labrat/robot/message.hpp>

#include <gtest/gtest.h>

#include <atomic>

namespace labrat::robot::test {

class TestNode : public labrat::robot::Node {
public:
  TestNode(const std::string &name, TopicMap &topic_map) : labrat::robot::Node(name, topic_map) {}

  template<typename MessageType, typename... Args>
  std::unique_ptr<Sender<MessageType>> addTestSender(const std::string &topic_name, Args &&... args) {
    return addSender<MessageType>(topic_name, std::forward<Args>(args)...);
  }

  template<typename MessageType, typename... Args>
  std::unique_ptr<Receiver<MessageType>> addTestReceiver(const std::string &topic_name, Args &&... args) {
    return addReceiver<MessageType>(topic_name, std::forward<Args>(args)...);
  }
};

class TestMessage : public labrat::robot::Message {
public:
  u64 integral_field = 0;
  f64 float_field = 0;

  bool operator==(const TestMessage& rhs) const {
    return (integral_field == rhs.integral_field) && (float_field == rhs.float_field);
  }
};

}  // namespace labrat::robot::test
