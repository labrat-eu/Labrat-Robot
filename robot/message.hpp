#pragma once

#include <labrat/robot/utils/concepts.hpp>

#include <chrono>
#include <concepts>
#include <utility>

#include <google/protobuf/message_lite.h>

namespace labrat::robot {

template <typename T>
requires std::is_base_of_v<google::protobuf::Message, T>
class Message {
public:
  using Content = T;

  Message() {
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  }

  Message(const Content &content) : content(content) {
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  }

  Message(Content &&content) : content(std::forward(content)) {
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  }

  inline Content &get() {
    return content;
  };

  inline const Content &get() const {
    return content;
  };

  inline Content &operator()() {
    return get();
  };

  inline const Content &operator()() const {
    return get();
  };

  inline std::chrono::nanoseconds getTimestamp() const {
    return timestamp;
  }

private:
  std::chrono::nanoseconds timestamp;
  Content content;
};

template <typename T>
concept is_message = utils::is_specialization_of_v<T, Message>;

template <typename T>
concept is_container = requires {
  typename T::MessageType;
};

template <typename OriginalType, typename ConvertedType>
using ConversionFunction = void (*)(const OriginalType &, ConvertedType &);

template <typename OriginalType, typename ConvertedType>
inline void defaultSenderConversionFunction(const ConvertedType &source,
  OriginalType &destination) requires std::is_same_v<OriginalType, ConvertedType> {
  destination = source;
}

template <typename OriginalType, typename ConvertedType>
inline void defaultSenderConversionFunction(const ConvertedType &source, OriginalType &destination) {
  destination = OriginalType();
  ConvertedType::toMessage(source, destination);
}

template <typename OriginalType, typename ConvertedType>
inline void defaultReceiverConversionFunction(const OriginalType &source,
  ConvertedType &destination) requires std::is_same_v<OriginalType, ConvertedType> {
  destination = source;
}

template <typename OriginalType, typename ConvertedType>
inline void defaultReceiverConversionFunction(const OriginalType &source, ConvertedType &destination) {
  ConvertedType::fromMessage(source, destination);
}

}  // namespace labrat::robot
