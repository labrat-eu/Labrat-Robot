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

  public:
    using Handle = std::size_t;

    Service(Handle handle, const std::string &name);

    void *getServer() const;
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
