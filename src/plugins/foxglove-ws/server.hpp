/**
 * @file server.hpp
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

class FoxgloveServerPrivate;

/**
 * @brief Class to register a plugin to the manager that will open a websocket server in order to forward massages to Foxglove Studio.
 *
 */
class FoxgloveServer : public UniquePlugin {
public:
  /**
   * @brief Construct a new Foxglove Server object.
   *
   */
  explicit FoxgloveServer(const PluginEnvironment &environment);

  /**
   * @brief Destroy the Foxglove Server object.
   *
   */
  ~FoxgloveServer();

  void topicCallback(const TopicInfo &info);
  void messageCallback(const MessageInfo &info);

private:
  FoxgloveServerPrivate *priv;
};

}  // namespace lbot::plugins
}  // namespace labrat
