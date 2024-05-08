#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <atomic>
#include <thread>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

void test_func(std::vector<int> vec, int *loop_count, std::atomic_bool *exit_flag) {
  EXPECT_EQ(vec.size(), 5);

  if (++(*loop_count) >= 10) {
    exit_flag->store(true);
    exit_flag->notify_all();
  }
}

class ThreadTest : public LbotTest {};

TEST_F(ThreadTest, loop) {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  int loop_count = 0;
  std::atomic_bool exit_flag;

  {
    lbot::LoopThread thread(&test_func, "name", 1, std::move(vec), &loop_count, &exit_flag);
    exit_flag.wait(false);
  }
}

TEST_F(ThreadTest, timer) {
  lbot::Manager::Ptr manager = lbot::Manager::get();

  std::vector<int> vec = {1, 2, 3, 4, 5};
  int loop_count = 0;
  std::atomic_bool exit_flag;

  {
    lbot::TimerThread thread(&test_func, std::chrono::seconds(0), "name", 1, std::move(vec), &loop_count, &exit_flag);
    exit_flag.wait(false);
  }
}

}  // namespace lbot::test
}  // namespace labrat
