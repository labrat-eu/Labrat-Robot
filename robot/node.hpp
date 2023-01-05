#pragma once

#include <labrat/robot/exception.hpp>
#include <labrat/robot/logger.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/plugin.hpp>
#include <labrat/robot/service.hpp>
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
    ServiceMap &service_map;
    Plugin::List &plugin_list;
  };

  ~Node() = default;

  template <typename MessageType, typename ContainerType = MessageType>
  requires is_message<MessageType>
  class Sender {
  private:
    Sender(const std::string &topic_name, Node &node,
      ConversionFunction<ContainerType, MessageType> conversion_function = defaultSenderConversionFunction<MessageType, ContainerType>,
      const void *user_ptr = nullptr) :
      topic_info(Node::getTopicInfo<MessageType>(topic_name)),
      node(node), conversion_function(conversion_function), user_ptr(user_ptr),
      topic(node.environment.topic_map.addSender<MessageType>(topic_name, this)) {
      for (Plugin &plugin : node.environment.plugin_list) {
        if (plugin.delete_flag.test()) {
          continue;
        }

        utils::Guard<u32> guard(plugin.use_count);

        plugin.topic_callback(plugin.user_ptr, topic_info);
      }

      callback_flag = false;
    }

    friend class Node;

    const Plugin::TopicInfo topic_info;

    Node &node;
    const ConversionFunction<ContainerType, MessageType> conversion_function;
    const void *const user_ptr;
    TopicMap::Topic &topic;

    bool callback_flag;

  public:
    using Ptr = std::unique_ptr<Sender<MessageType, ContainerType>>;

    ~Sender() {
      flush();

      node.environment.topic_map.removeSender(topic.name, this);
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

        if (receiver->callback.valid()) {
          if (callback_flag) {
            throw RuntimeRecursionException("Callback recursion detected.");
          }

          callback_flag = true;
          receiver->callback(*receiver, receiver->callback_ptr);
          callback_flag = false;
        }
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

        receiver->flush_flag.clear();
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
        throw SerializationException("Failure during message serialization.");
      }

      for (Plugin &plugin : node.environment.plugin_list) {
        if (plugin.delete_flag.test()) {
          continue;
        }

        utils::Guard<u32> guard(plugin.use_count);

        plugin.message_callback(plugin.user_ptr, message_info);
      }
    }

    inline const std::string &getTopicName() const {
      return topic.name;
    }
  };

  template <typename ContainerType>
  requires is_container<ContainerType>
  using ContainerSender = Sender<typename ContainerType::MessageType, ContainerType>;

  template <typename MessageType, typename ContainerType = MessageType>
  requires is_message<MessageType>
  class Receiver {
  private:
    Receiver(const std::string &topic_name, Node &node,
      ConversionFunction<MessageType, ContainerType> conversion_function = defaultReceiverConversionFunction<MessageType, ContainerType>,
      const void *user_ptr = nullptr, std::size_t buffer_size = 4) :
      topic_info(Node::getTopicInfo<MessageType>(topic_name)),
      index_mask(calculateBufferMask(buffer_size)), message_buffer(calculateBufferSize(buffer_size)), write_count(0),
      read_count(index_mask), node(node), conversion_function(conversion_function), user_ptr(user_ptr),
      topic(node.environment.topic_map.addReceiver<MessageType>(topic_name, this)) {
      next_count = index_mask;
      flush_flag.test_and_set();
    }

    static constexpr std::size_t calculateBufferSize(std::size_t buffer_size) {
      if (buffer_size < 4) {
        throw InvalidArgumentException("The buffer size for a Receiver must be at least 4.");
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
    friend class TopicMap;

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
    std::atomic_flag flush_flag;

    Node &node;
    const ConversionFunction<MessageType, ContainerType> conversion_function;
    const void *user_ptr;
    TopicMap::Topic &topic;

  public:
    using Ptr = std::unique_ptr<Receiver<MessageType, ContainerType>>;

    class CallbackFunction {
    public:
      template <typename DataType = void>
      using Function = void (*)(Receiver<MessageType, ContainerType> &, DataType *);

      CallbackFunction() : function(nullptr) {}

      template <typename DataType = void>
      CallbackFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      inline void operator()(Receiver<MessageType, ContainerType> &receiver, void *user_ptr) const {
        function(receiver, user_ptr);
      }

      inline bool valid() {
        return function != nullptr;
      }

    private:
      Function<void> function;
    };

    ~Receiver() {
      node.environment.topic_map.removeReceiver(topic.name, this);
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

      if (!flush_flag.test_and_set()) {
        throw TopicFlushException("Topic was flushed.");
      }

      const std::size_t index = read_count.load() & index_mask;

      {
        std::lock_guard guard(message_buffer[index].mutex);
        conversion_function(message_buffer[index].message, result, user_ptr);
      }

      next_count = index | (read_count.load() & ~index_mask);

      return result;
    };

    void setCallback(CallbackFunction function, void *ptr = nullptr) {
      callback = function;
      callback_ptr = ptr;
    }

    inline bool newDataAvailable() {
      return (read_count.load() != next_count);
    }

    inline std::size_t getBufferSize() const {
      return index_mask + 1;
    }

    inline const std::string &getTopicName() const {
      return topic.name;
    }

  private:
    CallbackFunction callback;
    void *callback_ptr;
  };

  template <typename ContainerType>
  requires is_container<ContainerType>
  using ContainerReceiver = Receiver<typename ContainerType::MessageType, ContainerType>;

  template <typename RequestType, typename ResponseType>
  class Server {
  public:
    class HandlerFunction {
    public:
      template <typename DataType = void>
      using Function = ResponseType (*)(const RequestType &, DataType *);

      template <typename DataType = void>
      HandlerFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      inline ResponseType operator()(const RequestType &request, void *user_ptr) const {
        return function(request, user_ptr);
      }

    private:
      Function<void> function;
    };

  private:
    Server(const std::string &service_name, Node &node, HandlerFunction handler_function, void *user_ptr = nullptr) :
      node(node), handler_function(handler_function), user_ptr(user_ptr),
      service(node.environment.service_map.addServer<RequestType, ResponseType>(service_name, this)) {}

    friend class Node;
    friend class Client;

    Node &node;
    const HandlerFunction handler_function;
    void *const user_ptr;
    ServiceMap::Service &service;

  public:
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    ~Server() {
      node.environment.service_map.removeServer(service.name, this);
    }

    inline const std::string &getServiceName() const {
      return service.name;
    }
  };

  template <typename RequestType, typename ResponseType>
  class Client {
  private:
    Client(const std::string &service_name, Node &node) :
      node(node), service(node.environment.service_map.getService<RequestType, ResponseType>(service_name)) {}

    friend class Node;

    Node &node;
    ServiceMap::Service &service;

  public:
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;

    ~Client() = default;

    ResponseType call(const RequestType &request) {
      Server<RequestType, ResponseType> *server = reinterpret_cast<Server<RequestType, ResponseType> *>(service.getServer());

      if (server == nullptr) {
        throw ServiceUnavailableException("Service is no longer available.");
      }

      return server->handler_function(request, server->user_ptr);
    }

    inline const std::string &getServiceName() const {
      return service.name;
    }
  };

  const std::string &getName() {
    return environment.name;
  }

protected:
  Node(const Environment &environment) : environment(environment) {}

  inline Logger getLogger() const {
    return Logger(environment.name);
  }

  template <typename MessageType, typename ContainerType = MessageType, typename... Args>
  typename Sender<MessageType, ContainerType>::Ptr addSender(const std::string &topic_name,
    Args &&...args) requires is_message<MessageType> {
    return std::unique_ptr<Sender<MessageType, ContainerType>>(
      new Sender<MessageType, ContainerType>(topic_name, *this, std::forward<Args>(args)...));
  }

  template <typename ContainerType, typename... Args>
  typename ContainerSender<ContainerType>::Ptr addSender(const std::string &topic_name,
    Args &&...args) requires is_container<ContainerType> {
    return std::unique_ptr<ContainerSender<ContainerType>>(
      new ContainerSender<ContainerType>(topic_name, *this, ContainerType::toMessage, std::forward<Args>(args)...));
  }

  template <typename MessageType, typename ContainerType = MessageType, typename... Args>
  typename Receiver<MessageType, ContainerType>::Ptr addReceiver(const std::string &topic_name,
    Args &&...args) requires is_message<MessageType> {
    return std::unique_ptr<Receiver<MessageType, ContainerType>>(
      new Receiver<MessageType, ContainerType>(topic_name, *this, std::forward<Args>(args)...));
  }

  template <typename ContainerType, typename... Args>
  typename ContainerReceiver<ContainerType>::Ptr addReceiver(const std::string &topic_name,
    Args &&...args) requires is_container<ContainerType> {
    return std::unique_ptr<ContainerReceiver<ContainerType>>(
      new ContainerReceiver<ContainerType>(topic_name, *this, ContainerType::fromMessage, std::forward<Args>(args)...));
  }

  template <typename RequestType, typename ResponseType, typename... Args>
  typename Server<RequestType, ResponseType>::Ptr addServer(const std::string &service_name, Args &&...args) {
    return std::unique_ptr<Server<RequestType, ResponseType>>(
      new Server<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  template <typename RequestType, typename ResponseType, typename... Args>
  typename Client<RequestType, ResponseType>::Ptr addClient(const std::string &service_name, Args &&...args) {
    return std::unique_ptr<Client<RequestType, ResponseType>>(
      new Client<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  friend class Manager;

private:
  const Environment environment;
};

}  // namespace labrat::robot
