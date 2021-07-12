/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
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
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/logger.h"

#ifdef _WIN32
#include <windows.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
struct Helper_props {
  std::mutex interrupt_mtx;
  std::condition_variable interrupt;
  std::atomic<bool> stop_interrupter = false;
  std::atomic<bool> our_interrupt = false;
};

Helper_props hprops;

Interrupt_windows_helper::Interrupt_windows_helper() {
  m_scoped_thread = mysqlsh::spawn_scoped_thread([] {
    while (!hprops.stop_interrupter) {
      std::unique_lock<std::mutex> lock(hprops.interrupt_mtx);
      hprops.interrupt.wait(lock, []() {
        return hprops.our_interrupt || hprops.stop_interrupter;
      });
      if (hprops.our_interrupt) {
        shcore::current_interrupt()->interrupt();
        hprops.our_interrupt = false;
      }
    }
  });
}

Interrupt_windows_helper::~Interrupt_windows_helper() {
  hprops.stop_interrupter = true;
  hprops.interrupt.notify_all();
  m_scoped_thread.join();
}

static std::atomic<uint64_t> windows_ctrl_handler_blocked = 0;

static BOOL windows_ctrl_handler(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      if (0 == windows_ctrl_handler_blocked) {
        try {
          // we're being called from another thread, first notify the condition
          // so thread_scope can do it's job and properly pass the interrupts
          // with scoped contexts
          hprops.our_interrupt = true;
          hprops.interrupt.notify_one();
        } catch (const std::exception &e) {
          log_error("Unhandled exception in SIGINT handler: %s", e.what());
        }
      }
      // Don't let the default handler terminate us
      return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      // TODO: Add proper exit handling if needed
      break;
  }
  // Pass signal to the next control handler function.
  return FALSE;
}

void Interrupt_helper::block() { ++windows_ctrl_handler_blocked; }

void Interrupt_helper::unblock(bool) {
  assert(windows_ctrl_handler_blocked > 0);
  --windows_ctrl_handler_blocked;
}

void Interrupt_helper::setup() {
  // if we're being executed using CreateProcess() with
  // CREATE_NEW_PROCESS_GROUP flag set, an implicit call to
  // SetConsoleCtrlHandler(NULL, TRUE) is made, need to revert that
  SetConsoleCtrlHandler(nullptr, FALSE);
  // set our own handler
  SetConsoleCtrlHandler(windows_ctrl_handler, TRUE);
}

#else  // !_WIN32

#include <signal.h>
#include <unistd.h>
/**
SIGINT signal handler.

@description
This function handles SIGINT (Ctrl-C). It will call a shell function
which should handle the interruption in an appropriate fashion.
If the interrupt() function returns false, it will interpret as a signal
that the shell itself should abort immediately.

@param [IN]               Signal number
*/

static void handle_ctrlc_signal(int /* sig */) {
  int errno_save = errno;
  try {
    shcore::current_interrupt()->interrupt();
  } catch (const std::exception &e) {
    log_error("Unhandled exception in SIGINT handler: %s", e.what());
  }
  if (shcore::current_interrupt()->propagates_interrupt()) {
    // propagate the ^C to the caller of the shell
    // this is the usual handling when we're running in batch mode
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
  errno = errno_save;
}

static void install_signal_handler() {
  struct sigaction sa;

  // install signal handler
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &handle_ctrlc_signal;
  sigemptyset(&sa.sa_mask);
  // allow system calls to be interrupted, do not set SA_RESTART flag, behaviour
  // introduced by a fix for BUG#27894642, restored by a fix for BUG#33096667
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);

  // Ignore broken pipe signal
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, nullptr);
}

void Interrupt_helper::setup() { install_signal_handler(); }

void Interrupt_helper::block() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_BLOCK, &sigset, nullptr);
}

void Interrupt_helper::unblock(bool clear_pending) {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);

  // do not use sigpending() to check if signal is pending, because it can be
  // delivered during sigprocmask(SIG_UNBLOCK) call, always clear pending
  // signals if instructed to do so
  if (clear_pending) {
    struct sigaction ign, old;
    // set to ignore SIGINT
    ign.sa_handler = SIG_IGN;
    ign.sa_flags = 0;
    sigaction(SIGINT, &ign, &old);
    // unblock and let SIGINT be delivered (and ignored)
    sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
    // restore original SIGINT handler
    sigaction(SIGINT, &old, nullptr);
  } else {
    sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
  }
}

#endif  //! _WIN32
