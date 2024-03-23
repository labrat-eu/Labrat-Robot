/**
 * @file config.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/config.hpp>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<labrat::lbot::ConfigValue> {
  static bool decode(const Node& node, labrat::lbot::ConfigValue& rhs) {
    switch (node.Type()) {
      case YAML::NodeType::Scalar: {
        try {
          rhs = node.as<bool>();
          return true;
        } catch (TypedBadConversion<bool> &) {}
        try {
          rhs = node.as<i64>();
          return true;
        } catch (TypedBadConversion<i64> &) {}
        try {
          rhs = node.as<double>();
          return true;
        } catch (TypedBadConversion<double> &) {}
        try {
          rhs = node.as<std::string>();
          return true;
        } catch (TypedBadConversion<std::string> &) {}

        return false;
      }

      case YAML::NodeType::Sequence: {
        std::vector<labrat::lbot::ConfigValue> sequence;
        sequence.reserve(node.size());

        for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter) {
          try {
            sequence.emplace_back(iter->as<labrat::lbot::ConfigValue>());
          } catch (TypedBadConversion<labrat::lbot::ConfigValue> &) {
            return false;
          }
        }

        rhs = std::move(sequence);
        return true;
      }

      default: {
        return false;
      }
    }    
  }
};

}

inline namespace labrat {
namespace lbot {

ConfigValue::ConfigValue() = default;

ConfigValue::ConfigValue(const ConfigValue &rhs) : value(rhs.value) {}

ConfigValue::ConfigValue(ConfigValue &&rhs) : value (std::move(rhs.value)) {}

ConfigValue::ConfigValue(bool value) : value(value) {}

ConfigValue::ConfigValue(i64 value) : value(value) {}

ConfigValue::ConfigValue(double value) : value(value) {}

ConfigValue::ConfigValue(const std::string &value) : value(value) {}

ConfigValue::ConfigValue(std::string &&value) : value(std::forward<std::string>(value)) {}

ConfigValue::ConfigValue(const std::vector<ConfigValue> &value) : value(value) {}

ConfigValue::ConfigValue(std::vector<ConfigValue> &&value) : value(std::forward<std::vector<ConfigValue>>(value)) {}

constexpr ConfigValue &ConfigValue::operator=(const ConfigValue &rhs) {
  value = rhs.value;
  return *this;
}

constexpr ConfigValue &ConfigValue::operator=(ConfigValue &&rhs) {
  value = std::move(rhs.value);
  return *this;
}

bool ConfigValue::isValid() const {
  return !std::holds_alternative<std::monostate>(value);
}

ConfigValue::operator bool() const {
  return isValid();
}

std::weak_ptr<Config> Config::instance;

Config::Config() = default;

Config::~Config() = default;

Config::Ptr Config::get() {
  if (instance.use_count()) {
    return instance.lock();
  }

  std::shared_ptr<Config> result(new Config());
  instance = result;

  return result;
}

const ConfigValue &Config::setParameter(const std::string &name, ConfigValue &&value) {
  ParameterMap::const_iterator iter = parameter_map.find(name);

  if (iter != parameter_map.end()) {
    parameter_map.erase(iter);
  }

  return parameter_map.emplace_hint(iter, std::make_pair(name, std::forward<ConfigValue>(value)))->second;
}

const ConfigValue &Config::getParameter(const std::string &name) const {
  ParameterMap::const_iterator iter = parameter_map.find(name);

  if (iter == parameter_map.end()) {
    throw ConfigAccessException("Failed to access config value. No parameter with the requested name exists.");
  }

  return iter->second;
}

void Config::removeParameter(const std::string &name) {
  ParameterMap::const_iterator iter = parameter_map.find(name);

  if (iter != parameter_map.end()) {
    parameter_map.erase(iter);
  }
}

void Config::clear() noexcept {
  parameter_map.clear();
}

void Config::load(const std::string &filename) {
  try {
    YAML::Node file = YAML::LoadFile(filename);

    for(YAML::const_iterator iter = file.begin(); iter != file.end(); ++iter) {
      ConfigValue value;

      try {
        setParameter(iter->first.as<std::string>(), iter->second.as<ConfigValue>());
      } catch (YAML::TypedBadConversion<ConfigValue> &) {
        throw ConfigParseException("Failed to parse '" + filename + "'. Invalid value on key '" + iter->first.as<std::string>() + "'.");
      }
    }
  } catch (YAML::BadFile &) {
    throw ConfigParseException("Failed to load '" + filename + "'.");
  }
}

}  // namespace lbot
}  // namespace labrat
