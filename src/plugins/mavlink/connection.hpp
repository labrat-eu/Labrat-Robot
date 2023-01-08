/**
 * @file connection.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/utils/types.hpp>

#include <memory>

namespace labrat::robot::plugins {

/**
 * @brief Abstract class to provide general means for a MavlinkNode to connect itself to a MAVLink network.
 *
 */
class MavlinkConnection {
public:
  using Ptr = std::unique_ptr<MavlinkConnection>;

  /**
   * @brief Construct a new Mavlink Connection object.
   *
   */
  MavlinkConnection() = default;

  /**
   * @brief Destroy the Mavlink Connection object.
   *
   */
  virtual ~MavlinkConnection() = default;

  /**
   * @brief Write bytes to the connection.
   *
   * @param buffer Buffer to be read from.
   * @param size Size of the buffer.
   * @return std::size_t Number of bytes written.
   */
  virtual std::size_t write(const u8 *buffer, std::size_t size) = 0;

  /**
   * @brief Read bytes from the connection.
   *
   * @param buffer Buffer to be written to.
   * @param size Size of the buffer.
   * @return std::size_t Number of bytes read.
   */
  virtual std::size_t read(u8 *buffer, std::size_t size) = 0;
};

}  // namespace labrat::robot::plugins
