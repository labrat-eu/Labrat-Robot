/**
 * @file info.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/service.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <string>

#include <flatbuffers/stl_emulation.h>

inline namespace labrat {
namespace lbot {

/**
 * @brief Information on a topic provided on callbacks.
 *
 */
struct TopicInfo {
  const std::size_t type_hash;
  const std::string type_name;
  const std::size_t topic_hash;
  const std::string topic_name;

  /**
   * @brief Get information about a topic by name.
   * This will not lookup any data.
   *
   * @tparam MessageType Type of the message sent over the relevant topic.
   * @param topic_name Name of the topic.
   * @return TopicInfo Information about the topic.
   */
  template <typename MessageType>
  requires is_message<MessageType> static TopicInfo get(const std::string &topic_name) {
    const TopicInfo result = {
      .type_hash = typeid(typename MessageType::Content).hash_code(),
      .type_name = MessageType::getName(),
      .topic_hash = std::hash<std::string>()(topic_name),
      .topic_name = topic_name,
    };

    return result;
  }
};

/**
 * @brief Information on a service provided on callbacks.
 *
 */
struct ServiceInfo {
  const std::size_t request_type_hash;
  const std::string request_type_name;
  const std::size_t response_type_hash;
  const std::string response_type_name;
  const std::size_t service_hash;
  const std::string service_name;
  ServiceMap::Service &service;

  /**
   * @brief Get information about a service by name.
   * This will not lookup any data.
   *
   * @tparam RequestType Type of the request message of the service.
   * @tparam ResponseType Type of the request message of the service.
   * @param service_name Name of the service.
   * @return ServiceInfo Information about the service.
   */
  template <typename RequestType, typename ResponseType>
  requires is_message<RequestType> && is_message<ResponseType>
  static ServiceInfo get(const std::string &service_name, ServiceMap::Service &service) {
    const ServiceInfo result = {.request_type_hash = typeid(typename RequestType::Content).hash_code(),
      .request_type_name = RequestType::getName(),
      .response_type_hash = typeid(typename ResponseType::Content).hash_code(),
      .response_type_name = ResponseType::getName(),
      .service_hash = std::hash<std::string>()(service_name),
      .service_name = service_name,
      .service = service};

    return result;
  }
};

/**
 * @brief Information on a message provided on callbacks.
 *
 */
struct MessageInfo {
  const TopicInfo &topic_info;
  Clock::time_point timestamp;
  flatbuffers::span<u8> serialized_message;
};

}  // namespace lbot
}  // namespace labrat
