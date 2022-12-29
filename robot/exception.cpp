#include <labrat/robot/exception.hpp>

#include <iostream>

#include <string.h>

namespace labrat::robot {

Exception::Exception(std::string message) : Exception(message, Logger("generic")) {}

Exception::Exception(std::string message, Logger &logger) : message(message) {
  logger.debug() << "Exception thrown: " << message;
}

Exception::Exception(std::string message, Logger &&logger) : Exception(message, std::forward<Logger &>(logger)) {}

Exception::Exception(std::string message, int code) : Exception(message, code, Logger("generic")) {}

Exception::Exception(std::string message, int code, Logger &logger) : Exception(message + " (" + strerror(code) + ")", logger) {}

Exception::Exception(std::string message, int code, Logger &&logger) :
  Exception(message + " (" + strerror(code) + ")", std::forward<Logger &>(logger)) {}

const char *Exception::what() const noexcept {
  return message.c_str();
}

}  // namespace labrat::robot
