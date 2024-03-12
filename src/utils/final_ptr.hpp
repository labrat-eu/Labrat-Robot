/**
 * @file final_ptr.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

#include <cassert>
#include <memory>

inline namespace utils {

/**
 * @brief Shared pointer instance that is required to be the last instance pointing to the managed object to be deleted.
 *
 * @tparam T Type pointed to by the shared pointer.
 */
template <typename T>
class FinalPtr : public std::shared_ptr<T> {
public:
  /**
   * @brief Construct a new Final Ptr object.
   *
   * @param ptr Shared pointer object to be moved.
   */
  FinalPtr(std::shared_ptr<T> &&ptr) noexcept : std::shared_ptr<T>(std::forward<std::shared_ptr<T> &&>(ptr)) {}

  /**
   * @brief Destroy the Final Ptr object and assert whether this instance is indeed the last instance to point to the managed object.
   *
   */
  ~FinalPtr() {
    // The use count of a final pointer shall not be greater than one upon destruction as this instance should be the last one to own the
    // managed object.
    assert(std::shared_ptr<T>::use_count() == 1);
  }
};

}  // namespace utils
