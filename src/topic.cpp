/**
 * @file topic.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/topic.hpp>

inline namespace labrat {
namespace lbot {

TopicMap::TopicMap() = default;

TopicMap::Topic::Topic(Handle handle, std::string name) :
  handle(handle),
  name(std::move(name))
{
  sender = nullptr;
}

void TopicMap::forceFlush()
{
  for (std::pair<const std::string, Topic> &entry : map) {
    for (void *pointer : entry.second.getReceivers()) {
      Node::GenericReceiver<void> *receiver = reinterpret_cast<Node::GenericReceiver<void> *>(pointer);

      receiver->flush_flag = true;
      receiver->condition.notify_one();
    }
  }
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic)
{
  if (topic.empty()) {
    throw ManagementException("Topic name name must be non-empty.");
  }

  const std::unordered_map<std::string, Topic>::iterator iterator = map.find(topic);

  if (iterator == map.end()) {
    throw ManagementException("Topic '" + topic + "' not found.");
  }

  return iterator->second;
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic, Topic::Handle handle)
{
  if (topic.empty()) {
    throw ManagementException("Topic name name must be non-empty.");
  }

  TopicMap::Topic &result =
    map.emplace(std::piecewise_construct, std::forward_as_tuple(topic), std::forward_as_tuple(handle, topic)).first->second;

  if (handle != result.handle) {
    throw ManagementException("Topic '" + topic + "' does not match the provided handle.");
  }

  return result;
}

void *TopicMap::Topic::getSender() const
{
  return sender;
}

void TopicMap::Topic::addSender(void *new_sender)
{
  if (sender != nullptr) {
    throw ManagementException("A sender has already been registered for this topic.");
  }

  sender = new_sender;
}

void TopicMap::Topic::removeSender(void *old_sender)
{
  if (sender != old_sender) {
    throw ManagementException("The sender to be removed does not match the existing sender.");
  }

  sender = nullptr;
}

void TopicMap::Topic::addReceiver(void *new_receiver, bool is_const)
{
  FlagGuard guard(change_flag);
  waitUntil<std::size_t>(use_count, 0);

  if (is_const) {
    const_receivers.emplace_back(new_receiver);
  } else {
    receivers.emplace_back(new_receiver);
  }
}

void TopicMap::Topic::removeReceiver(void *old_receiver)
{
  FlagGuard guard(change_flag);
  waitUntil<std::size_t>(use_count, 0);

  std::vector<void *>::iterator iterator = std::find(receivers.begin(), receivers.end(), old_receiver);
  std::vector<void *>::iterator const_iterator = std::find(const_receivers.begin(), const_receivers.end(), old_receiver);

  if (iterator != receivers.end()) {
    receivers.erase(iterator);
  } else if (const_iterator != const_receivers.end()) {
    const_receivers.erase(const_iterator);
  } else {
    throw ManagementException("Receiver to be removed not found.");
  }
}

}  // namespace lbot
}  // namespace labrat
