/**
 * @file condition.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/exception.hpp>

#include <condition_variable>
#include <memory>
#include <atomic>

inline namespace labrat {
namespace lbot {
inline namespace utils {

class ConditionVariable {
public:
  using NativeHandle = std::condition_variable::native_handle_type;

  ConditionVariable() {
    condition = std::make_shared<std::condition_variable>();
  }

  ConditionVariable(const ConditionVariable &) = delete;
  void operator=(const ConditionVariable &) = delete;

  ~ConditionVariable() = default;

  void notifyOne() noexcept {
    condition->notify_one();
  }

  void notifyAll() noexcept {
    condition->notify_all();
  }

  void wait(std::unique_lock<std::mutex> &lock) {
    condition->wait(lock);
  }

  template<class Predicate>
  void wait(std::unique_lock<std::mutex> &lock, Predicate pred) {
    condition->wait(lock, pred);
  }

  template<class Rep, class Period>
  std::cv_status waitFor(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &duration) {
    const Clock::time_point time_begin = Clock::now();
    return waitUntil(lock, time_begin + std::chrono::duration_cast<Clock::duration>(duration));
  }

  template<class Rep, class Period, class Predicate>
  bool waitFor(std::unique_lock<std::mutex> &lock, const std::chrono::duration<Rep, Period> &duration, Predicate pred) {
    const Clock::time_point time_begin = Clock::now();
    return waitUntil(lock, time_begin + std::chrono::duration_cast<Clock::duration>(duration), pred);
  }

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

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
