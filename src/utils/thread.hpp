/**
 * @file thread.hpp
 * @author Max Yvon Zimmermann
 * 
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 * 
 */

#pragma once

#include <chrono>
#include <functional>
#include <thread>

inline namespace utils {

/**
 * @brief Wrapper of a std::jthread to execute a function in an endless loop.
 *
 */
class LoopThread {
public:
  LoopThread() = default;
  LoopThread(LoopThread &) = delete;

  LoopThread(LoopThread &&rhs) : thread(std::move(rhs.thread)){};

  /**
   * @brief Start the thread.
   *
   * @tparam Function Type of the function to be executed repeatedly.
   * @tparam Args Types of the arguments of the function to be executed repeatedly.
   * @param function Function to be executed repeatedly.
   * @param args Arguments function to be executed repeatedly.
   */
  template <typename Function, typename... Args>
  LoopThread(Function &&function, Args &&...args) {
    thread = std::jthread(
      [](std::stop_token token, Function &&function, Args &&...args) {
      while (!token.stop_requested()) {
        std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);
      }
      },
      std::forward<Function>(function), std::forward<Args>(args)...);
  }

  void operator=(LoopThread &&rhs) {
    thread = std::move(rhs.thread);
  };

private:
  std::jthread thread;
};

/**
 * @brief Wrapper of a std::jthread to execute a function in an endless loop with a minimum time interval between calls.
 *
 */
class TimerThread {
public:
  TimerThread() = default;
  TimerThread(TimerThread &) = delete;

  TimerThread(TimerThread &&rhs) : thread(std::move(rhs.thread)){};

  /**
   * @brief Start the thread.
   *
   * @tparam Function Type of the function to be executed repeatedly.
   * @tparam Args Types of the arguments of the function to be executed repeatedly.
   * @param function Function to be executed repeatedly.
   * @param interval Minimum interval between calls to the function.
   * @param args Arguments function to be executed repeatedly.
   */
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
