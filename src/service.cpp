/**
 * @file service.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/service.hpp>

inline namespace labrat {
namespace lbot {

ServiceMap::ServiceMap() = default;

ServiceMap::Service::Service(Handle handle, std::string name) : handle(handle), name(std::move(name)) {
  server = nullptr;
}

ServiceMap::Service &ServiceMap::getServiceInternal(const std::string &service) {
  if (service.empty()) {
    throw ManagementException("Service name name must be non-empty.");
  }

  const std::unordered_map<std::string, Service>::iterator iterator = map.find(service);

  if (iterator == map.end()) {
    throw ManagementException("Service '" + service + "' not found.");
  }

  return iterator->second;
}

ServiceMap::Service &ServiceMap::getServiceInternal(const std::string &service, Service::Handle handle) {
  if (service.empty()) {
    throw ManagementException("Service name name must be non-empty.");
  }

  ServiceMap::Service &result =
    map.emplace(std::piecewise_construct, std::forward_as_tuple(service), std::forward_as_tuple(handle, service)).first->second;

  if (handle != result.handle) {
    throw ManagementException("Service '" + service + "' does not match the provided handle.");
  }

  return result;
}

void ServiceMap::Service::addServer(void *new_server) {
  waitUntil<std::size_t>(use_count, 0);

  if (server != nullptr) {
    throw ManagementException("A server has already been registered for this service.");
  }

  server = new_server;
}

void ServiceMap::Service::removeServer(void *old_server) {
  waitUntil<std::size_t>(use_count, 0);

  if (server != old_server) {
    throw ManagementException("The server to be removed does not match the existing server.");
  }

  server = nullptr;
}

}  // namespace lbot
}  // namespace labrat
