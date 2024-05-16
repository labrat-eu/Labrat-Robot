/**
 * @file node.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>

#include <span>
#include <forward_list>
#include <memory>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot::plugins {

class SerialBridge final : public Plugin {
public:
  /**
   * @brief Construct a new Serial Bridge object.
   *
   * @param port Path to the serial port.
   * @param baud_rate Baud rate of the serial port. Only a limited number of values are permitted.
   */
  SerialBridge(const std::string &port = "/dev/ttyUSB0", u64 baud_rate = 921600);
  ~SerialBridge();

  /**
   * @brief Register a sender with the MAVLink node. Incoming messages will be forwarded onto the sender.
   *
   * @tparam MessageType Message type of the sender.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerSender(const std::string topic_name) {
    node->registerSender<MessageType>(topic_name);
  }

  /**
   * @brief Register a receiver with the MAVLink node. Incoming messages will be forwarded onto the network.
   *
   * @tparam MessageType Message type of the receiver.
   * @param topic_name Name of the topic.
   */
  template <typename MessageType>
  void registerReceiver(const std::string topic_name) {
    node->registerReceiver<MessageType>(topic_name);
  }

private:
  class NodePrivate;

  /**
   * @brief Node to connect itself to another labrat robot system.
   * It will forward to and receive from the peer system.
   *
   */
  class Node final : public lbot::Node {
  private:
    struct PayloadInfo {
      std::size_t topic_hash;
      flatbuffers::span<u8> payload;
    };

    template <typename T>
    requires is_flatbuffer<T> struct PayloadMessage : public MessageBase<T, PayloadInfo> {
      std::size_t topic_hash;
      flatbuffers::span<u8> payload;

      static void convertFrom(const PayloadInfo &source, MessageBase<T, PayloadInfo> &destination) {
        flatbuffers::GetRoot<T>(source.payload.data())->UnPackTo(&destination);
      }

      static void convertTo(const MessageBase<T, PayloadInfo> &source, PayloadInfo &destination, const TopicInfo *topic_info) {
        destination.topic_hash = topic_info->topic_hash;

        flatbuffers::FlatBufferBuilder builder;
        builder.Finish(T::Pack(builder, &source));

        destination.payload = builder.GetBufferSpan();
      }
    };

  public:
    /**
     * @brief Construct a new Serial Bridge Node object.
     *
     * @param port Path to the serial port.
     * @param baud_rate Baud rate of the serial port. Only a limited number of values are permitted.
     */
    explicit Node(const std::string &port, u64 baud_rate);
    ~Node();

    /**
     * @brief Register a sender with the bridge. Incoming messages will be forwarded onto the sender.
     *
     * @tparam MessageType Message type of the sender.
     * @param topic_name Name of the topic.
     */
    template <typename MessageType>
    void registerSender(const std::string topic_name) {
      registerGenericSender(addSender<PayloadMessage<MessageType>>(topic_name));
    }

    /**
     * @brief Register a receiver with the bridge. Incoming messages will be forwarded onto the network.
     *
     * @tparam FlatbufferType Message type of the receiver.
     * @param topic_name Name of the topic.
     */
    template <typename FlatbufferType>
    requires is_flatbuffer<FlatbufferType>
    void registerReceiver(const std::string topic_name) {
      TopicInfo *topic_info = &topic_infos.emplace_front(TopicInfo::get<PayloadMessage<FlatbufferType>>(topic_name));
      typename Node::Receiver<PayloadMessage<FlatbufferType>>::Ptr receiver = addReceiver<PayloadMessage<FlatbufferType>>(topic_name, topic_info);
      receiver->setCallback(&Node::receiverCallback, priv);

      registerGenericReceiver(std::move(receiver));
    }

  private:
    void registerGenericSender(Node::GenericSender<PayloadInfo>::Ptr &&sender);
    void registerGenericReceiver(Node::GenericReceiver<PayloadInfo>::Ptr &&receiver);

    static void receiverCallback(const PayloadInfo &receiver, NodePrivate *node);

    std::forward_list<TopicInfo> topic_infos;
    NodePrivate *priv;

    friend NodePrivate;
  };

  std::shared_ptr<Node> node;
};

}  // namespace lbot::plugins
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
