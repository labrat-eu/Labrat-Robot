#pragma once

#include <labrat/robot/message.hpp>
#include <labrat/robot/msg/log.pb.h>

#include <chrono>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

namespace labrat::robot {

class LoggerNode;

class Logger {
public:
  enum class Verbosity {
    critical,
    warning,
    info,
    debug,
  };

  class LogStream {
  public:
    ~LogStream();

    template <typename T>
    inline LogStream &operator<<(T &message) {
      line << message;

      return *this;
    }

    LogStream &operator<<(std::ostream &(*func)(std::ostream &));

  private:
    LogStream(const Logger &logger, Verbosity verbosity);

    friend class Logger;

    const Logger &logger;
    const Verbosity verbosity;

    std::stringstream line;
  };

  class Entry {
  public:
    Verbosity verbosity;
    std::chrono::seconds timestamp;
    std::string logger_name;
    std::string message;

    static inline void toMessage(const Entry &source, Message<Log> &destination) {
      switch (source.verbosity) {
        case (Verbosity::critical): {
          destination().set_verbosity(Log_Verbosity::Log_Verbosity_critical);
          break;
        }

        case (Verbosity::warning): {
          destination().set_verbosity(Log_Verbosity::Log_Verbosity_warning);
          break;
        }

        case (Verbosity::info): {
          destination().set_verbosity(Log_Verbosity::Log_Verbosity_info);
          break;
        }

        case (Verbosity::debug): {
          destination().set_verbosity(Log_Verbosity::Log_Verbosity_debug);
          break;
        }
      }

      destination().set_timestamp(source.timestamp.count());
      destination().set_logger_name(source.logger_name);
      destination().set_message(source.message);
    }
  };

  Logger(const std::string &name);
  ~Logger() = default;

  inline LogStream operator()() {
    return info();
  }

  LogStream critical();
  LogStream warning();
  LogStream info();
  LogStream debug();

private:
  static void send(const Entry &message);

  friend class LogStream;

  const std::string name;

  static std::shared_ptr<LoggerNode> node;
};

}  // namespace labrat::robot
