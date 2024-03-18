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

class UdpBridgeNodePrivate;

/**
 * @brief Node to connect itself to another labrat robot system.
 * It will forward to and receive from the peer system.
 *
 */
class UdpBridgeNode final : public Node {
public:
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

    static void convertTo(const MessageBase<T, PayloadInfo> &source, PayloadInfo &destination, const GenericReceiver<PayloadInfo> *receiver) {
      destination.topic_hash = receiver->getTopicInfo().topic_hash;

      flatbuffers::FlatBufferBuilder builder;
      builder.Finish(T::Pack(builder, &source));

      destination.payload = builder.GetBufferSpan();
    }
  };

  /**
   * @brief Construct a new Udp Bridge Node object.
   *
   * @param environment Node environment.
   * @param address Remote address of the socket.
   * @param port Remote port of the socket.
   * @param local_port Local port of the socket.
   */
  UdpBridgeNode(const Node::Environment &environment, const std::string &address, u16 port, u16 local_port);
  ~UdpBridgeNode();

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
    receiver->setCallback(&UdpBridgeNode::receiverCallback);

    registerGenericReceiver(std::move(receiver));
  }

private:
  void registerGenericSender(Node::GenericSender<PayloadInfo>::Ptr &&sender);
  void registerGenericReceiver(Node::GenericReceiver<PayloadInfo>::Ptr &&receiver);

  static void receiverCallback(Node::GenericReceiver<PayloadInfo> &receiver, UdpBridgeNodePrivate *node);

  UdpBridgeNodePrivate *priv;

  friend UdpBridgeNodePrivate;
};

}  // namespace lbot::plugins
}  // namespace labrat
