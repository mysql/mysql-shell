/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/include/shellcore/interrupt_helper.h"

#ifdef _WIN32
#include <windows.h>
#else  // !_WIN32
#include <signal.h>
#include <unistd.h>
#endif  // !_WIN32

#include <cassert>
#include <cstring>

namespace shcore {
namespace {

class Helper final {
 public:
  Helper() = default;

  Helper(const Helper &) = delete;
  Helper(Helper &&) = delete;

  Helper &operator=(const Helper &) = delete;
  Helper &operator=(Helper &&) = delete;

  ~Helper() = default;

  void setup(Interrupts *handler) { m_handler = handler; }

  void signal() { m_handler->interrupt(); }

 private:
  Interrupts *m_handler = nullptr;
};

Helper &helper() {
  static Helper s_helper;
  return s_helper;
}

#ifdef _WIN32

BOOL handle_ctrlc_signal(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      helper().signal();
      // Don't let the default handler terminate us
      return TRUE;

    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      // TODO(anyone): Add proper exit handling if needed
      break;
  }

  // Pass signal to the next control handler function.
  return FALSE;
}

void install_signal_handler() {
  // if we're being executed using CreateProcess() with CREATE_NEW_PROCESS_GROUP
  // flag set, an implicit call to SetConsoleCtrlHandler(NULL, TRUE) is made,
  // need to revert that
  SetConsoleCtrlHandler(nullptr, FALSE);
  // set our own handler
  SetConsoleCtrlHandler(handle_ctrlc_signal, TRUE);
}

#else  // !_WIN32

void handle_ctrlc_signal(int /* sig */) {
  const auto errno_save = errno;

  helper().signal();

  errno = errno_save;
}

void install_signal_handler() {
  struct sigaction sa;

  // install signal handler
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &handle_ctrlc_signal;
  sigemptyset(&sa.sa_mask);
  // allow system calls to be interrupted, do not set SA_RESTART flag, behaviour
  // introduced by a fix for BUG#27894642, restored by a fix for BUG#33096667
  // by default, signal which triggered the handler is blocked while handler is
  // being invoked
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);

  // Ignore broken pipe signal
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, nullptr);
}

#endif  //! _WIN32

}  // namespace

void Signal_interrupt_helper::setup(Interrupts *handler) {
  helper().setup(handler);
  install_signal_handler();
}

}  // namespace shcore
