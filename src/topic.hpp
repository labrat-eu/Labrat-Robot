/**
 * @file topic.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/message.hpp>
#include <labrat/robot/utils/atomic.hpp>
#include <labrat/robot/utils/types.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace labrat::robot {

class TopicMap {
public:
  class Topic {
  private:
    void *sender;
    std::vector<void *> receivers;

    std::atomic_flag change_flag;
    std::atomic<std::size_t> use_count;

    friend class ReceiverList;

  public:
    using Handle = std::size_t;

    class ReceiverList {
    public:
      inline ReceiverList(Topic &topic) : topic(topic) {
        while (true) {
          topic.use_count.fetch_add(1);

          [[likely]] if (!topic.change_flag.test()) {
            break;
          }

          topic.use_count.fetch_sub(1);
          topic.use_count.notify_all();
          topic.change_flag.wait(true);
        }
      }

      inline ReceiverList(ReceiverList &&rhs) : topic(rhs.topic) {
        topic.use_count.fetch_add(1);
      }

      inline ~ReceiverList() {
        topic.use_count.fetch_sub(1);
        topic.use_count.notify_all();
      }

      inline std::vector<void *>::iterator begin() const {
        return topic.receivers.begin();
      }

      inline std::vector<void *>::iterator end() const {
        return topic.receivers.end();
      }

    private:
      Topic &topic;
    };

    Topic(Handle handle, const std::string &name);

    void *getSender() const;
    void addSender(void *new_sender);
    void removeSender(void *old_sender);

    inline ReceiverList getReceivers() {
      return ReceiverList(*this);
    }

    void addReceiver(void *new_receiver);
    void removeReceiver(void *old_receiver);

    const Handle handle;
    const std::string name;
  };

  TopicMap();
  ~TopicMap() = default;

  template <typename T>
  Topic &addSender(const std::string &topic_name, void *sender) {
    Topic &topic = getTopicInternal(topic_name, typeid(T).hash_code());

    topic.addSender(sender);

    return topic;
  }

  Topic &removeSender(const std::string &topic_name, void *sender) {
    Topic &topic = getTopicInternal(topic_name);

    topic.removeSender(sender);

    return topic;
  }

  template <typename T>
  Topic &addReceiver(const std::string &topic_name, void *receiver) {
    Topic &topic = getTopicInternal(topic_name, typeid(T).hash_code());

    topic.addReceiver(receiver);

    return topic;
  }

  Topic &removeReceiver(const std::string &topic_name, void *receiver) {
    Topic &topic = getTopicInternal(topic_name);

    topic.removeReceiver(receiver);

    return topic;
  }

  void forceFlush();

private:
  Topic &getTopicInternal(const std::string &topic);
  Topic &getTopicInternal(const std::string &topic, std::size_t handle);

  std::unordered_map<std::string, Topic> map;
};

}  // namespace labrat::robot
