#include <labrat/robot/logger.hpp>
#include <labrat/robot/manager.hpp>
#include <labrat/robot/msg/log.pb.h>
#include <labrat/robot/node.hpp>

#include <iostream>
#include <mutex>

namespace labrat::robot {

Logger::Verbosity Logger::log_level = Verbosity::info;
static std::mutex io_mutex;

class LoggerNode : public Node {
private:
  std::unique_ptr<Sender<Message<msg::Log>, Logger::Entry>> sender;

public:
  LoggerNode(const Node::Environment environment) : Node(environment) {
    sender = addSender<Message<msg::Log>, Logger::Entry>("/log");
  }

  void send(const Logger::Entry &message) {
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

Logger::LogStream Logger::critical() {
  return LogStream(*this, Verbosity::critical);
}

Logger::LogStream Logger::warning() {
  return LogStream(*this, Verbosity::warning);
}

Logger::LogStream Logger::info() {
  return LogStream(*this, Verbosity::info);
}

Logger::LogStream Logger::debug() {
  return LogStream(*this, Verbosity::debug);
}

void Logger::send(const Entry &message) {
  node->send(message);
}

Logger::LogStream::LogStream(const Logger &logger, Verbosity verbosity) : logger(logger), verbosity(verbosity) {}

Logger::LogStream::~LogStream() {
  if (verbosity <= Logger::log_level) {
    std::lock_guard guard(io_mutex);

    std::cout << getVerbosityColor(verbosity) << "[" << getVerbosityShort(verbosity) << "]" << Color() << " (" << logger.name << " @ "
              << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "): ";
    std::cout << line.str() << std::endl;
  }

  if (verbosity <= Verbosity::info) {
    Entry entry;
    entry.verbosity = verbosity;
    entry.timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    entry.logger_name = logger.name;
    entry.message = line.str();

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
    case (Logger::Verbosity::critical): {
      return Color(Color::Code::red);
    }

    case (Logger::Verbosity::warning): {
      return Color(Color::Code::yellow);
    }

    case (Logger::Verbosity::info): {
      return Color(Color::Code::cyan);
    }

    case (Logger::Verbosity::debug): {
      return Color(Color::Code::magenta);
    }

    default: {
      return Color();
    }
  }
}

}  // namespace labrat::robot
