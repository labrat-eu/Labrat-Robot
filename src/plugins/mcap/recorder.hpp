/**
 * @file recorder.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/plugin.hpp>

#include <mutex>
#include <string>
#include <unordered_map>

namespace labrat::robot::plugins {

class McapRecorderPrivate;

/**
 * @brief Class to register a plugin to the manager that will record messages into an MCAP file.
 *
 */
class McapRecorder {
public:
  /**
   * @brief Construct a new Mcap Recorder object.
   *
   * @param filename Path of the output MCAP file.
   */
  McapRecorder(const std::string &filename);

  /**
   * @brief Destroy the Mcap Recorder object.
   *
   */
  ~McapRecorder();

private:
  McapRecorderPrivate *priv;
};

}  // namespace labrat::robot::plugins
