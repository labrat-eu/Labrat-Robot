/**
 * @file signal.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>

#include <signal.h>

inline namespace labrat {
namespace lbot {
inline namespace utils {

/**
 * @brief Wait on a process signal.
 *
 * @return int Signal number of the catched signal.
 */
int signalWait();

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
