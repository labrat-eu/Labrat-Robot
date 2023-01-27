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

typedef struct __mavlink_message mavlink_message_t;

namespace labrat::robot::plugins {

class MavlinkNodePrivate;

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

  template <typename MessageType>
  void registerSender(Node::Sender<Message<MessageType>, mavlink_message_t>::Ptr &sender, u16 id) {
    registerSenderAdapter(Node::SenderAdapter<mavlink_message_t>::get(*sender), id);
  }

  template <typename MessageType>
  void registerReceiver(Node::Receiver<Message<MessageType>, mavlink_message_t>::Ptr &receiver) {
    receiver->setCallback(MavlinkNode::receiverCallback, priv);
  }

private:
  void registerSenderAdapter(Node::SenderAdapter<mavlink_message_t> &&adapter, u16 id);

  static void receiverCallback(Node::ReceiverAdapter<mavlink_message_t> &receiver, MavlinkNodePrivate *node);

  MavlinkNodePrivate *priv;

  friend MavlinkNodePrivate;
};

}  // namespace labrat::robot::plugins
