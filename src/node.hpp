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
#include <labrat/lbot/manager.hpp>
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
   * @brief Destroy the Node object.
   *
   */
  virtual ~Node() = default;

  template <typename MessageType>
  class Sender;

  template <typename MessageType>
  requires is_message<MessageType> class SenderBase;

  /**
   * @brief Generic sender to declare virtual functions for type specific receiver instances to define.
   * This allows access to sender objetcs without knowledge of the underlying message types.
   *
   * @tparam ConvertedType Container type of the sender object.
   */
  template <typename ConvertedType>
  class GenericSender {
  public:
    using Ptr = std::unique_ptr<GenericSender<ConvertedType>>;

    virtual ~GenericSender() = default;

    /**
     * @brief Send out a message onto the topic.
     *
     * @param container Object caintaining the data to be sent out.
     */
    virtual void put(const ConvertedType &container) = 0;

    /**
     * @brief Send out a message onto the topic by moving its contents onto the topic.
     * @details This operation is efficient when sending out large buffers as it avoids copying tht data.
     * However, this comes at the cost of a constant overhead as this operation is only possible when only one receiver or plugin
     * depends on the relevant topic.
     *
     * @param container Object containing the data to be sent out.
     */
    virtual void put(ConvertedType &&container) = 0;

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
    virtual void trace(const ConvertedType &container) = 0;

    /**
     * @brief Get information about the relevant topic.
     *
     * @return const TopicInfo& Information about the topic.
     */
    [[nodiscard]] inline const TopicInfo &getTopicInfo() const {
      return topic_info;
    }

  protected:
    GenericSender(TopicInfo topic_info, TopicMap::Topic &topic, Node &node) : topic_info(std::move(topic_info)), topic(topic), node(node) {}

    const TopicInfo topic_info;
    TopicMap::Topic &topic;

    Node &node;
  };

  /**
   * @brief Generic class to send out messages over a topic.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   */
  template <typename MessageType>
  requires is_message<MessageType> class SenderBase : public GenericSender<typename MessageType::Converted> {
  public:
    using Storage = typename MessageType::Storage;
    using Converted = typename MessageType::Converted;

    using Reference = GenericSender<Converted>;

    template <auto *Function>
    using Convert = ConversionFunction<Reference, Function>;
    template <auto *Function>
    using Move = MoveFunction<Reference, Function>;

    SenderBase(SenderBase &) = delete;
    SenderBase(SenderBase &&) = delete;

  private:
    /**
     * @brief Construct a new Sender object.
     *
     * @param topic_name Name of the topic.
     * @param node Reference to the parent node.
     * @param user_ptr User pointer to be used by the conversion function.
     */
    SenderBase(const std::string &topic_name, Node &node, const void *user_ptr = nullptr) requires can_convert_from<MessageType, Reference>
      :
      GenericSender<Converted>(TopicInfo::get<MessageType>(topic_name), node.environment.topic_map.addSender<MessageType>(topic_name, this),
        node),
      user_ptr(user_ptr) {
      for (PluginRegistration &plugin : GenericSender<Converted>::node.environment.plugin_list) {
        if (plugin.delete_flag.test() || !plugin.filter.check(GenericSender<Converted>::topic_info.topic_hash)) {
          continue;
        }

        utils::ConsumerGuard<u32> guard(plugin.use_count);

        if (plugin.topic_callback != nullptr) {
          plugin.topic_callback(plugin.user_ptr, GenericSender<Converted>::topic_info);
        }
      }
    }

    friend class Node;

    const void *const user_ptr;

  public:
    /**
     * @brief Destroy the Sender object.
     *
     */
    ~SenderBase() override {
      SenderBase::flush();

      GenericSender<Converted>::node.environment.topic_map.removeSender(GenericSender<Converted>::topic.name, this);
    }

    /**
     * @brief Send out a message onto the topic.
     *
     * @param container Object containing the data to be sent out.
     */
    void put(const Converted &container) override {
      for (void *pointer : GenericSender<Converted>::topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        const std::size_t local_count = receiver->count.load(std::memory_order_relaxed) + 1;
        const std::size_t index = local_count & receiver->index_mask;

        {
          std::lock_guard guard(receiver->message_buffer[index].mutex);
          Convert<MessageType::convertFrom>::call(container, receiver->message_buffer[index].message, user_ptr,
            *dynamic_cast<GenericSender<Converted> *>(this));
          receiver->message_buffer[index].update_flag = true;
          receiver->count.store(local_count, std::memory_order_release);
        }

        receiver->flush_flag = false;
        receiver->count.notify_one();

        if (receiver->callback.valid()) {
          receiver->callback(*receiver, receiver->user_ptr);
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
    void put(Converted &&container) override {
      if constexpr (!can_move_from<MessageType, Reference>) {
        throw ConversionException("A sender put() move method was called without its move function being properly set.");
      } else {
        std::size_t receive_count = GenericSender<Converted>::topic.getReceivers().size();

        const PluginRegistration::List::iterator plugin_end = GenericSender<Converted>::node.environment.plugin_list.end();
        PluginRegistration::List::iterator plugin_iterator = plugin_end;

        std::size_t i = 0;
        for (PluginRegistration::List::iterator iter = GenericSender<Converted>::node.environment.plugin_list.begin(); iter != plugin_end;
             ++iter) {
          PluginRegistration &plugin = *iter;

          if (!plugin.delete_flag.test() && plugin.filter.check(GenericSender<Converted>::topic_info.topic_hash)) {
            ++receive_count;
            plugin_iterator = iter;
          }

          ++i;
        }

        if (receive_count != 1) {
          if (receive_count > 1) {
            // If you use this function properly this branch should never get executed.
            GenericSender<Converted>::node.getLogger().logWarning()
              << "Sender move function is sending out to multiple receivers or plugins. This can cause performance issues.";
            put(container);
          }

          return;
        }

        if (plugin_iterator == plugin_end) {
          // Send to a receiver.
          Receiver<MessageType> *receiver =
            reinterpret_cast<Receiver<MessageType> *>(*GenericSender<Converted>::topic.getReceivers().begin());

          const std::size_t local_count = receiver->count.load(std::memory_order_relaxed) + 1;
          const std::size_t index = local_count & receiver->index_mask;

          {
            std::lock_guard guard(receiver->message_buffer[index].mutex);
            Move<MessageType::moveFrom>::call(std::forward<Converted>(container), receiver->message_buffer[index].message, user_ptr,
              *dynamic_cast<GenericSender<Converted> *>(this));
            receiver->message_buffer[index].update_flag = true;
            receiver->count.store(local_count, std::memory_order_release);
          }

          receiver->flush_flag = false;
          receiver->count.notify_one();

          if (receiver->callback.valid()) {
            receiver->callback(*receiver, receiver->user_ptr);
          }
        } else {
          // Send to a plugin.
          MessageType message;

          Move<MessageType::moveFrom>::call(std::forward<Converted>(container), message, user_ptr,
            *dynamic_cast<GenericSender<Converted> *>(this));

          flatbuffers::FlatBufferBuilder builder;
          builder.Finish(MessageType::Content::TableType::Pack(builder, &message));

          MessageInfo message_info = {.topic_info = GenericSender<Converted>::topic_info,
            .timestamp = message.getTimestamp(),
            .serialized_message = builder.GetBufferSpan()};

          PluginRegistration &plugin = *plugin_iterator;

          utils::ConsumerGuard<u32> guard(plugin.use_count);

          if (plugin.message_callback != nullptr) {
            plugin.message_callback(plugin.user_ptr, message_info);
          }
        }
      }
    }

    /**
     * @brief Flush all receivers of the relevant topic.
     * This will unblock any waiting receivers calling the next() function and will invalidate the data stored in their buffers.
     *
     */
    void flush() override {
      for (void *pointer : GenericSender<Converted>::topic.getReceivers()) {
        Receiver<MessageType> *receiver = reinterpret_cast<Receiver<MessageType> *>(pointer);

        receiver->flush_flag = true;
        receiver->count.fetch_add(1, std::memory_order_release);
        receiver->count.notify_one();
      }
    }

    /**
     * @brief Provide a message to the active plugins without actually send out any data over the topic.
     *
     * @param container Object caintaining the data to be sent out.
     */
    void trace(const Converted &container) override {
      MessageType message;
      MessageInfo message_info = {.topic_info = GenericSender<Converted>::topic_info};

      flatbuffers::FlatBufferBuilder builder;
      bool init_flag = false;

      for (PluginRegistration &plugin : GenericSender<Converted>::node.environment.plugin_list) {
        utils::ConsumerGuard<u32> guard(plugin.use_count);

        if (plugin.delete_flag.test() || !plugin.filter.check(GenericSender<Converted>::topic_info.topic_hash)) {
          continue;
        }

        if (!init_flag) {
          Convert<MessageType::convertFrom>::call(container, message, user_ptr, *dynamic_cast<GenericSender<Converted> *>(this));

          builder.Finish(MessageType::Content::TableType::Pack(builder, &message));

          message_info.timestamp = message.getTimestamp();
          message_info.serialized_message = builder.GetBufferSpan();

          init_flag = true;
        }

        if (plugin.message_callback != nullptr) {
          plugin.message_callback(plugin.user_ptr, message_info);
        }
      }
    }
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename MessageType>
  class Sender final : public SenderBase<MessageType> {
  public:
    using Super = SenderBase<MessageType>;
    using Ptr = std::unique_ptr<Sender<MessageType>>;

    template <typename... Args>
    explicit Sender(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType>
  requires is_flatbuffer<FlatbufferType> class Sender<FlatbufferType> final : public SenderBase<Message<FlatbufferType>> {
  public:
    using Super = SenderBase<Message<FlatbufferType>>;
    using Ptr = std::unique_ptr<Sender<FlatbufferType>>;

    template <typename... Args>
    explicit Sender(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename MessageType>
  class Receiver;

  template <typename MessageType>
  requires is_message<MessageType> class ReceiverBase;

  /**
   * @brief @brief Generic receiver to declare virtual functions for type specific receiver instances to define.
   * This allows access to receiver objetcs without knowledge of the underlying message types.
   *
   * @tparam ConvertedType Type of the objects provided by the receiver to be used by the user.
   */
  template <typename ConvertedType>
  class GenericReceiver {
  public:
    using Ptr = std::unique_ptr<GenericReceiver<ConvertedType>>;

    virtual ~GenericReceiver() = default;

    /**
     * @brief Get the lastest message sent over the topic.
     * This call is guaranteed to not block.
     *
     * @return ConvertedType The latest message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    virtual ConvertedType latest() = 0;

    /**
     * @brief Get the next message sent over the topic.
     * @details This call might block. However, it is guaranteed that successive calls will yield different messages.
     * Subsequent calls to latest() are unsafe.
     *
     * @return ConvertedType The next message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    virtual ConvertedType next() = 0;

    /**
     * @brief Get the internal buffer size.
     *
     * @return std::size_t
     */
    [[nodiscard]] inline std::size_t getBufferSize() const {
      return index_mask + 1;
    }

    /**
     * @brief Get information about the relevant topic.
     *
     * @return const TopicInfo& Information about the topic.
     */
    [[nodiscard]] inline const TopicInfo &getTopicInfo() const {
      return topic_info;
    }

  protected:
    GenericReceiver(TopicInfo topic_info, TopicMap::Topic &topic, Node &node, std::size_t buffer_size) :
      topic_info(std::move(topic_info)), topic(topic), node(node), index_mask(calculateBufferMask(buffer_size)), count(index_mask) {
      next_count = index_mask;
      flush_flag = true;
    }

    friend class TopicMap;

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

    const TopicInfo topic_info;
    TopicMap::Topic &topic;

    Node &node;

    const std::size_t index_mask;

    std::atomic<std::size_t> count;
    std::size_t next_count;
    bool flush_flag;
  };

  /**
   * @brief Generic class to receive messages from a topic.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   */
  template <typename MessageType>
  requires is_message<MessageType> class ReceiverBase : public GenericReceiver<typename MessageType::Converted> {
  public:
    using Storage = typename MessageType::Storage;
    using Converted = typename MessageType::Converted;

    using Reference = GenericReceiver<Converted>;

    template <auto *Function>
    using Convert = ConversionFunction<Reference, Function>;
    template <auto *Function>
    using Move = MoveFunction<Reference, Function>;

  private:
    /**
     * @brief Construct a new Receiver object.
     *
     * @param topic_name Name of the topic.
     * @param node Reference to the parent node.
     * @param user_ptr User pointer to be used by the conversion function.
     * @param buffer_size Size of the internal receiver buffer. It must be at least 4 and should ideally be a power of 2.
     */
    ReceiverBase(const std::string &topic_name, Node &node, const void *user_ptr = nullptr, std::size_t buffer_size = 4)
      requires can_convert_to<MessageType, Reference> :
      GenericReceiver<Converted>(TopicInfo::get<MessageType>(topic_name),
        node.environment.topic_map.addReceiver<MessageType>(topic_name, this), node, buffer_size),
      user_ptr(user_ptr), message_buffer(GenericReceiver<Converted>::calculateBufferSize(buffer_size)) {}

    friend class Node;
    friend class Sender<MessageType>;

    const void *const user_ptr;

    /**
     * @brief Internal message buffer.
     *
     */
    class MessageBuffer {
    public:
      struct MessageData {
        Storage message;
        std::mutex mutex;
        bool update_flag = false;
      };

      /**
       * @brief Construct a new Message Buffer object given its size.
       *
       * @param size Size of the message buffer.
       */
      explicit MessageBuffer(std::size_t size) : size(size) {
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
      MessageData &operator[](std::size_t index) {
        return buffer[index];
      }

    private:
      MessageData *buffer;
      std::allocator<MessageData> allocator;

      const std::size_t size;
    };

    MessageBuffer message_buffer;

  public:
    ReceiverBase(ReceiverBase &) = delete;
    ReceiverBase(ReceiverBase &&) = delete;

    /**
     * @brief Callback function to be called by the corresponding sender on each put operation.
     *
     */
    class CallbackFunction {
    public:
      template <typename DataType>
      using Function = void (*)(GenericReceiver<Converted> &, DataType *);

      /**
       * @brief Default constructor invalidating the object.
       *
       */
      CallbackFunction() : function(nullptr) {}

      /**
       * @brief Construct a new Callback Function object by providing a function and user pointer.
       *
       * @param function Function to be used as a callback function.
       */
      template <typename DataType>
      CallbackFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      /**
       * @brief Call the stored callback function.
       *
       * @param receiver Reference to the relevant generic receiver instance.
       * @param user_ptr User pointer to access generic external data.
       */
      inline void operator()(GenericReceiver<Converted> &receiver, const void *user_ptr) const {
        function(receiver, const_cast<void *>(user_ptr));
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
    ~ReceiverBase() override {
      GenericReceiver<Converted>::node.environment.topic_map.removeReceiver(GenericReceiver<Converted>::topic.name, this);
    }

    /**
     * @brief Get the lastest message sent over the topic.
     * This call is guaranteed to not block.
     *
     * @return Converted The latest message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    Converted latest() override {
      if (GenericReceiver<Converted>::flush_flag) {
        throw TopicNoDataAvailableException("Topic was flushed.", GenericReceiver<Converted>::node.getLogger());
      }

      Converted result;

      const std::size_t local_count = GenericReceiver<Converted>::count.load(std::memory_order_acquire);
      const std::size_t index = local_count & GenericReceiver<Converted>::index_mask;

      if (!message_buffer[index].update_flag && mode == Mode::next) {
        throw TopicNoDataAvailableException("No new data after next() call.", GenericReceiver<Converted>::node.getLogger());
      }

      {
        std::lock_guard guard(message_buffer[index].mutex);
        Convert<MessageType::convertTo>::call(message_buffer[index].message, result, user_ptr,
          *dynamic_cast<GenericReceiver<Converted> *>(this));
        message_buffer[index].update_flag = false;
      }

      mode = Mode::latest;

      return result;
    }

    /**
     * @brief Get the next message sent over the topic.
     * @details This call might block. However, it is guaranteed that successive calls will yield different messages.
     * Subsequent calls to latest() are unsafe.
     *
     * @return Converted The next message sent over the topic.
     * @throw TopicNoDataAvailableException When the topic has no valid data available.
     */
    Converted next() override {
      if (GenericReceiver<Converted>::flush_flag) {
        throw TopicNoDataAvailableException("Topic was flushed.", GenericReceiver<Converted>::node.getLogger());
      }

      Converted result;

      std::size_t local_count = GenericReceiver<Converted>::next_count;
      std::size_t index;

      do {
        GenericReceiver<Converted>::count.wait(local_count, std::memory_order_acquire);

        if (GenericReceiver<Converted>::flush_flag) {
          throw TopicNoDataAvailableException("Topic was flushed during wait operation.", GenericReceiver<Converted>::node.getLogger());
        }

        local_count = GenericReceiver<Converted>::count.load(std::memory_order_acquire);
        index = local_count & GenericReceiver<Converted>::index_mask;
      } while (!message_buffer[index].update_flag);

      {
        std::lock_guard guard(message_buffer[index].mutex);

        if constexpr (can_move_from<MessageType, Reference>) {
          Move<MessageType::moveTo>::call(std::move(message_buffer[index].message), result, user_ptr,
            *dynamic_cast<GenericReceiver<Converted> *>(this));
        } else {
          Convert<MessageType::convertTo>::call(message_buffer[index].message, result, user_ptr,
            *dynamic_cast<GenericReceiver<Converted> *>(this));
        }
        message_buffer[index].update_flag = false;
      }

      mode = Mode::next;
      GenericReceiver<Converted>::next_count = local_count;

      return result;
    }

    /**
     * @brief Register a callback function.
     *
     * @param function Callback function to be registered.
     */
    void setCallback(CallbackFunction function) {
      callback = function;
    }

  private:
    CallbackFunction callback;

    enum class Mode : u8 {
      latest,
      next,
    } mode = Mode::latest;
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename MessageType>
  class Receiver final : public ReceiverBase<MessageType> {
  public:
    using Super = ReceiverBase<MessageType>;
    using Ptr = std::unique_ptr<Receiver<MessageType>>;

    template <typename... Args>
    explicit Receiver(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename FlatbufferType>
  requires is_flatbuffer<FlatbufferType> class Receiver<FlatbufferType> final : public ReceiverBase<Message<FlatbufferType>> {
  public:
    using Super = ReceiverBase<Message<FlatbufferType>>;
    using Ptr = std::unique_ptr<Receiver<FlatbufferType>>;

    template <typename... Args>
    explicit Receiver(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  class Server;

  template <typename RequestConverted, typename ResponseConverted>
  class GenericServer {
  protected:
    explicit GenericServer(ServiceInfo service_info) : service_info(std::move(service_info)) {}

    const ServiceInfo service_info;

  public:
    /**
     * @brief Get information about the relevant service.
     *
     * @return const ServiceInfo& Information about the service.
     */
    [[nodiscard]] inline const ServiceInfo &getServiceInfo() const {
      return service_info;
    }
  };

  /**
   * @brief Generic class to handle requests made to a service.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   */
  template <typename RequestType, typename ResponseType>
  requires is_message<RequestType> && is_message<ResponseType>
  class ServerBase : public GenericServer<typename RequestType::Converted, typename ResponseType::Converted> {
  public:
    using RequestStorage = typename RequestType::Storage;
    using ResponseStorage = typename ResponseType::Storage;
    using RequestConverted = typename RequestType::Converted;
    using ResponseConverted = typename ResponseType::Converted;

    using Reference = GenericServer<RequestConverted, ResponseConverted>;

    template <auto *Function>
    using Convert = ConversionFunction<Reference, Function>;
    template <auto *Function>
    using Move = MoveFunction<Reference, Function>;

    /**
     * @brief Handler function to handle requests made to a service.
     *
     */
    class HandlerFunction {
    public:
      template <typename DataType>
      using Function = ResponseConverted (*)(const RequestConverted &, DataType *);

      /**
       * @brief Construct a new handler function.
       *
       * @param function Function to be used as a handler function.
       */
      template <typename DataType>
      HandlerFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

      /**
       * @brief Call the stored conversion function.
       *
       * @param request Request sent by the client.
       * @param user_ptr User pointer to access generic external data.
       * @return ResponseType Response to be sent to the client.
       */
      inline ResponseStorage call(const RequestStorage &request, const void *user_ptr,
        ServerBase<RequestType, ResponseType> *reference) const {
        RequestConverted request_converted;
        Convert<RequestType::convertTo>::call(request, request_converted, user_ptr,
          *dynamic_cast<GenericServer<RequestConverted, ResponseConverted> *>(reference));

        ResponseConverted response_converted = function(request_converted, const_cast<void *>(user_ptr));
        ResponseStorage response;

        if constexpr (can_move_from<ResponseType, Reference>) {
          Move<ResponseType::moveFrom>::call(std::move(response_converted), response, user_ptr,
            *dynamic_cast<GenericServer<RequestConverted, ResponseConverted> *>(reference));
        } else {
          Convert<ResponseType::convertFrom>::call(response_converted, response, user_ptr,
            *dynamic_cast<GenericServer<RequestConverted, ResponseConverted> *>(reference));
        }

        return response;
      }

    private:
      Function<void> function;
    };

    ServerBase(ServerBase &) = delete;
    ServerBase(ServerBase &&) = delete;

  private:
    /**
     * @brief Construct a new Server object
     *
     * @param service_name Name of the service.
     * @param node Reference to the parent node.
     * @param handler_function Handler function to handle requests made to a service.
     * @param user_ptr User pointer to be used by the handler function.
     */
    ServerBase(const std::string &service_name, Node &node, HandlerFunction handler_function, const void *user_ptr = nullptr)
      requires can_convert_to<RequestType, Reference> && can_convert_from<ResponseType, Reference> :
      GenericServer<RequestConverted, ResponseConverted>(ServiceInfo::get<RequestType, ResponseType>(service_name,
        node.environment.service_map.addServer<RequestType, ResponseType>(service_name, this))),
      node(node), handler_function(handler_function), user_ptr(user_ptr) {
      for (PluginRegistration &plugin : node.environment.plugin_list) {
        if (plugin.delete_flag.test()
          || !plugin.filter.check(GenericServer<RequestConverted, ResponseConverted>::service_info.service_hash)) {
          continue;
        }

        utils::ConsumerGuard<u32> guard(plugin.use_count);

        if (plugin.service_callback != nullptr) {
          plugin.service_callback(plugin.user_ptr, GenericServer<RequestConverted, ResponseConverted>::service_info);
        }
      }
    }

    friend class Node;
    friend class Client;

    Node &node;
    const HandlerFunction handler_function;
    const void *const user_ptr;

  public:
    /**
     * @brief Destroy the Server object.
     *
     */
    ~ServerBase() {
      GenericServer<RequestConverted, ResponseConverted>::service_info.service.removeServer(this);
    }
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename RequestType, typename ResponseType>
  class Server final : public ServerBase<RequestType, ResponseType> {
  public:
    using Super = ServerBase<RequestType, ResponseType>;
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Server(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<RequestType>
  class Server<RequestType, ResponseType> final : public ServerBase<Message<RequestType>, ResponseType> {
  public:
    using Super = ServerBase<Message<RequestType>, ResponseType>;
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Server(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<ResponseType>
  class Server<RequestType, ResponseType> final : public ServerBase<RequestType, Message<ResponseType>> {
  public:
    using Super = ServerBase<RequestType, Message<ResponseType>>;
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Server(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<RequestType> && is_flatbuffer<ResponseType>
  class Server<RequestType, ResponseType> final : public ServerBase<Message<RequestType>, Message<ResponseType>> {
  public:
    using Super = ServerBase<Message<RequestType>, Message<ResponseType>>;
    using Ptr = std::unique_ptr<Server<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Server(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  class Client;

  template <typename RequestConverted, typename ResponseConverted>
  class GenericClient {
  protected:
    explicit GenericClient(ServiceInfo service_info) : service_info(std::move(service_info)) {}

    const ServiceInfo service_info;

  public:
    /**
     * @brief Get information about the relevant service.
     *
     * @return const ServiceInfo& Information about the service.
     */
    [[nodiscard]] inline const ServiceInfo &getServiceInfo() const {
      return service_info;
    }
  };

  /**
   * @brief Generic class to perform requests to a service.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   */
  template <typename RequestType, typename ResponseType>
  requires is_message<RequestType> && is_message<ResponseType>
  class ClientBase : public GenericClient<typename RequestType::Converted, typename ResponseType::Converted> {
  public:
    using RequestStorage = typename RequestType::Storage;
    using ResponseStorage = typename ResponseType::Storage;
    using RequestConverted = typename RequestType::Converted;
    using ResponseConverted = typename ResponseType::Converted;

    using Reference = GenericClient<RequestConverted, ResponseConverted>;

    template <auto *Function>
    using Convert = ConversionFunction<Reference, Function>;
    template <auto *Function>
    using Move = MoveFunction<Reference, Function>;

    ClientBase(ClientBase &) = delete;
    ClientBase(ClientBase &&) = delete;

  private:
    /**
     * @brief Construct a new Client object.
     *
     * @param service_name Name of the service.
     * @param node Reference to the parent node.
     * @param user_ptr User pointer to be forwarded to conversion and move functions.
     */
    ClientBase(const std::string &service_name, Node &node, const void *user_ptr = nullptr)
      requires can_convert_from<RequestType, Reference> && can_convert_to<ResponseType, Reference> :
      GenericClient<RequestConverted, ResponseConverted>(ServiceInfo::get<RequestType, ResponseType>(service_name,
        node.environment.service_map.getService<RequestType, ResponseType>(service_name))),
      node(node), user_ptr(user_ptr) {}

    friend class Node;

    Node &node;
    const void *const user_ptr;

  public:
    using Future = std::shared_future<ResponseConverted>;

    /**
     * @brief Destroy the Client object.
     *
     */
    ~ClientBase() = default;

    /**
     * @brief Make a request to a service asynchronously.
     * A call to this function will not block.
     *
     * @param request Object containing the data to be processed by the corresponding server.
     * @return Future Future to be completed by the server.
     * @throw ServiceUnavailableException When no server is handling requests to the relevant service.
     */
    Future callAsync(const RequestConverted &request) {
      std::promise<ResponseConverted> promise;
      Future future = promise.get_future();

      std::thread(
        [this](std::promise<ResponseConverted> promise, RequestConverted request) {
        try {
          ServiceMap::Service::ServerReference reference =
            GenericClient<RequestConverted, ResponseConverted>::service_info.service.getServer();
          Server<RequestType, ResponseType> *server = reference;

          if (server == nullptr) {
            throw ServiceUnavailableException("Service is not available.", node.getLogger());
          }

          RequestStorage request_storage;

          if constexpr (can_move_from<RequestType, Reference>) {
            Move<RequestType::moveFrom>::call(std::move(request), request_storage, user_ptr,
              *dynamic_cast<GenericClient<RequestConverted, ResponseConverted> *>(this));
          } else {
            Convert<RequestType::convertFrom>::call(request, request_storage, user_ptr,
              *dynamic_cast<GenericClient<RequestConverted, ResponseConverted> *>(this));
          }

          ResponseStorage response_storage = server->handler_function.call(request_storage, server->user_ptr, server);
          ResponseConverted response;

          if constexpr (can_move_to<ResponseType, Reference>) {
            Move<ResponseType::moveTo>::call(std::move(response_storage), response, user_ptr,
              *dynamic_cast<GenericClient<RequestConverted, ResponseConverted> *>(this));
          } else {
            Convert<ResponseType::convertTo>::call(response_storage, response, user_ptr,
              *dynamic_cast<GenericClient<RequestConverted, ResponseConverted> *>(this));
          }

          promise.set_value(std::move(response));
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
    ResponseConverted callSync(const RequestConverted &request) {
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
    ResponseConverted callSync(const RequestConverted &request, const std::chrono::duration<R, P> &timeout_duration) {
      Future future = callAsync(request);

      if (future.wait_for(timeout_duration) != std::future_status::ready) {
        throw ServiceTimeoutException("Service took too long to respond.", node.getLogger());
      }

      return future.get();
    }
  };

  // Wrapper classes to allow flatbuffer types to also work as template arguments.
  template <typename RequestType, typename ResponseType>
  class Client final : public ClientBase<RequestType, ResponseType> {
  public:
    using Super = ClientBase<RequestType, ResponseType>;
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Client(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<RequestType>
  class Client<RequestType, ResponseType> final : public ClientBase<Message<RequestType>, ResponseType> {
  public:
    using Super = ClientBase<Message<RequestType>, ResponseType>;
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Client(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<ResponseType>
  class Client<RequestType, ResponseType> final : public ClientBase<RequestType, Message<ResponseType>> {
  public:
    using Super = ClientBase<RequestType, Message<ResponseType>>;
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Client(Args &&...args) : Super(std::forward<Args>(args)...){};
  };

  template <typename RequestType, typename ResponseType>
  requires is_flatbuffer<RequestType> && is_flatbuffer<ResponseType>
  class Client<RequestType, ResponseType> final : public ClientBase<Message<RequestType>, Message<ResponseType>> {
  public:
    using Super = ClientBase<Message<RequestType>, Message<ResponseType>>;
    using Ptr = std::unique_ptr<Client<RequestType, ResponseType>>;

    template <typename... Args>
    explicit Client(Args &&...args) : Super(std::forward<Args>(args)...){};
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
  explicit Node(NodeEnvironment environment) : environment(std::move(environment)), logger(environment.name) {}

  /**
   * @brief Get a logger with the name of the node.
   *
   * @return Logger A logger with the name of the node.
   */
  [[nodiscard]] inline Logger &getLogger() {
    return logger;
  }

  /**
   * @brief Construct and add a sender to the node.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam Args Types of the arguments to be forwarded to the sender specific constructor.
   * @param args Arguments to be forwarded to the sender specific constructor.
   * @return Sender<MessageType>::Ptr Pointer to the sender.
   */
  template <typename MessageType, typename... Args>
  typename Sender<MessageType>::Ptr addSender(const std::string &topic_name, Args &&...args)
    requires is_message<MessageType> || is_flatbuffer<MessageType> {
    using Ptr = Sender<MessageType>::Ptr;
    return Ptr(new Sender<MessageType>(topic_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a receiver to the node.
   *
   * @tparam MessageType Type of the messages sent over the topic.
   * @tparam Args Types of the arguments to be forwarded to the receiver specific constructor.
   * @param args Arguments to be forwarded to the receiver specific constructor.
   * @return Receiver<MessageType>::Ptr Pointer to the receiver.
   */
  template <typename MessageType, typename... Args>
  typename Receiver<MessageType>::Ptr addReceiver(const std::string &topic_name, Args &&...args)
    requires is_message<MessageType> || is_flatbuffer<MessageType> {
    using Ptr = Receiver<MessageType>::Ptr;
    return Ptr(new Receiver<MessageType>(topic_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a server to the node.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   * @param args Arguments to be forwarded to the server specific constructor.
   * @return Server<RequestType, ResponseType>::Ptr Pointer to the server.
   */
  template <typename RequestType, typename ResponseType, typename... Args>
  typename Server<RequestType, ResponseType>::Ptr addServer(const std::string &service_name, Args &&...args)
    requires(is_message<RequestType> || is_flatbuffer<RequestType>) && (is_message<ResponseType> || is_flatbuffer<ResponseType>) {
    using Ptr = Server<RequestType, ResponseType>::Ptr;
    return Ptr(new Server<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  /**
   * @brief Construct and add a client to the node.
   *
   * @tparam RequestType Type of the request made to a service.
   * @tparam ResponseType Type of the response made to by a service.
   * @param args Arguments to be forwarded to the client specific constructor.
   * @return Client<RequestType, ResponseType>::Ptr Pointer to the client.
   */
  template <typename RequestType, typename ResponseType, typename... Args>
  typename Client<RequestType, ResponseType>::Ptr addClient(const std::string &service_name, Args &&...args)
    requires(is_message<RequestType> || is_flatbuffer<RequestType>) && (is_message<ResponseType> || is_flatbuffer<ResponseType>) {
    using Ptr = Client<RequestType, ResponseType>::Ptr;
    return Ptr(new Client<RequestType, ResponseType>(service_name, *this, std::forward<Args>(args)...));
  }

  friend class Manager;

private:
  const NodeEnvironment environment;
  Logger logger;
};

}  // namespace lbot
}  // namespace labrat
