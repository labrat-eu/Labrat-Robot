#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

inline namespace utils {

class TimerThread {
public:
  TimerThread() {
    skip_cleanup = true;
  }

  TimerThread(TimerThread &) = delete;

  TimerThread(TimerThread &&rhs) : thread(std::move(rhs.thread)), exit_flag(rhs.exit_flag) {
    skip_cleanup = rhs.skip_cleanup;
    rhs.skip_cleanup = true;
  };

  template <typename Function, typename R, typename P, typename... Args>
  TimerThread(Function &&function, const std::chrono::duration<R, P> &interval, Args &&...args) {
    exit_flag = std::make_shared<std::atomic_flag>();

    thread = std::thread(
      [interval](Function &&function, std::shared_ptr<std::atomic_flag> &&exit_flag, Args &&...args) {
      while (true) {
        const std::chrono::time_point<std::chrono::steady_clock> time_begin = std::chrono::steady_clock::now();

        std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);

        if (exit_flag->test()) {
          break;
        }

        std::this_thread::sleep_until(time_begin + interval);
      }
      },
      std::forward<Function>(function), std::shared_ptr<std::atomic_flag>(exit_flag), std::forward<Args>(args)...);

    skip_cleanup = false;
  }

  ~TimerThread() {
    if (!skip_cleanup) {
      exit_flag->test_and_set();
      thread.join();
    }
  }

  void operator=(TimerThread &&rhs) {
    if (!skip_cleanup) {
      exit_flag->test_and_set();
      thread.join();
    }

    thread = std::move(rhs.thread);
    exit_flag = rhs.exit_flag;

    skip_cleanup = rhs.skip_cleanup;
    rhs.skip_cleanup = true;
  };

private:
  std::thread thread;
  std::shared_ptr<std::atomic_flag> exit_flag;
  bool skip_cleanup;
};

}  // namespace utils
