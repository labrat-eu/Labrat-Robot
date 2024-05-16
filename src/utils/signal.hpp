/**
 * @file signal.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>

#include <signal.h>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

/**
 * @brief Wait on a process signal.
 *
 * @return int Signal number of the catched signal.
 */
int signalWait();

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
