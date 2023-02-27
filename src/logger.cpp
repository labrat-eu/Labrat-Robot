/**
 * @file logger.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/msg/log_generated.h>
#include <labrat/robot/node.hpp>

#include <iostream>
#include <mutex>
#include <ctime>
#include <iomanip>

namespace labrat::robot {

Logger::Verbosity Logger::log_level = Verbosity::info;
bool Logger::use_color = true;
bool Logger::print_location = false;
bool Logger::print_time = true;

static std::mutex io_mutex;

class LoggerNode : public Node {
private:
  std::unique_ptr<Sender<Message<foxglove::Log>, Logger::Entry>> sender;

public:
  LoggerNode(const Node::Environment environment) : Node(environment) {
    sender = addSender<Message<foxglove::Log>, Logger::Entry>("/log");
  }

  void send(const Logger::Entry &entry) {
    sender->put(entry);
  }
};

class Color {
public:
  enum class Code : i16 {
    black = 30,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    white = 37,
    normal = 39,
  };

  Color(bool enable_color, Code code = Code::normal) : code(code), enable_color(enable_color) {}

  friend std::ostream &

  operator<<(std::ostream &stream, const Color &color) {
    if (color.enable_color) {
      return stream << "\033[" << static_cast<i16>(color.code) << "m";
    } else {
      return stream;
    }
  }

private:
  const Code code;
  const bool enable_color;
};

const std::string getVerbosityLong(Logger::Verbosity verbosity);
const std::string getVerbosityShort(Logger::Verbosity verbosity);
const Color getVerbosityColor(Logger::Verbosity verbosity);

std::shared_ptr<LoggerNode> Logger::node(Manager::get().addNode<LoggerNode>("logger"));

Logger::Logger(const std::string &name) : name(std::move(name)) {}

Logger::LogStream Logger::critical(LoggerLocation &&location) {
  return LogStream(*this, Verbosity::critical, std::move(location));
}

Logger::LogStream Logger::error(LoggerLocation &&location) {
  return LogStream(*this, Verbosity::error, std::move(location));
}

Logger::LogStream Logger::warning(LoggerLocation &&location) {
  return LogStream(*this, Verbosity::warning, std::move(location));
}

Logger::LogStream Logger::info(LoggerLocation &&location) {
  return LogStream(*this, Verbosity::info, std::move(location));
}

Logger::LogStream Logger::debug(LoggerLocation &&location) {
  return LogStream(*this, Verbosity::debug, std::move(location));
}

void Logger::send(const Entry &message) {
  node->send(message);
}

Logger::LogStream::LogStream(const Logger &logger, Verbosity verbosity, LoggerLocation &&location) : logger(logger), verbosity(verbosity), location(location) {}

Logger::LogStream::~LogStream() {
  if (verbosity <= Logger::log_level) {
    const std::time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::lock_guard guard(io_mutex);

    std::cout << getVerbosityColor(verbosity) << "[" << getVerbosityShort(verbosity) << "]" << Color(isColorEnabled()) << " (" << logger.name;
    
    if (isLocationEnabled() || isTimeEnabled()) {
      std::cout << " @";
    }

    if (isTimeEnabled()) {
      std::cout << " " << std::put_time(std::localtime(&current_time), "%T");
    }
    if (isLocationEnabled()) {
      std::cout << " " << location.file << ":" << location.line;
    }

    std::cout << "): ";

    std::cout << line.str() << std::endl;
  }

  if (verbosity <= Verbosity::info) {
    Entry entry;
    entry.verbosity = verbosity;
    entry.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
    entry.logger_name = logger.name;
    entry.message = line.str();
    entry.file = location.file;
    entry.line = location.line;

    logger.send(entry);
  }
}

Logger::LogStream &Logger::LogStream::operator<<(std::ostream &(*func)(std::ostream &)) {
  line << func;

  return *this;
}

const std::string getVerbosityLong(Logger::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::Verbosity::critical): {
      return "critical";
    }

    case (Logger::Verbosity::error): {
      return "error";
    }

    case (Logger::Verbosity::warning): {
      return "warning";
    }

    case (Logger::Verbosity::info): {
      return "info";
    }

    case (Logger::Verbosity::debug): {
      return "debug";
    }

    default: {
      return "";
    }
  }
}

const std::string getVerbosityShort(Logger::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::Verbosity::critical): {
      return "CRIT";
    }

    case (Logger::Verbosity::error): {
      return "ERRO";
    }

    case (Logger::Verbosity::warning): {
      return "WARN";
    }

    case (Logger::Verbosity::info): {
      return "INFO";
    }

    case (Logger::Verbosity::debug): {
      return "DBUG";
    }

    default: {
      return "";
    }
  }
}

const Color getVerbosityColor(Logger::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::Verbosity::critical):
    case (Logger::Verbosity::error): {
      return Color(Logger::isColorEnabled(), Color::Code::red);
    }

    case (Logger::Verbosity::warning): {
      return Color(Logger::isColorEnabled(), Color::Code::yellow);
    }

    case (Logger::Verbosity::info): {
      return Color(Logger::isColorEnabled(), Color::Code::cyan);
    }

    case (Logger::Verbosity::debug): {
      return Color(Logger::isColorEnabled(), Color::Code::magenta);
    }

    default: {
      return Color(Logger::isColorEnabled());
    }
  }
}

}  // namespace labrat::robot
