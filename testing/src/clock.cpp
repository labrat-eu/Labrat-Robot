#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/msg/timestamp.hpp>
#include <labrat/lbot/msg/timesync.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/utils/condition.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class TimeMessage : public MessageBase<labrat::lbot::Timestamp, lbot::Clock::time_point>
{
public:
  static void convertFrom(const Converted &source, Storage &destination)
  {
    const lbot::Clock::duration duration = source.time_since_epoch();
    destination.value = std::make_unique<foxglove::Time>(
      std::chrono::duration_cast<std::chrono::seconds>(duration).count(), (duration % std::chrono::seconds(1)).count()
    );
  }

  static void convertTo(const Storage &source, Converted &destination)
  {
    destination = lbot::Clock::time_point(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::seconds(source.value->sec()) + std::chrono::nanoseconds(source.value->nsec())
    ));
  }
};

class SynchronizedTimeNode : public lbot::Node
{
public:
  SynchronizedTimeNode()
  {
    sender_response = addSender<lbot::Timesync>("/synchronized_time/response");

    receiver_request = addReceiver<const lbot::Timesync>("/synchronized_time/request");
    receiver_request->setCallback(&SynchronizedTimeNode::callback, this);
  }

private:
  static void callback(const lbot::Message<lbot::Timesync> &message_in, SynchronizedTimeNode *self)
  {
    const auto now = std::chrono::system_clock::now().time_since_epoch();

    lbot::Message<lbot::Timesync> message_out;

    message_out.request = std::make_unique<foxglove::Time>(*message_in.request);
    message_out.response = std::make_unique<foxglove::Time>(
      std::chrono::duration_cast<std::chrono::seconds>(now).count(),
      (std::chrono::duration_cast<std::chrono::nanoseconds>(now) % std::chrono::seconds(1)).count()
    );

    self->sender_response->put(message_out);
  }

  Sender<lbot::Timesync>::Ptr sender_response;
  Receiver<const lbot::Timesync>::Ptr receiver_request;
};

class SteppedTimeNode : public UniqueNode
{
public:
  SteppedTimeNode()
  {
    sender = addSender<TimeMessage>("/stepped_time/input");
  }

  void updateTime(lbot::Clock::time_point time)
  {
    sender->put(time);
  }

  void updateTimeAsync(lbot::Clock::time_point time, std::chrono::milliseconds duration)
  {
    threads.emplace_back([&](lbot::Clock::time_point time, std::chrono::milliseconds duration) {
      std::this_thread::sleep_for(duration);
      sender->put(time);
    }, time, duration);
  }

private:
  Sender<TimeMessage>::Ptr sender;
  std::vector<std::jthread> threads;
};

class ClockTest : public LbotTestWithParam<std::string>
{};

TEST_P(ClockTest, setup)
{
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());

  ASSERT_FALSE(lbot::Clock::initialized());
  EXPECT_EQ(lbot::Clock::now(), lbot::Clock::time_point());

  lbot::Manager::Ptr manager = lbot::Manager::get();

  if (GetParam() == "synchronized") {
    ASSERT_FALSE(lbot::Clock::initialized());

    lbot::Clock::time_point t1 = lbot::Clock::now();
    lbot::Clock::time_point t2 = lbot::Clock::now();

    EXPECT_EQ(t1, t2);

    std::shared_ptr<SynchronizedTimeNode> node_synchronized = manager->addNode<SynchronizedTimeNode>("test");

    Clock::waitUntilInitializedOrExit();
    ASSERT_TRUE(lbot::Clock::initialized());

    lbot::Clock::time_point t3 = lbot::Clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    lbot::Clock::time_point t4 =
      lbot::Clock::time_point(std::chrono::duration_cast<lbot::Clock::duration>(std::chrono::system_clock::now().time_since_epoch()));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    lbot::Clock::time_point t5 = lbot::Clock::now();

    EXPECT_LE(t3, t4);
    EXPECT_LE(t3, t5);
    EXPECT_LE(t4, t5);
  } else if (GetParam() == "stepped") {
    ASSERT_FALSE(lbot::Clock::initialized());

    lbot::Clock::time_point t1 = lbot::Clock::now();
    lbot::Clock::time_point t2 = lbot::Clock::now();

    EXPECT_EQ(t1, t2);

    std::shared_ptr<SteppedTimeNode> node_stepped = manager->addNode<SteppedTimeNode>("test");
    node_stepped->updateTime(t1 + std::chrono::milliseconds(100));

    ASSERT_TRUE(lbot::Clock::initialized());
    lbot::Clock::time_point t3 = lbot::Clock::now();

    EXPECT_EQ(t3, t1 + std::chrono::milliseconds(100));
  } else {
    ASSERT_TRUE(lbot::Clock::initialized());

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
  }
}

