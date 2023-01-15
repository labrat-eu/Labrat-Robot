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
   * @param filter Topic filter to specifiy which topics should be handled by the plugin.
   */
  McapRecorder(const std::string &filename, const Plugin::Filter &filter = Plugin::Filter());

  /**
   * @brief Destroy the Mcap Recorder object.
   *
   */
  ~McapRecorder();

private:
  McapRecorderPrivate *priv;
};

}  // namespace labrat::robot::plugins
