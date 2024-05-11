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

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot::plugins {

/**
 * @brief Plugin to connect to a MAVLink network.
 * It will publish messages received from peer systems to specific topics.
 * It will also receive specific topics and forward their contents to its peer systems via MAVLink.
 * In addition, servers are provided to issue commands and read out parameters.
 *
 */
class Mavlink final : public Plugin {
public:
  /**
   * @brief Construct a new Mavlink object.
   *
   * @param environment Plugin environment.
   * @param connection MavlinkConnection to be used by this instance.
   */
  Mavlink(MavlinkConnection::Ptr &&connection);
  ~Mavlink();

  /**
   * @brief Register a sender with the MAVLink node. Incoming MAVLink messages will be forwarded onto the sender.
   *
   * @tparam MessageType Message type of the sender.
   * @param topic_name Name of the topic.
   * @param conversion_function Conversion function used by the sender.
   * @param id Message ID of the underlying MAVLink message.
   */
  template <typename MessageType>
  void registerSender(std::string topic_name, u16 id) {
    node->registerSender<MessageType>(std::forward<std::string>(topic_name), id);
  }

  /**
   * @brief Register a receiver with the MAVLink node. Incoming messages will be forwarded onto the MAVLink network.
   *
   * @tparam MessageType Message type of the receiver.
   * @param topic_name Name of the topic.
   * @param conversion_function Conversion function used by the receiver.
   */
  template <typename MessageType>
  void registerReceiver(std::string topic_name) {
    node->registerReceiver<MessageType>(std::forward<std::string>(topic_name));
  }

private:
  class NodePrivate;

  struct SystemInfo {
    NodePrivate *node;
    u8 channel_id;
    u8 system_id;
    u8 component_id;
  };

  class Node final : public lbot::Node {
  private:
    template <typename FlatbufferType>
    class MavlinkMessage : public MessageBase<FlatbufferType, mavlink_message_t> {
    public:
      static void convertFrom(const mavlink_message_t &source, MessageBase<FlatbufferType, mavlink_message_t> &destination);
      static void convertTo(const MessageBase<FlatbufferType, mavlink_message_t> &source, mavlink_message_t &destination,
        const Mavlink::SystemInfo *info);
    };

  public:
    /**
     * @brief Construct a new Mavlink Node object.
     *
     * @param connection MavlinkConnection to be used by this instance.
     */
    Node(MavlinkConnection::Ptr &&connection);
    ~Node();

    /**
     * @brief Get the MAVLink system info about the local system.
     *
     * @return SystemInfo& System information.
     */
    [[nodiscard]] SystemInfo &getSystemInfo() const;

    /**
     * @brief Register a sender with the MAVLink node. Incoming MAVLink messages will be forwarded onto the sender.
     *
     * @tparam MessageType Message type of the sender.
     * @param topic_name Name of the topic.
     * @param conversion_function Conversion function used by the sender.
     * @param id Message ID of the underlying MAVLink message.
     */
    template <typename MessageType>
    void registerSender(std::string &&topic_name, u16 id) {
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
    void registerReceiver(std::string &&topic_name) {
      typename Node::Receiver<MessageType>::Ptr receiver = addReceiver<MessageType>(topic_name);
      receiver->setCallback(&Node::receiverCallback, &getSystemInfo());

      registerGenericReceiver(std::move(receiver));
    }

  private:
    void registerGenericSender(Node::GenericSender<mavlink_message_t>::Ptr &&sender, u16 id);
    void registerGenericReceiver(Node::GenericReceiver<mavlink_message_t>::Ptr &&receiver);

    static void receiverCallback(const mavlink_message_t &message, SystemInfo *system_info);

    NodePrivate *priv;

    friend NodePrivate;
  };

  std::shared_ptr<Node> node;
};

}  // namespace lbot::plugins
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
