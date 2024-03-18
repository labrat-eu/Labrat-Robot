/**
 * @file message.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/utils/concepts.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <chrono>
#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

#include <flatbuffers/flatbuffers.h>

inline namespace labrat {
namespace lbot {

template <auto* Function>
class ConversionFunction {};

template <typename OriginalType, typename ConvertedType, typename DataType, auto (*Function)(const OriginalType &, ConvertedType &, DataType *) -> void>
class ConversionFunction<Function> {
public:
    /**
   * @brief Call the stored conversion function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   * @param user_ptr User pointer to access generic external data.
   */
  static inline void call(const OriginalType &source, ConvertedType &destination, const void *user_ptr) {
    void *ptr = const_cast<void *>(user_ptr);
    Function(source, destination, reinterpret_cast<DataType *>(ptr));
  }
};

template <typename OriginalType, typename ConvertedType, auto (*Function)(const OriginalType &, ConvertedType &) -> void>
class ConversionFunction<Function> {
public:
    /**
   * @brief Call the stored conversion function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   */
  static inline void call(const OriginalType &source, ConvertedType &destination, const void *) {
    Function(source, destination);
  }
};

template <typename T>
concept can_convert_to_noptr = requires(const T::Storage &source, T::Converted &destination) {
  {T::convertTo(source, destination)};
};
template <typename T>
concept can_convert_to_ptr = requires(const T::Storage &source, T::Converted &destination) {
  {T::convertTo(source, destination, nullptr)};
};
template <typename T>
concept can_convert_to = can_convert_to_noptr<T> || can_convert_to_ptr<T>;

template <typename T>
concept can_convert_from_noptr = requires(const T::Converted &source, T::Storage &destination) {
  {T::convertFrom(source, destination)};
};
template <typename T>
concept can_convert_from_ptr = requires(const T::Converted &source, T::Storage &destination) {
  {T::convertFrom(source, destination, nullptr)};
};
template <typename T>
concept can_convert_from = can_convert_from_noptr<T> || can_convert_from_ptr<T>;

template <auto* Function>
class MoveFunction {};

template <typename OriginalType, typename ConvertedType, typename DataType, auto (*Function)(OriginalType &&, ConvertedType &, DataType *) -> void>
class MoveFunction<Function> {
public:
    /**
   * @brief Call the stored conversion function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   * @param user_ptr User pointer to access generic external data.
   */
  static inline void call(OriginalType &&source, ConvertedType &destination, const void *user_ptr) {
    void *ptr = const_cast<void *>(user_ptr);
    Function(std::forward<OriginalType>(source), destination, reinterpret_cast<const DataType *>(ptr));
  }
};

template <typename OriginalType, typename ConvertedType, auto (*Function)(OriginalType &&, ConvertedType &) -> void>
class MoveFunction<Function> {
public:
    /**
   * @brief Call the stored conversion function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   */
  static inline void call(OriginalType &&source, ConvertedType &destination, const void *) {
    Function(std::forward<OriginalType>(source), destination);
  }
};

template <typename T>
concept can_move_to_noptr = requires(T::Storage &&source, T::Converted &destination) {
  {T::moveTo(std::move(source), destination)};
};
template <typename T>
concept can_move_to_ptr = requires(T::Storage &&source, T::Converted &destination) {
  {T::moveTo(std::move(source), destination, nullptr)};
};
template <typename T>
concept can_move_to = can_move_to_noptr<T> || can_move_to_ptr<T>;

template <typename T>
concept can_move_from_noptr = requires(T::Converted &&source, T::Storage &destination) {
  {T::moveFrom(std::move(source), destination)};
};
template <typename T>
concept can_move_from_ptr = requires(T::Converted &&source, T::Storage &destination) {
  {T::moveFrom(std::move(source), destination, nullptr)};
};
template <typename T>
concept can_move_from = can_move_from_noptr<T> || can_move_from_ptr<T>;

/**
 * @brief Abstract time class for Message.
 *
 */
