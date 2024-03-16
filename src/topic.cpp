/**
 * @file topic.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/topic.hpp>

inline namespace labrat {
namespace lbot {

TopicMap::TopicMap() {}

TopicMap::Topic::Topic(Handle handle, const std::string &name) : handle(handle), name(name) {
  sender = nullptr;
}

void TopicMap::forceFlush() {
  for (std::pair<const std::string, Topic> &entry : map) {
    for (void *pointer : entry.second.getReceivers()) {
      Node::GenericReceiver<void> *receiver = reinterpret_cast<Node::GenericReceiver<void> *>(pointer);

      const std::size_t count = receiver->write_count.fetch_add(1, std::memory_order_relaxed);

      receiver->flush_flag = true;
      receiver->read_count.store(count);
      receiver->read_count.notify_one();
    }
  }
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic) {
  const std::unordered_map<std::string, Topic>::iterator iterator = map.find(topic);

  if (iterator == map.end()) {
    throw ManagementException("Topic '" + topic + "' not found.");
  }

  return iterator->second;
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic, Topic::Handle handle) {
  TopicMap::Topic &result =
    map.emplace(std::piecewise_construct, std::forward_as_tuple(topic), std::forward_as_tuple(handle, topic)).first->second;

  if (handle != result.handle) {
    throw ManagementException("Topic '" + topic + "' does not match the provided handle.");
  }

  return result;
}

void *TopicMap::Topic::getSender() const {
  return sender;
}

void TopicMap::Topic::addSender(void *new_sender) {
  if (sender != nullptr) {
    throw ManagementException("A sender has already been registered for this topic.");
  }

  sender = new_sender;
}

void TopicMap::Topic::removeSender(void *old_sender) {
  if (sender != old_sender) {
    throw ManagementException("The sender to be removed does not match the existing sender.");
  }

  sender = nullptr;
}

void TopicMap::Topic::addReceiver(void *new_receiver) {
  utils::FlagGuard guard(change_flag);
  utils::waitUntil<std::size_t>(use_count, 0);

  receivers.emplace_back(new_receiver);
}

void TopicMap::Topic::removeReceiver(void *old_receiver) {
  utils::FlagGuard guard(change_flag);
  utils::waitUntil<std::size_t>(use_count, 0);

  std::vector<void *>::iterator iterator = std::find(receivers.begin(), receivers.end(), old_receiver);

  if (iterator == receivers.end()) {
    throw ManagementException("Receiver to be removed not found.");
  }

  receivers.erase(iterator);
}

}  // namespace lbot
}  // namespace labrat
