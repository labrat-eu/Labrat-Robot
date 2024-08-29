/**
 * @file config.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/config.hpp>

#include <list>
#include <ranges>

#include <yaml-cpp/yaml.h>

namespace YAML {

template <>
struct convert<labrat::lbot::ConfigValue> {
  static bool decode(const Node &node, labrat::lbot::ConfigValue &rhs) {
    switch (node.Type()) {
      case YAML::NodeType::Scalar: {
        try {
          rhs = node.as<bool>();
          return true;
        } catch (TypedBadConversion<bool> &) {
        }
        try {
          rhs = node.as<lbot::i64>();
          return true;
        } catch (TypedBadConversion<lbot::i64> &) {
        }
        try {
          rhs = node.as<double>();
          return true;
        } catch (TypedBadConversion<double> &) {
        }
        try {
          rhs = node.as<std::string>();
          return true;
        } catch (TypedBadConversion<std::string> &) {
        }

        return false;
      }

      case YAML::NodeType::Sequence: {
        labrat::lbot::ConfigValue::Sequence sequence;
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

}  // namespace YAML

inline namespace labrat {
namespace lbot {

ConfigValue::ConfigValue() = default;

ConfigValue::ConfigValue(const ConfigValue &rhs) : value(rhs.value) {}

ConfigValue::ConfigValue(ConfigValue &&rhs) : value(std::move(rhs.value)) {}

ConfigValue::ConfigValue(bool value) : value(value) {}

ConfigValue::ConfigValue(const char *value) : value(std::string(value)) {}

ConfigValue::ConfigValue(const std::string &value) : value(value) {}

ConfigValue::ConfigValue(std::string &&value) : value(std::forward<std::string>(value)) {}

ConfigValue::ConfigValue(const Sequence &value) : value(value) {}

ConfigValue::ConfigValue(Sequence &&value) : value(std::forward<Sequence>(value)) {}

const ConfigValue &ConfigValue::operator=(const ConfigValue &rhs) {
  value = rhs.value;
  return *this;
}

const ConfigValue &ConfigValue::operator=(ConfigValue &&rhs) {
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

const ConfigValue &Config::getParameterFallback(const std::string &name, ConfigValue &&fallback) const {
  try {
    return getParameter(name);
  } catch (ConfigAccessException &) {
    return fallback;
  }
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

Config::ParameterMap::const_iterator Config::begin() const {
  return parameter_map.begin();
}

Config::ParameterMap::const_iterator Config::end() const {
  return parameter_map.end();
}

void Config::load(const std::string &filename) {
  YAML::Node file;

  try {
    file = YAML::LoadFile(filename);
  } catch (YAML::BadFile &) {
    throw ConfigParseException("Failed to load '" + filename + "'.");
  }

  struct NodeInfo {
    const std::string name;
    YAML::const_iterator iter;
    YAML::const_iterator end;
  };
  std::list<NodeInfo> node_stack;
  node_stack.emplace_back("/", file.begin(), file.end());

  while (!node_stack.empty()) {
    NodeInfo &top = node_stack.back();

    if (top.iter == top.end) {
      node_stack.pop_back();
      ++(node_stack.back().iter);

      continue;
    }

    const YAML::Node &node = top.iter->second;
    const std::string &name = top.iter->first.as<std::string>();

    if (node.IsMap()) {
      node_stack.emplace_back(name + "/", node.begin(), node.end());
      continue;
    }

    auto to_string = [](const NodeInfo &info) -> std::string {
      return info.name;
    };

    std::string full_name;
    std::ranges::copy(std::views::transform(node_stack, to_string) | std::views::join, std::back_inserter(full_name));
    full_name += name;

    try {
      setParameter(full_name, node.as<ConfigValue>());
    } catch (YAML::TypedBadConversion<ConfigValue> &) {
      throw ConfigParseException("Failed to parse '" + filename + "'. Invalid value on key '" + full_name + "'.");
    }

    ++(top.iter);
  }
}

}  // namespace lbot
}  // namespace labrat
