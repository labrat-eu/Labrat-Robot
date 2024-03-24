/**
 * @file config.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/types.hpp>

#include <vector>
#include <string>
#include <variant>

inline namespace labrat {
namespace lbot {

class ConfigValue {
public:
  using Sequence = std::vector<ConfigValue>;

  ConfigValue();
  ConfigValue(const ConfigValue &rhs);
  ConfigValue(ConfigValue &&rhs);
  ConfigValue(bool value);
  ConfigValue(i64 value);
  ConfigValue(double value);
  ConfigValue(const std::string &value);
  ConfigValue(std::string &&value);
  ConfigValue(const Sequence &value);
  ConfigValue(Sequence &&value);

  constexpr ConfigValue &operator=(const ConfigValue &rhs);
  constexpr ConfigValue &operator=(ConfigValue &&rhs);

  /**
   * @brief Check whether a valid value is contained.
   * 
   * @return true A valid value is contained
   * @return false No value is contained
   */
  bool isValid() const;

  /**
   * @brief Alias for isValid()
   */
  operator bool() const;

  /**
   * @brief Check if the specified type is contained in the ConfigValue.
   * 
   * @tparam T Type to check
   * @return true The specified type is contained
   * @return false The specified type is not contained
   */
  template<typename T>
  inline bool contains() const {
    return std::holds_alternative<T>(value);
  }

  /**
   * @brief Get the contents.
   * 
   * @tparam T Type to get
   * @return const T& Contents
   */
  template<typename T>
  inline const T &get() const {
    try {
      return std::get<T>(value);
    } catch (const std::bad_variant_access &) {
      throw ConfigAccessException("Failed to access config value. The expected type does not match the contained type.");
    }
  }

private:
  std::variant<std::monostate, bool, i64, double, std::string, Sequence> value;
};

/**
 * @brief Central configuration storage class.
 *
 */
class Config {
private:
  /**
   * @brief Construct a new Config object.
   *
   */
  Config();

  static std::weak_ptr<Config> instance;

  using ParameterMap = std::unordered_map<std::string, ConfigValue>;
  ParameterMap parameter_map;

public:
  using Ptr = std::shared_ptr<Config>;

  /**
   * @brief Destroy the Config object.
   *
   */
  ~Config();

  /**
   * @brief Get the static instance.
   *
   * @return Config::Ptr Pointer to the static instance.
   */
  static Config::Ptr get();

  /**
   * @brief Set a parameter.
   * 
   * @param name Name of the parameter
   * @param value Value of the parameter
   * @return ConfigValue& Value of the created parameter
   */
  const ConfigValue &setParameter(const std::string &name, ConfigValue &&value);

  /**
   * @brief Get a parameter.
   * 
   * @param name Name of the parameter
   * @return const ConfigValue& Value of the parameter
   * 
   * @throw ConfigAccessException If the requested parameter does not exist
   */
  const ConfigValue &getParameter(const std::string &name) const;

  /**
   * @brief Remove a parameter. If it does not exist, do nothing.
   * 
   * @param name Name of the parameter
   */
  void removeParameter(const std::string &name);

  /**
   * @brief Remove all parameters.
   */
  void clear() noexcept;

  ParameterMap::const_iterator begin() const;
  ParameterMap::const_iterator end() const;

  /**
   * @brief Parse parameters from a configuration file. This will remove all existing parameters.
   */
  void load(const std::string &filename);
};

}  // namespace lbot
}  // namespace labrat
