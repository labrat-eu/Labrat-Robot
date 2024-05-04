#include <labrat/gz-lbot-bridge/bridge.hpp>
#include <labrat/gz-lbot-bridge/protocol.hpp>

#include <gz/plugin/Register.hh>

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <sys/socket.h>
#include <sys/epoll.h>

GZ_ADD_PLUGIN(gz::sim::systems::LbotBridgePlugin, gz::sim::System, gz::sim::ISystemPostUpdate)
GZ_ADD_PLUGIN_ALIAS(gz::sim::systems::LbotBridgePlugin, "LbotBridgePlugin")

bool operator==(const struct sockaddr_un &lhs, const struct sockaddr_un &rhs) {
  if (lhs.sun_family != lhs.sun_family) {
    return false;
  }

  return strncmp(lhs.sun_path, rhs.sun_path, sizeof(lhs.sun_path)) == 0;
}

namespace gz::sim::systems {

LbotBridgePlugin::LbotBridgePlugin() {
  file_descriptor = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (file_descriptor < 0) {
    perror("Error while creating socket");
    return;
  }

  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, gz_lbot_bridge_socket_path, sizeof(address.sun_path));
  unlink(gz_lbot_bridge_socket_path);

  if (bind(file_descriptor, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Error binding to socket");
    return;
  }

  epoll_handle = epoll_create(1);

  epoll_event event;
  event.events = EPOLLIN;

  if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, file_descriptor, &event)) {
    perror("Failed to create epoll instance");
    return;
  }

  thread = std::jthread([this](std::stop_token token) {
    while (!token.stop_requested()) {
      {
        epoll_event event;
        const ssize_t result = epoll_wait(epoll_handle, &event, 1, timeout);

        if (result <= 0) {
          if ((result == -1) && (errno != EINTR)) {
            perror("Failure during epoll wait");
            return;
          }

          continue;
        }
      }

      struct sockaddr_un peer_address;
      socklen_t address_length = sizeof(peer_address);
      const ssize_t result = recvfrom(file_descriptor, reinterpret_cast<void *>(buffer.data()), buffer_size, 0, reinterpret_cast<sockaddr *>(&peer_address), &address_length);

      if (result < 0) {
        perror("Failed to read from socket");
        return;
      }

      if (std::find(peer_addresses.begin(), peer_addresses.end(), peer_address) == peer_addresses.end()) {
        peer_addresses.emplace_back(peer_address);
        printf("[lbot] Client connected (%s)\n", peer_address.sun_path);
      }
    }
  });
}

LbotBridgePlugin::~LbotBridgePlugin() {
  thread.request_stop();
  thread.join();
  close(file_descriptor);
}

void LbotBridgePlugin::PostUpdate(const gz::sim::UpdateInfo &info, const gz::sim::EntityComponentManager &) {
  GzLbotBrideProtocolMessage message;
  message.real_time.seconds = std::chrono::duration_cast<std::chrono::seconds>(info.realTime).count();
  message.real_time.nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(info.realTime).count() % std::nano::den;
  message.simulation_time.seconds = std::chrono::duration_cast<std::chrono::seconds>(info.simTime).count();
  message.simulation_time.nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(info.simTime).count() % std::nano::den;
  message.iterations = info.iterations;
  message.paused = info.paused;

  for (std::list<struct sockaddr_un>::const_iterator iter = peer_addresses.cbegin(); iter != peer_addresses.cend();) {
    const struct sockaddr_un &peer = *iter;
    const int result = sendto(file_descriptor, &message, sizeof(message), 0, (struct sockaddr *)&peer, sizeof(struct sockaddr_un));

    if (result < 0) {
      std::list<struct sockaddr_un>::const_iterator last_iter = iter;
      ++iter;

      if (errno != ECONNREFUSED) {
        perror("Error while sending message");
      }

      printf("[lbot] Client disconnected (%s)\n", peer.sun_path);
      peer_addresses.erase(last_iter);

      continue;
    }

    ++iter;
  }
}

}
