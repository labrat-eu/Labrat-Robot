/**
 * @file logger.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */
#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/message.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include <foxglove/Log.fb.hpp>

inline namespace labrat {
namespace lbot {

class LoggerNode;

struct LoggerLocation {
  std::string file;
  u32 line;

  LoggerLocation(std::filesystem::path &&path, u32 line) : line(line) {
    file = path.filename();
  }
};

#define LBOT_LOGINIT labrat::lbot::LoggerLocation(std::filesystem::path(__FILE__), __LINE__)

#define logCritical() write(labrat::lbot::Logger::Verbosity::critical, LBOT_LOGINIT)
#define logError() write(labrat::lbot::Logger::Verbosity::error, LBOT_LOGINIT)
#define logWarning() write(labrat::lbot::Logger::Verbosity::warning, LBOT_LOGINIT)
#define logInfo() write(labrat::lbot::Logger::Verbosity::info, LBOT_LOGINIT)
#define logDebug() write(labrat::lbot::Logger::Verbosity::debug, LBOT_LOGINIT)

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
  enum class Verbosity : u8 {
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
  };

  /**
   * @brief Construct a new Logger object.
   *
   * @param name Name of the logger.
   */
  explicit Logger(std::string name);

  /**
   * @brief Destroy the Logger object.
   *
   */
  ~Logger() = default;

  /**
   * @brief Write to the logger with the specified verbosity.
   *
   * @param verbosity Verbosity of the log entry.
   * @param location Internal struct to specify the code location.
   * @return LogStream The temporary LogStream object.
   */
  LogStream write(Verbosity verbosity, LoggerLocation &&location);

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
  static void initialize();
  static void deinitialize();

  /**
   * @brief Send out a message entry over the logger node.
   *
   * @param entry Log entry to be sent out.
   */
  static void send(const Entry &entry);

  /**
   * @brief Trace a message entry over the logger node.
   *
   * @param entry Log entry to be sent out.
   */
  static void trace(const Entry &entry);

  friend class LogStream;

  const std::string name;

  static std::shared_ptr<LoggerNode> node;

  static Verbosity log_level;
  static bool use_color;
  static bool print_location;
  static bool print_time;

  friend class Manager;
};

}  // namespace lbot
}  // namespace labrat
