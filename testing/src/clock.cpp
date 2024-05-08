#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/thread.hpp>
#include <labrat/lbot/msg/timestamp.fb.hpp>

#include <mutex>
#include <chrono>
#include <thread>

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
    std::thread([&](lbot::Clock::time_point time, std::chrono::milliseconds duration){
      std::this_thread::sleep_for(duration);
      sender->put(time);
    }, time, duration).detach();
  }

private:
  Sender<TimeMessage>::Ptr sender;
};

class ClockTest : public LbotTest {};

TEST_F(ClockTest, system) {
  EXPECT_THROW(lbot::Clock::now(), lbot::ClockException);

  ASSERT_FALSE(lbot::Clock::initialized());
  lbot::Manager::Ptr manager = lbot::Manager::get();
  ASSERT_TRUE(lbot::Clock::initialized());

  lbot::Clock::time_point t1 = lbot::Clock::now();
  lbot::Clock::time_point t2 = lbot::Clock::time_point(std::chrono::duration_cast<lbot::Clock::duration>(std::chrono::system_clock::now().time_since_epoch()));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_LE(t1, t2);
  EXPECT_LE(t1, t3);
  EXPECT_LE(t2, t3);

  EXPECT_THROW(manager->addNode<TimeNode>("test"), lbot::ManagementException);
}

TEST_F(ClockTest, steady) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", "steady");

  EXPECT_THROW(lbot::Clock::now(), lbot::ClockException);

  ASSERT_FALSE(lbot::Clock::initialized());
  lbot::Manager::Ptr manager = lbot::Manager::get();
  ASSERT_TRUE(lbot::Clock::initialized());


  lbot::Clock::time_point t1 = lbot::Clock::now();
  lbot::Clock::time_point t2 = lbot::Clock::time_point(std::chrono::duration_cast<lbot::Clock::duration>(std::chrono::steady_clock::now().time_since_epoch()));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_LE(t1, t2);
  EXPECT_LE(t1, t3);
  EXPECT_LE(t2, t3);

  EXPECT_THROW(manager->addNode<TimeNode>("test"), lbot::ManagementException);
}

TEST_F(ClockTest, custom) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", "custom");

  EXPECT_THROW(lbot::Clock::now(), lbot::ClockException);

  ASSERT_FALSE(lbot::Clock::initialized());
  lbot::Manager::Ptr manager = lbot::Manager::get();
  ASSERT_TRUE(lbot::Clock::initialized());


  lbot::Clock::time_point t1 = lbot::Clock::now();
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_EQ(t1, t2);

  std::shared_ptr<TimeNode> node = manager->addNode<TimeNode>("test");
  node->updateTime(t1 + std::chrono::seconds(1));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_LE(t1, t3);
}

TEST_F(ClockTest, sleep) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", "custom");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<TimeNode> node = manager->addNode<TimeNode>("test");
  lbot::Clock::time_point t1 = lbot::Clock::now();

  node->updateTimeAsync(t1 + std::chrono::seconds(1), std::chrono::milliseconds(100));
  lbot::Thread::sleepFor(std::chrono::seconds(1));
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_GE(t2, t1 + std::chrono::seconds(1));

  node->updateTimeAsync(t1 + std::chrono::seconds(2), std::chrono::milliseconds(100));
  lbot::Thread::sleepUntil(t1 + std::chrono::seconds(2));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_GE(t3, t1 + std::chrono::seconds(2));

  lbot::Thread::sleepFor(std::chrono::seconds(0));
  lbot::Clock::time_point t4 = lbot::Clock::now();

  EXPECT_GE(t4, t3);

  node->updateTimeAsync(t3 + std::chrono::seconds(2), std::chrono::milliseconds(100));
  lbot::Thread::sleepUntil(t3 + std::chrono::seconds(1));
  lbot::Clock::time_point t5 = lbot::Clock::now();

  EXPECT_GE(t5, t3 + std::chrono::seconds(2));
}

}  // namespace lbot::test
}  // namespace labrat
