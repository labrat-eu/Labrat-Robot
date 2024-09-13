/**
 * @file clock.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <memory>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <compare>
#include <string>
#include <vector>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {

class Manager;
class FreezeGuard;

/** @cond INTERNAL */
inline namespace utils {
/** @endcond */
class Thread;
class ConditionVariable;
/** @cond INTERNAL */
}
/** @endcond */

/**
 * @brief Central clock class to access the current time.
 * @details This class is compatible with the STL chrono library.
 */
class Clock {
public:
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<Clock>;

  Clock() = delete;
  
  /**
   * @brief Check whether the clock has been intialized by the manager.
   * 
   * @return true The clock is initialized and time related functions work properly.
   * @return false The clock is not yet initialized. Time related functions may not work properly.
   */
  static bool initialized();

  /**
   * @brief Wait until the clock is initialized.
   * 
   * @details The function may also return after a spurious wakeup or when the program is shutting down.
   */
  static void waitUntilInitialized();

  /**
   * @brief Get the current time.
   * 
   * @return time_point The current time.
   */
  static time_point now() noexcept;

  /**
   * @brief Convert a time point into a human readable string.
   * 
   * @param time Time point to format.
   * @return std::string Formatted time.
   */
  static std::string format(const time_point time);

  static constexpr bool is_steady = false;

private:
  class Node;
  class SenderNode;
  class SynchronizedNode;
  class SteppedNode;

  enum class Mode {
    system,
    steady,
    synchronized,
    stepped
  };

  struct WaiterRegistration {
    time_point wakeup_time;
    std::shared_ptr<std::condition_variable> condition;
    std::shared_ptr<std::cv_status> status;
    bool waitable;
  };

  static void initialize();
  static void deinitialize();
  static void cleanup();

  static void synchronize(duration offset, i32 drift, std::chrono::steady_clock::duration now);

  static void setTime(time_point time);
  static WaiterRegistration registerWaiter(time_point wakeup_time, std::shared_ptr<std::condition_variable> condition = std::make_shared<std::condition_variable>());

  static Mode mode;
  static bool is_initialized;
  static std::condition_variable is_initialized_condition;
  static std::atomic_flag exit_flag;

  static duration current_offset;
  static i32 current_drift;
  static std::chrono::steady_clock::duration last_sync;

  static std::atomic<time_point> current_time;

  static thread_local time_point freeze_time;
  static thread_local u32 freeze_count;
  
  static std::vector<std::shared_ptr<Node>> nodes;

  static std::priority_queue<WaiterRegistration> waiter_queue;
  static std::mutex mutex;

  friend class SynchronizedNode;
  friend class SteppedNode;

  friend class Manager;
  friend class utils::Thread;
  friend class utils::ConditionVariable;

  friend class FreezeGuard;

  friend constexpr std::strong_ordering operator<=>(const WaiterRegistration& lhs, const WaiterRegistration& rhs);
};

#ifndef __clang__
// TODO: Remove in future release
static_assert(std::chrono::is_clock_v<Clock>);
#endif

/**
 * @brief Guard class that freezes the time of the local thread until it is destroyed.
 * 
 */
class FreezeGuard {
public:
  inline FreezeGuard() {
    Clock::freeze_time = Clock::now();
    ++Clock::freeze_count;
  }

  inline ~FreezeGuard() {
    --Clock::freeze_count;
  }
};

}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
