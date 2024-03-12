#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugins/udp-bridge/node.hpp>

#include <gtest/gtest.h>
#include <sys/wait.h>
#include <unistd.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(udp_bridge, fork) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  int pid = fork();

  if (pid == 0) {
    // Child branch.
    std::shared_ptr<TestNode> source(manager->addNode<TestNode>("source", "/network"));
    std::shared_ptr<labrat::lbot::plugins::UdpBridgeNode> bridge(
      manager->addNode<labrat::lbot::plugins::UdpBridgeNode>("bridge", "127.0.0.1", 5001, 5002));

    bridge->registerReceiver<TestMessage>("/network");

    for (u64 i = 0; i < 5000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = 1.0;

      source->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    source = std::shared_ptr<TestNode>();
    manager->removeNode("source");
    bridge = std::shared_ptr<labrat::lbot::plugins::UdpBridgeNode>();
    manager->removeNode("bridge");

    exit(0);
  } else {
    // Parent branch.
    std::shared_ptr<TestNode> sink(manager->addNode<TestNode>("sink", "", "/network"));
    std::shared_ptr<labrat::lbot::plugins::UdpBridgeNode> bridge(
      manager->addNode<labrat::lbot::plugins::UdpBridgeNode>("bridge", "127.0.0.1", 5002, 5001));

    bridge->registerSender<TestMessage>("/network");

    TestContainer message;
    message.integral_field = -1;
    message.float_field = 0.0;

    for (u64 i = 0; i < 5000; ++i) {
      try {
        message = sink->receiver->latest();
      } catch (labrat::lbot::TopicNoDataAvailableException &) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      break;
    }

    ASSERT_GE(message.integral_field, 0);
    ASSERT_EQ(message.float_field, 1.0);

    sink = std::shared_ptr<TestNode>();
    ASSERT_NO_THROW(manager->removeNode("sink"));
    bridge = std::shared_ptr<labrat::lbot::plugins::UdpBridgeNode>();
    ASSERT_NO_THROW(manager->removeNode("bridge"));

    int status;
    ASSERT_GE(waitpid(pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
  }
}

}  // namespace lbot::test
}  // namespace labrat
