#pragma once

#include <cstdint>
#include <array>
#include <list>
#include <thread>

#include <sys/un.h>

#include <gz/sim/System.hh>
#include <sdf/sdf.hh>

namespace gz::sim::systems {

class GZ_SIM_VISIBLE LbotBridgePlugin: public gz::sim::System, public gz::sim::ISystemPostUpdate {
public:
  LbotBridgePlugin();
  ~LbotBridgePlugin();

  void PostUpdate(const gz::sim::UpdateInfo &info, const gz::sim::EntityComponentManager &ecm) final;

private:
  static constexpr ssize_t buffer_size = 64;
  static constexpr ssize_t timeout = 1000;

  ssize_t file_descriptor;
  ssize_t epoll_handle;
  std::array<uint8_t, buffer_size> buffer;
  struct sockaddr_un address;
  std::list<struct sockaddr_un> peer_addresses;
  std::jthread thread;
};

}
