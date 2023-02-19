/**
 * @file node.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/base.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>

#include <span>

namespace labrat::robot::plugins {

class SerialBridgeNodePrivate;

/**
 * @brief Node to connect itself to another labrat robot system.
 * It will forward to and receive from the peer system.
 *
 */
class SerialBridgeNode final : public Node {
public:
  struct PayloadInfo {
    std::size_t topic_hash;
    flatbuffers::span<u8> payload;
  };

  /**
   * @brief Construct a new Serial Bridge Node object.
   *
   * @param environment Node environment.
   * @param port Path to the serial port.
   * @param baud_rate Baud rate of the serial port. Only a limited number of values are permitted.
   */
  SerialBridgeNode(const Node::Environment &environment, const std::string &port = "/dev/ttyUSB0", u64 baud_rate = 921600);
  ~SerialBridgeNode();

  /**
   * @brief Register a sender with the MAVLink node. Incoming messages will be forwarded onto the sender.
   * 
   * @tparam MessageType Message type of the sender.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerSender(const std::string topic_name) {
    registerGenericSender(addSender<MessageType, PayloadInfo>(topic_name, senderConversionFunction<MessageType>));
  }

  /**
   * @brief Register a receiver with the MAVLink node. Incoming messages will be forwarded onto the network.
   * 
   * @tparam MessageType Message type of the receiver.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerReceiver(const std::string topic_name) {
    typename Node::Receiver<MessageType, PayloadInfo>::Ptr receiver = addReceiver<MessageType, PayloadInfo>(topic_name, receiverConversionFunction<MessageType>);
    receiver->setCallback(&SerialBridgeNode::receiverCallback, priv);

    registerGenericReceiver(std::move(receiver));
  }

private:
  template <typename MessageType>
  static void senderConversionFunction(const PayloadInfo &source, MessageType &destination, const GenericSender<PayloadInfo> *sender) {
    flatbuffers::GetRoot<typename MessageType::Content::TableType>(source.payload.data())->UnPackTo(&destination());
  }

  template <typename MessageType>
  static void receiverConversionFunction(const MessageType &source, PayloadInfo &destination, const GenericReceiver<PayloadInfo> *receiver) {
    destination.topic_hash = receiver->getTopicInfo().topic_hash;

    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(MessageType::Content::TableType::Pack(builder, &source()));

    destination.payload = builder.GetBufferSpan();
  }

  void registerGenericSender(Node::GenericSender<PayloadInfo>::Ptr &&sender);
  void registerGenericReceiver(Node::GenericReceiver<PayloadInfo>::Ptr &&receiver);

  static void receiverCallback(Node::GenericReceiver<PayloadInfo> &receiver, SerialBridgeNodePrivate *node);

  SerialBridgeNodePrivate *priv;

  friend SerialBridgeNodePrivate;
};

}  // namespace labrat::robot::plugins
