/**
 * @file logger.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */
#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <ostream>
#include <string>
#include <source_location>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {

/**
 * @brief Class to print log messages and send them out as messages.
 *
 */
class Logger {
public:
  /** @cond INTERNAL */
  class Private;
  /** @endcond */

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
    LogStream(LogStream &&rhs) : logger(rhs.logger), verbosity(rhs.verbosity), message(std::forward<std::stringstream>(rhs.message)), location(rhs.location) {}

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
     * @param part Object streamed into the Logger.
     * @return LogStream& Self reference.
     */
    template <typename T>
    inline LogStream &operator<<(const T &part) {
      message << part;

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
    LogStream(const Logger &logger, Verbosity verbosity, const std::source_location &location);

    friend class Logger;

    const Logger &logger;
    const Verbosity verbosity;

    std::stringstream message;
    std::source_location location;
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
  LogStream log(Verbosity verbosity, const std::source_location &location = std::source_location::current());

  inline LogStream logCritical(const std::source_location &location = std::source_location::current()) {
    return log(Verbosity::critical, location); 
  }

  inline LogStream logError(const std::source_location &location = std::source_location::current()) {
    return log(Verbosity::error, location); 
  }

  inline LogStream logWarning(const std::source_location &location = std::source_location::current()) {
    return log(Verbosity::warning, location); 
  }

  inline LogStream logInfo(const std::source_location &location = std::source_location::current()) {
    return log(Verbosity::info, location); 
  }

  inline LogStream logDebug(const std::source_location &location = std::source_location::current()) {
    return log(Verbosity::debug, location); 
  }

  /**
   * @brief Set the log level of the application.
   *
   * @param level The verbosity below no log entries are printed out to the console.
   */
  static void setLogLevel(Verbosity level);

  /**
   * @brief Get the log level of the application.
   *
   */
  static Verbosity getLogLevel();

  /**
   * @brief Enable the generation of messages onto the /log topic from this instance.
   */
  void enableTopic();

  /**
   * @brief Disable the generation of messages onto the /log topic from this instance.
   */
  void disableTopic();

  /**
   * @brief Check whether the generation of messages onto the /log topic from this instance is enabled.
   *
   */
  bool isTopicEnabled() const;

  /**
   * @brief Enable colored console output.
   */
  static void enableColor();

  /**
   * @brief Disable colored console output.
   */
  static void disableColor();

  /**
   * @brief Check whether colored output is enabled.
   *
   */
  static bool isColorEnabled();

  /**
   * @brief Enable file location output.
   */
  static void enableLocation();

  /**
   * @brief Disable file location output.
   */
  static void disableLocation();

  /**
   * @brief Check whether file location output is enabled.
   *
   */
  static bool isLocationEnabled();

  /**
   * @brief Enable time output.
   */
  static void enableTime();

  /**
   * @brief Disable time output.
   */
  static void disableTime();

  /**
   * @brief Check whether time output is enabled.
   *
   */
  static bool isTimeEnabled();

private:
  static void initialize();
  static void deinitialize();

  const std::string name;
  bool send_topic = true;

  friend class LogStream;
  friend class Manager;
};

}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