class MessageTime {
public:
  /**
   * @brief Get the timestamp of the object.
   *
   * @return std::chrono::nanoseconds
   */
  inline std::chrono::nanoseconds getTimestamp() const {
    return lbot_message_base_timestamp;
  }

protected:
  /**
   * @brief Construct a new Message Base object and set the timestamp to the current time.
   *
   */
  MessageTime() {
    lbot_message_base_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  }

private:
  // Long name to make ambiguity less likely.
  std::chrono::nanoseconds lbot_message_base_timestamp;
};

template <typename T>
concept is_flatbuffer = std::is_base_of_v<flatbuffers::Table, T>;

/**
 * @brief Unsafe wrapper of a flatbuf message for use within this library.
 *
 * @tparam T
 */
template <typename FlatbufferType, typename ConvertedType>
requires is_flatbuffer<FlatbufferType>
class MessageBase : public MessageTime, public FlatbufferType::NativeTableType {
public:
  using Storage = MessageBase<FlatbufferType, ConvertedType>;
  using Flatbuffer = FlatbufferType;
  using Content = typename FlatbufferType::NativeTableType;
  using Converted = ConvertedType;

  /**
   * @brief Default constructor to only set the timestamp to the current time.
   *
   */
  MessageBase() {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  MessageBase(const Content &content) : Content(content) {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  MessageBase(Content &&content) : Content(std::forward<Content>(content)) {}

  /**
   * @brief Get the fully qualified type name.
   *
   * @return constexpr std::string Name of the type.
   */
  static inline constexpr std::string getName() {
    return std::string(Content::TableType::GetFullyQualifiedName());
  }
};

template <typename T, typename Flatbuffer = T::Flatbuffer, typename Converted = T::Converted>
concept is_message = std::derived_from<T, MessageBase<Flatbuffer, Converted>>;

/**
 * @brief Safe wrapper of a flatbuf message for use within this library.
 *
 * @tparam T
 */
template <typename FlatbufferType>
requires is_flatbuffer<FlatbufferType>
class Message final : public MessageBase<FlatbufferType, Message<FlatbufferType>> {
public:
  using Super = MessageBase<FlatbufferType, Message<FlatbufferType>>;
  using Content = typename Super::Content;

  /**
   * @brief Default constructor to only set the timestamp to the current time.
   *
   */
  Message() : Super() {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  Message(const Content &content) : Super(content) {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  Message(Content &&content) : Super(std::forward<Content>(content)) {}

  static inline void convertTo(const Super::Storage &source, Super::Converted &destination) {
    destination = source;
  }

  static inline void moveTo(Super::Storage &&source, Super::Converted &destination) {
    destination = std::move(source);
  }

  static inline void convertFrom(const Super::Converted &source, Super::Storage &destination) {
    destination = source;
  }

  static inline void moveFrom(Super::Converted &&source, Super::Storage &destination) {
    destination = std::move(source);
  }
};

/**
 * @brief Non-templated container to store reflection info about a Message type.
 *
 */
class MessageReflection {
public:
  /**
   * @brief Construct a new Message Reflection object by type template.
   *
   * @tparam T
   * @return requires
   */
  template <typename T>
  requires is_flatbuffer<T>
  MessageReflection() : MessageReflection(Message<T>::getName()) {}

  /**
   * @brief Construct a new Message Reflection object by type name.
   *
   * @param name Name of the type.
   */
  MessageReflection(const std::string &name);

  /**
   * @brief Get buffer containing the type information.
   * @details For this function to work properly, the LBOT_REFLECTION_PATH environment variable must be set correctly.
   *
   * @return const std::string& String contianing the binary type information.
   */
  inline const std::string &getBuffer() const {
    return buffer;
  }

  /**
   * @brief Check whether or not the requested type could be found and the buffer is valid.
   *
   * @return true The buffer is valid.
   * @return false The type could not be found and the type is invalid.
   */
  inline bool isValid() const {
    return valid;
  }

private:
  std::string buffer;
  bool valid;
};

}  // namespace lbot
}  // namespace labrat
