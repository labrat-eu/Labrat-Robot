/**
 * @file server.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/robot/plugin.hpp>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace labrat::robot::plugins {

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
   * @param name Name of the server.
   * @param port Local port on which the server should listen for new connections.
   */
  FoxgloveServer(const std::string &name, u16 port = 8765);
  ~FoxgloveServer();

private:
  FoxgloveServerPrivate *priv;
};

}  // namespace labrat::robot::plugins
