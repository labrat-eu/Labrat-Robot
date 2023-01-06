#pragma once

#include <labrat/robot/logger.hpp>

#include <exception>
#include <system_error>

namespace labrat::robot {

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
  Exception(std::string message);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, Logger &&logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   */
  Exception(std::string message, std::error_condition condition);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, std::error_condition condition, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] condition Error condition of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, std::error_condition condition, Logger &&logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   */
  Exception(std::string message, int code);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, int code, Logger &logger);

  /**
   * @brief Construct a new Exception object
   *
   * @param[in] message Message of the exception.
   * @param[in] code Error code (errno) of the exception.
   * @param[in] logger Logger to print the error to.
   */
  Exception(std::string message, int code, Logger &&logger);

  /**
   * @brief Get the message of the exception.
   */
  const char *what() const noexcept;

protected:
  // Message of the exception.
  const std::string message;
};

class IoException : public Exception {
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

}  // namespace labrat::robot
