#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/condition.hpp>
#include <labrat/lbot/utils/thread.hpp>
#include <labrat/lbot/msg/timestamp.hpp>

#include <mutex>
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class TimeMessage : public MessageBase<labrat::lbot::Timestamp, lbot::Clock::time_point> {
public:
  static void convertFrom(const Converted &source, Storage &destination) {
    const lbot::Clock::duration duration = source.time_since_epoch();
    destination.value = std::make_unique<foxglove::Time>(std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
      (duration % std::chrono::seconds(1)).count());
  }

  static void convertTo(const Storage &source, Converted &destination) {
    destination = lbot::Clock::time_point(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(source.value->sec()) + std::chrono::nanoseconds(source.value->nsec())));
  }
};

class TimeNode : public UniqueNode {
public:
  TimeNode() {
    sender = addSender<TimeMessage>("/time");
  }

  void updateTime(lbot::Clock::time_point time) {
    sender->put(time);
  }

  void updateTimeAsync(lbot::Clock::time_point time, std::chrono::milliseconds duration) {
    threads.emplace_back([&](lbot::Clock::time_point time, std::chrono::milliseconds duration){
      std::this_thread::sleep_for(duration);
      sender->put(time);
    }, time, duration);
  }

private:
  Sender<TimeMessage>::Ptr sender;
  std::vector<std::jthread> threads;
};

class ClockTest : public LbotTestWithParam<std::string> {};

TEST_P(ClockTest, setup) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());

  EXPECT_THROW(lbot::Clock::now(), lbot::ClockException);

  ASSERT_FALSE(lbot::Clock::initialized());
  lbot::Manager::Ptr manager = lbot::Manager::get();
  ASSERT_TRUE(lbot::Clock::initialized());

  if (GetParam() == "custom") {
    lbot::Clock::time_point t1 = lbot::Clock::now();
    lbot::Clock::time_point t2 = lbot::Clock::now();

    EXPECT_EQ(t1, t2);

    std::shared_ptr<TimeNode> node = manager->addNode<TimeNode>("test");
    node->updateTime(t1 + std::chrono::milliseconds(100));
    lbot::Clock::time_point t3 = lbot::Clock::now();

    EXPECT_EQ(t3, t1 + std::chrono::milliseconds(100));
  } else {
    lbot::Clock::time_point t1 = lbot::Clock::now();
    lbot::Clock::time_point t2;
    if (GetParam() == "system") {
      t2 = lbot::Clock::time_point(std::chrono::duration_cast<lbot::Clock::duration>(std::chrono::system_clock::now().time_since_epoch()));
    } else if (GetParam() == "steady") {
      t2 = lbot::Clock::time_point(std::chrono::duration_cast<lbot::Clock::duration>(std::chrono::steady_clock::now().time_since_epoch()));
    }
    lbot::Clock::time_point t3 = lbot::Clock::now();

    EXPECT_LE(t1, t2);
    EXPECT_LE(t1, t3);
    EXPECT_LE(t2, t3);

    EXPECT_THROW(manager->addNode<TimeNode>("test"), lbot::ManagementException);
  }
}

