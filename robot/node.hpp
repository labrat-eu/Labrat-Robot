#pragma once

#include <labrat/robot/exception.hpp>
#include <labrat/robot/logger.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/plugin.hpp>
#include <labrat/robot/topic.hpp>
#include <labrat/robot/utils/fifo.hpp>
#include <labrat/robot/utils/types.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace labrat::robot {

class Manager;

class Node {
private:
  template <typename MessageType>
  requires is_message<MessageType>
  static Plugin::TopicInfo getTopicInfo(const std::string &topic_name) {
    const Plugin::TopicInfo result = {
      .type_hash = typeid(MessageType).hash_code(),
      .topic_hash = std::hash<std::string>()(topic_name),
      .topic_name = topic_name,
      .type_descriptor = MessageType::Content::descriptor(),
    };

    return result;
  }

public:
  struct Environment {
    const std::string name;

    TopicMap &topic_map;
    Plugin::List &plugin_list;
  };

  ~Node() = default;

  template <typename MessageType, typename ContainerType = MessageType, typename DataType = void,
    ConversionFunction<ContainerType, MessageType, DataType> conversion_function =
      defaultSenderConversionFunction<MessageType, ContainerType>>
  requires is_message<MessageType>
  class Sender {
  private:
    Sender(const std::string &topic_name, Node &node, const DataType *user_ptr = nullptr) :
      topic_info(Node::getTopicInfo<MessageType>(topic_name)), node(node), user_ptr(user_ptr),
      topic(node.environment.topic_map.addSender<MessageType>(topic_name, this)) {
      for (Plugin &plugin : node.environment.plugin_list) {
        if (plugin.delete_flag.test()) {
          continue;
        }

        utils::Guard<u32> guard(plugin.use_count);

        plugin.topic_callback(plugin.user_ptr, topic_info);
      }
    }

    friend class Node;

    const Plugin::TopicInfo topic_info;

    Node &node;
    const DataType *user_ptr;
    TopicMap::Topic &topic;

  public:
    using Ptr = std::unique_ptr<Sender<MessageType, ContainerType, DataType, conversion_function>>;

    ~Sender() {
      flush();

      node.environment.topic_map.removeSender(topic_info.topic_name, this);
    }

    void put(const ContainerType &container) {
      for (void *pointer : topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);
        const std::size_t index = count & receiver->index_mask;

        {
          std::lock_guard guard(receiver->message_buffer[index].mutex);
          conversion_function(container, receiver->message_buffer[index].message, user_ptr);

          receiver->read_count.store(count);
        }

        receiver->read_count.notify_one();
      }

      log(container);
    }

