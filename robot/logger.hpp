#pragma once

#include <string>
#include <ostream>
#include <sstream>
#include <memory>
#include <chrono>

namespace labrat::robot {

namespace msg {
  class Log;
}


class LoggerNode;

class Logger {
public:
  class LogStream {
  public:
    enum class Verbosity {
      critical,
      warning,
      info,
      debug,
    };

    ~LogStream();

    template<typename T>
    inline LogStream &operator<<(T& message) {
      line << message;

      return *this;
    }

    LogStream &operator<<(std::ostream& (*func)(std::ostream&));

  private:
    LogStream(const Logger &logger, Verbosity verbosity);

    friend class Logger;

    const Logger &logger;
    const Verbosity verbosity;

    std::stringstream line;
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
  static void send(const msg::Log &message);

  friend class LogStream;

  const std::string name;

  static std::shared_ptr<LoggerNode> node;
};

}  // namespace labrat::robot
