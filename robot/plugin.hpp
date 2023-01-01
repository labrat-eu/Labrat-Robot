#pragma once

#include <labrat/robot/utils/types.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>

namespace labrat::robot {

class Plugin {
public:
  using List = std::vector<Plugin>;

  void *user_ptr;

  struct TopicInfo {
    const std::size_t type_hash;
    const std::size_t topic_hash;
    const std::string topic_name;
    const google::protobuf::Descriptor *type_descriptor;
  };

  void (*topic_callback)(void *user_ptr, const TopicInfo &info);

  struct MessageInfo {
    const TopicInfo &topic_info;
    const std::chrono::nanoseconds timestamp;
    std::string serialized_message;
  };

  void (*message_callback)(void *user_ptr, const MessageInfo &info);
};

}  // namespace labrat::robot
