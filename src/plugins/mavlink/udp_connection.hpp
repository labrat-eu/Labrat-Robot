/**
 * @file udp_connection.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/plugins/mavlink/connection.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <string>

#include <netinet/in.h>
#include <signal.h>

inline namespace labrat {
namespace lbot::plugins {

/**
 * @brief MavlinkConnection implementation for UDP sockets.
 *
 */
class MavlinkUdpConnection final : public MavlinkConnection {
public:
  /**
   * @brief Construct a new Mavlink Udp Connection object.
   *
   * @param address Remote address of the socket.
   * @param port Remote port of the socket.
   * @param local_port Local port of the socket.
   */
  MavlinkUdpConnection(const std::string &address = "127.0.0.1", u16 port = 14580, u16 local_port = 14540);

  /**
   * @brief Destroy the Mavlink Udp Connection object.
   *
   */
  virtual ~MavlinkUdpConnection();

  /**
   * @brief Write bytes to the UDP socket.
   *
   * @param buffer Buffer to be read from.
   * @param size Size of the buffer.
   * @return std::size_t Number of bytes written.
   */
  std::size_t write(const u8 *buffer, std::size_t size) override;

  /**
   * @brief Read bytes from the UDP socket.
   *
   * @param buffer Buffer to be written to.
   * @param size Size of the buffer.
   * @return std::size_t Number of bytes read.
   */
  std::size_t read(u8 *buffer, std::size_t size) override;

private:
  ssize_t file_descriptor;
  ssize_t epoll_handle;

  sockaddr_in local_address;
  sockaddr_in remote_address;

  static constexpr i32 timeout = 1000;
  sigset_t signal_mask;
};

}  // namespace lbot::plugins
}  // namespace labrat
