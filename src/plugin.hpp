/**
 * @file plugin.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/robot/utils/types.hpp>

#include <atomic>
#include <chrono>
#include <list>
#include <string>

#include <google/protobuf/descriptor.h>

namespace labrat::robot {

/**
 * @brief Class containing data required for registering a plugin.
 *
 */
class Plugin {
public:
  using List = std::list<Plugin>;

  /**
   * @brief Default constructor to initialize the user count to zero.
   *
   */
  inline Plugin() {
    use_count = 0;
  }

  /**
   * @brief Copy constructor to copy the members and to initialize the user count to zero.
   *
   * @param rhs Object to be copied.
   */
  inline Plugin(const Plugin &rhs) : user_ptr(rhs.user_ptr), topic_callback(rhs.topic_callback), message_callback(rhs.message_callback) {
    use_count = 0;
  }

  /**
   * @brief User pointer to be returned on any callback performed.
   *
   */
  void *user_ptr;

  /**
   * @brief Information on a topic provided on callbacks.
   *
   */
  struct TopicInfo {
    const std::size_t type_hash;
    const std::size_t topic_hash;
    const std::string topic_name;
    const google::protobuf::Descriptor *type_descriptor;
  };

  /**
   * @brief Callback function pointer for when a sender is created.
   *
   */
  void (*topic_callback)(void *user_ptr, const TopicInfo &info);

  /**
   * @brief Information on a message provided on callbacks.
   *
   */
  struct MessageInfo {
    const TopicInfo &topic_info;
    const std::chrono::nanoseconds timestamp;
    std::string serialized_message;
  };

  /**
   * @brief Callback function pointer for when a message is sent over a topic.
   *
   */
  void (*message_callback)(void *user_ptr, const MessageInfo &info);

private:
  std::atomic_flag delete_flag;
  std::atomic<u32> use_count;

  friend class Manager;
  friend class Node;
};

}  // namespace labrat::robot
