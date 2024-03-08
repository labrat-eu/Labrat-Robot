/**
 * @file signal.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/robot/base.hpp>
#include <labrat/robot/exception.hpp>

#include <signal.h>

inline namespace utils {

/**
 * @brief Wait on a process signal.
 * 
 * @return int Signal number of the catched signal.
 */
int signalWait() {
  sigset_t signal_mask;
  if (int res = sigemptyset(&signal_mask)) {
    throw labrat::robot::SystemException("Failed to create signal set.", res);
  }
  if (int res = sigaddset(&signal_mask, SIGINT)) {
    throw labrat::robot::SystemException("Failed to add signal to set.", res);
  }
  if (int res = sigprocmask(SIG_BLOCK, &signal_mask, NULL)) {
    throw labrat::robot::SystemException("Failed to add signal to blocking set.", res);
  }

  int signal;
  if (int res = sigwait(&signal_mask, &signal)) {
    throw labrat::robot::SystemException("Failure while waiting on signal.", res);
  }

  return signal;
}

}  // namespace utils
