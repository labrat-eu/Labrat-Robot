/**
 * @file node.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/plugin.hpp>
#include <labrat/lbot/service.hpp>
#include <labrat/lbot/topic.hpp>
#include <labrat/lbot/utils/fifo.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

inline namespace labrat {
namespace lbot {

class Manager;

/**
 * @brief Base class for all nodes. A node should perform a specific task within the application.
 *
 */
class Node {
public:
  /**
   * @brief Information on the environment in which the node is created.
   * This data will be copied by a node upon construction.
   *
   */
  struct Environment {
    const std::string name;

    TopicMap &topic_map;
    ServiceMap &service_map;
    Plugin::List &plugin_list;
  };

  /**
   * @brief Destroy the Node object.
   *
   */
  ~Node() = default;

  template <typename MessageType, typename ContainerType>
  class Sender;

  template <typename MessageType, typename ContainerType>
  requires is_message<MessageType>
  class _Sender;

  /**
   * @brief Generic sender to declare virtual functions for type specific receiver instances to define.
   * This allows access to sender objetcs without knowledge of the underlying message types.
   *
   * @tparam ContainerType Container type of the sender object.
   */
  template <typename ContainerType>
  class GenericSender {
  public:
    using Ptr = std::unique_ptr<GenericSender<ContainerType>>;

    virtual ~GenericSender() = default;

    /**
     * @brief Send out a message onto the topic.
     *
     * @param container Object caintaining the data to be sent out.
     */
    virtual void put(const ContainerType &container) = 0;

    /**
     * @brief Send out a message onto the topic by moving its contents onto the topic.
     * @details This operation is efficient when sending out large buffers as it avoids copying tht data.
     * However, this comes at the cost of a constant overhead as this operation is only possible when only one receiver or plugin
     * depends on the relevant topic.
     *
     * @param container Object containing the data to be sent out.
     */
    virtual void move(ContainerType &&container) = 0;

    /**
     * @brief Flush all receivers of the relevant topic.
     * This will unblock any waiting receivers calling the next() function and will invalidate the data stored in their buffers.
     *
     */
    virtual void flush() = 0;

    /**
     * @brief Provide a message to the active plugins without actually send out any data over the topic.
     *
     * @param container Object caintaining the data to be sent out.
     */
    virtual void trace(const ContainerType &container) = 0;

    /**
     * @brief Get information about the relevant topic.
     *
     * @return const Plugin::TopicInfo& Information about the topic.
     */
    inline const Plugin::TopicInfo &getTopicInfo() const {
      return topic_info;
    }

  protected:
    GenericSender(const Plugin::TopicInfo &topic_info, TopicMap::Topic &topic, Node &node, const void *user_ptr) :
      topic_info(topic_info), topic(topic), node(node), user_ptr(user_ptr) {}

    const Plugin::TopicInfo topic_info;
    TopicMap::Topic &topic;

    Node &node;
    const void *const user_ptr;
  };

  /**
   * @brief Generic class to send out messages over a topic.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam ContainerType Type of the objects provided by the user to be accepted by the sender.
   */
  template <typename MessageType, typename ContainerType>
  requires is_message<MessageType>
  class _Sender : public GenericSender<ContainerType> {
  private:
    /**
     * @brief Construct a new Sender object.
     *
     * @param topic_name Name of the topic.
     * @param node Reference to the parent node.
     * @param conversion_function Conversion function to convert from ContainerType to MessageType.
     * @param user_ptr User pointer to be used by the conversion function.
     */
    _Sender(const std::string &topic_name, Node &node,
      ConversionFunction<ContainerType, MessageType> conversion_function = defaultSenderConversionFunction<MessageType, ContainerType>,
      const void *user_ptr = nullptr) :
      GenericSender<ContainerType>(Plugin::TopicInfo::get<MessageType>(topic_name),
        node.environment.topic_map.addSender<MessageType>(topic_name, this), node,
        user_ptr == nullptr ? dynamic_cast<GenericSender<ContainerType> *>(this) : user_ptr),
      conversion_function(conversion_function) {
      for (Plugin &plugin : GenericSender<ContainerType>::node.environment.plugin_list) {
        if (plugin.delete_flag.test() || !plugin.filter.check(GenericSender<ContainerType>::topic_info.topic_hash)) {
          continue;
        }

        utils::ConsumerGuard<u32> guard(plugin.use_count);

        plugin.topic_callback(plugin.user_ptr, GenericSender<ContainerType>::topic_info);
      }
    }

