/**
 * @file message.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/robot/message.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <forward_list>
#include <fstream>
#include <cctype>

namespace labrat::robot {

static const std::forward_list<std::string> getPaths(std::string name);
static void toPath(std::string &name);

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

static const std::forward_list<std::string> getPaths(std::string name) {
  toPath(name);

  std::forward_list<std::string> result;
  const char *environment = std::getenv("LABRAT_ROBOT_REFLECTION_PATH");

  if (environment == nullptr) {
    return result;
  }

  std::stringstream environment_paths(environment);

  std::string environment_part;
  while (std::getline(environment_paths, environment_part, ':')) {
    result.emplace_front(environment_part + "/" + name + ".bfbs");
  }

  return result;
}

static void toPath(std::string &name) {
  std::string result;
  result.reserve(name.size() * 2);

  bool first_flag = true;

  for (const char &character : name) {
    if (std::isupper(character)) {
      if (!first_flag) {
        result.push_back('_');
      }

      result.push_back(std::tolower(character));
    } else {
      if (character == '.') {
        result.push_back('/');
        first_flag = true;

        continue;
      }

      result.push_back(character);
    }

    first_flag = false;
  }

  name = result;
}

}  // namespace labrat::robot
