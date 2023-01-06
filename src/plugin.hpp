#pragma once

#include <labrat/robot/utils/types.hpp>

#include <atomic>
#include <chrono>
#include <list>
#include <string>

#include <google/protobuf/descriptor.h>

namespace labrat::robot {

class Plugin {
public:
  using List = std::list<Plugin>;

  inline Plugin() {
    use_count = 0;
  }

  inline Plugin(const Plugin &rhs) : user_ptr(rhs.user_ptr), topic_callback(rhs.topic_callback), message_callback(rhs.message_callback) {
    use_count = 0;
  }

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

private:
  std::atomic_flag delete_flag;
  std::atomic<u32> use_count;

  friend class Manager;
  friend class Node;
};

}  // namespace labrat::robot
