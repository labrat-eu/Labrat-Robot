#pragma once

#include <chrono>
#include <functional>
#include <thread>

inline namespace utils {

class TimerThread {
public:
  TimerThread() = default;
  TimerThread(TimerThread &) = delete;

  TimerThread(TimerThread &&rhs) : thread(std::move(rhs.thread)) {};

  template <typename Function, typename R, typename P, typename... Args>
  TimerThread(Function &&function, const std::chrono::duration<R, P> &interval, Args &&...args) {
    thread = std::jthread(
      [interval](std::stop_token token, Function &&function, Args &&...args) {
      while (true) {
        const std::chrono::time_point<std::chrono::steady_clock> time_begin = std::chrono::steady_clock::now();

        std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);

        if (token.stop_requested()) {
          break;
        }

        std::this_thread::sleep_until(time_begin + interval);
      }
      },
      std::forward<Function>(function), std::forward<Args>(args)...);
  }

  void operator=(TimerThread &&rhs) {
    thread = std::move(rhs.thread);
  };

private:
  std::jthread thread;
};

}  // namespace utils
