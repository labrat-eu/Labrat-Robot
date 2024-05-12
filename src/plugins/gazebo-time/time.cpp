/**
 * @file time.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/msg/timestamp.hpp>
#include <labrat/lbot/plugins/gazebo-time/msg/gazebo_time.hpp>
#include <labrat/lbot/plugins/gazebo-time/time.hpp>
#include <labrat/lbot/plugins/gazebo-time/protocol.hpp>

#include <array>
#include <thread>
#include <string>
#include <mutex>
#include <memory>
#include <atomic>

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>

inline namespace labrat {
namespace lbot::plugins {

class GazeboTimeConv : public MessageBase<GazeboTime, GzLbotBrideProtocolMessage> {
public:
  static void convertFrom(const GzLbotBrideProtocolMessage &source, Storage &destination) {
    destination.real_time = std::make_unique<TimeNative>();
    destination.real_time->seconds = source.real_time.seconds;
    destination.real_time->nanoseconds = source.real_time.nanoseconds;

    destination.simulation_time = std::make_unique<TimeNative>();
    destination.simulation_time->seconds = source.simulation_time.seconds;
    destination.simulation_time->nanoseconds = source.simulation_time.nanoseconds;
    
    destination.iterations = source.iterations;
    destination.paused = source.paused;
  }
};

class TimestampConv : public MessageBase<Timestamp, GzLbotBrideProtocolMessage> {
public:
  static void convertFrom(const GzLbotBrideProtocolMessage &source, Storage &destination) {
    destination.value = std::make_unique<foxglove::Time>(source.simulation_time.seconds, source.simulation_time.nanoseconds);
  }
};

class GazeboTimeSourceNode : public UniqueNode {
public:
  GazeboTimeSourceNode();
  ~GazeboTimeSourceNode();

private:
  static constexpr ssize_t timeout = 1000;

  Sender<TimestampConv>::Ptr sender_time;
  Sender<GazeboTimeConv>::Ptr sender_debug;

  ssize_t file_descriptor;
  ssize_t epoll_handle;
  GzLbotBrideProtocolMessage buffer;
  struct sockaddr_un address;
  struct sockaddr_un client_address;
  std::string client_path;
  std::shared_ptr<std::timed_mutex> mutex;
  std::atomic_flag start_flag;
  std::jthread read_thread;
  std::jthread connection_thread;
};

GazeboTimeSource::GazeboTimeSource() {
  addNode<GazeboTimeSourceNode>("gazebo-time");
}

GazeboTimeSource::~GazeboTimeSource() = default;

GazeboTimeSourceNode::GazeboTimeSourceNode() {
  sender_time = addSender<TimestampConv>("/time");
  sender_debug = addSender<GazeboTimeConv>("/gazebo/time");

  mutex = std::make_shared<std::timed_mutex>();
  mutex->lock();

  file_descriptor = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (file_descriptor < 0) {
    throw IoException("Error while creating socket", errno);
  }

  const pid_t pid = getpid();
  client_path = std::string(gz_lbot_bridge_socket_path) + "_" + std::to_string(pid);

  memset(&client_address, 0, sizeof(client_address));
  client_address.sun_family = AF_UNIX;
  strncpy(client_address.sun_path, client_path.c_str(), sizeof(client_address.sun_path));
  unlink(client_path.c_str());

  if (bind(file_descriptor, (struct sockaddr *)&client_address, sizeof(client_address)) < 0) {
    throw IoException("Error binding to socket", errno);
  }

  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, gz_lbot_bridge_socket_path, sizeof(address.sun_path));

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event)) {
    throw IoException("Failed to create epoll instance", errno);
  }

  read_thread = std::jthread([this](std::stop_token token) {
    while (true) {
      while (!start_flag.test()) {
        start_flag.wait(false);
      }

      if (token.stop_requested()) {
        return;
      }

      {
        epoll_event event;
        const ssize_t result = epoll_wait(epoll_handle, &event, 1, timeout);

        if (result <= 0) {
          if ((result == -1) && (errno != EINTR)) {
            throw IoException("Failure during epoll wait", errno);
          }

          continue;
        }
      }

      const ssize_t result = recv(file_descriptor, reinterpret_cast<void *>(&buffer), sizeof(GazeboTimeSourceNode), 0);

      if (result < 0) {
        throw IoException("Failed to read from socket", errno);
      }

      sender_time->put(buffer);
      sender_debug->put(buffer);
    }
  });

  connection_thread = std::jthread([this](std::stop_token token, std::shared_ptr<std::timed_mutex> mutex) {
    while (!token.stop_requested()) {
      const std::chrono::time_point<std::chrono::steady_clock> time_begin = std::chrono::steady_clock::now();

      if (connect(file_descriptor, (struct sockaddr *)&address, sizeof(address)) < 0) {
        if (errno != ECONNREFUSED) {
          throw IoException("Error binding to socket", errno);
        }
      } else {
        if (send(file_descriptor, nullptr, 0, 0) == -1) {
          throw IoException("Failed to send to socket", errno);
        }

        if (!start_flag.test_and_set()) {
          getLogger().logInfo() << "Connected to gazebo";
          start_flag.test_and_set();
          start_flag.notify_all();
        }
      }

      if (mutex->try_lock_until(time_begin + std::chrono::seconds(1))) {
        break;
      }
    }

    start_flag.test_and_set();
    start_flag.notify_all();
  }, mutex);
}

GazeboTimeSourceNode::~GazeboTimeSourceNode() {
  read_thread.request_stop();
  connection_thread.request_stop();

  mutex->unlock();
  connection_thread.join();

  read_thread.request_stop();
  read_thread.join();

  close(file_descriptor);
};

}  // namespace lbot::plugins
}  // namespace labrat
