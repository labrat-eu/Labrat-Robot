/**
 * @file async.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

/**
 * @brief Execution policy for callback functions.
 *
 */
enum class ExecutionPolicy
{
  /** Execution is performed within the same thread. Preferred for callbacks with a small computational cost. */
  serial,
  /** A new thread will be created. This allows multiple callbacks to be executed concurrently. Preferred for callbacks with a large
   * computational cost. */
  parallel,
};

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
