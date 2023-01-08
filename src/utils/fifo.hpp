/**
 * @file fifo.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/utils/fifo.hpp>

#include <vector>

inline namespace utils {

/**
 * @brief Fixed sized FIFO buffer (ring buffer).
 *
 * @tparam T Type of the values stored.
 */
template <typename T>
class Fifo {
public:
  /**
   * @brief Construct a new Fifo object. The buffer will be initialized with zeroes.
   *
   * @param[in] size Number of elements to be stored.
   */
  Fifo(std::size_t size) : data(size) {
    current_index = 0;
  }

  /**
   * @brief Push a value into the buffer.
   *
   * @param[in] value Value to be stored.
   * @return T Last value stored inside the buffer that was pushed out.
   */
  T push(T value) {
    // Increment the current index.
    current_index = (current_index + 1) % data.size();

    // Exchange the value stored.
    const T result = data[current_index];
    data[current_index] = value;

    return result;
  }

  /**
   * @brief Peak into the buffer without modifying it.
   *
   * @param[in] i Index relative to the last element pushed.
   * @return T Stored value.
   */
  T peak_front(std::size_t i = 0) const {
    return data[(current_index - (i % data.size()) + data.size()) % data.size()];
  }

  /**
   * @brief Peak into the buffer without modifying it.
   *
   * @param[in] i Index relative to the first element pushed.
   * @return T Stored value.
   */
  T peak_back(std::size_t i = 0) const {
    return data[(current_index + 1 + i) % data.size()];
  }

protected:
  // Data vector to store the values.
  std::vector<T> data;

  // Index where to perform the push operation.
  std::size_t current_index;
};

}  // namespace utils
