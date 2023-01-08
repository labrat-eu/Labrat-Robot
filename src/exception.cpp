/**
 * @file exception.cpp
 * @author Max Yvon Zimmermann
 * 
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 * 
 */

#include <labrat/robot/exception.hpp>

#include <iostream>

namespace labrat::robot {

Exception::Exception(std::string message) : Exception(message, Logger("generic")) {}

Exception::Exception(std::string message, Logger &logger) : message(message) {
  logger.debug() << "Exception thrown: " << message;
}

Exception::Exception(std::string message, Logger &&logger) : Exception(message, std::forward<Logger &>(logger)) {}

Exception::Exception(std::string message, std::error_condition condition) :
  Exception(message, condition, Logger(condition.category().name())) {}

Exception::Exception(std::string message, std::error_condition condition, Logger &logger) :
  Exception(message + " (" + std::to_string(condition.value()) + ": " + condition.message() + ")", logger) {}

Exception::Exception(std::string message, std::error_condition condition, Logger &&logger) :
  Exception(message, condition, std::forward<Logger &>(logger)) {}

Exception::Exception(std::string message, int code) : Exception(message, std::generic_category().default_error_condition(code)) {}

Exception::Exception(std::string message, int code, Logger &logger) :
  Exception(message, std::generic_category().default_error_condition(code), logger) {}

Exception::Exception(std::string message, int code, Logger &&logger) : Exception(message, code, std::forward<Logger &>(logger)) {}

const char *Exception::what() const noexcept {
  return message.c_str();
}

}  // namespace labrat::robot
