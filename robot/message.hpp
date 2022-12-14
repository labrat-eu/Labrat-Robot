#pragma once

#include <labrat/robot/utils/types.hpp>

#include <string>
#include <vector>

namespace labrat::robot {

class Message {
public:
  Message(bool string_convertable = false, bool serial_convertable = false) : string_convertable(string_convertable), serial_convertable(serial_convertable) {};
  ~Message() = default;

  inline bool isStringConvertable() const {
    return string_convertable;
  }

  inline bool isSerialConvertable() const {
    return serial_convertable;
  }

  virtual void toString(std::string &result);
  virtual void toSerial(std::vector<u8> &result);

private:
  bool string_convertable;
  bool serial_convertable;
};

}  // namespace labrat::robot
