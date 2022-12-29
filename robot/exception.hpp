#pragma once

#include <labrat/robot/logger.hpp>

#include <exception>

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

}  // namespace labrat::robot
