#include <helper.hpp>

#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>

#include <cmath>
#include <thread>
#include <filesystem>

#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(mcap, recorder) {
  {
    labrat::robot::Manager::Ptr manager = labrat::robot::Manager::get();

    plugins::McapRecorder recorder("test.mcap");

    std::shared_ptr<TestNode> node_a(manager->addNode<TestNode>("node_a", "/topic_a", "/topic_b"));
    std::shared_ptr<TestNode> node_b(manager->addNode<TestNode>("node_b", "/topic_b", "/topic_a"));

    for (u64 i = 0; i < 1000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = std::sin(static_cast<double>(i) / 100);

      node_a->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    node_b->getLogger().logInfo() << "Node is publishing.";

    for (u64 i = 0; i < 1000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = std::cos(static_cast<double>(i) / 100);

      node_b->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    node_a = std::shared_ptr<TestNode>();
    ASSERT_NO_THROW(manager->removeNode("node_a"));
    node_b = std::shared_ptr<TestNode>();
    ASSERT_NO_THROW(manager->removeNode("node_b"));
  }

  ASSERT_NE(0, std::filesystem::file_size("test.mcap"));
}

}  // namespace labrat::robot::test
