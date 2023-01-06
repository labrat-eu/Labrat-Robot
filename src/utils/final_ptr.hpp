#pragma once

#include <cassert>
#include <memory>

inline namespace utils {

template <typename T>
class FinalPtr : public std::shared_ptr<T> {
public:
  FinalPtr(std::shared_ptr<T> &&ptr) noexcept : std::shared_ptr<T>(std::forward<std::shared_ptr<T> &&>(ptr)) {}

  ~FinalPtr() {
    // The use count of a final pointer shall not be greater than one upon destruction as this instance should be the last one to own the
    // managed object.
    assert(std::shared_ptr<T>::use_count() == 1);
  }
};

}  // namespace utils
