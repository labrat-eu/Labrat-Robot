#include <labrat/robot/logger.hpp>
#include <labrat/robot/node.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/msg/log.hpp>

#include <iostream>

namespace labrat::robot {

class LoggerNode : public Node {
private:
  std::unique_ptr<Sender<msg::Log>> sender;

public:
  LoggerNode(const std::string &name, TopicMap &topic_map) : Node(name, topic_map) {
    sender = addSender<msg::Log>("/log");
  }

  void send(const msg::Log &message) {
    sender->put(message);
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

  Color(Code code = Code::normal, bool enable_color = true) : code(code), enable_color(enable_color) {}

  friend std::ostream&

  operator<<(std::ostream& stream, const Color& color) {
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

const std::string getVerbosityLong(Logger::LogStream::Verbosity verbosity);
const std::string getVerbosityShort(Logger::LogStream::Verbosity verbosity);
const Color getVerbosityColor(Logger::LogStream::Verbosity verbosity);

std::shared_ptr<LoggerNode> Logger::node(Manager::get().addNode<LoggerNode>("logger"));

Logger::Logger(const std::string &name) : name(std::move(name)) {}

Logger::LogStream Logger::critical() {
  return LogStream(*this, LogStream::Verbosity::critical);
}

Logger::LogStream Logger::warning() {
  return LogStream(*this, LogStream::Verbosity::warning);
}

Logger::LogStream Logger::info() {
  return LogStream(*this, LogStream::Verbosity::info);
}

Logger::LogStream Logger::debug() {
  return LogStream(*this, LogStream::Verbosity::debug);
}

void Logger::send(const msg::Log &message) {
  node->send(message);
}

Logger::LogStream::LogStream(const Logger &logger, Verbosity verbosity) :logger(logger), verbosity(verbosity) {}

Logger::LogStream::~LogStream() {
  std::cout << getVerbosityColor(verbosity) << "[" << getVerbosityShort(verbosity) << "]" << Color() << " (" << logger.name << " @ " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "): ";
  std::cout << line.str() << std::endl;

  msg::Log log_message;
  log_message.logger_name = logger.name;
  log_message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
  log_message.message = line.str();

  logger.send(log_message);
}

Logger::LogStream &Logger::LogStream::operator<<(std::ostream& (*func)(std::ostream&)) {
  line << func;

  return *this;
}

const std::string getVerbosityLong(Logger::LogStream::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::LogStream::Verbosity::critical): {
      return "critical";
    }

    case (Logger::LogStream::Verbosity::warning): {
      return "warning";
    }

    case (Logger::LogStream::Verbosity::info): {
      return "info";
    }

    case (Logger::LogStream::Verbosity::debug): {
      return "debug";
    }

    default: {
      return "";
    }
  }
}

const std::string getVerbosityShort(Logger::LogStream::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::LogStream::Verbosity::critical): {
      return "CRIT";
    }

    case (Logger::LogStream::Verbosity::warning): {
      return "WARN";
    }

    case (Logger::LogStream::Verbosity::info): {
      return "INFO";
    }

    case (Logger::LogStream::Verbosity::debug): {
      return "DBUG";
    }

    default: {
      return "";
    }
  }
}

const Color getVerbosityColor(Logger::LogStream::Verbosity verbosity) {
  switch (verbosity) {
    case (Logger::LogStream::Verbosity::critical): {
      return Color(Color::Code::red);
    }
  
    case (Logger::LogStream::Verbosity::warning): {
      return Color(Color::Code::yellow);
    }

    case (Logger::LogStream::Verbosity::info): {
      return Color(Color::Code::cyan);
    }

    case (Logger::LogStream::Verbosity::debug): {
      return Color(Color::Code::magenta);
    }

    default: {
      return Color();
    }
  }
}

}  // namespace labrat::robot