    void flush() {
      for (void *pointer : topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);
        const std::size_t index = count & receiver->index_mask;
        const std::size_t last_index = (count - 1) & receiver->index_mask;

        {
          std::lock_guard guard(receiver->message_buffer[index].mutex);
          receiver->message_buffer[index].message = receiver->message_buffer[last_index].message;

          receiver->read_count.store(count);
        }

        receiver->read_count.notify_one();
      }
    }

    void log(const ContainerType &container) {
      if (node.environment.plugin_list.empty()) {
        return;
      }

      MessageType message;
      conversion_function(container, message, user_ptr);

      Plugin::MessageInfo message_info = {
        .topic_info = topic_info,
        .timestamp = message.getTimestamp(),
      };

      if (!message().SerializeToString(&message_info.serialized_message)) {
        throw Exception("Failure during message serialization.");
      }

      for (Plugin &plugin : node.environment.plugin_list) {
        if (plugin.delete_flag.test()) {
          continue;
        }

        utils::Guard<u32> guard(plugin.use_count);

        plugin.message_callback(plugin.user_ptr, message_info);
      }
    }

    inline const std::string &getTopicName() {
      return topic_info.topic_name;
    }
  };

  template <typename ContainerType>
  requires is_container<ContainerType>
  using ContainerSender = Sender<typename ContainerType::MessageType, ContainerType>;

  template <typename MessageType, typename ContainerType = MessageType, typename DataType = void,
    ConversionFunction<MessageType, ContainerType, DataType> conversion_function =
      defaultReceiverConversionFunction<MessageType, ContainerType>>
  requires is_message<MessageType>
  class Receiver {
  private:
    Receiver(const std::string &topic_name, Node &node, const DataType *user_ptr = nullptr, std::size_t buffer_size = 4) :
      topic_info(Node::getTopicInfo<MessageType>(topic_name)), index_mask(calculateBufferMask(buffer_size)),
      message_buffer(calculateBufferSize(buffer_size)), write_count(0), read_count(index_mask), node(node), user_ptr(user_ptr),
      topic(node.environment.topic_map.addReceiver<MessageType>(topic_info.topic_name, this)) {
      next_count = index_mask;
    }

    static constexpr std::size_t calculateBufferSize(std::size_t buffer_size) {
      if (buffer_size < 4) {
        throw Exception("The buffer size for a Receiver must be at least 4.");
      }

      std::size_t mask = std::size_t(1) << ((sizeof(std::size_t) * 8) - 1);

      while (true) {
        if (mask & buffer_size) {
          if (mask ^ buffer_size) {
            return mask << 1;
          } else {
            return mask;
          }
        }

        mask >>= 1;
      }
    }

    static constexpr std::size_t calculateBufferMask(std::size_t buffer_size) {
      return calculateBufferSize(buffer_size) - 1;
    }

    friend class Node;
    friend class Sender<MessageType>;

    const Plugin::TopicInfo topic_info;

    const std::size_t index_mask;

    class MessageBuffer {
    public:
      struct MessageData {
        MessageType message;
        std::mutex mutex;
      };

      MessageBuffer(std::size_t size) : size(size) {
        buffer = allocator.allocate(size);

        for (std::size_t i = 0; i < size; ++i) {
          std::construct_at<MessageData>(buffer + i);
        }
      }

      ~MessageBuffer() {
        for (std::size_t i = 0; i < size; ++i) {
          std::destroy_at<MessageData>(buffer + i);
        }

        allocator.deallocate(buffer, size);
      }

      MessageData &operator[](std::size_t index) volatile {
        return buffer[index];
      }

    private:
      MessageData *buffer;
      std::allocator<MessageData> allocator;

      const std::size_t size;
    };

    volatile MessageBuffer message_buffer;

    std::atomic<std::size_t> write_count;
    std::atomic<std::size_t> read_count;
    std::size_t next_count;

    Node &node;
    const DataType *user_ptr;
    TopicMap::Topic &topic;

    ContainerType latest_result;

  public:
    using Ptr = std::unique_ptr<Receiver<MessageType, ContainerType, DataType, conversion_function>>;

    ~Receiver() {
      node.environment.topic_map.removeReceiver(topic_info.topic_name, this);
    }

    ContainerType latest() {
      ContainerType result;

      const std::size_t index = read_count.load() & index_mask;

      {
        std::lock_guard guard(message_buffer[index].mutex);
        conversion_function(message_buffer[index].message, result, user_ptr);
      }

      return result;
    };

    ContainerType next() {
      ContainerType result;

      read_count.wait(next_count);
      const std::size_t index = read_count.load() & index_mask;

      {
        std::lock_guard guard(message_buffer[index].mutex);
        conversion_function(message_buffer[index].message, result, user_ptr);
      }

      next_count = index | (read_count.load() & ~index_mask);

      return result;
    };

    inline bool newDataAvailable() {
      return (read_count.load() != next_count);
    }

    inline std::size_t getBufferSize() const {
      return index_mask + 1;
    }

    inline const std::string &getTopicName() {
      return topic_info.topic_name;
    }
  };

  template <typename ContainerType>
  requires is_container<ContainerType>
  using ContainerReceiver = Receiver<typename ContainerType::MessageType, ContainerType>;

  const std::string &getName() {
    return environment.name;
  }

protected:
  Node(const Environment &environment) : environment(environment) {}

  inline Logger getLogger() const {
    return Logger(environment.name);
  }

  template <typename MessageType, typename ContainerType = MessageType, typename DataType = void,
    ConversionFunction<ContainerType, MessageType, DataType> conversion_function =
      defaultSenderConversionFunction<MessageType, ContainerType>,
    typename... Args>
  typename Sender<MessageType, ContainerType, DataType, conversion_function>::Ptr addSender(const std::string &topic_name,
    Args &&...args) requires is_message<MessageType> {
    return std::unique_ptr<Sender<MessageType, ContainerType, DataType, conversion_function>>(
      new Sender<MessageType, ContainerType, DataType, conversion_function>(topic_name, *this, std::forward<Args>(args)...));
  }

  template <typename ContainerType, typename... Args>
  typename ContainerSender<ContainerType>::Ptr addSender(const std::string &topic_name,
    Args &&...args) requires is_container<ContainerType> {
    return std::unique_ptr<ContainerSender<ContainerType>>(
      new ContainerSender<ContainerType>(topic_name, *this, nullptr, std::forward<Args>(args)...));
  }

  template <typename MessageType, typename ContainerType = MessageType, typename DataType = void,
    ConversionFunction<MessageType, ContainerType, DataType> conversion_function =
      defaultReceiverConversionFunction<MessageType, ContainerType>,
    typename... Args>
  typename Receiver<MessageType, ContainerType, DataType, conversion_function>::Ptr addReceiver(const std::string &topic_name,
    Args &&...args) requires is_message<MessageType> {
    return std::unique_ptr<Receiver<MessageType, ContainerType, DataType, conversion_function>>(
      new Receiver<MessageType, ContainerType, DataType, conversion_function>(topic_name, *this, std::forward<Args>(args)...));
  }

  template <typename ContainerType, typename... Args>
  typename ContainerReceiver<ContainerType>::Ptr addReceiver(const std::string &topic_name,
    Args &&...args) requires is_container<ContainerType> {
    return std::unique_ptr<ContainerReceiver<ContainerType>>(
      new ContainerReceiver<ContainerType>(topic_name, *this, nullptr, std::forward<Args>(args)...));
  }

  friend class Manager;

private:
  const Environment environment;
};

}  // namespace labrat::robot
