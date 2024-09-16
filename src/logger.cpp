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
#include <chrono>
#include <sstream>
#include <memory>

inline namespace labrat {
namespace lbot {

class Logger::Private {
public:
  class Entry {
  public:
    Verbosity verbosity;
    Clock::time_point timestamp;
    std::string logger_name;
    std::string message;
    std::source_location location;
  };

  class EntryMessage : public MessageBase<foxglove::Log, Entry> {
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

  class Node : public UniqueNode {
  private:
    Sender<EntryMessage>::Ptr sender;

  public:
    explicit Node() : UniqueNode("logger") {
      sender = addSender<EntryMessage>("/log");
    }

    void send(const Entry &entry) {
      sender->put(entry);
    }

    void trace(const Entry &entry) {
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

  static std::string getVerbosityLong(Logger::Verbosity verbosity) {
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

  static std::string getVerbosityShort(Logger::Verbosity verbosity) {
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

  static Color getVerbosityColor(Logger::Verbosity verbosity) {
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

  Logger::Verbosity log_level = Verbosity::info;
  bool use_color = true;
  bool print_location = false;
  bool print_time = true;

  std::shared_ptr<Node> node;
  std::mutex io_mutex;
};

static Logger::Private priv;

Logger::Logger(std::string name) : name(std::move(name)) {}

void Logger::initialize() {
  priv.node = Manager::get()->addNode<Private::Node>("logger");
}

void Logger::deinitialize() {
  priv.node.reset();
}

Logger::LogStream Logger::log(Verbosity verbosity, const std::source_location &location) {
  return LogStream(*this, verbosity, location);
}

Logger::LogStream::LogStream(const Logger &logger, Verbosity verbosity, const std::source_location &location) :
  logger(logger), verbosity(verbosity), location(location) {}

Logger::LogStream::~LogStream() {
  const Clock::time_point now = Clock::now();

  if (verbosity <= priv.log_level) { 
    std::lock_guard guard(priv.io_mutex);

    std::cout << Private::getVerbosityColor(verbosity) << "[" << Private::getVerbosityShort(verbosity) << "]" << Logger::Private::Color(isColorEnabled()) << " ("
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

  Private::Entry entry;
  entry.verbosity = verbosity;
  entry.timestamp = now;
  entry.logger_name = logger.name;
  entry.message = message.str();
  entry.location = location;

  if (!logger.send_topic) {
    return;
  }

  if (priv.node) {
    if (verbosity <= Verbosity::info) {
      priv.node->send(entry);
    } else {
      priv.node->trace(entry);
    }
  }
}

Logger::LogStream &Logger::LogStream::operator<<(std::ostream &(*func)(std::ostream &)) {
  message << func;

  return *this;
}

void Logger::setLogLevel(Verbosity level) {
  priv.log_level = level;
}

Logger::Verbosity Logger::getLogLevel() {
  return priv.log_level;
}

void Logger::enableTopic() {
  send_topic = true;
}

void Logger::disableTopic() {
  send_topic = false;
}

bool Logger::isTopicEnabled() const {
  return send_topic;
}

void Logger::enableColor() {
  priv.use_color = true;
}

void Logger::disableColor() {
  priv.use_color = false;
}

bool Logger::isColorEnabled() {
  return priv.use_color;
}

void Logger::enableLocation() {
  priv.print_location = true;
}

void Logger::disableLocation() {
  priv.print_location = false;
}

bool Logger::isLocationEnabled() {
  return priv.print_location;
}

void Logger::enableTime() {
  priv.print_time = true;
}

void Logger::disableTime() {
  priv.print_time = false;
}

bool Logger::isTimeEnabled() {
  return priv.print_time;
}

}  // namespace lbot
}  // namespace labrat
