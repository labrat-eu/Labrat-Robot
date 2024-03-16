#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugins/serial-bridge/node.hpp>

#include <filesystem>
#include <system_error>

#include <errno.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

TEST(serial_bridge, fork) {
  labrat::lbot::Manager::Ptr manager = labrat::lbot::Manager::get();

  std::filesystem::remove("test0");
  std::filesystem::remove("test1");

  int init_pid = fork();

  if (init_pid == 0) {
    const int rc = execlp("/usr/bin/socat", "-dddd", "pty,raw,echo=0,link=test0", "pty,raw,echo=0,link=test1", nullptr);

    EXPECT_EQ(rc, 0);

    if (rc) {
      const std::error_condition error = std::system_category().default_error_condition(errno);
      FAIL() << error.value() << ": " << error.message();
    }
  }

  for (u64 i = 0; i < 5000; ++i) {
    if (std::filesystem::exists("test0") && std::filesystem::exists("test1")) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  EXPECT_TRUE(std::filesystem::exists("test0"));
  EXPECT_TRUE(std::filesystem::exists("test1"));

  int setup_pid = fork();

  if (setup_pid == 0) {
    std::shared_ptr<TestNode> source(manager->addNode<TestNode>("source", "/network"));
    std::shared_ptr<labrat::lbot::plugins::SerialBridgeNode> bridge(
      manager->addNode<labrat::lbot::plugins::SerialBridgeNode>("bridge", "test0"));

    bridge->registerReceiver<TestFlatbuffer>("/network");

    for (u64 i = 0; i < 5000; ++i) {
      TestContainer message;
      message.integral_field = i;
      message.float_field = 1.0;

      source->sender->put(message);

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    source = std::shared_ptr<TestNode>();
    manager->removeNode("source");
    bridge = std::shared_ptr<labrat::lbot::plugins::SerialBridgeNode>();
    manager->removeNode("bridge");

    exit(0);
  } else {
    std::shared_ptr<TestNode> sink(manager->addNode<TestNode>("sink", "", "/network"));
    std::shared_ptr<labrat::lbot::plugins::SerialBridgeNode> bridge(
      manager->addNode<labrat::lbot::plugins::SerialBridgeNode>("bridge", "test1"));

    bridge->registerSender<TestFlatbuffer>("/network");

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
    bridge = std::shared_ptr<labrat::lbot::plugins::SerialBridgeNode>();
    ASSERT_NO_THROW(manager->removeNode("bridge"));

    int status;
    ASSERT_GE(waitpid(setup_pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    kill(init_pid, SIGKILL);

    std::filesystem::remove("test0");
    std::filesystem::remove("test1");
  }
}

}  // namespace lbot::test
}  // namespace labrat
