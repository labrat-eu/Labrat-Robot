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
class FoxgloveServer {
public:
  /**
   * @brief Construct a new Foxglove Server object.
   *
   * @param filter Topic filter to specifiy which topics should be handled by the plugin.
   */
  explicit FoxgloveServer(const Plugin::Filter &filter = Plugin::Filter());
  ~FoxgloveServer();

private:
  FoxgloveServerPrivate *priv;
};

}  // namespace lbot::plugins
}  // namespace labrat
