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
#include <foxglove/Log_generated.h>

#include <chrono>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <filesystem>

namespace labrat::robot {

class LoggerNode;

struct LoggerLocation {
  std::string file;
  u32 line;

  LoggerLocation(std::filesystem::path &&path, u32 line) : line(line) {
    file = path.filename();
  }
};

#define LOGINIT labrat::robot::LoggerLocation(std::filesystem::path(__FILE__), __LINE__)

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
    error,
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
    LogStream(const Logger &logger, Verbosity verbosity, LoggerLocation &&location);

    friend class Logger;

    const Logger &logger;
    const Verbosity verbosity;

    std::stringstream line;
    LoggerLocation location;
  };

  /**
   * @brief Log entry to be sent out as a foxglove::Log message.
   *
   */
  class Entry {
  public:
    Verbosity verbosity;
    std::chrono::nanoseconds timestamp;
    std::string logger_name;
    std::string message;
    std::string file;
    u32 line;

    /**
     * @brief Forward conversion function to be used by the logger node.
     *
     * @param source Reference to the Entry object to be converted.
     * @param destination Reference to the foxglove::Log message to convert to.
     */
    static inline void toMessage(const Entry &source, Message<foxglove::Log> &destination) {
      switch (source.verbosity) {
        case (Verbosity::critical): {
          destination().level = foxglove::LogLevel::LogLevel_FATAL;
          break;
        }

        case (Verbosity::error): {
          destination().level = foxglove::LogLevel::LogLevel_ERROR;
          break;
        }

        case (Verbosity::warning): {
          destination().level = foxglove::LogLevel::LogLevel_WARNING;
          break;
        }

        case (Verbosity::info): {
          destination().level = foxglove::LogLevel::LogLevel_INFO;
          break;
        }

        case (Verbosity::debug): {
          destination().level = foxglove::LogLevel::LogLevel_DEBUG;
          break;
        }
      }
      
      destination().timestamp = std::make_unique<foxglove::Time>(std::chrono::duration_cast<std::chrono::seconds>(source.timestamp).count(), (source.timestamp % std::chrono::seconds(1)).count());
      destination().name = source.logger_name;
      destination().message = source.message;
      destination().file = source.file;
      destination().line = source.line;
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
  inline LogStream operator()(LoggerLocation &&location) {
    return info(std::move(location));
  }

  /**
   * @brief Write to the logger with the critical verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream critical(LoggerLocation &&location);

  /**
   * @brief Write to the logger with the error verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream error(LoggerLocation &&location);

  /**
   * @brief Write to the logger with the warning verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream warning(LoggerLocation &&location);

  /**
   * @brief Write to the logger with the info verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream info(LoggerLocation &&location);

  /**
   * @brief Write to the logger with the debug verbosity.
   *
   * @return LogStream The temporary LogStream object.
   */
  LogStream debug(LoggerLocation &&location);

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
  static inline bool isColorEnabled() {
    return use_color;
  }

  /**
   * @brief Enable file location output.
   */
  static inline void enableLocation() {
    print_location = true;
  }

  /**
   * @brief Disable file location output.
   */
  static inline void disableLocation() {
    print_location = false;
  }

  /**
   * @brief Check whether file location output is enabled.
   *
   */
  static inline bool isLocationEnabled() {
    return print_location;
  }

  /**
   * @brief Enable time output.
   */
  static inline void enableTime() {
    print_time = true;
  }

  /**
   * @brief Disable time output.
   */
  static inline void disableTime() {
    print_time = false;
  }

  /**
   * @brief Check whether time output is enabled.
   *
   */
  static inline bool isTimeEnabled() {
    return print_time;
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
  static bool print_location;
  static bool print_time;
};

}  // namespace labrat::robot
