/**
 * @file serial.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <termios.h>

inline namespace labrat {
namespace lbot {
inline namespace utils {

/**
 * @brief Convert baud rates to Linux speed values.
 *
 * @param baud_rate Baud rate to convert.
 * @return speed_t Linux speed value.
 */
speed_t toSpeed(u64 baud_rate);

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
