#include <labrat/robot/exception.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/plugins/mcap/recorder.hpp>

#include <chrono>
#include <queue>

#include <google/protobuf/descriptor.pb.h>

#include <mcap/internal.hpp>
#include <mcap/types.inl>
#include <mcap/writer.inl>

namespace labrat::robot::plugins {

google::protobuf::FileDescriptorSet buildFileDescriptorSet(const google::protobuf::Descriptor *top_descriptor);

McapRecorder::McapRecorder(const std::string &filename) {
  const mcap::McapWriterOptions options("");
  const mcap::Status result = writer.open(filename, options);

  if (!result.ok()) {
    throw Exception("Failed to open '" + filename + "'.");
  }

  const Plugin plugin_info = {
    .user_ptr = reinterpret_cast<void *>(this),
    .topic_callback = McapRecorder::topicCallback,
    .message_callback = McapRecorder::messageCallback,
  };

  Manager::get().addPlugin(plugin_info);
}

McapRecorder::~McapRecorder() {
  writer.close();
}

void McapRecorder::topicCallback(void *user_ptr, const Plugin::TopicInfo &info) {
  McapRecorder *self = reinterpret_cast<McapRecorder *>(user_ptr);

  auto schema_iterator = self->schema_map.find(info.type_hash);
  if (schema_iterator == self->schema_map.end()) {
    schema_iterator = self->schema_map.emplace_hint(schema_iterator, std::piecewise_construct, std::forward_as_tuple(info.type_hash),
      std::forward_as_tuple(info.type_descriptor->full_name(), "protobuf",
        buildFileDescriptorSet(info.type_descriptor).SerializeAsString()));

    self->writer.addSchema(schema_iterator->second);
  }

  const auto channel_iterator = self->channel_map.emplace(std::piecewise_construct, std::forward_as_tuple(info.topic_hash),
    std::forward_as_tuple(info.topic_name, "protobuf", schema_iterator->second.id));

  if (channel_iterator.second) {
    self->writer.addChannel(channel_iterator.first->second.channel);
  }
}

void McapRecorder::messageCallback(void *user_ptr, const Plugin::MessageInfo &info) {
  McapRecorder *self = reinterpret_cast<McapRecorder *>(user_ptr);

  const auto channel_iterator = self->channel_map.find(info.topic_hash);
  if (channel_iterator == self->channel_map.end()) {
    throw Exception("Unknown topic.");
  }

  mcap::Message message;
  message.channelId = channel_iterator->second.channel.id;
  message.sequence = channel_iterator->second.index++;
  message.publishTime = info.timestamp.count();
  message.logTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  message.data = reinterpret_cast<const std::byte *>(info.serialized_message.data());
  message.dataSize = info.serialized_message.size();

  const mcap::Status result = self->writer.write(message);
  if (!result.ok()) {
    self->writer.terminate();
    self->writer.close();

    throw Exception("Failed to write message.");
  }
}

google::protobuf::FileDescriptorSet buildFileDescriptorSet(const google::protobuf::Descriptor *top_descriptor) {
  google::protobuf::FileDescriptorSet result;
  std::queue<const google::protobuf::FileDescriptor *> queue;

  queue.push(top_descriptor->file());

  std::unordered_set<std::string> dependencies;

  while (!queue.empty()) {
    const google::protobuf::FileDescriptor *next = queue.front();
    queue.pop();
    next->CopyTo(result.add_file());

    for (int i = 0; i < next->dependency_count(); ++i) {
      const auto &dep = next->dependency(i);

      if (dependencies.find(dep->name()) == dependencies.end()) {
        dependencies.insert(dep->name());
        queue.push(dep);
      }
    }
  }

  return result;
}

}  // namespace labrat::robot::plugins
