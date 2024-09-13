/**
 * @file logger.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/logger.hpp>
#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/message.hpp>
#include <labrat/lbot/msg/foxglove/Log.hpp>
#include <labrat/lbot/node.hpp>

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

inline namespace labrat {
namespace lbot {

Logger::Verbosity Logger::log_level = Verbosity::info;
bool Logger::use_color = true;
bool Logger::print_location = false;
bool Logger::print_time = true;

static std::mutex io_mutex;

class EntryMessage : public MessageBase<foxglove::Log, Logger::Entry> {
public:
  static void convertFrom(const Converted &source, Storage &destination) {
    switch (source.verbosity) {
      case (Logger::Verbosity::critical): {
        destination.level = foxglove::LogLevel::FATAL;
        break;
      }

      case (Logger::Verbosity::error): {
        destination.level = foxglove::LogLevel::ERROR;
        break;
      }

      case (Logger::Verbosity::warning): {
        destination.level = foxglove::LogLevel::WARNING;
        break;
      }

      case (Logger::Verbosity::info): {
        destination.level = foxglove::LogLevel::INFO;
        break;
      }

      case (Logger::Verbosity::debug): {
        destination.level = foxglove::LogLevel::DEBUG;
        break;
      }
    }

    const Clock::duration duration = source.timestamp.time_since_epoch();
    destination.timestamp = std::make_unique<foxglove::Time>(std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
      (duration % std::chrono::seconds(1)).count());
    destination.name = source.logger_name;
    destination.message = source.message;
    destination.file = source.location.file_name();
    destination.line = source.location.line();
  }
};

class LoggerNode : public UniqueNode {
private:
  Sender<EntryMessage>::Ptr sender;

public:
  explicit LoggerNode() : UniqueNode("logger") {
    sender = addSender<EntryMessage>("/log");
  }

  void send(const Logger::Entry &entry) {
    sender->put(entry);
  }

  void trace(const Logger::Entry &entry) {
    sender->trace(entry);
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

  explicit Color(bool enable_color, Code code = Code::normal) : code(code), enable_color(enable_color) {}

  friend std::ostream &

  operator<<(std::ostream &stream, const Color &color) {
    if (color.enable_color) {
      return stream << "\033[" << static_cast<i16>(color.code) << "m";
    }

    return stream;
  }

private:
  const Code code;
  const bool enable_color;
};

std::string getVerbosityLong(Logger::Verbosity verbosity);
std::string getVerbosityShort(Logger::Verbosity verbosity);
Color getVerbosityColor(Logger::Verbosity verbosity);

std::shared_ptr<LoggerNode> Logger::node;

Logger::Logger(std::string name) : name(std::move(name)) {}

void Logger::initialize() {
  node = Manager::get()->addNode<LoggerNode>("logger");
}

void Logger::deinitialize() {
  node.reset();
}

Logger::LogStream Logger::log(Verbosity verbosity, const std::source_location &location) {
  return LogStream(*this, verbosity, location);
}

void Logger::send(const Entry &entry) {
  if (node) {
    node->send(entry);
  }
}

void Logger::trace(const Entry &entry) {
  if (node) {
    node->trace(entry);
  }
}

Logger::LogStream::LogStream(const Logger &logger, Verbosity verbosity, const std::source_location &location) :
  logger(logger), verbosity(verbosity), location(location) {}

Logger::LogStream::~LogStream() {
  const Clock::time_point now = Clock::now();

  if (verbosity <= Logger::log_level) { 
    std::lock_guard guard(io_mutex);

    std::cout << getVerbosityColor(verbosity) << "[" << getVerbosityShort(verbosity) << "]" << Color(isColorEnabled()) << " ("
              << logger.name;

    if (isLocationEnabled() || isTimeEnabled()) {
      std::cout << " @";
    }

    if (isTimeEnabled()) {
      std::cout << " " << Clock::format(now);
    }
    if (isLocationEnabled()) {
      std::cout << " " << location.file_name() << ":" << location.line();
    }

    std::cout << "): ";

    std::cout << message.str() << std::endl;
  }

  Entry entry;
  entry.verbosity = verbosity;
  entry.timestamp = now;
  entry.logger_name = logger.name;
  entry.message = message.str();
  entry.location = location;

  if (!logger.send_topic) {
    return;
  }

  if (verbosity <= Verbosity::info) {
    logger.send(entry);
  } else {
    logger.trace(entry);
  }
}

Logger::LogStream &Logger::LogStream::operator<<(std::ostream &(*func)(std::ostream &)) {
  message << func;

  return *this;
}

std::string getVerbosityLong(Logger::Verbosity verbosity) {
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

std::string getVerbosityShort(Logger::Verbosity verbosity) {
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

Color getVerbosityColor(Logger::Verbosity verbosity) {
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

}  // namespace lbot
}  // namespace labrat
