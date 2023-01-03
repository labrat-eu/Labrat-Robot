#pragma once

#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugins/mavlink/connection.hpp>

#include <atomic>
#include <thread>

typedef struct __mavlink_message mavlink_message_t;

namespace labrat::robot::plugins {

struct MavlinkSender;

class MavlinkNode : public Node {
public:
  MavlinkNode(const Node::Environment &environment, MavlinkConnection::Ptr &&connection);
  ~MavlinkNode();

private:
  void receiveLoop();
  void heartbeatLoop();

  void receiveMessage(mavlink_message_t &message);

  MavlinkConnection::Ptr connection;

  std::thread receive_thread;
  std::thread heartbeat_thread;
  std::atomic_flag exit_flag;

  MavlinkSender *sender;

  u8 channel_id;
  u8 system_id;
  u8 component_id;

  static constexpr std::size_t buffer_size = 1024;
};

}  // namespace labrat::robot::plugins
