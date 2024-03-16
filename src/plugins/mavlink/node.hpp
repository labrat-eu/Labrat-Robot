/**
 * @file node.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/mavlink/connection.hpp>

typedef struct __mavlink_message mavlink_message_t;

inline namespace labrat {
namespace lbot::plugins {

class MavlinkNodePrivate;

/**
 * @brief Node to connect itself to a MAVLink network.
 * It will publish messages received from peer systems to specific topics.
 * It will also receive specific topics and forward their contents to its peer systems via MAVLink.
 * In addition, servers are provided to issue commands and read out parameters.
 *
 */
class MavlinkNode final : public Node {
public:
  struct SystemInfo {
    u8 channel_id;
    u8 system_id;
    u8 component_id;
  };

private:
  template <typename FlatbufferType>
  class MavlinkMessage : public UnsafeMessage<FlatbufferType, mavlink_message_t> {
  public:
    static void convertFrom(const mavlink_message_t &source, MavlinkMessage<FlatbufferType> &destination);
    static void convertTo(const MavlinkMessage<FlatbufferType> &source, mavlink_message_t &destination,
      const MavlinkNode::SystemInfo *info);
  };

public:
  /**
   * @brief Construct a new Mavlink Node object.
   *
   * @param environment Node environment.
   * @param connection MavlinkConnection to be used by this instance.
   */
  MavlinkNode(const Node::Environment &environment, MavlinkConnection::Ptr &&connection);
  ~MavlinkNode();

  /**
   * @brief Get the MAVLink system info about the local system.
   *
   * @return const SystemInfo& System information.
   */
  const SystemInfo &getSystemInfo() const;

  /**
   * @brief Register a sender with the MAVLink node. Incoming MAVLink messages will be forwarded onto the sender.
   *
   * @tparam MessageType Message type of the sender.
   * @param topic_name Name of the topic.
   * @param conversion_function Conversion function used by the sender.
   * @param id Message ID of the underlying MAVLink message.
   */
  template <typename MessageType>
  void registerSender(const std::string topic_name, u16 id) {
    registerGenericSender(addSender<MessageType>(topic_name), id);
  }

  /**
   * @brief Register a receiver with the MAVLink node. Incoming messages will be forwarded onto the MAVLink network.
   *
   * @tparam MessageType Message type of the receiver.
   * @param topic_name Name of the topic.
   * @param conversion_function Conversion function used by the receiver.
   */
  template <typename MessageType>
  void registerReceiver(const std::string topic_name, const SystemInfo *system_info) {
    typename Node::Receiver<MessageType>::Ptr receiver = addReceiver<MessageType>(topic_name, system_info);
    receiver->setCallback(&MavlinkNode::receiverCallback, priv);

    registerGenericReceiver(std::move(receiver));
  }

private:
  void registerGenericSender(Node::GenericSender<mavlink_message_t>::Ptr &&sender, u16 id);
  void registerGenericReceiver(Node::GenericReceiver<mavlink_message_t>::Ptr &&receiver);

  static void receiverCallback(Node::GenericReceiver<mavlink_message_t> &receiver, MavlinkNodePrivate *node);

  MavlinkNodePrivate *priv;

  friend MavlinkNodePrivate;
};

}  // namespace lbot::plugins
}  // namespace labrat
