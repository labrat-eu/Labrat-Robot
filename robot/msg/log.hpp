#include <labrat/robot/message.hpp>

#include <string>
#include <chrono>

namespace labrat::robot::msg {

class Log : public Message {
public:
  std::string logger_name;
  std::chrono::seconds timestamp;

  std::string message;
};

}
