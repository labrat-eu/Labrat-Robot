#include <labrat/robot/exception.hpp>
#include <labrat/robot/topic.hpp>

namespace labrat::robot {

TopicMap::TopicMap() {}

TopicMap::Topic::Topic(Handle handle, const std::string &name) : handle(handle), name(name) {
  sender = nullptr;
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic) {
  const std::unordered_map<std::string, Topic>::iterator iterator = map.find(topic);

  if (iterator == map.end()) {
    throw Exception("Topic '" + topic + "' not found.");
  }

  return iterator->second;
}

TopicMap::Topic &TopicMap::getTopicInternal(const std::string &topic, Topic::Handle handle) {
  TopicMap::Topic &result =
    map.emplace(std::piecewise_construct, std::forward_as_tuple(topic), std::forward_as_tuple(handle, topic)).first->second;

  if (handle != result.handle) {
    throw Exception("Topic '" + topic + "' does not match the provided handle.");
  }

  return result;
}

void *TopicMap::Topic::getSender() const {
  return sender;
}

void TopicMap::Topic::addSender(void *new_sender) {
  if (sender != nullptr) {
    throw Exception("A sender has already been registered for this topic.");
  }

  sender = new_sender;
}

void TopicMap::Topic::removeSender(void *old_sender) {
  if (sender != old_sender) {
    throw Exception("The sender to be removed does not match the existing sender.");
  }

  sender = nullptr;
}

void TopicMap::Topic::addReceiver(void *new_receiver) {
  utils::FlagGuard guard(change_flag);
  utils::SpinUntil<std::size_t>(use_count, 0);

  receivers.emplace_back(new_receiver);
}

void TopicMap::Topic::removeReceiver(void *old_receiver) {
  utils::FlagGuard guard(change_flag);
  utils::SpinUntil<std::size_t>(use_count, 0);

  std::vector<void *>::iterator iterator = std::find(receivers.begin(), receivers.end(), old_receiver);

  if (iterator == receivers.end()) {
    throw Exception("Receiver to be removed not found.");
  }

  receivers.erase(iterator);
}

}  // namespace labrat::robot
