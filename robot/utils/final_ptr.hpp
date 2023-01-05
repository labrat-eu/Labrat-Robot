#pragma once

#include <iostream>
#include <memory>

inline namespace utils {

template <typename T>
class FinalPtr : public std::shared_ptr<T> {
public:
  FinalPtr(std::shared_ptr<T> &&ptr) noexcept : std::shared_ptr<T>(std::forward<std::shared_ptr<T> &&>(ptr)) {}

  ~FinalPtr() {
    if (std::shared_ptr<T>::use_count() > 1) {
      std::cerr << "The use count of a final pointer is greater than one upon destruction." << std::endl;
    }
  }
};

}  // namespace utils
