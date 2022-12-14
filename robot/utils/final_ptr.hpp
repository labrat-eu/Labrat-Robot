#include <memory>

inline namespace utils {

template<typename T>
class FinalPtr : public std::shared_ptr<T> {
public:
  FinalPtr(std::shared_ptr<T> &&ptr) noexcept : std::shared_ptr<T>(std::forward<std::shared_ptr<T> &&>(ptr)) {}

  ~FinalPtr() {
    if (std::shared_ptr<T>::use_count() > 1) {
      // throw
    }
  }

};

}