    friend class Node;

    const ConversionFunction<ContainerType, MessageType> conversion_function;
    MoveFunction<ContainerType, MessageType> move_function;

  public:
    /**
     * @brief Destroy the Sender object.
     *
     */
    ~_Sender() {
      flush();

      GenericSender<ContainerType>::node.environment.topic_map.removeSender(GenericSender<ContainerType>::topic.name, this);
    }

    /**
     * @brief Send out a message onto the topic.
     *
     * @param container Object containing the data to be sent out.
     */
    void put(const ContainerType &container) {
      for (void *pointer : GenericSender<ContainerType>::topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);
        const std::size_t index = count & receiver->index_mask;

        {
          std::lock_guard guard(receiver->message_buffer[index].mutex);
          conversion_function(container, receiver->message_buffer[index].message, GenericSender<ContainerType>::user_ptr);

          receiver->read_count.store(count);
        }

        receiver->flush_flag = false;
        receiver->read_count.notify_one();

        if (receiver->callback.valid()) {
          receiver->callback(*receiver, receiver->callback_ptr);
        }
      }

      trace(container);
    }

    /**
     * @brief Send out a message onto the topic by moving its contents onto the topic.
     * @details This operation is efficient when sending out large buffers as it avoids copying tht data.
     * However, this comes at the cost of a constant overhead as this operation is only possible when only one receiver or plugin
     * depends on the relevant topic.
     *
     * @param container Object containing the data to be sent out.
     */
    void move(ContainerType &&container) {
      if (!move_function) {
        throw ConversionException("A sender move method was called without its move function being properly set.");
      }

      std::size_t receive_count = GenericSender<ContainerType>::topic.getReceivers().size();

      const Plugin::List::iterator plugin_end = GenericSender<ContainerType>::node.environment.plugin_list.end();
      Plugin::List::iterator plugin_iterator = plugin_end;

      std::size_t i = 0;
      for (Plugin::List::iterator iter = GenericSender<ContainerType>::node.environment.plugin_list.begin(); iter != plugin_end; ++iter) {
        Plugin &plugin = *iter;

        if (!plugin.delete_flag.test() && plugin.filter.check(GenericSender<ContainerType>::topic_info.topic_hash)) {
          ++receive_count;
          plugin_iterator = iter;
        }

        ++i;
      }

      if (receive_count != 1) {
        if (receive_count > 1) {
          // If you use this function properly this branch should never get executed.
          GenericSender<ContainerType>::node.getLogger().logWarning()
            << "Sender move function is sending out to multiple receivers or plugins. This can cause performance issues.";
          put(container);
        }

        return;
      }

      if (plugin_iterator == plugin_end) {
        // Send to a receiver.
        Receiver<MessageType> *receiver =
          reinterpret_cast<Receiver<MessageType> *>(*GenericSender<ContainerType>::topic.getReceivers().begin());

        const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);
        const std::size_t index = count & receiver->index_mask;

        {
          std::lock_guard guard(receiver->message_buffer[index].mutex);
          move_function(std::forward<ContainerType>(container), receiver->message_buffer[index].message,
            GenericSender<ContainerType>::user_ptr);

          receiver->read_count.store(count);
        }

        receiver->flush_flag = false;
        receiver->read_count.notify_one();

        if (receiver->callback.valid()) {
          receiver->callback(*receiver, receiver->callback_ptr);
        }
      } else {
        // Send to a plugin.
        MessageType message;

        move_function(std::forward<ContainerType>(container), message, GenericSender<ContainerType>::user_ptr);

        flatbuffers::FlatBufferBuilder builder;
        builder.Finish(MessageType::Content::TableType::Pack(builder, &message));

        Plugin::MessageInfo message_info = {.topic_info = GenericSender<ContainerType>::topic_info,
          .timestamp = message.getTimestamp(),
          .serialized_message = builder.GetBufferSpan()};

        Plugin &plugin = *plugin_iterator;

        utils::ConsumerGuard<u32> guard(plugin.use_count);

        plugin.message_callback(plugin.user_ptr, message_info);
      }
    }

    /**
     * @brief Flush all receivers of the relevant topic.
     * This will unblock any waiting receivers calling the next() function and will invalidate the data stored in their buffers.
     *
     */
    void flush() {
      for (void *pointer : GenericSender<ContainerType>::topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);

        receiver->flush_flag = true;
        receiver->read_count.store(count);
        receiver->read_count.notify_one();
      }
    }

    /**
     * @brief Provide a message to the active plugins without actually send out any data over the topic.
     *
     * @param container Object caintaining the data to be sent out.
     */
    void trace(const ContainerType &container) {
      MessageType message;
      Plugin::MessageInfo message_info = {.topic_info = GenericSender<ContainerType>::topic_info};

      flatbuffers::FlatBufferBuilder builder;
      bool init_flag = false;

      for (Plugin &plugin : GenericSender<ContainerType>::node.environment.plugin_list) {
        utils::ConsumerGuard<u32> guard(plugin.use_count);

        if (plugin.delete_flag.test() || !plugin.filter.check(GenericSender<ContainerType>::topic_info.topic_hash)) {
          continue;
        }

        if (!init_flag) {
          conversion_function(container, message, GenericSender<ContainerType>::user_ptr);

          builder.Finish(MessageType::Content::TableType::Pack(builder, &message));

          message_info.timestamp = message.getTimestamp();
          message_info.serialized_message = builder.GetBufferSpan();

          init_flag = true;
        }

        plugin.message_callback(plugin.user_ptr, message_info);
      }
    }

    /**
     * @brief Register a move function.
     *
     * @param function Move function to be registered.
     */
    void setMove(MoveFunction<ContainerType, MessageType> function) {
      move_function = function;
    }
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename MessageType, typename ContainerType = MessageType>
  class Sender final : public _Sender<MessageType, ContainerType> {
  public:
    using Super = _Sender<MessageType, ContainerType>;
    using Ptr = std::unique_ptr<Sender<MessageType, ContainerType>>;

    template <typename... Args>
    Sender(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType, typename ContainerType>
  requires is_flatbuffer<FlatbufferType>
  class Sender<FlatbufferType, ContainerType> final : public _Sender<Message<FlatbufferType>, ContainerType> {
  public:
    using Super = _Sender<Message<FlatbufferType>, ContainerType>;
    using Ptr = std::unique_ptr<Sender<FlatbufferType, ContainerType>>;

    template <typename... Args>
    Sender(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType, typename ContainerType>
  requires is_flatbuffer<FlatbufferType> && std::same_as<FlatbufferType, ContainerType>
  class Sender<FlatbufferType, ContainerType> final : public _Sender<Message<FlatbufferType>, Message<FlatbufferType>> {
  public:
    using Super = _Sender<Message<FlatbufferType>, Message<FlatbufferType>>;
    using Ptr = std::unique_ptr<Sender<FlatbufferType>>;

    template <typename... Args>
    Sender(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename ContainerType, typename Placeholder>
  requires is_container<ContainerType> && std::same_as<ContainerType, Placeholder>
  class Sender<ContainerType, Placeholder> final : public _Sender<typename ContainerType::MessageType, ContainerType> {
  public:
    using Super = _Sender<typename ContainerType::MessageType, ContainerType>;
    using Ptr = std::unique_ptr<Sender<ContainerType, Placeholder>>;

    template <typename... Args>
    Sender(const std::string &topic_name, Node &node, Args &&...args) :
      Super(topic_name, node, ContainerType::toMessage, std::forward<Args>(args)...){};
  };

  template <typename MessageType, typename ContainerType>
  class Receiver;

  template <typename MessageType, typename ContainerType>
  requires is_message<MessageType>
  class _Receiver;

  /**
   * @brief @brief Generic receiver to declare virtual functions for type specific receiver instances to define.
   * This allows access to receiver objetcs without knowledge of the underlying message types.
   *
   * @tparam ContainerType Type of the objects provided by the receiver to be used by the user.
   */
  template <typename ContainerType>
  class GenericReceiver {
  public:
    using Ptr = std::unique_ptr<GenericReceiver<ContainerType>>;

    virtual ~GenericReceiver() = default;

    /**
     * @brief Get the lastest message sent over the topic.
     * This call is guaranteed to not block.
     *
     * @return ContainerType The latest message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    virtual ContainerType latest() = 0;

    /**
     * @brief Get the next message sent over the topic.
     * @details This call might block. However, it is guaranteed that successive calls will yield different messages.
     * Subsequent calls to latest() are unsafe.
     *
     * @return ContainerType The next message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    virtual ContainerType next() = 0;

    /**
     * @brief Check whether new data is available on the topic and a call to next() will not block.
     *
     * @return true New data is availabe, a call to next() will not block.
     * @return false No new data is availabe, a call to next() will block.
     */
    inline bool newDataAvailable() {
      return (read_count.load() != next_count);
    }

    /**
     * @brief Get the internal buffer size.
     *
     * @return std::size_t
     */
    inline std::size_t getBufferSize() const {
      return index_mask + 1;
    }

    /**
     * @brief Get information about the relevant topic.
     *
     * @return const Plugin::TopicInfo& Information about the topic.
     */
    inline const Plugin::TopicInfo &getTopicInfo() const {
      return topic_info;
    }

  protected:
    GenericReceiver(const Plugin::TopicInfo &topic_info, TopicMap::Topic &topic, Node &node, const void *user_ptr,
      std::size_t buffer_size) :
      topic_info(topic_info), topic(topic), node(node), user_ptr(user_ptr), index_mask(calculateBufferMask(buffer_size)), write_count(0),
      read_count(index_mask) {
      next_count = index_mask;
      flush_flag = true;
    }

    /**
     * @brief Calculate the internal buffer size required to satisfy the provided buffer size.
     * This will either be the provided buffer size itself or the next power of 2.
     *
     * @param buffer_size Provided buffer size of the receiver.
     * @return std::size_t Internal buffer size.
     */
    std::size_t calculateBufferSize(std::size_t buffer_size) {
      if (buffer_size < 4) {
        throw InvalidArgumentException("The buffer size for a Receiver must be at least 4.", node.getLogger());
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

    /**
     * @brief Calculate the buffer mask used internally to compute a buffer index from a receive count.
     *
     * @param buffer_size Provided buffer size of the receiver.
     * @return std::size_t Internal buffer mask.
     */
    std::size_t calculateBufferMask(std::size_t buffer_size) {
      return calculateBufferSize(buffer_size) - 1;
    }

    const Plugin::TopicInfo topic_info;
    TopicMap::Topic &topic;

    Node &node;
    const void *user_ptr;

    const std::size_t index_mask;

    std::atomic<std::size_t> write_count;
    std::atomic<std::size_t> read_count;
    std::size_t next_count;
    volatile bool flush_flag;
  };

  /**
   * @brief Generic class to receive messages from a topic.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam ContainerType Type of the objects provided by the receiver to be used by the user.
   */
  template <typename MessageType, typename ContainerType>
  requires is_message<MessageType>
  class _Receiver : public GenericReceiver<ContainerType> {
  private:
    /**
     * @brief Construct a new Receiver object.
     *
     * @param topic_name Name of the topic.
     * @param node Reference to the parent node.
     * @param conversion_function Conversion function convert from MessageType to ContainerType.
     * @param user_ptr User pointer to be used by the conversion function.
     * @param buffer_size Size of the internal receiver buffer. It must be at least 4 and should ideally be a power of 2.
     */
    _Receiver(const std::string &topic_name, Node &node,
      ConversionFunction<MessageType, ContainerType> conversion_function = defaultReceiverConversionFunction<MessageType, ContainerType>,
      const void *user_ptr = nullptr, std::size_t buffer_size = 4) :
      GenericReceiver<ContainerType>(Plugin::TopicInfo::get<MessageType>(topic_name),
        node.environment.topic_map.addReceiver<MessageType>(topic_name, this), node,
        user_ptr == nullptr ? dynamic_cast<GenericReceiver<ContainerType> *>(this) : user_ptr, buffer_size),
      conversion_function(conversion_function), message_buffer(GenericReceiver<ContainerType>::calculateBufferSize(buffer_size)) {}

    friend class Node;
    friend class Sender<MessageType>;
    friend class TopicMap;

    /**
     * @brief Internal message buffer.
     *
     */
    class MessageBuffer {
    public:
      struct MessageData {
        MessageType message;
        std::mutex mutex;
      };

      /**
       * @brief Construct a new Message Buffer object given its size.
       *
       * @param size Size of the message buffer.
       */
      MessageBuffer(std::size_t size) : size(size) {
        buffer = allocator.allocate(size);

        for (std::size_t i = 0; i < size; ++i) {
          std::construct_at<MessageData>(buffer + i);
        }
      }

      /**
       * @brief Destroy the Message Buffer object.
       *
       */
      ~MessageBuffer() {
        for (std::size_t i = 0; i < size; ++i) {
          std::destroy_at<MessageData>(buffer + i);
        }

        allocator.deallocate(buffer, size);
      }

      /**
       * @brief Access elements in the buffer.
       *
       * @param index Index of the requested element.
       * @return MessageData& Reference to the requested element.
       */
      MessageData &operator[](std::size_t index) volatile {
        return buffer[index];
      }

    private:
      MessageData *buffer;
      std::allocator<MessageData> allocator;

      const std::size_t size;
    };

    const ConversionFunction<MessageType, ContainerType> conversion_function;
    MoveFunction<MessageType, ContainerType> move_function;

    volatile MessageBuffer message_buffer;

  public:
    /**
     * @brief Callback function to be called by the corresponding sender on each put operation.
     *
     */
    class CallbackFunction {
    public:
      template <typename DataType = void>
      using Function = void (*)(GenericReceiver<ContainerType> &, DataType *);

      /**
       * @brief Default constructor invalidating the object.
       *
       */
      CallbackFunction() : function(nullptr) {}

      /**
       * @brief Construct a new Callback Function object by providing a function and user pointer.
       *
       * @tparam DataType Type of the data pointed to by the user pointer.
       * @param function Function to be used as a callback function.
       */
      template <typename DataType = void>
      CallbackFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      /**
       * @brief Call the stored callback function.
       *
       * @param receiver Reference to the relevant generic receiver instance.
       * @param user_ptr User pointer to access generic external data.
       */
      inline void operator()(GenericReceiver<ContainerType> &receiver, void *user_ptr) const {
        function(receiver, user_ptr);
      }

      /**
       * @brief Validate that a function is stored in this instance.
       *
       * @return true A function is stored.
       * @return false A function is not stored.
       */
      inline bool valid() {
        return function != nullptr;
      }

    private:
      Function<void> function;
    };

    /**
     * @brief Destroy the Receiver object.
     *
     */
    ~_Receiver() {
      GenericReceiver<ContainerType>::node.environment.topic_map.removeReceiver(GenericReceiver<ContainerType>::topic.name, this);
    }

    /**
     * @brief Get the lastest message sent over the topic.
     * This call is guaranteed to not block.
     *
     * @return ContainerType The latest message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    ContainerType latest() override {
      ContainerType result;

      const std::size_t index = GenericReceiver<ContainerType>::read_count.load() & GenericReceiver<ContainerType>::index_mask;

      if (GenericReceiver<ContainerType>::flush_flag) {
        throw TopicNoDataAvailableException("Topic was flushed.", GenericReceiver<ContainerType>::node.getLogger());
      }

      {
        std::lock_guard guard(message_buffer[index].mutex);
        conversion_function(message_buffer[index].message, result, GenericReceiver<ContainerType>::user_ptr);
      }

      return result;
    };

    /**
     * @brief Get the next message sent over the topic.
     * @details This call might block. However, it is guaranteed that successive calls will yield different messages.
     * Subsequent calls to latest() are unsafe.
     *
     * @return ContainerType The next message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    ContainerType next() override {
      if (GenericReceiver<ContainerType>::flush_flag) {
        throw TopicNoDataAvailableException("Topic was flushed.", GenericReceiver<ContainerType>::node.getLogger());
      }

      ContainerType result;

      GenericReceiver<ContainerType>::read_count.wait(GenericReceiver<ContainerType>::next_count);

      if (GenericReceiver<ContainerType>::flush_flag) {
        throw TopicNoDataAvailableException("Topic was flushed during wait operation.", GenericReceiver<ContainerType>::node.getLogger());
      }

      const std::size_t index = GenericReceiver<ContainerType>::read_count.load() & GenericReceiver<ContainerType>::index_mask;

      if (move_function) {
        std::lock_guard guard(message_buffer[index].mutex);
        move_function(std::move<MessageType &>(message_buffer[index].message), result, GenericReceiver<ContainerType>::user_ptr);
      } else {
        std::lock_guard guard(message_buffer[index].mutex);
        conversion_function(message_buffer[index].message, result, GenericReceiver<ContainerType>::user_ptr);
      }

      GenericReceiver<ContainerType>::next_count =
        index | (GenericReceiver<ContainerType>::read_count.load() & ~GenericReceiver<ContainerType>::index_mask);

      return result;
    };

    /**
     * @brief Register a callback function.
     *
     * @param function Callback function to be registered.
     * @param ptr User pointer to access generic external data.
     */
    void setCallback(CallbackFunction function, void *ptr = nullptr) {
      callback = function;
      callback_ptr = ptr;
    }

    /**
     * @brief Register a move function.
     *
     * @param function Move function to be registered.
     */
    void setMove(MoveFunction<MessageType, ContainerType> function) {
      move_function = function;
    }

  private:
    CallbackFunction callback;
    void *callback_ptr;
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename MessageType, typename ContainerType = MessageType>
  class Receiver final : public _Receiver<MessageType, ContainerType> {
  public:
    using Super = _Receiver<MessageType, ContainerType>;
    using Ptr = std::unique_ptr<Receiver<MessageType, ContainerType>>;

    template <typename... Args>
    Receiver(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType, typename ContainerType>
  requires is_flatbuffer<FlatbufferType>
  class Receiver<FlatbufferType, ContainerType> final : public _Receiver<Message<FlatbufferType>, ContainerType> {
  public:
    using Super = _Receiver<Message<FlatbufferType>, ContainerType>;
    using Ptr = std::unique_ptr<Receiver<FlatbufferType, ContainerType>>;

    template <typename... Args>
    Receiver(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType, typename ContainerType>
  requires is_flatbuffer<FlatbufferType> && std::same_as<FlatbufferType, ContainerType>
  class Receiver<FlatbufferType, ContainerType> final : public _Receiver<Message<FlatbufferType>, Message<FlatbufferType>> {
  public:
    using Super = _Receiver<Message<FlatbufferType>, Message<FlatbufferType>>;
    using Ptr = std::unique_ptr<Receiver<FlatbufferType>>;

    template <typename... Args>
    Receiver(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename ContainerType, typename Placeholder>
  requires is_container<ContainerType> && std::same_as<ContainerType, Placeholder>
  class Receiver<ContainerType, Placeholder> final : public _Receiver<typename ContainerType::MessageType, ContainerType> {
  public:
    using Super = _Receiver<typename ContainerType::MessageType, ContainerType>;
    using Ptr = std::unique_ptr<Receiver<ContainerType, Placeholder>>;

    template <typename... Args>
    Receiver(const std::string &topic_name, Node &node, Args &&...args) :
      Super(topic_name, node, ContainerType::fromMessage, std::forward<Args>(args)...){};
  };

  /**
   * @brief Generic class to handle requests made to a service.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   */
  template <typename RequestType, typename ResponseType>
  class Server {
  public:
    /**
     * @brief Handler function to handle requests made to a service.
     *
     */
    class HandlerFunction {
    public:
      template <typename DataType = void>
      using Function = ResponseType (*)(const RequestType &, DataType *);

      /**
       * @brief Construct a new handler function.
       *
       * @tparam DataType Type of the data pointed to by the user pointer.
       * @param function Function to be used as a handler function.
       */
      template <typename DataType = void>
      HandlerFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      /**
       * @brief Call the stored conversion function.
       *
       * @param request Request sent by the client.
       * @param user_ptr User pointer to access generic external data.
       * @return ResponseType Response to be sent to the client.
       */
      inline ResponseType operator()(const RequestType &request, void *user_ptr) const {
        return function(request, user_ptr);
      }

    private:
      Function<void> function;
    };

  private:
    /**
     * @brief Construct a new Server object
     *
     * @param service_name Name of the service.
     * @param node Reference to the parent node.
     * @param handler_function Handler function to handle requests made to a service.
     * @param user_ptr User pointer to be used by the handler function.
     */
    Server(const std::string &service_name, Node &node, HandlerFunction handler_function, void *user_ptr = nullptr) :
      node(node), handler_function(handler_function), user_ptr(user_ptr == nullptr ? this : user_ptr),
      service(node.environment.service_map.addServer<RequestType, ResponseType>(service_name, this)) {}

    friend class Node;
    friend class Client;

    Node &node;
    const HandlerFunction handler_function;
    void *const user_ptr;
    ServiceMap::Service &service;

  public:
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    /**
     * @brief Destroy the Server object.
     *
     */
    ~Server() {
      service.removeServer(this);
    }

    /**
     * @brief Get the name of the relevant service.
     *
     * @return const std::string& Name of the service.
     */
    inline const std::string &getServiceName() const {
      return service.name;
    }
  };

  /**
   * @brief Generic class to perform requests to a service.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   */
  template <typename RequestType, typename ResponseType>
  class Client {
  private:
    /**
     * @brief Construct a new Client object.
     *
     * @param service_name Name of the service.
     * @param node Reference to the parent node.
     */
    Client(const std::string &service_name, Node &node) :
      node(node), service(node.environment.service_map.getService<RequestType, ResponseType>(service_name)) {}

    friend class Node;

    Node &node;
    ServiceMap::Service &service;

  public:
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;
    using Future = std::shared_future<ResponseType>;

    /**
     * @brief Destroy the Client object.
     *
     */
    ~Client() = default;

    /**
     * @brief Make a request to a service asynchronously.
     * A call to this function will not block.
     *
     * @param request Object containing the data to be processed by the corresponding server.
     * @return Future Future to be completed by the server.
     * @throw ServiceUnavailableException When no server is handling requests to the relevant service.
     */
    Future callAsync(const RequestType &request) {
      std::promise<ResponseType> promise;
      Future future = promise.get_future();

      std::thread(
        [this](std::promise<ResponseType> promise, const RequestType request) {
        try {
          ServiceMap::Service::ServerReference reference = service.getServer();
          Server<RequestType, ResponseType> *server = reference;

          if (server == nullptr) {
            throw ServiceUnavailableException("Service is not available.", node.getLogger());
          }

          promise.set_value(server->handler_function(request, server->user_ptr));
        } catch (...) {
          promise.set_exception(std::current_exception());
        }
      }, std::move(promise), request)
        .detach();

      return future;
    }

    /**
     * @brief Make a request to a service synchronously.
     * A call to this function will block.
     *
     * @param request Object containing the data to be processed by the corresponding server.
     * @return ResponseType Response from the server.
     * @throw ServiceUnavailableException When no server is handling requests to the relevant service.
     */
    ResponseType callSync(const RequestType &request) {
      Future future = callAsync(request);

      return future.get();
    }

    /**
     * @brief Make a request to a service synchronously.
     * A call to this function will block but is guaranteed to not exceed the specified timeout.
     *
     * @param request Object containing the data to be processed by the corresponding server.
     * @param timeout_duration Duration of the timeout after which an exception will be thrown.RequestType
     * @return ResponseType Response from the server.
     * @throw ServiceUnavailableException When no server is handling requests to the relevant service.
     * @throw ServiceTimeoutException When the timeout is exceeded.
     */
    template <typename R, typename P>
    ResponseType callSync(const RequestType &request, const std::chrono::duration<R, P> &timeout_duration) {
      Future future = callAsync(request);

      if (future.wait_for(timeout_duration) != std::future_status::ready) {
        throw ServiceTimeoutException("Service took too long to respond.", node.getLogger());
      }

      return future.get();
    }

    /**
     * @brief Get the name of the relevant service.
     *
     * @return const std::string& Name of the service.
     */
    inline const std::string &getServiceName() const {
      return service.name;
    }
  };

  /**
   * @brief Get the name of the node.
   *
   * @return const std::string& Name of the node.
   */
  const std::string &getName() {
    return environment.name;
  }

protected:
  /**
   * @brief Construct a new Node object.
   *
   * @param environment Node environment data for the node to copy internally.
   */
  Node(const Environment &environment) : environment(environment) {}

  /**
   * @brief Get a logger with the name of the node.
   *
   * @return Logger A logger with the name of the node.
   */
  inline Logger getLogger() const {
    return Logger(environment.name);
  }

  /**
   * @brief Construct and add a sender to the node.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam ContainerType Type of the objects provided by the user to be accepted by the sender.
   * @tparam Args Types of the arguments to be forwarded to the sender specific constructor.
   * @param args Arguments to be forwarded to the sender specific constructor.
   * @return Sender<MessageType, ContainerType>::Ptr Pointer to the sender.
   */
  template <typename MessageType, typename ContainerType = MessageType, typename... Args>
  typename Sender<MessageType, ContainerType>::Ptr addSender(const std::string &topic_name, Args &&...args)
  requires is_message<MessageType> || is_flatbuffer<MessageType> || is_container<MessageType>
  {
    using Ptr = Sender<MessageType, ContainerType>::Ptr;
    return Ptr(new Sender<MessageType, ContainerType>(topic_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a receiver to the node.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam ContainerType Type of the objects provided by the receiver to be used by the user.
   * @tparam Args Types of the arguments to be forwarded to the receiver specific constructor.
   * @param args Arguments to be forwarded to the receiver specific constructor.
   * @return Receiver<MessageType, ContainerType>::Ptr Pointer to the receiver.
   */
  template <typename MessageType, typename ContainerType = MessageType, typename... Args>
  typename Receiver<MessageType, ContainerType>::Ptr addReceiver(const std::string &topic_name, Args &&...args)
  requires is_message<MessageType> || is_flatbuffer<MessageType> || is_container<MessageType>
  {
    using Ptr = Receiver<MessageType, ContainerType>::Ptr;
    return Ptr(new Receiver<MessageType, ContainerType>(topic_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a server to the node.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   * @param args Arguments to be forwarded to the server specific constructor.
   * @return Receiver<MessageType, ContainerType>::Ptr Pointer to the server.
   */
  template <typename RequestType, typename ResponseType, typename... Args>
  typename Server<RequestType, ResponseType>::Ptr addServer(const std::string &service_name, Args &&...args) {
    using Ptr = Server<RequestType, ResponseType>::Ptr;
    return Ptr(new Server<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a client to the node.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   * @param args Arguments to be forwarded to the client specific constructor.
   * @return Receiver<MessageType, ContainerType>::Ptr Pointer to the client.
   */
  template <typename RequestType, typename ResponseType, typename... Args>
  typename Client<RequestType, ResponseType>::Ptr addClient(const std::string &service_name, Args &&...args) {
    using Ptr = Client<RequestType, ResponseType>::Ptr;
    return Ptr(new Client<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  friend class Manager;

private:
  const Environment environment;
};

}  // namespace lbot
}  // namespace labrat
