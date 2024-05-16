/**
 * @file node.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/serial-bridge/node.hpp>
#include <labrat/lbot/utils/serial.hpp>
#include <labrat/lbot/utils/string.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <cstring>
#include <iomanip>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <crc_cpp.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/signal.h>
#include <termios.h>
#include <unistd.h>

inline namespace labrat {
namespace lbot::plugins {

class SerialBridge::NodePrivate {
public:
  struct BridgeSender {
    std::unordered_map<std::string, Node::GenericSender<SerialBridge::Node::PayloadInfo>::Ptr> map;
    std::unordered_map<std::size_t, Node::GenericSender<SerialBridge::Node::PayloadInfo>::Ptr &> adapter;
  } sender;

  struct BridgeReceiver {
    std::vector<Node::GenericReceiver<SerialBridge::Node::PayloadInfo>::Ptr> vector;
  } receiver;

  NodePrivate(const std::string &port, u64 baud_rate, SerialBridge::Node &node);
  ~NodePrivate();

  void writePayloadMessage(const SerialBridge::Node::PayloadInfo &message);

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

    u16 checksum;

    static constexpr u8 magic_check = 0x69;
    static constexpr u8 version_major_check = GIT_VERSION_MAJOR;
    static constexpr u8 version_minor_check = GIT_VERSION_MINOR;

    inline void init() {
      magic = magic_check;
      version_major = version_major_check;
      version_minor = version_minor_check;
    }

    inline void finalize() {
      checksum = calculateChecksum();
    }

    [[nodiscard]] inline bool check() const {
      return checksum == calculateChecksum();
    }

    [[nodiscard]] inline u16 calculateChecksum() const {
      crc_cpp::crc16_arc crc;
      const u8 *buffer = reinterpret_cast<const u8 *>(this);

      for (std::size_t i = 0; i < offsetof(HeaderWire, checksum); ++i) {
        crc.update(buffer[i]);
      }

      return htobe16(crc.final());
    }
  };

  template <std::size_t Size>
  class ChecksumBuffer : public std::array<u8, Size + sizeof(u16)> {
  public:
    inline void finalize(std::size_t offset, u8 *buffer = nullptr) {
      u8 *ptr = (buffer == nullptr) ? this->data() : buffer;
      u16 *checksum = reinterpret_cast<u16 *>(ptr + offset);

      *checksum = calculateChecksum(offset, ptr);
    }

    inline bool check(std::size_t offset, const u8 *buffer = nullptr) const {
      const u8 *ptr = (buffer == nullptr) ? this->data() : buffer;
      const u16 *checksum = reinterpret_cast<const u16 *>(ptr + offset);

      return *checksum == calculateChecksum(offset, ptr);
    }

    inline u16 calculateChecksum(std::size_t offset, const u8 *buffer) const {
      crc_cpp::crc16_arc crc;

      for (std::size_t i = 0; i < offset; ++i) {
        crc.update(buffer[i]);
      }

      return htobe16(crc.final());
    }
  };

  struct TopicWire {
    HeaderWire header;

    u16 topic_name_end;
    u16 type_name_end;

    ChecksumBuffer<payload_size> data;
  };

  static_assert(sizeof(TopicWire) <= buffer_size);

  struct PayloadWire {
    HeaderWire header;

    ChecksumBuffer<payload_size> data;
  };

  static_assert(sizeof(PayloadWire) <= buffer_size);

  void readLoop();
  void unescapeAndProcessMessage(const u8 *buffer, std::size_t size);
  void processMessage(u8 *buffer, std::size_t size);

  void escapeAndWriteMessage(const u8 *buffer, std::size_t size);

  void readPayloadMessage(const SerialBridge::Node::PayloadInfo &message);
  void readTopicInfoMessage(const TopicInfo &message);
  void readTopicRequestMessage(std::size_t topic_hash);

  void writeTopicInfoMessage(const TopicInfo &message);
  void writeTopicRequestMessage(std::size_t topic_hash);

  std::size_t write(const u8 *buffer, std::size_t size) const;
  std::size_t read(u8 *buffer, std::size_t size) const;

  SerialBridge::Node &node;

  std::size_t file_descriptor;
  std::size_t epoll_handle;

  static constexpr i32 timeout = 1000;
  sigset_t signal_mask;

  std::mutex mutex;

  std::array<u8, buffer_size> decode_buffer;
  std::size_t decode_index;

  bool escape_flag;

  LoopThread read_thread;

  static constexpr u8 end_code = 0x57;
  static constexpr u8 esc_code = 0x59;
  static constexpr u8 esc_offset = 0x10;
};

SerialBridge::SerialBridge(const std::string &port, u64 baud_rate) : Plugin() {
  node = addNode<SerialBridge::Node>(getName(), port, baud_rate);
}

SerialBridge::~SerialBridge() = default;

SerialBridge::Node::Node(const std::string &port, u64 baud_rate) {
  priv = new SerialBridge::NodePrivate(port, baud_rate, *this);
}

SerialBridge::Node::~Node() {
  delete priv;
}

void SerialBridge::Node::registerGenericSender(Node::GenericSender<SerialBridge::Node::PayloadInfo>::Ptr &&sender) {
  if (!priv->sender.map
         .try_emplace(sender->getTopicInfo().topic_name, std::forward<Node::GenericSender<SerialBridge::Node::PayloadInfo>::Ptr>(sender))
         .second) {
    throw ManagementException("A sender has already been registered for the topic name '" + sender->getTopicInfo().topic_name + "'.");
  }
}

void SerialBridge::Node::registerGenericReceiver(Node::GenericReceiver<SerialBridge::Node::PayloadInfo>::Ptr &&receiver) {
  priv->receiver.vector.emplace_back(std::forward<Node::GenericReceiver<SerialBridge::Node::PayloadInfo>::Ptr>(receiver));
}

void SerialBridge::Node::receiverCallback(const SerialBridge::Node::PayloadInfo &message, SerialBridge::NodePrivate *node) {
  node->writePayloadMessage(message);
}

SerialBridge::NodePrivate::NodePrivate(const std::string &port, u64 baud_rate, SerialBridge::Node &node) : node(node) {
  file_descriptor = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

  if (file_descriptor == -1) {
    throw IoException("Could not open serial port.", errno);
  }

  if (!isatty(file_descriptor)) {
    throw IoException("The opened flie descriptor is not a serial port.");
  }

  termios config;
  if (tcgetattr(file_descriptor, &config) < 0) {
    throw IoException("Failed to read serial port configuration.", errno);
  }

  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
  config.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);

#ifdef OLCUC
  config.c_oflag &= ~OLCUC;
#endif

#ifdef ONOEOT
  config.c_oflag &= ~ONOEOT;
#endif

  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;

  config.c_cc[VMIN] = sizeof(HeaderWire);
  config.c_cc[VTIME] = 1;

  // Set baud rate.
  const speed_t speed = toSpeed(baud_rate);
  cfsetispeed(&config, speed);
  cfsetospeed(&config, speed);

  if (tcsetattr(file_descriptor, TCSAFLUSH, &config) < 0) {
    throw IoException("Failed to write serial port configuration.", errno);
  }

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event)) {
    throw IoException("Failed to create epoll instance.", errno);
  }

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);

  decode_index = 0;
  escape_flag = false;

  read_thread = LoopThread(&SerialBridge::NodePrivate::readLoop, "serial bridge", 1, this);
}

SerialBridge::NodePrivate::~NodePrivate() = default;

void SerialBridge::NodePrivate::readLoop() {
  std::array<u8, buffer_size> buffer;
  const std::size_t size = read(buffer.data(), buffer_size);

  unescapeAndProcessMessage(buffer.data(), size);
}

void SerialBridge::NodePrivate::unescapeAndProcessMessage(const u8 *buffer, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    u8 current_byte = buffer[i];

    if (current_byte == end_code) {
      processMessage(decode_buffer.data(), decode_index);
      decode_index = 0;
      continue;
    }

    if (current_byte == esc_code) {
      escape_flag = true;
      continue;
    }

    if (escape_flag) {
      current_byte -= esc_offset;
      escape_flag = false;
    }

    decode_buffer[decode_index] = current_byte;
    ++decode_index;
  }
}

void SerialBridge::NodePrivate::escapeAndWriteMessage(const u8 *buffer, std::size_t size) {
  std::array<u8, buffer_size> encode_buffer;
  std::size_t encode_index = 0;

  for (std::size_t i = 0; i < size; ++i) {
    u8 current_byte = buffer[i];

    if (current_byte == end_code || current_byte == esc_code) {
      encode_buffer[encode_index] = esc_code;
      ++encode_index;

      encode_buffer[encode_index] = current_byte + esc_offset;
    } else {
      encode_buffer[encode_index] = current_byte;
    }

    ++encode_index;
  }

  encode_buffer[encode_index] = end_code;
  ++encode_index;

  write(encode_buffer.data(), encode_index);
}

void SerialBridge::NodePrivate::processMessage(u8 *buffer, std::size_t size) {
  HeaderWire *header = reinterpret_cast<HeaderWire *>(buffer);

  if (size > buffer_size) {
    node.getLogger().logWarning() << "Received message is too long.";
    return;
  }

  if (size < sizeof(HeaderWire)) {
    node.getLogger().logWarning() << "Received message with wrong header length.";
    return;
  }

  if (!header->check()) {
    node.getLogger().logWarning() << "Received message with wrong header checksum.";
    return;
  }

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
  if (header->length != size - sizeof(HeaderWire)) {
    node.getLogger().logError() << "Datagram and header length mismatch, discarding message. (" << header->length << "/"
                                << size - sizeof(HeaderWire) << ")";
    return;
  }

  switch (header->type) {
    case (HeaderWire::Type::payload): {
      PayloadWire *raw = reinterpret_cast<PayloadWire *>(buffer);

      if (!raw->data.check(header->length - sizeof(u16))) {
        node.getLogger().logWarning() << "Received message with wrong checksum.";
        return;
      }

      SerialBridge::Node::PayloadInfo message;

      message.topic_hash = raw->header.topic_hash;
      message.payload = flatbuffers::span<u8>(raw->data.data(), header->length);

      readPayloadMessage(message);

      break;
    }

    case (HeaderWire::Type::topic_info): {
      TopicWire *raw = reinterpret_cast<TopicWire *>(buffer);

      if (!raw->data.check(header->length - sizeof(u16), reinterpret_cast<const u8 *>(&raw->topic_name_end))) {
        node.getLogger().logWarning() << "Received message with wrong checksum.";
        return;
      }

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

void SerialBridge::NodePrivate::readPayloadMessage(const SerialBridge::Node::PayloadInfo &message) {
  auto iterator = sender.adapter.find(message.topic_hash);

  if (iterator == sender.adapter.end()) {
    node.getLogger().logDebug() << "Received bridge message without adapter entry (remote topic hash: " << message.topic_hash << ").";
    writeTopicRequestMessage(message.topic_hash);

    return;
  }

  iterator->second->put(message);
}

void SerialBridge::NodePrivate::readTopicInfoMessage(const TopicInfo &message) {
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

void SerialBridge::NodePrivate::readTopicRequestMessage(std::size_t topic_hash) {
  for (const Node::GenericReceiver<SerialBridge::Node::PayloadInfo>::Ptr &receiver : receiver.vector) {
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

void SerialBridge::NodePrivate::writePayloadMessage(const SerialBridge::Node::PayloadInfo &message) {
  PayloadWire raw;

  if (message.payload.size() > payload_size) {
    node.getLogger().logError() << "Maximum payload size exceeded.";
    return;
  }

  const u16 length = offsetof(PayloadWire, data) + message.payload.size() + sizeof(u16);

  raw.header.init();
  raw.header.type = HeaderWire::Type::payload;
  raw.header.flags = 0;
  raw.header.length = htobe16(length - sizeof(HeaderWire));
  raw.header.topic_hash = htobe64(message.topic_hash);
  raw.header.finalize();

  std::memcpy(raw.data.data(), message.payload.data(), message.payload.size());
  raw.data.finalize(message.payload.size());

  std::lock_guard guard(mutex);

  escapeAndWriteMessage(reinterpret_cast<u8 *>(&raw), length);
}

void SerialBridge::NodePrivate::writeTopicInfoMessage(const TopicInfo &message) {
  if (message.topic_name.empty()) {
    node.getLogger().logError() << "The sent topic name must not be empty.";
    return;
  }
  if (message.type_name.empty()) {
    node.getLogger().logError() << "The sent type name must not be empty.";
    return;
  }

  TopicWire raw;
  const u16 length = offsetof(TopicWire, data) + message.topic_name.size() + message.type_name.size() + sizeof(u16);

  if (length > payload_size) {
    node.getLogger().logError() << "Maximum payload size exceeded.";
    return;
  }

  raw.header.init();
  raw.header.type = HeaderWire::Type::topic_info;
  raw.header.flags = 0;
  raw.header.length = htobe16(length - sizeof(HeaderWire));
  raw.header.topic_hash = htobe64(message.topic_hash);
  raw.header.finalize();

  std::size_t offset = 0;

  std::memcpy(raw.data.data() + offset, message.topic_name.data(), message.topic_name.size());
  offset += message.topic_name.size();
  raw.topic_name_end = htobe16(offset);

  std::memcpy(raw.data.data() + offset, message.type_name.data(), message.type_name.size());
  offset += message.type_name.size();
  raw.type_name_end = htobe16(offset);

  raw.data.finalize(length - sizeof(HeaderWire) - sizeof(u16), reinterpret_cast<u8 *>(&raw.topic_name_end));

  std::lock_guard guard(mutex);

  escapeAndWriteMessage(reinterpret_cast<u8 *>(&raw), length);
}

void SerialBridge::NodePrivate::writeTopicRequestMessage(std::size_t topic_hash) {
  HeaderWire raw;

  raw.init();
  raw.type = HeaderWire::Type::topic_request;
  raw.flags = 0;
  raw.length = 0;
  raw.topic_hash = htobe64(topic_hash);
  raw.finalize();

  std::lock_guard guard(mutex);

  escapeAndWriteMessage(reinterpret_cast<u8 *>(&raw), sizeof(HeaderWire));
}

std::size_t SerialBridge::NodePrivate::write(const u8 *buffer, std::size_t size) const {
  const ssize_t result = ::write(file_descriptor, buffer, size);

  if (result < 0 && errno != EAGAIN) {
    throw IoException("Failed to write to socket.", errno);
  }

  // Wait until all data has been written.
  tcdrain(file_descriptor);

  return result;
}

std::size_t SerialBridge::NodePrivate::read(u8 *buffer, std::size_t size) const {
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

  const ssize_t result = ::read(file_descriptor, reinterpret_cast<void *>(buffer), size);

  if (result < 0) {
    throw IoException("Failed to read from serial port.", errno);
  }

  return result;
}

}  // namespace lbot::plugins
}  // namespace labrat
