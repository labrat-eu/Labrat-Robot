/**
 * @file cleanup.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <atomic>

inline namespace labrat {
namespace lbot {
inline namespace utils {
class CleanupLock {
public:
  class Guard {
  public:
    explicit Guard(CleanupLock &lock) : lock(lock) {
      State expected_state = State::unlocked;
      lock.state.compare_exchange_strong(expected_state, State::locked);
    }

    Guard(Guard &&rhs) noexcept : lock(rhs.lock) {
      rhs.unlock_at_destroy = false;
    }

    ~Guard() {
      if (unlock_at_destroy) {
        lock.state.store(State::unlocked);
      }
    }

    [[nodiscard]] inline bool valid() const {
      return lock.state.load() == State::locked;
    }

  private:
    CleanupLock &lock;
    bool unlock_at_destroy = true;
  };

  CleanupLock() : state(State::unlocked) {}

  ~CleanupLock() {
    destroy();
  }

  Guard lock() {
    return Guard(*this);
  }

  void destroy() {
    if (state.load() == State::deleted) {
      return;
    }

    while (true) {
      State expected_state = State::unlocked;

      if (state.compare_exchange_strong(expected_state, State::deleted)) {
        break;
      }

      state.wait(State::locked);
    }
  }

private:
  enum class State : u8 {
    unlocked,
    locked,
    deleted,
  };

  std::atomic<State> state;

  friend class Guard;
};

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
