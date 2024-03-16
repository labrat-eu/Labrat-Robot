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

/**
 * @brief Conversion function to convert one data type into another.
 *
 * @tparam OriginalType Type to be converted from.
 * @tparam ConvertedType Type to be converted to.
 */
template <typename OriginalType, typename ConvertedType>
class ConversionFunction {
public:
  template <typename DataType = void>
  using Function = void (*)(const OriginalType &, ConvertedType &, const DataType *);
  using FunctionWithoutUserPtr = void (*)(const OriginalType &, ConvertedType &);

  /**
   * @brief Construct a new conversion function.
   *
   * @tparam DataType Type of the data pointed to by the user pointer.
   * @param function Function to be used as a conversion function.
   */
  template <typename DataType = void>
  ConversionFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

  /**
   * @brief Construct a new conversion function.
   *
   * @param function Function to be used as a conversion function.
   */
  ConversionFunction(FunctionWithoutUserPtr function) : function(reinterpret_cast<Function<void>>(function)) {}

  /**
   * @brief Call the stored conversion function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   * @param user_ptr User pointer to access generic external data.
   */
  inline void operator()(const OriginalType &source, ConvertedType &destination, const void *user_ptr) const {
    function(source, destination, user_ptr);
  }

private:
  Function<void> function;
};

template <typename T>
concept can_convert_to = requires(const T &source, T::Converted &destination) {
  {T::convertTo(source, destination)};
} || requires(const T &source, T::Converted &destination) {
  {T::convertTo(source, destination, nullptr)};
};

template <typename T>
concept can_convert_from = requires(const T::Converted &source, T &destination) {
  {T::convertFrom(source, destination)};
} || requires(const T::Converted &source, T &destination) {
  {T::convertFrom(source, destination, nullptr)};
};

/**
 * @brief Move function to convert one data type into another.
 *
 * @tparam OriginalType Type to be converted from.
 * @tparam ConvertedType Type to be converted to.
 */
template <typename OriginalType, typename ConvertedType>
class MoveFunction {
public:
  template <typename DataType = void>
  using Function = void (*)(OriginalType &&, ConvertedType &, const DataType *);
  using FunctionWithoutUserPtr = void (*)(OriginalType &&, ConvertedType &);

  /**
   * @brief Default-construct a new Move Function object.
   *
   */
  MoveFunction() : function(nullptr) {}

  /**
   * @brief Construct a new move function.
   *
   * @tparam DataType Type of the data pointed to by the user pointer.
   * @param function Function to be used as a move function.
   */
  template <typename DataType = void>
  MoveFunction(Function<DataType> function) : function(reinterpret_cast<Function<void>>(function)) {}

  /**
   * @brief Construct a new move function.
   *
   * @param function Function to be used as a move function.
   */
  MoveFunction(FunctionWithoutUserPtr function) : function(reinterpret_cast<Function<void>>(function)) {}

  /**
   * @brief Call the stored move function.
   *
   * @param source Object to be converted.
   * @param destination Object to be converted to.
   * @param user_ptr User pointer to access generic external data.
   */
  inline void operator()(OriginalType &&source, ConvertedType &destination, const void *user_ptr) const {
    function(std::forward<OriginalType>(source), destination, user_ptr);
  }

  /**
   * @brief Check whether a valid function is stored.
   *
   * @return true The function is valid.
   * @return false The function is invalid.
   */
  inline operator bool() const {
    return (function != nullptr);
  }

private:
  Function<void> function;
};

template <typename T>
concept can_move_to = requires(T &&source, T::Converted &destination) {
  {T::moveTo(std::move(source), destination)};
} || requires(T &&source, T::Converted &destination) {
  {T::moveTo(std::move(source), destination, nullptr)};
};

template <typename T>
concept can_move_from = requires(T::Converted &&source, T &destination) {
  {T::moveFrom(std::move(source), destination)};
} || requires(T::Converted &&source, T &destination) {
  {T::moveFrom(std::move(source), destination, nullptr)};
};

/**
 * @brief Abstract base class for Message.
 *
 */
class MessageBase {
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
  MessageBase() {
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
class UnsafeMessage : public MessageBase, public FlatbufferType::NativeTableType {
public:
  using Self = UnsafeMessage<FlatbufferType, ConvertedType>;
  using FlatbufferArg = FlatbufferType;
  using ConvertedArg = ConvertedType;
  using Content = typename FlatbufferType::NativeTableType;
  using Converted =
    typename std::conditional_t<std::is_same_v<FlatbufferType, ConvertedType>, UnsafeMessage<FlatbufferType, ConvertedType>, ConvertedType>;

  /**
   * @brief Default constructor to only set the timestamp to the current time.
   *
   */
  UnsafeMessage() {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  UnsafeMessage(const Content &content) : Content(content) {}

  /**
   * @brief Construct a new Message object by specifying the contents stored within the message.
   * Also set the timestamp to the current time.
   *
   * @param content Contents of the message.
   */
  UnsafeMessage(Content &&content) : Content(std::forward<Content>(content)) {}

  /**
   * @brief Get the fully qualified type name.
   *
   * @return constexpr std::string Name of the type.
   */
  static inline constexpr std::string getName() {
    return std::string(Content::TableType::GetFullyQualifiedName());
  }
};

template <typename T, typename FlatbufferArg = T::FlatbufferArg, typename ConvertedArg = T::ConvertedArg>
concept is_message = std::derived_from<T, UnsafeMessage<FlatbufferArg, ConvertedArg>>;

/**
 * @brief Safe wrapper of a flatbuf message for use within this library.
 *
 * @tparam T
 */
template <typename FlatbufferType>
requires is_flatbuffer<FlatbufferType>
class Message final : public UnsafeMessage<FlatbufferType, FlatbufferType> {
public:
  using Super = UnsafeMessage<FlatbufferType, FlatbufferType>;
  using Self = Message<FlatbufferType>;
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

  static void convertTo(const Self &source, Super::Converted &destination) {
    destination = source;
  }

  static void convertFrom(const Super::Converted &source, Self &destination) {
    destination = source;
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
  requires std::is_base_of_v<flatbuffers::Table, T>
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
