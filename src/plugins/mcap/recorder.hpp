/**
 * @file recorder.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/plugin.hpp>

#include <string>

inline namespace labrat {
namespace lbot::plugins {

class McapRecorderPrivate;

/**
 * @brief Class to register a plugin to the manager that will record messages into an MCAP file.
 *
 */
class McapRecorder : public UniquePlugin {
public:
  /**
   * @brief Construct a new Mcap Recorder object.
   *
   */
  explicit McapRecorder();

  /**
   * @brief Destroy the Mcap Recorder object.
   *
   */
  ~McapRecorder();

  void topicCallback(const TopicInfo &info);
  void messageCallback(const MessageInfo &info);

private:
  McapRecorderPrivate *priv;
};

}  // namespace lbot::plugins
}  // namespace labrat
