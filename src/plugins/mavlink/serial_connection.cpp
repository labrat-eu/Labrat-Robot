/**
 * @file serial_connection.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/plugins/mavlink/serial_connection.hpp>
#include <labrat/lbot/utils/serial.hpp>

#include <cstring>

#include <fcntl.h>
#include <sys/epoll.h>
#include <termios.h>
#include <unistd.h>

inline namespace labrat {
namespace lbot::plugins {

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

  config.c_cc[VMIN] = 1;
  config.c_cc[VTIME] = 10;

  // Set baud rate.
  const speed_t speed = utils::toSpeed(baud_rate);
  cfsetispeed(&config, speed);
  cfsetospeed(&config, speed);

  if (tcsetattr(file_descriptor, TCSAFLUSH, &config) < 0) {
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

  if (result < 0 && errno != EAGAIN) {
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

}  // namespace lbot::plugins
}  // namespace labrat
