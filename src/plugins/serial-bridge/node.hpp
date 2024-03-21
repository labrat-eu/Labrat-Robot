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

#include <span>

inline namespace labrat {
namespace lbot::plugins {

class SerialBridgeNodePrivate;

/**
 * @brief Node to connect itself to another labrat robot system.
 * It will forward to and receive from the peer system.
 *
 */
class SerialBridgeNode final : public Node {
private:
  struct PayloadInfo {
    std::size_t topic_hash;
    flatbuffers::span<u8> payload;
  };

  template <typename T>
  requires is_flatbuffer<T>
  struct PayloadMessage : public MessageBase<T, PayloadInfo> {
    std::size_t topic_hash;
    flatbuffers::span<u8> payload;

    static void convertFrom(const PayloadInfo &source, MessageBase<T, PayloadInfo> &destination) {
      flatbuffers::GetRoot<T>(source.payload.data())->UnPackTo(&destination);
    }

    static void convertTo(const MessageBase<T, PayloadInfo> &source, PayloadInfo &destination, const void *, const GenericReceiver<PayloadInfo> &receiver) {
      destination.topic_hash = receiver.getTopicInfo().topic_hash;

      flatbuffers::FlatBufferBuilder builder;
      builder.Finish(T::Pack(builder, &source));

      destination.payload = builder.GetBufferSpan();
    }
  };

public:
  /**
   * @brief Construct a new Serial Bridge Node object.
   *
   * @param environment Node environment.
   * @param port Path to the serial port.
   * @param baud_rate Baud rate of the serial port. Only a limited number of values are permitted.
   */
  explicit SerialBridgeNode(const Node::Environment &environment, const std::string &port = "/dev/ttyUSB0", u64 baud_rate = 921600);
  ~SerialBridgeNode();

  /**
   * @brief Register a sender with the MAVLink node. Incoming messages will be forwarded onto the sender.
   *
   * @tparam MessageType Message type of the sender.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerSender(const std::string topic_name) {
    registerGenericSender(addSender<PayloadMessage<MessageType>>(topic_name));
  }

  /**
   * @brief Register a receiver with the MAVLink node. Incoming messages will be forwarded onto the network.
   *
   * @tparam MessageType Message type of the receiver.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerReceiver(const std::string topic_name) {
    typename Node::Receiver<PayloadMessage<MessageType>>::Ptr receiver = addReceiver<PayloadMessage<MessageType>>(topic_name, priv);
    receiver->setCallback(&SerialBridgeNode::receiverCallback);

    registerGenericReceiver(std::move(receiver));
  }

private:
  void registerGenericSender(Node::GenericSender<PayloadInfo>::Ptr &&sender);
  void registerGenericReceiver(Node::GenericReceiver<PayloadInfo>::Ptr &&receiver);

  static void receiverCallback(Node::GenericReceiver<PayloadInfo> &receiver, SerialBridgeNodePrivate *node);

  SerialBridgeNodePrivate *priv;

  friend SerialBridgeNodePrivate;
};

}  // namespace lbot::plugins
}  // namespace labrat
