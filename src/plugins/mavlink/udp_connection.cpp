/**
 * @file udp_connection.cpp
 * @author Max Yvon Zimmermann
 * 
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 * 
 */

#include <labrat/robot/exception.hpp>
#include <labrat/robot/plugins/mavlink/udp_connection.hpp>

#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace labrat::robot::plugins {

MavlinkUdpConnection::MavlinkUdpConnection(const std::string &address, u16 port, u16 local_port) {
  file_descriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  std::memset(&local_address, 0, sizeof(local_address));
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = INADDR_ANY;
  local_address.sin_port = htons(local_port);

  if (bind(file_descriptor, (struct sockaddr *)&local_address, sizeof(struct sockaddr)) == -1) {
    throw IoException("Failed to bind to address.", errno);
  }

  if (fcntl(file_descriptor, F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
    throw IoException("Failed to set socket options.", errno);
  }

  memset(&remote_address, 0, sizeof(remote_address));
  remote_address.sin_family = AF_INET;
  remote_address.sin_addr.s_addr = inet_addr(address.c_str());
  remote_address.sin_port = htons(port);

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event) != 0) {
    throw IoException("Failed to create epoll instance.", errno);
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);
}

MavlinkUdpConnection::~MavlinkUdpConnection() {
  close(epoll_handle);
  close(file_descriptor);
}

std::size_t MavlinkUdpConnection::write(const u8 *buffer, std::size_t size) {
  const ssize_t result = sendto(file_descriptor, buffer, size, 0, reinterpret_cast<sockaddr *>(&remote_address), sizeof(sockaddr_in));

  if (result < 0) {
    throw IoException("Failed to write to socket.", errno);
  }

  return result;
}

std::size_t MavlinkUdpConnection::read(u8 *buffer, std::size_t size) {
  {
    epoll_event event;
    const i32 result = epoll_pwait(epoll_handle, &event, 1, timeout, &signal_mask);

    if (result <= 0) {
      if ((result == -1) && (errno != EINTR)) {
        throw IoException("Failure suring epoll wait.", errno);
      }

      return 0;
    }
  }

  socklen_t address_length = sizeof(remote_address);
  const ssize_t result =
    recvfrom(file_descriptor, reinterpret_cast<void *>(buffer), size, 0, reinterpret_cast<sockaddr *>(&remote_address), &address_length);

  if (result < 0) {
    throw IoException("Failed to read from socket.", errno);
  }

  return result;
}

}  // namespace labrat::robot::plugins