TEST_P(ClockTest, sleep)
{
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<SynchronizedTimeNode> node_synchronized;
  if (GetParam() == "synchronized") {
    node_synchronized = manager->addNode<SynchronizedTimeNode>("test");

    Clock::waitUntilInitializedOrExit();
    ASSERT_TRUE(lbot::Clock::initialized());
  }

  std::shared_ptr<SteppedTimeNode> node_stepped;
  if (GetParam() == "stepped") {
    node_stepped = manager->addNode<SteppedTimeNode>("test");

    node_stepped->updateTime(lbot::Clock::now());
  }

  lbot::Clock::time_point t1 = lbot::Clock::now();

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t1 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepFor(std::chrono::milliseconds(100));
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_GE(t2, t1 + std::chrono::milliseconds(100));

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t1 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepUntil(t1 + std::chrono::milliseconds(200));
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_GE(t3, t1 + std::chrono::milliseconds(200));

  lbot::Thread::sleepFor(std::chrono::milliseconds(000));
  lbot::Clock::time_point t4 = lbot::Clock::now();

  EXPECT_GE(t4, t3);

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t3 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  lbot::Thread::sleepUntil(t3 + std::chrono::milliseconds(100));
  lbot::Clock::time_point t5 = lbot::Clock::now();

  EXPECT_GE(t5, t3 + std::chrono::milliseconds(100));
  if (GetParam() == "stepped") {
    EXPECT_EQ(t5, t3 + std::chrono::milliseconds(200));
  }

  bool check_a = false;
  std::jthread thread_a([&check_a]() {
    lbot::Thread::sleepFor(std::chrono::milliseconds(100));
    check_a = true;
  });

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t5 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node_stepped->updateTimeAsync(t5 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
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

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t6 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node_stepped->updateTimeAsync(t6 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
  }
  lbot::Thread::sleepUntil(t6 + std::chrono::milliseconds(100));

  EXPECT_FALSE(check_b);
  thread_b.join();
}

TEST_P(ClockTest, condition)
{
  lbot::Config::Ptr config = lbot::Config::get();
  config->setParameter("/lbot/clock_mode", GetParam());
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::shared_ptr<SynchronizedTimeNode> node_synchronized;
  if (GetParam() == "synchronized") {
    node_synchronized = manager->addNode<SynchronizedTimeNode>("test");

    Clock::waitUntilInitializedOrExit();
    ASSERT_TRUE(lbot::Clock::initialized());
  }

  std::shared_ptr<SteppedTimeNode> node_stepped;
  if (GetParam() == "stepped") {
    node_stepped = manager->addNode<SteppedTimeNode>("test");

    node_stepped->updateTime(lbot::Clock::now());
  }

  std::mutex mutex;
  std::unique_lock lock(mutex);
  lbot::ConditionVariable condition;

  lbot::Clock::time_point t1 = lbot::Clock::now();

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t1 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitFor(lock, std::chrono::milliseconds(100)), std::cv_status::timeout);
  lbot::Clock::time_point t2 = lbot::Clock::now();

  EXPECT_GE(t2, t1 + std::chrono::milliseconds(100));

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t1 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitUntil(lock, t1 + std::chrono::milliseconds(200)), std::cv_status::timeout);
  lbot::Clock::time_point t3 = lbot::Clock::now();

  EXPECT_GE(t3, t1 + std::chrono::milliseconds(200));

  EXPECT_EQ(condition.waitFor(lock, std::chrono::milliseconds(000)), std::cv_status::timeout);
  lbot::Clock::time_point t4 = lbot::Clock::now();

  EXPECT_GE(t4, t3);

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t3 + std::chrono::milliseconds(200), std::chrono::milliseconds(100));
  }
  EXPECT_EQ(condition.waitUntil(lock, t3 + std::chrono::milliseconds(100)), std::cv_status::timeout);
  lbot::Clock::time_point t5 = lbot::Clock::now();

  EXPECT_GE(t5, t3 + std::chrono::milliseconds(100));
  if (GetParam() == "stepped") {
    EXPECT_EQ(t5, t3 + std::chrono::milliseconds(200));
  }

  std::jthread thread_a([&condition]() {
    lbot::Thread::sleepFor(std::chrono::milliseconds(100));
    condition.notifyOne();
  });

  if (GetParam() == "stepped") {
    node_stepped->updateTimeAsync(t5 + std::chrono::milliseconds(100), std::chrono::milliseconds(100));
    node_stepped->updateTimeAsync(t5 + std::chrono::milliseconds(200), std::chrono::milliseconds(200));
  }
  EXPECT_EQ(condition.waitUntil(lock, t5 + std::chrono::milliseconds(200)), std::cv_status::no_timeout);

  lbot::Clock::time_point t6 = lbot::Clock::now();

  EXPECT_GE(t6, t5 + std::chrono::milliseconds(100));
  EXPECT_LT(t6, t5 + std::chrono::milliseconds(200));

  thread_a.join();
}

INSTANTIATE_TEST_SUITE_P(clock, ClockTest, testing::Values("system", "steady", "synchronized", "stepped"));

}  // namespace lbot::test
}  // namespace labrat
