/**
 * @file serial.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/utils/serial.hpp>

inline namespace labrat {
namespace lbot {
inline namespace utils {

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
      throw labrat::lbot::IoException("Unknown baud rate.");
    }
  }
}

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
