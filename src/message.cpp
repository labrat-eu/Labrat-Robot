/**
 * @file message.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/message.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <forward_list>
#include <fstream>
#include <sstream>

inline namespace labrat {
namespace lbot {

static std::forward_list<std::string> getPaths(const std::string &name);
static std::vector<std::string> toPaths(const std::string &name);

MessageReflection::MessageReflection(const std::string &name) {
  valid = false;
  const std::forward_list<std::string> paths = getPaths(name);

  for (const std::string &path : paths) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.good()) {
      continue;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(size);
    if (file.read(buffer.data(), size)) {
      valid = true;
      return;
    }
  }
}

static std::forward_list<std::string> getPaths(const std::string &name) {
  std::vector<std::string> paths = toPaths(name);

  std::forward_list<std::string> result;
  const char *environment = std::getenv("LBOT_REFLECTION_PATH");

  if (environment == nullptr) {
    return result;
  }

  std::stringstream environment_paths(environment);

  std::string environment_part;
  while (std::getline(environment_paths, environment_part, ':')) {
    for (const std::string &path : paths) {
      result.emplace_front(environment_part + "/" + path + ".bfbs");
    }
  }

  return result;
}

static std::vector<std::string> toPaths(const std::string &name) {
  std::vector<std::string> result;
  result.resize(2);

  for (std::string &path : result) {
    path.reserve(name.size() * 2);
  }

  bool first_flag = true;

  for (const char &character : name) {
    if (std::isupper(character)) {
      if (!first_flag) {
        result[0].push_back('_');
      }

      result[0].push_back(std::tolower(character));
      result[1].push_back(character);
    } else {
      if (character == '.') {
        for (std::string &path : result) {
          path.push_back('/');
        }

        first_flag = true;

        continue;
      }

      for (std::string &path : result) {
        path.push_back(character);
      }
    }

    first_flag = false;
  }

  return result;
}

}  // namespace lbot
}  // namespace labrat
