#pragma once

#include <atomic>

inline namespace utils {

template <typename T>
class Guard {
public:
  inline Guard(std::atomic<T> &object) : object(object) {
    ++object;
  }

  inline ~Guard() {
    --object;
    object.notify_all();
  }

private:
  std::atomic<T> &object;
};

class FlagGuard {
public:
  inline FlagGuard(std::atomic_flag &flag) : flag(flag) {
    while (flag.test_and_set()) {
      flag.wait(true);
    }
  }

  inline ~FlagGuard() {
    flag.clear();
    flag.notify_all();
  }

private:
  std::atomic_flag &flag;
};

template <bool RequiredValue = true>
class FlagBlock {
public:
  inline FlagBlock(std::atomic_flag &flag) : flag(flag) {
    while (flag.test() != RequiredValue) {
      flag.wait(!RequiredValue);
    }
  }

private:
  std::atomic_flag &flag;
};

template <typename T>
class WaitUntil {
public:
  inline WaitUntil(std::atomic<T> &value, T required) {
    while (true) {
      const std::size_t result = value.load();

      if (result == required) {
        break;
      }

      value.wait(result);
    }
  }
};

template <typename T>
class SpinUntil {
public:
  inline SpinUntil(std::atomic<T> &value, T required) {
    while (true) {
      const std::size_t result = value.load();

      if (result == required) {
        break;
      }
    }
  }
};

}  // namespace utils
