/**
 * @file thread.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/condition.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <cerrno>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include <sched.h>
#include <sys/prctl.h>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

/**
 * @brief Abstract thread wrapper.
 *
 */
class Thread {
public:
  ~Thread() = default;

  /**
   * @brief Pause execution of the current thread for the specified duration.
   * 
   * @param duration Relative duration to sleep.
   */
  template<class Rep, class Period>
  static void sleepFor(const std::chrono::duration<Rep, Period> &duration) {
    const Clock::time_point time_begin = Clock::now();
    sleepUntil(time_begin + std::chrono::duration_cast<Clock::duration>(duration));
  }

  /**
   * @brief Pause execution of the current thread until the specified timestamp.
   * 
   * @param duration Absolute timestamp until to sleep.
   */
  template<class Duration>
  static void sleepUntil(const std::chrono::time_point<Clock, Duration> &time) {
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    ConditionVariable condition;
    condition.waitUntil(lock, time);
  }

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

  LoopThread(LoopThread &&rhs) noexcept : thread(std::move(rhs.thread)){};

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
        [](Function function, Args... args) {
          std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);
        }(function, args...);
      }
    }, std::forward<Function>(function), std::forward<Args>(args)...);
  }

  void operator=(LoopThread &&rhs) noexcept {
    thread = std::move(rhs.thread);
  }

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

  TimerThread(TimerThread &&rhs) noexcept : condition(std::move(rhs.condition)), exit_flag(std::move(rhs.exit_flag)), thread(std::move(rhs.thread)) {};

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
    condition = std::make_shared<ConditionVariable>();
    exit_flag = std::make_shared<bool>(false);

    thread = std::jthread(
      [interval, name, priority](std::stop_token token, std::shared_ptr<ConditionVariable> condition, std::shared_ptr<bool> exit_flag, Function &&function, Args &&...args) {
      setup(name, priority);

      std::mutex mutex;
      std::unique_lock lock(mutex);

      while (!token.stop_requested()) {
        const Clock::time_point time_begin = Clock::now();

        [](Function function, Args... args) {
          std::invoke<Function, Args...>(std::forward<Function>(function), std::forward<Args>(args)...);
        }(function, args...);

        if (*exit_flag) {
          break;
        }

        condition->waitUntil(lock, time_begin + interval);
      }
    }, condition, exit_flag, std::forward<Function>(function), std::forward<Args>(args)...);
  }

  ~TimerThread() {
    if (exit_flag) {
      *exit_flag = true;
    }

    if (condition) {
      condition->notifyOne();
    }
  }

  void operator=(TimerThread &&rhs) noexcept {
    condition = std::move(rhs.condition);
    exit_flag = std::move(rhs.exit_flag);
    thread = std::move(rhs.thread);
  }

private:
  std::shared_ptr<ConditionVariable> condition;
  std::shared_ptr<bool> exit_flag;
  std::jthread thread;
};

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
