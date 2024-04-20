/**
 * @file node.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/udp-bridge/node.hpp>
#include <labrat/lbot/utils/string.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <unistd.h>

inline namespace labrat {
namespace lbot::plugins {

class UdpBridge::NodePrivate {
public:
  struct BridgeSender {
    std::unordered_map<std::string, Node::GenericSender<UdpBridge::Node::PayloadInfo>::Ptr> map;
    std::unordered_map<std::size_t, Node::GenericSender<UdpBridge::Node::PayloadInfo>::Ptr &> adapter;
  } sender;

  struct BridgeReceiver {
    std::vector<Node::GenericReceiver<UdpBridge::Node::PayloadInfo>::Ptr> vector;
  } receiver;

  NodePrivate(const std::string &address, u16 port, u16 local_port, UdpBridge::Node &node);
  ~NodePrivate();

  void writePayloadMessage(const UdpBridge::Node::PayloadInfo &message);

private:
  // Please do not send gigantic packets.
  static constexpr std::size_t buffer_size = 16384;
  static constexpr std::size_t payload_size = buffer_size / 2;

  struct TopicInfo {
    std::size_t topic_hash;
    std::string_view topic_name;
    std::string_view type_name;
  };

  struct HeaderWire {
    u8 magic;
    u8 version_major;
    u8 version_minor;

    enum class Type : u8 {
      payload = 0,
      topic_info = 1,
      topic_request = 2,
    } type;

    u16 flags;
    u16 length;

    u64 topic_hash;

    static constexpr u8 magic_check = 0x69;
    static constexpr u8 version_major_check = GIT_VERSION_MAJOR;
    static constexpr u8 version_minor_check = GIT_VERSION_MINOR;

    inline void init() {
      magic = magic_check;
      version_major = version_major_check;
      version_minor = version_minor_check;
    }
  };

  struct TopicWire {
    HeaderWire header;

    u16 topic_name_end;
    u16 type_name_end;

    std::array<u8, payload_size> data;
  };

  static_assert(sizeof(TopicWire) <= buffer_size);

  struct PayloadWire {
    HeaderWire header;

    std::array<u8, payload_size> data;
  };

  static_assert(sizeof(PayloadWire) <= buffer_size);

  void readLoop();

  void readPayloadMessage(const UdpBridge::Node::PayloadInfo &message);
  void readTopicInfoMessage(const TopicInfo &message);
  void readTopicRequestMessage(std::size_t topic_hash);

  void writeTopicInfoMessage(const TopicInfo &message);
  void writeTopicRequestMessage(std::size_t topic_hash);

  std::size_t write(const u8 *buffer, std::size_t size);
  std::size_t read(u8 *buffer, std::size_t size);

  UdpBridge::Node &node;

  ssize_t file_descriptor;
  ssize_t epoll_handle;

  sockaddr_in local_address;
  sockaddr_in remote_address;

  static constexpr i32 timeout = 1000;
  sigset_t signal_mask;

  std::mutex mutex;

  utils::LoopThread read_thread;
};

UdpBridge::UdpBridge(const PluginEnvironment &environment, const std::string &address, u16 port, u16 local_port) :
  SharedPlugin(environment) {
  node = addNode<UdpBridge::Node>(getName(), address, port, local_port);
}

UdpBridge::~UdpBridge() = default;

UdpBridge::Node::Node(const NodeEnvironment &environment, const std::string &address, u16 port, u16 local_port) : lbot::Node(environment) {
  priv = new UdpBridge::NodePrivate(address, port, local_port, *this);
}

UdpBridge::Node::~Node() {
  delete priv;
}

void UdpBridge::Node::registerGenericSender(Node::GenericSender<UdpBridge::Node::PayloadInfo>::Ptr &&sender) {
  if (!priv->sender.map
         .try_emplace(sender->getTopicInfo().topic_name, std::forward<Node::GenericSender<UdpBridge::Node::PayloadInfo>::Ptr>(sender))
         .second) {
    throw ManagementException("A sender has already been registered for the topic name '" + sender->getTopicInfo().topic_name + "'.");
  }
}

void UdpBridge::Node::registerGenericReceiver(Node::GenericReceiver<UdpBridge::Node::PayloadInfo>::Ptr &&receiver) {
  priv->receiver.vector.emplace_back(std::forward<Node::GenericReceiver<UdpBridge::Node::PayloadInfo>::Ptr>(receiver));
}

void UdpBridge::Node::receiverCallback(Node::GenericReceiver<UdpBridge::Node::PayloadInfo> &receiver, UdpBridge::NodePrivate *node) {
  node->writePayloadMessage(receiver.latest());
}

UdpBridge::NodePrivate::NodePrivate(const std::string &address, u16 port, u16 local_port, UdpBridge::Node &node) : node(node) {
  file_descriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (file_descriptor == -1) {
    throw IoException("Failed to create socket.", errno);
  }

  std::memset(&local_address, 0, sizeof(local_address));
  local_address.sin_family = AF_INET;
  local_address.sin_addr.s_addr = INADDR_ANY;
  local_address.sin_port = htobe16(local_port);

  if (bind(file_descriptor, (struct sockaddr *)&local_address, sizeof(struct sockaddr)) == -1) {
    throw IoException("Failed to bind to address.", errno);
  }

  if (fcntl(file_descriptor, F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
    throw IoException("Failed to set socket options.", errno);
  }

  memset(&remote_address, 0, sizeof(remote_address));
  remote_address.sin_family = AF_INET;
  remote_address.sin_addr.s_addr = inet_addr(address.c_str());
  remote_address.sin_port = htobe16(port);

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event)) {
    throw IoException("Failed to create epoll instance.", errno);
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);

  read_thread = utils::LoopThread(&UdpBridge::NodePrivate::readLoop, "udp bridge", 1, this);
}

UdpBridge::NodePrivate::~NodePrivate() = default;

void UdpBridge::NodePrivate::readLoop() {
  std::array<u8, buffer_size> buffer;
  const std::size_t number_bytes = read(buffer.data(), buffer_size);

  if (number_bytes == 0) {
    return;
  }

  HeaderWire *header = reinterpret_cast<HeaderWire *>(buffer.data());
  header->length = be16toh(header->length);
  header->topic_hash = be64toh(header->topic_hash);

  if (header->magic != HeaderWire::magic_check) {
    node.getLogger().logError() << "Received message with wrong magic byte.";
    return;
  }
  if (header->version_major != HeaderWire::version_major_check) {
    node.getLogger().logError() << "Received message of different major version number, discarding message.";
    return;
  }
  if (header->version_minor != HeaderWire::version_minor_check) {
    node.getLogger().logWarning() << "Received message of different minor version number.";
  }
  if (header->length != number_bytes - sizeof(HeaderWire)) {
    node.getLogger().logError() << "Datagram and header length mismatch, discarding message. (" << header->length << "/"
                                << number_bytes - sizeof(HeaderWire) << ")";
    return;
  }

  switch (header->type) {
    case (HeaderWire::Type::payload): {
      PayloadWire *raw = reinterpret_cast<PayloadWire *>(buffer.data());
      UdpBridge::Node::PayloadInfo message;

      message.topic_hash = raw->header.topic_hash;
      message.payload = flatbuffers::span<u8>(raw->data.data(), header->length);

      readPayloadMessage(message);

      break;
    }

    case (HeaderWire::Type::topic_info): {
      TopicWire *raw = reinterpret_cast<TopicWire *>(buffer.data());
      TopicInfo message;

      raw->topic_name_end = be16toh(raw->topic_name_end);
      raw->type_name_end = be16toh(raw->type_name_end);

      message.topic_hash = raw->header.topic_hash;

      std::size_t offset = 0;

      message.topic_name = std::string_view(reinterpret_cast<const char *>(raw->data.data()) + offset, raw->topic_name_end - offset);
      offset = raw->topic_name_end;

      message.type_name = std::string_view(reinterpret_cast<const char *>(raw->data.data()) + offset, raw->type_name_end - offset);

      readTopicInfoMessage(message);

      break;
    }

    case (HeaderWire::Type::topic_request): {
      readTopicRequestMessage(header->topic_hash);

      break;
    }

    default: {
      node.getLogger().logError() << "Received unknown message type.";
    }
  }
}

void UdpBridge::NodePrivate::readPayloadMessage(const UdpBridge::Node::PayloadInfo &message) {
  auto iterator = sender.adapter.find(message.topic_hash);

  if (iterator == sender.adapter.end()) {
    node.getLogger().logDebug() << "Received bridge message without adapter entry (remote topic hash: " << message.topic_hash << ").";
    writeTopicRequestMessage(message.topic_hash);

    return;
  }

  iterator->second->put(message);
}

void UdpBridge::NodePrivate::readTopicInfoMessage(const TopicInfo &message) {
  auto iterator = sender.map.find(std::string(message.topic_name));

  if (iterator == sender.map.end()) {
    node.getLogger().logWarning() << "No registered handling implementation found (topic name: " << message.topic_name << ").";
    return;
  }

  if (iterator->second->getTopicInfo().type_name != message.type_name) {
    node.getLogger().logWarning() << "Local and remote type names do not match (" << iterator->second->getTopicInfo().type_name << "/"
                                  << message.type_name << ").";
    return;
  }

  sender.adapter.try_emplace(message.topic_hash, iterator->second);
}

void UdpBridge::NodePrivate::readTopicRequestMessage(std::size_t topic_hash) {
  for (const Node::GenericReceiver<UdpBridge::Node::PayloadInfo>::Ptr &receiver : receiver.vector) {
    if (receiver->getTopicInfo().topic_hash == topic_hash) {
      TopicInfo message;
      message.topic_hash = receiver->getTopicInfo().topic_hash;
      message.topic_name = receiver->getTopicInfo().topic_name;
      message.type_name = receiver->getTopicInfo().type_name;

      writeTopicInfoMessage(message);
      return;
    }
  }

  node.getLogger().logDebug() << "Requested topic hash receiver not found (topic hash: " << topic_hash << ").";
}

void UdpBridge::NodePrivate::writePayloadMessage(const UdpBridge::Node::PayloadInfo &message) {
  PayloadWire raw;

  if (message.payload.size() > payload_size) {
    node.getLogger().logError() << "Maximum payload size exceeded.";
    return;
  }

  const u16 length = offsetof(PayloadWire, data) + message.payload.size();

  raw.header.init();
  raw.header.type = HeaderWire::Type::payload;
  raw.header.flags = 0;
  raw.header.length = htobe16(length - sizeof(HeaderWire));
  raw.header.topic_hash = htobe64(message.topic_hash);

  std::memcpy(raw.data.data(), message.payload.data(), message.payload.size());

  std::lock_guard guard(mutex);

  write(reinterpret_cast<u8 *>(&raw), length);
}

void UdpBridge::NodePrivate::writeTopicInfoMessage(const TopicInfo &message) {
  if (message.topic_name.empty()) {
    node.getLogger().logError() << "The sent topic name must not be empty.";
    return;
  }
  if (message.type_name.empty()) {
    node.getLogger().logError() << "The sent type name must not be empty.";
    return;
  }

  TopicWire raw;
  const u16 length = offsetof(TopicWire, data) + message.topic_name.size() + message.type_name.size();

  if (length > payload_size) {
    node.getLogger().logError() << "Maximum payload size exceeded.";
    return;
  }

  raw.header.init();
  raw.header.type = HeaderWire::Type::topic_info;
  raw.header.flags = 0;
  raw.header.length = htobe16(length - sizeof(HeaderWire));
  raw.header.topic_hash = htobe64(message.topic_hash);

  std::size_t offset = 0;

  std::memcpy(raw.data.data() + offset, message.topic_name.data(), message.topic_name.size());
  offset += message.topic_name.size();
  raw.topic_name_end = htobe16(offset);

  std::memcpy(raw.data.data() + offset, message.type_name.data(), message.type_name.size());
  offset += message.type_name.size();
  raw.type_name_end = htobe16(offset);

  std::lock_guard guard(mutex);

  write(reinterpret_cast<u8 *>(&raw), length);
}

void UdpBridge::NodePrivate::writeTopicRequestMessage(std::size_t topic_hash) {
  HeaderWire raw;

  raw.init();
  raw.type = HeaderWire::Type::topic_request;
  raw.flags = 0;
  raw.length = 0;
  raw.topic_hash = htobe64(topic_hash);

  std::lock_guard guard(mutex);

  write(reinterpret_cast<u8 *>(&raw), sizeof(HeaderWire));
}

std::size_t UdpBridge::NodePrivate::write(const u8 *buffer, std::size_t size) {
  const ssize_t result = sendto(file_descriptor, buffer, size, 0, reinterpret_cast<sockaddr *>(&remote_address), sizeof(sockaddr_in));

  if (result < 0) {
    throw IoException("Failed to write to socket.", errno);
  }

  return result;
}

std::size_t UdpBridge::NodePrivate::read(u8 *buffer, std::size_t size) {
  {
    epoll_event event;
    const ssize_t result = epoll_pwait(epoll_handle, &event, 1, timeout, &signal_mask);

    if (result <= 0) {
      if ((result == -1) && (errno != EINTR)) {
        throw IoException("Failure during epoll wait.", errno);
      }

      return 0;
    }
  }

  socklen_t address_length = sizeof(remote_address);
  const ssize_t result =
    recvfrom(file_descriptor, reinterpret_cast<void *>(buffer), size, 0, reinterpret_cast<sockaddr *>(&remote_address), &address_length);

  if (result < 0) {
    throw IoException("Failed to read from socket.", errno);
  }

  return result;
}

}  // namespace lbot::plugins
}  // namespace labrat
