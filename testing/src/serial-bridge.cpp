#include <helper.hpp>

#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/serial-bridge/node.hpp>

#include <filesystem>

#include <stdlib.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <gtest/gtest.h>

namespace labrat::robot::test {

TEST(serial_bridge, fork) {
  std::filesystem::remove("test0");
  std::filesystem::remove("test1");

  int init_pid = fork();

  if (init_pid == 0) {
    execlp("socat", "-d", "pty,raw,echo=0,link=test0", "pty,raw,echo=0,link=test1");
  }

  bool init_flag = false;

  for (u64 i = 0; i < 5000; ++i) {
    if (std::filesystem::exists("test0") && std::filesystem::exists("test1")) {
      init_flag = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  ASSERT_TRUE(init_flag);

  int setup_pid = fork();

  if (setup_pid == 0) {
    std::shared_ptr<TestNode> source(labrat::robot::Manager::get().addNode<TestNode>("source", "/network"));
    std::shared_ptr<labrat::robot::plugins::SerialBridgeNode> bridge(labrat::robot::Manager::get().addNode<labrat::robot::plugins::SerialBridgeNode>("bridge", "test0"));

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
    bridge = std::shared_ptr<labrat::robot::plugins::SerialBridgeNode>();
    labrat::robot::Manager::get().removeNode("bridge");

    exit(0);
  } else {
    std::shared_ptr<TestNode> sink(labrat::robot::Manager::get().addNode<TestNode>("sink", "", "/network"));
    std::shared_ptr<labrat::robot::plugins::SerialBridgeNode> bridge(labrat::robot::Manager::get().addNode<labrat::robot::plugins::SerialBridgeNode>("bridge", "test1"));

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
    bridge = std::shared_ptr<labrat::robot::plugins::SerialBridgeNode>();
    ASSERT_NO_THROW(labrat::robot::Manager::get().removeNode("bridge"));

    int status;
    ASSERT_GE(waitpid(setup_pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    kill(init_pid, SIGKILL);

    std::filesystem::remove("test0");
    std::filesystem::remove("test1");
  }
}

}  // namespace labrat::robot::test
