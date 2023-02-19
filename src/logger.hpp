/**
 * @file logger.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/robot/base.hpp>
#include <labrat/robot/message.hpp>
#include <labrat/robot/msg/log_generated.h>

#include <chrono>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

namespace labrat::robot {

class LoggerNode;

/**
 * @brief Class to print log messages and send them out as messages.
 *
 */
class Logger {
public:
  /**
   * @brief Verbosity level of the application.
   *
   */
  enum class Verbosity {
    critical,
    warning,
    info,
    debug,
  };

  /**
   * @brief Temporary object to be used for stream operations on the Logger.
   *
   */
  class LogStream {
  public:
    /**
     * @brief Destroy the Log Stream object.
     * This writes the contents streamed into this instance to the console and sends out a log message.
     *
     */
    ~LogStream();

    /**
     * @brief Generic stream operator overload.
     *
     * @tparam T Type of the object streamed into the Logger.
     * @param message Object streamed into the Logger.
     * @return LogStream& Self reference.
     */
    template <typename T>
    inline LogStream &operator<<(const T &message) {
      line << message;

      return *this;
    }

    /**
     * @brief Modification function stream operator overload.
     *
     * @param func Function to operate on the Logger.
     * @return LogStream& Self reference.
     */
    LogStream &operator<<(std::ostream &(*func)(std::ostream &));

  private:
    /**
     * @brief Construct a new Log Stream object.
     *
     * @param logger Associated logger instance.
     * @param verbosity Verbosity of this instance.
     */
    LogStream(const Logger &logger, Verbosity verbosity);

    friend class Logger;

    const Logger &logger;
    const Verbosity verbosity;

    std::stringstream line;
  };

  /**
   * @brief Log entry to be sent out as a msg::Log message.
   *
   */
  class Entry {
  public:
    Verbosity verbosity;
    std::chrono::seconds timestamp;
    std::string logger_name;
    std::string message;

    /**
     * @brief Forward conversion function to be used by the logger node.
     *
     * @param source Reference to the Entry object to be converted.
     * @param destination Reference to the msg::Log message to convert to.
     */
    static inline void toMessage(const Entry &source, Message<msg::Log> &destination) {
      switch (source.verbosity) {
        case (Verbosity::critical): {
          destination().verbosity = msg::Verbosity::Verbosity_critical;
          break;
        }

        case (Verbosity::warning): {
          destination().verbosity = msg::Verbosity::Verbosity_warning;
          break;
        }

        case (Verbosity::info): {
          destination().verbosity = msg::Verbosity::Verbosity_info;
          break;
        }

        case (Verbosity::debug): {
          destination().verbosity = msg::Verbosity::Verbosity_debug;
          break;
        }
      }

      destination().timestamp = source.timestamp.count();
      destination().logger_name = source.logger_name;
      destination().message = source.message;
    }
  };

  /**
   * @brief Construct a new Logger object.
   *
   * @param name Name of the logger.
   */
  Logger(const std::string &name);

  /**
   * @brief Destroy the Logger object.
   *
   */
  ~Logger() = default;

  /**
   * @brief Alias of info().
   *
   * @return LogStream The temporary LogStream object.
   */
  inline LogStream operator()() {
    return info();
  }

  /**
   * @brief Write to the logger with the critical verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream critical();

  /**
   * @brief Write to the logger with the warning verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream warning();

  /**
   * @brief Write to the logger with the info verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream info();

  /**
   * @brief Write to the logger with the debug verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream debug();

  /**
   * @brief Set the log level of the application.
   *
   * @param level The verbosity below no log entries are printed out to the console.
   */
  static inline void setLogLevel(Verbosity level) {
    log_level = level;
  }

  /**
   * @brief Get the log level of the application.
   *
   */
  static inline Verbosity getLogLevel() {
    return log_level;
  }

  /**
   * @brief Enable colored console output.
   */
  static inline void enableColor() {
    use_color = true;
  }

  /**
   * @brief Disable colored console output.
   */
  static inline void disableColor() {
    use_color = false;
  }

  /**
   * @brief Check whether colored output is enabled.
   *
   */
  static inline bool isColor() {
    return use_color;
  }

private:
  /**
   * @brief Send out a message entry over the logger node.
   *
   * @param entry Log entry to be sent out.
   */
  static void send(const Entry &entry);

  friend class LogStream;

  const std::string name;

  static std::shared_ptr<LoggerNode> node;

  static Verbosity log_level;
  static bool use_color;
};

}  // namespace labrat::robot