TEST_P(ClockTest, sleep) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TimeNode> node;
  if (GetParam() == "custom") {
    node = manager->addNode<TimeNode>("test");
  } else {
    ASSERT_THROW(manager->addNode<TimeNode>("test"), lbot::ManagementException);
  }

  lbot::Clock::time_point t1 = lbot::Clock::now();

  if (GetParam() == "custom") {
    node->updateTimeAsync(t1 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepFor(std::chrono::milliseconds(100));
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_GE(t2, t1 + std::chrono::milliseconds(100));

  if (GetParam() == "custom") {
    node->updateTimeAsync(t1 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepUntil(t1 + std::chrono::milliseconds(200));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_GE(t3, t1 + std::chrono::milliseconds(200));

  lbot::Thread::sleepFor(std::chrono::milliseconds(000));
  lbot::Clock::time_point t4 = lbot::Clock::now();

  EXPECT_GE(t4, t3);

  if (GetParam() == "custom") {
    node->updateTimeAsync(t3 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepUntil(t3 + std::chrono::milliseconds(100));
  lbot::Clock::time_point t5 = lbot::Clock::now();

  EXPECT_GE(t5, t3 + std::chrono::milliseconds(100));
  if (GetParam() == "custom") {
    EXPECT_EQ(t5, t3 + std::chrono::milliseconds(200));
  }

  bool check_a = false;
  std::jthread thread_a([&check_a]() {
    lbot::Thread::sleepFor(std::chrono::milliseconds(100));
    check_a = true;
  });

  if (GetParam() == "custom") {
    node->updateTimeAsync(t5 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node->updateTimeAsync(t5 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
  }
  lbot::Thread::sleepUntil(t5 + std::chrono::milliseconds(200));

  EXPECT_TRUE(check_a);
  thread_a.join();

  lbot::Clock::time_point t6 = lbot::Clock::now();

  bool check_b = false;
  std::jthread thread_b([&check_b]() {
    lbot::Thread::sleepFor(std::chrono::milliseconds(200));
    check_b = true;
  });

  if (GetParam() == "custom") {
    node->updateTimeAsync(t6 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node->updateTimeAsync(t6 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
  }
  lbot::Thread::sleepUntil(t6 + std::chrono::milliseconds(100));

  EXPECT_FALSE(check_b);
  thread_b.join();
}

TEST_P(ClockTest, condition) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TimeNode> node;
  if (GetParam() == "custom") {
    node = manager->addNode<TimeNode>("test");
  } else {
    ASSERT_THROW(manager->addNode<TimeNode>("test"), lbot::ManagementException);
  }

  std::mutex mutex;
  std::unique_lock lock(mutex);
  lbot::ConditionVariable condition;

  lbot::Clock::time_point t1 = lbot::Clock::now();

  if (GetParam() == "custom") {
    node->updateTimeAsync(t1 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitFor(lock, std::chrono::milliseconds(100)), std::cv_status::timeout);
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_GE(t2, t1 + std::chrono::milliseconds(100));

  if (GetParam() == "custom") {
    node->updateTimeAsync(t1 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitUntil(lock, t1 + std::chrono::milliseconds(200)), std::cv_status::timeout);
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_GE(t3, t1 + std::chrono::milliseconds(200));

  EXPECT_EQ(condition.waitFor(lock, std::chrono::milliseconds(000)), std::cv_status::timeout);
  lbot::Clock::time_point t4 = lbot::Clock::now();

  EXPECT_GE(t4, t3);

  if (GetParam() == "custom") {
    node->updateTimeAsync(t3 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitUntil(lock, t3 + std::chrono::milliseconds(100)), std::cv_status::timeout);
  lbot::Clock::time_point t5 = lbot::Clock::now();

  EXPECT_GE(t5, t3 + std::chrono::milliseconds(100));
  if (GetParam() == "custom") {
    EXPECT_EQ(t5, t3 + std::chrono::milliseconds(200));
  }

  std::jthread thread_a([&condition]() {
    lbot::Thread::sleepFor(std::chrono::milliseconds(100));
    condition.notifyOne();
  });

  if (GetParam() == "custom") {
    node->updateTimeAsync(t5 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node->updateTimeAsync(t5 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
  }
  EXPECT_EQ(condition.waitUntil(lock, t5 + std::chrono::milliseconds(200)), std::cv_status::no_timeout);

  lbot::Clock::time_point t6 = lbot::Clock::now();

  EXPECT_GE(t6, t5 + std::chrono::milliseconds(100));
  EXPECT_LT(t6, t5 + std::chrono::milliseconds(200));

  thread_a.join();
}

INSTANTIATE_TEST_SUITE_P(clock, ClockTest, testing::Values("system", "steady", "custom"));

}  // namespace lbot::test
}  // namespace labrat
