/**
 * @file string.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/utils/string.hpp>

inline namespace labrat {
namespace lbot {
inline namespace utils {

void shrinkString(std::string &string)
{
  const std::size_t size = string.find_first_of('\0');

  if (size != std::string::npos) {
    string.resize(size);
  }
}

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
