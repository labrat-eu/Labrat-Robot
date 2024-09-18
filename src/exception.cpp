/**
 * @file exception.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/exception.hpp>

inline namespace labrat {
namespace lbot {

Exception::Exception(const std::string &message) :
  Exception(message, Logger("generic"))
{}

Exception::Exception(const std::string &message, Logger &logger) :
  message(message)
{
  logger.logDebug() << "Exception thrown: " << message;
}

Exception::Exception(const std::string &message, Logger &&logger) :
  Exception(message, std::forward<Logger &>(logger))
{}

Exception::Exception(const std::string &message, std::error_condition condition) :
  Exception(message, condition, Logger(condition.category().name()))
{}

Exception::Exception(const std::string &message, std::error_condition condition, Logger &logger) :
  Exception(message + " (" + std::to_string(condition.value()) + ": " + condition.message() + ")", logger)
{}

Exception::Exception(const std::string &message, std::error_condition condition, Logger &&logger) :
  Exception(message, condition, std::forward<Logger &>(logger))
{}

Exception::Exception(const std::string &message, int code) :
  Exception(message, std::generic_category().default_error_condition(code))
{}

Exception::Exception(const std::string &message, int code, Logger &logger) :
  Exception(message, std::generic_category().default_error_condition(code), logger)
{}

Exception::Exception(const std::string &message, int code, Logger &&logger) :
  Exception(message, code, std::forward<Logger &>(logger))
{}

const char *Exception::what() const noexcept
{
  return message.c_str();
}

}  // namespace lbot
}  // namespace labrat
