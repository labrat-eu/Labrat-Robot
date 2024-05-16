/**
 * @file condition.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/exception.hpp>

#include <condition_variable>
#include <memory>
#include <atomic>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

/**
 * @brief A condition variable primitive.
 * @details This class is compatible with the STL std::mutex and std::unique_lock classes.
 */
class ConditionVariable {
public:
  using NativeHandle = std::condition_variable::native_handle_type;

  ConditionVariable() {
    condition = std::make_shared<std::condition_variable>();
  }

  ConditionVariable(const ConditionVariable &) = delete;
  void operator=(const ConditionVariable &) = delete;

  ~ConditionVariable() = default;

  /**
   * @brief Notify one thread waiting on the condition variable.
   */
  void notifyOne() noexcept {
    condition->notify_one();
  }

  /**
   * @brief Notify all threads waiting on the condition variable.
   */
  void notifyAll() noexcept {
    condition->notify_all();
  }

  /**
   * @brief Wait on the condition variable.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   */
  void wait(std::unique_lock<std::mutex> &lock) {
    condition->wait(lock);
  }

  /**
   * @brief Wait on the condition variable.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details When the thread wakes up, pred will be called. If it returns false, the thread will continue waiting.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   * @param pred Callable object to call on a wakeup.
   */
  template<class Predicate>
  void wait(std::unique_lock<std::mutex> &lock, Predicate pred) {
    condition->wait(lock, pred);
  }

  /**
   * @brief Wait on the condition variable. Once the specified timeout is reached, the thread will unblock regardless of any notify calls.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   * @param duration Relative duration after the thread will wakeup.
   */
  template<class Rep, class Period>
  std::cv_status waitFor(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &duration) {
    const Clock::time_point time_begin = Clock::now();
    return waitUntil(lock, time_begin + std::chrono::duration_cast<Clock::duration>(duration));
  }

  /**
   * @brief Wait on the condition variable. Once the specified timeout is reached, the thread will unblock regardless of any notify calls.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details When the thread wakes up, pred will be called. If it returns false, the thread will continue waiting.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   * @param duration Relative duration after the thread will wakeup.
   * @param pred Callable object to call on a wakeup.
   */
  template<class Rep, class Period, class Predicate>
  bool waitFor(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &duration, Predicate pred) {
    const Clock::time_point time_begin = Clock::now();
    return waitUntil(lock, time_begin + std::chrono::duration_cast<Clock::duration>(duration), pred);
  }

  /**
   * @brief Wait on the condition variable. Once the specified timeout is reached, the thread will unblock regardless of any notify calls.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   * @param time Absolute timestamp after the thread will wakeup.
   */
  template<class Duration>
  std::cv_status waitUntil(std::unique_lock<std::mutex> &lock, const std::chrono::time_point<Clock, Duration> &time) {
    switch (Clock::mode) {
      case (Clock::Mode::system): {
        return condition->wait_until(lock, std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(time.time_since_epoch())));
      }
      
      case (Clock::Mode::steady): {
        return condition->wait_until(lock, std::chrono::steady_clock::time_point(std::chrono::duration_cast<std::chrono::steady_clock::duration>(time.time_since_epoch())));
      }

      case (Clock::Mode::custom): {
        Clock::WaiterRegistration registration = Clock::registerWaiter(std::chrono::time_point_cast<Clock::duration>(time), condition);

        if (registration.waitable) {
          condition->wait(lock);
          return *registration.status;
        }

        return std::cv_status::timeout;
      }

      default: {
        throw ClockException("Unknown clock mode");
      }
    }
  }

  /**
   * @brief Wait on the condition variable. Once the specified timeout is reached, the thread will unblock regardless of any notify calls.
   * @details Blocks execution of the current thread until the condition is notified or a spurious wakeup occurs.
   * @details When the thread wakes up, pred will be called. If it returns false, the thread will continue waiting.
   * @details The provided lock must be locked when calling this function. While waiting the associated mutex will be unlocked. Once the function returns the lock is once again acquired atomically.
   * 
   * @param lock Lock to unlock while waiting.
   * @param time Absolute timestamp after the thread will wakeup.
   * @param pred Callable object to call on a wakeup.
   */
  template<class Duration, class Predicate>
  bool waitUntil(std::unique_lock<std::mutex> &lock, const std::chrono::time_point<Clock, Duration> &time, Predicate pred) {
    while (!pred()) {
      if (wait_until(lock, time) == std::cv_status::timeout) {
        return pred();
      }
    }

    return true;
  }

private:
  std::shared_ptr<std::condition_variable> condition;
};

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
