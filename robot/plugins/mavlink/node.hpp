#pragma once

#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugins/mavlink/connection.hpp>

#include <atomic>
#include <mutex>
#include <thread>

typedef struct __mavlink_message mavlink_message_t;

namespace labrat::robot::plugins {

struct MavlinkSender;
struct MavlinkReceiver;

class MavlinkNode : public Node {
public:
  MavlinkNode(const Node::Environment &environment, MavlinkConnection::Ptr &&connection);
  ~MavlinkNode();

  struct SystemInfo {
    u8 channel_id;
    u8 system_id;
    u8 component_id;
  };

private:
  void readLoop();
  void heartbeatLoop();

  void readMessage(const mavlink_message_t &message);
  void writeMessage(const mavlink_message_t &message);

  MavlinkConnection::Ptr connection;

  std::thread read_thread;
  std::thread write_thread;
  std::thread heartbeat_thread;
  std::atomic_flag exit_flag;

  MavlinkSender *sender;
  MavlinkReceiver *receiver;

  std::mutex mutex;

  SystemInfo system_info;

  static constexpr std::size_t buffer_size = 1024;

  friend MavlinkSender;
  friend MavlinkReceiver;
};

}  // namespace labrat::robot::plugins
