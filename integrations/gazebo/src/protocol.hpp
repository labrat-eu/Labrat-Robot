#pragma once

#include <cstdint>

static const char gz_lbot_bridge_socket_path[] = "/var/tmp/gz_lbot";

struct GzLbotBrideProtocolMessage {
  struct Time {
    int64_t seconds;
    uint64_t nanoseconds;
  };

  Time real_time;
  Time simulation_time;

  uint64_t iterations;
  bool paused;
};
