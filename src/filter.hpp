/**
 * @file filter.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

#include <string>
#include <unordered_set>

inline namespace labrat {
namespace lbot {

class Filter {
public:
  Filter() = default;

  /**
   * @brief Check whether a callback should be performed for the topic with the supplied hash code.
   *
   * @param topic_hash Hash code of the topic.
   * @return true The callback should be performed.
   * @return false The callback should not be performed.
   */
  inline bool check(std::size_t topic_hash) const {
    return set.contains(topic_hash) ^ mode;
  }

  /**
   * @brief Check whether a callback should be performed for the topic with the supplied hash code.
   *
   * @param topic_name Name of the topic.
   * @return true The callback should be performed.
   * @return false The callback should not be performed.
   */
  inline bool check(const std::string &topic_name) const {
    return check(std::hash<std::string>()(topic_name));
  }

  /**
   * @brief Whitelist a topic.
   * All previously blacklisted topics will be removed from the filter.
   *
   * @param topic_name Name of the topic.
   */
  inline void whitelist(const std::string &topic_name) {
    add<false>(std::hash<std::string>()(topic_name));
  }

  /**
   * @brief Blacklist a topic.
   * All previously whitelisted topics will be removed from the filter.
   *
   * @param topic_name Name of the topic.
   */
  inline void blacklist(const std::string &topic_name) {
    add<true>(std::hash<std::string>()(topic_name));
  }

private:
  /**
   * @brief Add a topic to the filter.
   * Depending on the supplied mode this will either whitelist or blacklist the relevant topic.
   * If the mode is changed, all previously added topics will be removed from the filter.
   *
   * @tparam Mode New mode of the filter.
   * @param topic_hash Hash of the topic to be added to the filter-
   */
  template <bool Mode>
  void add(std::size_t topic_hash) {
    if (mode != Mode) {
      set.clear();
      mode = Mode;
    }

    set.emplace(topic_hash);
  }

  std::unordered_set<std::size_t> set;

  /**
   * @brief Mode of the filter.
   * When true specified blacklisted topics will be blocked and all other topics will pass.
   * When false specified whitelisted topics will pass and all other topics will be blocked.
   *
   */
  bool mode = true;
};

}  // namespace lbot
}  // namespace labrat
