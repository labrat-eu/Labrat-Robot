/**
 * @file plugin.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/robot/base.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/utils/types.hpp>

#include <atomic>
#include <chrono>
#include <list>
#include <string>
#include <unordered_set>

#include <flatbuffers/stl_emulation.h>

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
  inline Plugin(const Plugin &rhs) :
    user_ptr(rhs.user_ptr), topic_callback(rhs.topic_callback), message_callback(rhs.message_callback), filter(rhs.filter) {
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
    const std::string type_name;
    const std::size_t topic_hash;
    const std::string topic_name;

    /**
     * @brief Get information about a topic by name.
     * This will not lookup any data.
     *
     * @tparam MessageType Type of the message sent over the relevant topic.
     * @param topic_name Name of the topic.
     * @return Plugin::TopicInfo Information about the topic.
     */
    template <typename MessageType>
    requires is_message<MessageType>
    static Plugin::TopicInfo get(const std::string &topic_name) {
      const Plugin::TopicInfo result = {
        .type_hash = typeid(MessageType).hash_code(),
        .type_name = MessageType::getName(),
        .topic_hash = std::hash<std::string>()(topic_name),
        .topic_name = topic_name,
      };

      return result;
    }
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
    const flatbuffers::span<u8> serialized_message;
  };

  /**
   * @brief Callback function pointer for when a message is sent over a topic.
   *
   */
  void (*message_callback)(void *user_ptr, const MessageInfo &info);

  class Filter {
  public:
    Filter() : mode(true) {}

    /**
     * @brief Check whether a callback should be performed for the topic with the supplied hash code.
     *
     * @param topic_hash Hash code of the topic.
     * @return true The callback should be performed.
     * @return false The callback should not be performed.
     */
    inline bool check(std::size_t topic_hash) const {
      return (set.contains(topic_hash) ^ mode);
    }

    /**
     * @brief Check whether a callback should be performed for the topic with the supplied hash code.
     *
     * @param topic_name Name of the topic.
     * @return true The callback should be performed.
     * @return false The callback should not be performed.
     */
    inline bool check(const std::string &topic_name) const {
      return check(std::hash<std::string>()(topic_name));
    }

    /**
     * @brief Add a topic to the filter.
     * Depending on the supplied mode this will either whitelist or blacklist the relevant topic.
     * If the mode is changed, all previously added topics will be removed from the filter.
     *
     * @tparam Mode New mode of the filter.
     * @param topic_hash Hash of the topic to be added to the filter-
     */
    template <bool Mode>
    void add(std::size_t topic_hash) {
      if (mode != Mode) {
        set.clear();
        mode = Mode;
      }

      set.emplace(topic_hash);
    }

    /**
     * @brief Whitelist a topic.
     * All previously blacklisted topics will be removed from the filter.
     *
     * @param topic_name Name of the topic.
     */
    inline void whitelist(const std::string &topic_name) {
      add<false>(std::hash<std::string>()(topic_name));
    }

    /**
     * @brief Blacklist a topic.
     * All previously whitelisted topics will be removed from the filter.
     *
     * @param topic_name Name of the topic.
     */
    inline void blacklist(const std::string &topic_name) {
      add<true>(std::hash<std::string>()(topic_name));
    }

  private:
    std::unordered_set<std::size_t> set;

    /**
     * @brief Mode of the filter.
     * When true specified blacklisted topics will be blocked and all other topics will pass.
     * When false specified whitelisted topics will pass and all other topics will be blocked.
     *
     */
    bool mode;
  };

  Filter filter;

private:
  std::atomic_flag delete_flag;
  std::atomic<u32> use_count;

  friend class Manager;
  friend class Node;
};

}  // namespace labrat::robot
