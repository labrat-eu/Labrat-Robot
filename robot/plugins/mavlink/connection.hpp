#pragma once

#include <labrat/robot/utils/types.hpp>

#include <memory>

namespace labrat::robot::plugins {

class MavlinkConnection {
public:
  using Ptr = std::unique_ptr<MavlinkConnection>;

  MavlinkConnection() = default;
  virtual ~MavlinkConnection() = default;

  virtual std::size_t write(const u8 *buffer, std::size_t size) = 0;
  virtual std::size_t read(u8 *buffer, std::size_t size) = 0;
};

}  // namespace labrat::robot::plugins
