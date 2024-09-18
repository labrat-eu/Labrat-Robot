#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugins/udp-bridge/node.hpp>

#include <gtest/gtest.h>
#include <sys/wait.h>
#include <unistd.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class UdpBridgeTest : public LbotTest
{};

TEST_F(UdpBridgeTest, fork)
{
  lbot::Manager::Ptr manager = lbot::Manager::get();

  int pid = fork();

  if (pid == 0) {
    // Child branch.
    std::shared_ptr<TestNode> source(manager->addNode<TestNode>("source", "/network"));
    std::shared_ptr<lbot::plugins::UdpBridge> bridge(manager->addPlugin<lbot::plugins::UdpBridge>("bridge", "127.0.0.1", 5001, 5002));

    bridge->registerReceiver<TestFlatbuffer>("/network");

    for (u64 i = 0; i < 5000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = 1.0;

      source->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    exit(0);
  } else {
    // Parent branch.
    std::shared_ptr<TestNode> sink(manager->addNode<TestNode>("sink", "", "/network"));
    std::shared_ptr<lbot::plugins::UdpBridge> bridge(manager->addPlugin<lbot::plugins::UdpBridge>("bridge", "127.0.0.1", 5002, 5001));

    bridge->registerSender<TestFlatbuffer>("/network");

    TestContainer message;
    message.integral_field = -1;
    message.float_field = 0.0;

    for (u64 i = 0; i < 5000; ++i) {
      try {
        message = sink->receiver->latest();
      } catch (lbot::TopicNoDataAvailableException &) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      break;
    }

    ASSERT_GE(message.integral_field, 0);
    ASSERT_EQ(message.float_field, 1.0);

    int status;
    ASSERT_GE(waitpid(pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
  }
}

}  // namespace lbot::test
}  // namespace labrat
