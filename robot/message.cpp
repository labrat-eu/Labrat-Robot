#include <labrat/robot/message.hpp>
#include <labrat/robot/exception.hpp>

namespace labrat::robot {

void Message::toString(std::string &result) {
  (void)result;

  if (isStringConvertable()) {
    throw Exception("Method toString() not defined in derived class.");
  } else {
    throw Exception("Message is not string convertable.");
  }
}

void Message::toSerial(std::vector<u8> &result) {
  (void)result;

  if (isSerialConvertable()) {
    throw Exception("Method toSerial() not defined in derived class.");
  } else {
    throw Exception("Message is not serial convertable.");
  }
}

}  // namespace labrat::robot
