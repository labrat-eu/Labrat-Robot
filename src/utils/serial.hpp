/**
 * @file serial.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/base.hpp>
#include <labrat/robot/exception.hpp>
#include <labrat/robot/utils/types.hpp>

#include <termios.h>

inline namespace utils {

/**
 * @brief Convert baud rates to Linux speed values.
 * 
 * @param baud_rate Baud rate to convert.
 * @return speed_t Linux speed value.
 */
speed_t toSpeed(u64 baud_rate) {
  switch (baud_rate) {
    case (57600): {
      return B57600;
    }

    case (115200): {
      return B115200;
    }

    case (230400): {
      return B230400;
    }

    case (460800): {
      return B460800;
    }

    case (500000): {
      return B500000;
    }

    case (576000): {
      return B576000;
    }

    case (921600): {
      return B921600;
    }

    case (1000000): {
      return B1000000;
    }

    case (1152000): {
      return B1152000;
    }

    case (1500000): {
      return B1500000;
    }

    case (2000000): {
      return B2000000;
    }

    case (2500000): {
      return B2500000;
    }

    case (3000000): {
      return B3000000;
    }

    case (3500000): {
      return B3500000;
    }

    case (4000000): {
      return B4000000;
    }

    default: {
      throw labrat::robot::IoException("Unknown baud rate.");
    }
  }
}

}  // namespace utils
