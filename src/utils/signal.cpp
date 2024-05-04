/**
 * @file signal.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#include <labrat/lbot/utils/signal.hpp>

inline namespace labrat {
namespace lbot {
inline namespace utils {

int signalWait() {
  sigset_t signal_mask;
  if (int res = sigemptyset(&signal_mask)) {
    throw labrat::lbot::SystemException("Failed to create signal set.", res);
  }
  if (int res = sigaddset(&signal_mask, SIGINT)) {
    throw labrat::lbot::SystemException("Failed to add signal to set.", res);
  }
  if (int res = sigprocmask(SIG_BLOCK, &signal_mask, NULL)) {
    throw labrat::lbot::SystemException("Failed to add signal to blocking set.", res);
  }

  int signal;
  if (int res = sigwait(&signal_mask, &signal)) {
    throw labrat::lbot::SystemException("Failure while waiting on signal.", res);
  }

  return signal;
}


}  // namespace utils
}  // namespace lbot
}  // namespace labrat
