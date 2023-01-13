/**
 * @file serial_connection.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/exception.hpp>
#include <labrat/robot/plugins/mavlink/serial_connection.hpp>

#include <cstring>

#include <sys/epoll.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

namespace labrat::robot::plugins {

static speed_t toSpeed(u64 baud_rate);

MavlinkSerialConnection::MavlinkSerialConnection(const std::string &port, u64 baud_rate) {
  file_descriptor = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

  if (file_descriptor == -1) {
    throw IoException("Could not open serial port.", errno);
  }

  if (!isatty(file_descriptor)) {
    throw IoException("The opened flie descriptor is not a serial port.");
  }

  termios config;
  if (tcgetattr(file_descriptor, &config) < 0) {
    throw IoException("Failed to read serial port configuration.", errno);
  }

  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
  config.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);

#ifdef OLCUC
  config.c_oflag &= ~OLCUC;
#endif

#ifdef ONOEOT
  config.c_oflag &= ~ONOEOT;
#endif

  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;

  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 10;

  // Set baud rate.
  const speed_t speed = toSpeed(baud_rate);
  cfsetispeed(&config, speed);
  cfsetospeed(&config, speed);

  if(tcsetattr(file_descriptor, TCSAFLUSH, &config) < 0) {
    throw IoException("Failed to write serial port configuration.", errno);
  }

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event) != 0) {
    throw IoException("Failed to create epoll instance.", errno);
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);
}

MavlinkSerialConnection::~MavlinkSerialConnection() {
  close(epoll_handle);
  close(file_descriptor);
}

std::size_t MavlinkSerialConnection::write(const u8 *buffer, std::size_t size) {
  const ssize_t result = ::write(file_descriptor, buffer, size);

  if (result < 0) {
    throw IoException("Failed to write to socket.", errno);
  }

  // Wait until all data has been written.
  tcdrain(file_descriptor);

  return result;
}

std::size_t MavlinkSerialConnection::read(u8 *buffer, std::size_t size) {
  {
    epoll_event event;
    const i32 result = epoll_pwait(epoll_handle, &event, 1, timeout, &signal_mask);

    if (result <= 0) {
      if ((result == -1) && (errno != EINTR)) {
        throw IoException("Failure during epoll wait.", errno);
      }

      return 0;
    }
  }

  const std::size_t result = ::read(file_descriptor, reinterpret_cast<void *>(buffer), size);

  if (result < 0) {
    throw IoException("Failed to read from serial port.", errno);
  }

  return result;
}

static speed_t toSpeed(u64 baud_rate) {
  switch (baud_rate) {
    case(57600): {
      return B57600;
    }
    
    case(115200): {
      return B115200;
    }

    case(230400): {
      return B230400;
    }

    case(460800): {
      return B460800;
    }

    case(500000): {
      return B500000;
    }

    case(576000): {
      return B576000;
    }

    case(921600): {
      return B921600;
    }

    case(1000000): {
      return B1000000;
    }
    
    case(1152000): {
      return B1152000;
    }

    case(1500000): {
      return B1500000;
    }

    case(2000000): {
      return B2000000;
    }

    case(2500000): {
      return B2500000;
    }

    case(3000000): {
      return B3000000;
    }

    case(3500000): {
      return B3500000;
    }

    case(4000000): {
      return B4000000;
    }

    default: {
      throw IoException("Unknown baud rate.");
    }
  }
}



}  // namespace labrat::robot::plugins
