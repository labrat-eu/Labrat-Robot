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

#include <chrono>
#include <condition_variable>
#include <memory>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {

class Manager;

/** @cond INTERNAL */
inline namespace utils {
/** @endcond */
class Thread;
class ConditionVariable;
/** @cond INTERNAL */
}  // namespace utils
/** @endcond */

/**
 * @brief Central clock class to access the current time.
 * @details This class is compatible with the STL chrono library.
 */
class Clock
{
public:
  /** @cond INTERNAL */
  class Private;
  /** @endcond */

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
  enum class Mode
  {
    system,
    steady,
    synchronized,
    stepped
  };

  struct SynchronizationParameters
  {
    duration offset;
    i32 drift;
    std::chrono::steady_clock::duration last_sync;
  };

  struct WaiterRegistration
  {
    time_point wakeup_time;
    std::shared_ptr<std::condition_variable> condition;
    std::shared_ptr<std::cv_status> status;
    bool waitable;
  };

  static void initialize();
  static void deinitialize();
  static void cleanup();

  static Mode getMode();
  static SynchronizationParameters getSynchronizationParameters();
  static WaiterRegistration
  registerWaiter(time_point wakeup_time, std::shared_ptr<std::condition_variable> condition = std::make_shared<std::condition_variable>());

  friend class Private;

  friend class Manager;
  friend class utils::ConditionVariable;

  friend constexpr std::strong_ordering operator<=>(const WaiterRegistration &lhs, const WaiterRegistration &rhs);
};

#ifndef __clang__
// TODO: Remove in future release
static_assert(std::chrono::is_clock_v<Clock>);
#endif

}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
