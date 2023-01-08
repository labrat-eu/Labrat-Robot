/**
 * @file service.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/message.hpp>
#include <labrat/robot/utils/atomic.hpp>
#include <labrat/robot/utils/types.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

namespace labrat::robot {

class ServiceMap {
public:
  class Service {
  private:
    void *server;

    std::atomic<std::size_t> use_count;

  public:
    using Handle = std::size_t;

    class ServerReference {
    public:
      inline ServerReference(Service &service) : service(service) {
        service.use_count.fetch_add(1);
      }

      inline ServerReference(ServerReference &&rhs) : service(rhs.service) {
        service.use_count.fetch_add(1);
      }

      inline ~ServerReference() {
        service.use_count.fetch_sub(1);
        service.use_count.notify_all();
      }

      template <typename T>
      inline operator T *() {
        return reinterpret_cast<T *>(service.server);
      }

    private:
      Service &service;
    };

    Service(Handle handle, const std::string &name);

    ServerReference getServer() {
      return ServerReference(*this);
    }

    void addServer(void *new_server);
    void removeServer(void *old_server);

    const Handle handle;
    const std::string name;
  };

  ServiceMap();
  ~ServiceMap() = default;

  template <typename T, typename U>
  Service &addServer(const std::string &service_name, void *server) {
    Service &service = getServiceInternal(service_name, typeid(T).hash_code() ^ typeid(U).hash_code());

    service.addServer(server);

    return service;
  }

  Service &removeServer(const std::string &service_name, void *server) {
    Service &service = getServiceInternal(service_name);

    service.removeServer(server);

    return service;
  }

  template <typename T, typename U>
  Service &getService(const std::string &service_name) {
    return getServiceInternal(service_name, typeid(T).hash_code() ^ typeid(U).hash_code());
  }

private:
  Service &getServiceInternal(const std::string &service);
  Service &getServiceInternal(const std::string &service, std::size_t handle);

  std::unordered_map<std::string, Service> map;
};

}  // namespace labrat::robot
