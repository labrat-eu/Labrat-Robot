/**
 * @file exception.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/logger.hpp>

#include <exception>
#include <system_error>

inline namespace labrat {
namespace lbot {

/**
 * @brief Generic exception class.
 *
 */
class Exception : std::exception {
public:
  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   */
  explicit Exception(const std::string &message);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, Logger &&logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   */
  Exception(const std::string &message, std::error_condition condition);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, std::error_condition condition, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, std::error_condition condition, Logger &&logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   */
  Exception(const std::string &message, int code);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, int code, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(const std::string &message, int code, Logger &&logger);

  /**
   * @brief Get the message of the exception.
   */
  [[nodiscard]] const char *what() const noexcept final;

protected:
  // Message of the exception.
  const std::string message;
};

class SystemException : public Exception {
  using Exception::Exception;
};

class IoException : public Exception {
  using Exception::Exception;
};

class RuntimeException : public Exception {
  using Exception::Exception;
};

class RuntimeRecursionException : public Exception {
  using Exception::Exception;
};

class InvalidArgumentException : public Exception {
  using Exception::Exception;
};

class ManagementException : public Exception {
  using Exception::Exception;
};

class TopicNoDataAvailableException : public Exception {
  using Exception::Exception;
};

class ServiceUnavailableException : public Exception {
  using Exception::Exception;
};

class ServiceTimeoutException : public Exception {
  using Exception::Exception;
};

class SerializationException : public Exception {
  using Exception::Exception;
};

class ConversionException : public Exception {
  using Exception::Exception;
};

class SchemaUnknownException : public Exception {
  using Exception::Exception;
};

class ConfigAccessException : public Exception {
  using Exception::Exception;
};

class ConfigParseException : public Exception {
  using Exception::Exception;
};

}  // namespace lbot
}  // namespace labrat
