#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>
#include <labrat/robot/test/helper.hpp>

#include <cmath>
#include <thread>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(mcap, recorder) {
  plugins::McapRecorder recorder("test.mcap");

  std::shared_ptr<TestNode> node_a(labrat::robot::Manager::get().addNode<TestNode>("node_a", "/topic_a", "/topic_b"));
  std::shared_ptr<TestNode> node_b(labrat::robot::Manager::get().addNode<TestNode>("node_b", "/topic_b", "/topic_a"));

  for (u64 i = 0; i < 1000; ++i) {
    TestContainer message;
    message.integral_field = i;
    message.float_field = std::sin(static_cast<double>(i) / 100);

    node_a->sender->put(message);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  node_b->getLogger().info() << "Node is publishing.";

  for (u64 i = 0; i < 1000; ++i) {
    TestContainer message;
    message.integral_field = i;
    message.float_field = std::cos(static_cast<double>(i) / 100);

    node_b->sender->put(message);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  node_a = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_a"));
  node_b = std::shared_ptr<TestNode>();
  ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("node_b"));
}

}  // namespace labrat::robot::test
