#pragma once

#include <labrat/robot/exception.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/topic.hpp>
#include <labrat/robot/utils/final_ptr.hpp>
#include <labrat/robot/utils/types.hpp>

#include <concepts>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace labrat::robot {

class Manager {
private:
  Manager();

  static std::unique_ptr<Manager> instance;

  std::unordered_map<std::string, utils::FinalPtr<Node>> node_map;
  TopicMap topic_map;

public:
  ~Manager();

  static Manager &get();

  template <typename T, typename... Args>
  std::weak_ptr<T> addNode(const std::string &name, Args &&...args) requires std::is_base_of_v<Node, T> {
    const std::pair<std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator, bool> result =
      node_map.emplace(name, std::make_shared<T>(name, topic_map, std::forward<Args>(args)...));

    if (!result.second) {
      throw Exception("Node not added.");
    }

    return std::weak_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  template <typename T>
  void removeNode(const std::string &name) requires std::is_base_of_v<Node, T> {
    const std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator iterator = node_map.find(name);

    if (iterator == node_map.end()) {
      throw Exception("Node not found.");
    }

    node_map.erase(iterator);
  }
};

}  // namespace labrat::robot
