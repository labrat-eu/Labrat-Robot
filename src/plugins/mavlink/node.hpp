/**
 * @file node.hpp
 * @author Max Yvon Zimmermann
 * 
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 * 
 */

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
struct MavlinkServer;

/**
 * @brief Node to connect itself to a MAVLink network.
 * It will publish messages received from peer systems to specific topics.
 * It will also receive specific topics and forward their contents to its peer systems via MAVLink.
 * In addition, servers are provided to issue commands and read out parameters.
 *
 */
class MavlinkNode : public Node {
public:
  /**
   * @brief Construct a new Mavlink Node object
   *
   * @param environment Node environment.
   * @param connection MavlinkConnection to be used by this instance.
   */
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
  std::thread heartbeat_thread;
  std::atomic_flag exit_flag;

  MavlinkSender *sender;
  MavlinkReceiver *receiver;
  MavlinkServer *server;

  std::mutex mutex;

  SystemInfo system_info;

  static constexpr std::size_t buffer_size = 1024;

  friend MavlinkSender;
  friend MavlinkReceiver;
  friend MavlinkServer;
};

}  // namespace labrat::robot::plugins
