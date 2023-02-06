#include <helper.hpp>

#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/udp-bridge/node.hpp>

#include <unistd.h>
#include <sys/wait.h>
#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(udp_bridge, fork) {
  int pid = fork();

  if (pid == 0) {
    // Child branch.
    std::shared_ptr<TestNode> source(labrat::robot::Manager::get().addNode<TestNode>("source", "/network"));
    std::shared_ptr<labrat::robot::plugins::UdpBridgeNode> bridge(labrat::robot::Manager::get().addNode<labrat::robot::plugins::UdpBridgeNode>("bridge", "127.0.0.1", 5001, 5002));

    bridge->registerReceiver<TestMessage>("/network");

    for (u64 i = 0; i < 5000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = 1.0;

      source->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    source = std::shared_ptr<TestNode>();
    labrat::robot::Manager::get().removeNode("source");
    bridge = std::shared_ptr<labrat::robot::plugins::UdpBridgeNode>();
    labrat::robot::Manager::get().removeNode("bridge");

    exit(0);
  } else {
    // Parent branch.
    std::shared_ptr<TestNode> sink(labrat::robot::Manager::get().addNode<TestNode>("sink", "", "/network"));
    std::shared_ptr<labrat::robot::plugins::UdpBridgeNode> bridge(labrat::robot::Manager::get().addNode<labrat::robot::plugins::UdpBridgeNode>("bridge", "127.0.0.1", 5002, 5001));

    bridge->registerSender<TestMessage>("/network");

    TestContainer message;
    message.integral_field = -1;
    message.float_field = 0.0;

    for (u64 i = 0; i < 5000; ++i) {
      try {
        message = sink->receiver->latest();
      } catch (labrat::robot::TopicNoDataAvailableException &) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      break;
    }

    ASSERT_GE(message.integral_field, 0);
    ASSERT_EQ(message.float_field, 1.0);

    sink = std::shared_ptr<TestNode>();
    ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("sink"));
    bridge = std::shared_ptr<labrat::robot::plugins::UdpBridgeNode>();
    ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("bridge"));

    int status;
    ASSERT_GE(waitpid(pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
  }
}

}  // namespace labrat::robot::test
