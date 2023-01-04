#pragma once

#include <labrat/robot/exception.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/plugin.hpp>
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
  Plugin::List plugin_list;

public:
  ~Manager();

  static Manager &get();

  template <typename T, typename... Args>
  std::weak_ptr<T> addNode(const std::string &name, Args &&...args) requires std::is_base_of_v<Node, T> {
    const Node::Environment environment = {
      .name = name,
      .topic_map = topic_map,
      .plugin_list = plugin_list,
    };

    const std::pair<std::unordered_map<std::string, utils::FinalPtr<Node>>::iterator, bool> result =
      node_map.emplace(name, std::make_shared<T>(environment, std::forward<Args>(args)...));

    if (!result.second) {
      throw Exception("Node not added.");
    }

    return std::weak_ptr<T>(reinterpret_pointer_cast<T>(result.first->second));
  }

  void removeNode(const std::string &name);

  Plugin::List::iterator addPlugin(const Plugin &Plugin);
  void removePlugin(Plugin::List::iterator iterator);

  inline const Plugin::List &getPlugins() const {
    return plugin_list;
  }
};

}  // namespace labrat::robot
