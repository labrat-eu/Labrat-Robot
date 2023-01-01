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
  using SchemaMap = std::unordered_map<std::size_t, mcap::Schema>;

  struct ChannelInfo {
    mcap::Channel channel;
    u64 index;

    ChannelInfo(const std::string_view topic, const std::string_view encoding, mcap::SchemaId schema) :
      channel(topic, encoding, schema), index(0) {}
  };

  using ChannelMap = std::unordered_map<std::size_t, ChannelInfo>;

  static void topicCallback(void *user_ptr, const Plugin::TopicInfo &info);
  static void messageCallback(void *user_ptr, const Plugin::MessageInfo &info);

  ChannelMap::iterator handleTopic(const Plugin::TopicInfo &info);
  ChannelMap::iterator handleMessage(const Plugin::MessageInfo &info);

  SchemaMap schema_map;
  ChannelMap channel_map;

  mcap::McapWriter writer;
};

}  // namespace labrat::robot::plugins
