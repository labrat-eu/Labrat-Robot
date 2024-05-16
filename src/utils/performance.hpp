/**
 * @file performance.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/logger.hpp>

#include <chrono>
#include <string>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

class TimerTraceBase {
protected:
  TimerTraceBase() = default;

  static labrat::lbot::Logger default_logger;
};

template <typename DurationResolution = std::chrono::milliseconds>
class TimerTrace : public TimerTraceBase {
public:
  TimerTrace(std::string &&description, labrat::lbot::Logger &logger = default_logger) :
    logger(logger), description(std::forward<std::string>(description)), start_time(std::chrono::high_resolution_clock::now()) {}

  ~TimerTrace() {
    const DurationResolution duration =
      std::chrono::duration_cast<DurationResolution>(std::chrono::high_resolution_clock::now() - start_time);

    std::string suffix;

    if constexpr (std::is_same_v<DurationResolution, std::chrono::days>) {
      suffix = "d";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::hours>) {
      suffix = "h";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::minutes>) {
      suffix = "m";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::seconds>) {
      suffix = "s";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::milliseconds>) {
      suffix = "ms";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::microseconds>) {
      suffix = "us";
    } else if constexpr (std::is_same_v<DurationResolution, std::chrono::nanoseconds>) {
      suffix = "ns";
    } else {
      suffix = " ticks";
    }

    logger.logDebug() << description << ": " << duration.count() << suffix;
  }

private:
  labrat::lbot::Logger &logger;

  const std::string description;
  const std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
