#include <labrat/robot/message.hpp>
#include <google/protobuf/message.h>

#include <utility>

namespace labrat::robot::msg {

template<typename NetworkMessage>
requires std::is_base_of_v<google::protobuf::Message, NetworkMessage>
class Network : Message {
public:
  Network() {}
  Network(NetworkMessage &content) : content(content) {}
  Network(NetworkMessage &&content) : content(std::forward(content)) {}

  inline NetworkMessage &get() {
    return content;
  };

  inline NetworkMessage &operator()() {
    return get();
  };

private:
  NetworkMessage content;
};

}
