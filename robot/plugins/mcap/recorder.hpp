#pragma once

#include <labrat/robot/plugin.hpp>

#include <string>
#include <unordered_map>

#include <mcap/writer.hpp>

namespace labrat::robot::plugins {

class McapRecorder {
public:
  McapRecorder(const std::string &filename);
  ~McapRecorder();

private:
  static void topicCallback(void *user_ptr, const Plugin::TopicInfo &info);
  static void messageCallback(void *user_ptr, const Plugin::MessageInfo &info);

  std::unordered_map<std::size_t, mcap::Schema> schema_map;

  struct ChannelInfo {
    mcap::Channel channel;
    u64 index;

    ChannelInfo(const std::string_view topic, const std::string_view encoding, mcap::SchemaId schema) :
      channel(topic, encoding, schema), index(0) {}
  };

  std::unordered_map<std::size_t, ChannelInfo> channel_map;

  mcap::McapWriter writer;
};

}  // namespace labrat::robot::plugins
