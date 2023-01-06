#pragma once

#include <labrat/robot/plugins/mavlink/connection.hpp>
#include <labrat/robot/utils/types.hpp>

#include <string>

#include <netinet/in.h>
#include <signal.h>

namespace labrat::robot::plugins {

class MavlinkUdpConnection : public MavlinkConnection {
public:
  MavlinkUdpConnection(const std::string &address = "127.0.0.1", u16 port = 14580, u16 local_port = 14540);
  virtual ~MavlinkUdpConnection();

  virtual std::size_t write(const u8 *buffer, std::size_t size);
  virtual std::size_t read(u8 *buffer, std::size_t size);

private:
  std::size_t file_descriptor;
  std::size_t epoll_handle;

  sockaddr_in local_address;
  sockaddr_in remote_address;

  static constexpr i32 timeout = 1000;
  sigset_t signal_mask;
};

}  // namespace labrat::robot::plugins
