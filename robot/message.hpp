#pragma once

#include <labrat/robot/utils/concepts.hpp>

#include <concepts>
#include <utility>

#include <google/protobuf/message.h>

namespace labrat::robot {

template <typename Content>
requires std::is_base_of_v<google::protobuf::Message, Content>
class Message {
public:
  Message() {}
  Message(Content &content) : content(content) {}
  Message(Content &&content) : content(std::forward(content)) {}

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

private:
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
