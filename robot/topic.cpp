#include <labrat/robot/topic.hpp>

namespace labrat::robot {

TopicMap::TopicMap() {}

TopicMap::Topic::Topic(Handle handle) : handle(handle) {
  sender = nullptr;
}

TopicMap::Topic &TopicMap::getTopic(const std::string &topic) {
  const std::unordered_map<std::string, Topic>::iterator iterator = map.find(topic);

  if (iterator == map.end()) {
    // throw
  }

  return iterator->second;
}

TopicMap::Topic &TopicMap::getTopic(const std::string &topic, Topic::Handle handle) {
  TopicMap::Topic &result = map.emplace(topic, handle).first->second;

  if (handle != result.handle) {
    // throw
  }

  return result;
}

void *TopicMap::Topic::getSender() const {
  return sender;
}

void TopicMap::Topic::addSender(void *new_sender) {
  if (sender != nullptr) {
    // throw
  }

  sender = new_sender;
}

void TopicMap::Topic::removeSender(void *old_sender) {
  if (sender != old_sender) {
    // throw
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
    // throw
  }

  receivers.erase(iterator);
}

}  // namespace labrat::robot
