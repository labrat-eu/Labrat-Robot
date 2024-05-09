/**
 * @file clock.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
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

inline namespace labrat {
namespace lbot {

class Manager;

inline namespace utils {
class Thread;
class ConditionVariable;
}

class Clock {
public:
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<Clock>;

  Clock() = delete;
  
  static bool initialized();
  static time_point now();

  static constexpr bool is_steady = false;

private:
  class Node;
  class SenderNode;
  class ReceiverNode;

  enum class Mode {
    system,
    steady,
    custom,
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

  static void setTime(time_point time);
  static WaiterRegistration registerWaiter(time_point wakeup_time, std::shared_ptr<std::condition_variable> condition = std::make_shared<std::condition_variable>());

  static bool is_initialized;
  static Mode mode;
  static std::atomic<time_point> current_time;
  static std::atomic_flag exit_flag;
  
  static std::shared_ptr<Node> node;

  static std::priority_queue<WaiterRegistration> waiter_queue;
  static std::mutex mutex;

  friend class ReceiverNode;

  friend class Manager;
  friend class utils::Thread;
  friend class utils::ConditionVariable;

  friend constexpr std::strong_ordering operator<=>(const WaiterRegistration& lhs, const WaiterRegistration& rhs);
};

#ifndef __clang__
// TODO: Remove in future release
static_assert(std::chrono::is_clock_v<Clock>);
#endif

}  // namespace lbot
}  // namespace labrat
