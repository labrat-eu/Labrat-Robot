/**
 * @file thread.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <chrono>
#include <functional>
#include <thread>
#include <cerrno>

#include <sched.h>
#include <sys/prctl.h>

inline namespace utils {

/**
 * @brief Abstract thread wrapper.
 *
 */
class Thread {
public:
  ~Thread() = default;

protected:
  Thread() = default;

  static void setup(const std::string &name, i32 priority) {
    // Set name of the thread in applications like top/htop for better performance debugging.
    if (prctl(PR_SET_NAME, name.c_str())) {
      throw labrat::lbot::SystemException("Failed to set thread name.", errno);
    }

    if (priority > sched_get_priority_max(SCHED_RR) || priority < sched_get_priority_min(SCHED_RR)) {
      throw labrat::lbot::InvalidArgumentException("The specified scheduling priority is not within the permitted range.");
    }

    try {
      // Select real-time scheduler.
      sched_param scheduler_parameter = {.sched_priority = priority};
      if (sched_setscheduler(0, SCHED_RR, &scheduler_parameter)) {
        throw labrat::lbot::SystemException("Failed to specify scheduling algorithm.", errno);
      }
    } catch (labrat::lbot::SystemException &) {
    }
  }
};

/**
 * @brief Wrapper of a std::jthread to execute a function in an endless loop.
 *
 */
class LoopThread : public Thread {
public:
  LoopThread() = default;
  LoopThread(LoopThread &) = delete;

  LoopThread(LoopThread &&rhs) noexcept : thread(std::move(rhs.thread)) {};

  /**
   * @brief Start the thread.
   *
   * @tparam Function Type of the function to be executed repeatedly.
   * @tparam Args Types of the arguments of the function to be executed repeatedly.
   * @param function Function to be executed repeatedly.
   * @param args Arguments function to be executed repeatedly.
   */
  template <typename Function, typename... Args>
  LoopThread(Function &&function, const std::string &name, i32 priority, Args &&...args) {
    thread = std::jthread([name, priority](std::stop_token token, Function &&function, Args &&...args) {
      setup(name, priority);

      while (!token.stop_requested()) {
        [](Function function, Args ...args) {
          std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);
        }(function, args...);
      }
    }, std::forward<Function>(function), std::forward<Args>(args)...);
  }

  void operator=(LoopThread &&rhs) noexcept {
    thread = std::move(rhs.thread);
  };

private:
  std::jthread thread;
};

/**
 * @brief Wrapper of a std::jthread to execute a function in an endless loop with a minimum time interval between calls.
 *
 */
class TimerThread : public Thread {
public:
  TimerThread() = default;
  TimerThread(TimerThread &) = delete;

  TimerThread(TimerThread &&rhs) noexcept : thread(std::move(rhs.thread)) {};

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
  TimerThread(Function &&function, const std::chrono::duration<R, P> &interval, const std::string &name, i32 priority, Args &&...args) {
    thread = std::jthread([interval, name, priority](std::stop_token token, Function &&function, Args &&...args) {
      setup(name, priority);

      while (true) {
        const std::chrono::time_point<std::chrono::steady_clock> time_begin = std::chrono::steady_clock::now();

        [](Function function, Args ...args) {
          std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);
        }(function, args...);

        if (token.stop_requested()) {
          break;
        }

        std::this_thread::sleep_until(time_begin + interval);
      }
    }, std::forward<Function>(function), std::forward<Args>(args)...);
  }

  void operator=(TimerThread &&rhs) noexcept {
    thread = std::move(rhs.thread);
  };

private:
  std::jthread thread;
};

}  // namespace utils
