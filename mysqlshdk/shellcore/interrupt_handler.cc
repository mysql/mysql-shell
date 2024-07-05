/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#include "shellcore/interrupt_handler.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else  // !_WIN32
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !_WIN32

#include <cassert>
#include <stdexcept>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"

#ifdef _WIN32
#define MYSQLSH_SYNCHRONOUS_SIGNAL_DELIVERY 0
#else  // !_WIN32
#define MYSQLSH_SYNCHRONOUS_SIGNAL_DELIVERY 1
#endif  // !_WIN32

#ifdef _WIN32

namespace {

int pipe(int pipefd[2]) { return _pipe(pipefd, 256, _O_BINARY); }

}  // namespace

#endif  // !_WIN32

namespace shcore {

/** User interruption (^C) handler. Called by SIGINT handler in console shell.

  ### User Interruption Handling Mechanics

  Program must provide an interrupt handling helper object (Interrupt_handler).
  In a console program, setup() would install a signal handler that calls
  Interrupts::interrupt() on SIGINT. It must also implement block() and
  unblock(), which would block and unblock SIGINT from being delivered
  (using sigprocmask).

  The interrupt manager keeps a stack of callbacks installed by the program.
  Whenever the program is about to start an action that can be interrupted,
  it must push a callback and pop it when done.
  This handler must not do anything that is not re-entrant; most handlers
  will just set a flag.

  When an interruption happens, the callbacks in the stack are called.
  The callbacks are expected to do whatever is needed for execution to be
  cancelled at the first chance, when control returns from the signal handler
  and/or whatever function they called exits (normally or via exception).
  If a callback returns false, the remaining ones in the stack will not be
  called. Callbacks should return true in general.

  In an interactive shell, if a command is interrupted, control will be
  returned to the prompt. If the prompt is idle, it will do nothing.
  If a command is interrupted in a non-interactive shell, the shell itself is
  expected to terminate once the command it was being executed finishes.
  In that case, the shell should terminate with a SIGINT itself, to propagate
  the interruption to its parent process.

  ### Handling Interruptions

  Generally, only actions that may take a long time to execute need to be
  interruptible, like loops or calls to external programs, as well as
  I/O ops that can block for a while (specially network ops).

  How to interrupt the code depends on the action being executed.

  At any moment, one of the following types of code can be in execution:
  - application code (userlevel code on our side)
  - library code (userlevel code outside of our own code)
  - system calls (kernel level code)

  (of course, in the end everything is application code, but the call stack will
  have different things interleaved)

  #### Interrupting Application Code

  To interrupt application code, setting a flag that is used to break out of
  loops and make the routine being executed return as soon as possible is the
  usual method. If the application code is waiting on something else to finish,
  it may actively cancel that action using whatever methods are relevant.

  For example, a MySQL query being executed should be killed on the server side
  to trigger a killed query error at the server, which would bubble up to the
  user as MySQL error. Simply closing the connection or handling it at the
  client side would leave the query running in the server, which may not be
  wanted (if it was an accidental DELETE FROM TABLE, for example).

  #### Interrupting syscalls

  Interrupting system calls is relatively simple, because the syscall will
  be interrupted by the kernel when a SIGINT is received, causing them to
  return with an EINTR error. But that assumes the interruption is being
  triggered by ^C in the first place, which may not be case if the shell has a
  custom frontend or in Windows. Also, syscalls are usually buried deep inside
  other libraries, it's up to these libraries how EINTR is handled. Some times
  the syscall is simply retried on EINTR (SIGINT is not the only signal that
  can cause them).

  #### Interrupting external code

  Interrupting function calls in external libraries is the most challenging.

  Things are simple if the library provides a method for interrupting itself.
  Otherwise, it may be harder or impossible to interrupt it, other than killing
  the whole process.

  If the library calls a syscall, the notes for syscalls apply.

  Sometimes, the function that needs to be interrupted have alternative lower
  level functions that allow the work it does to be performed in smaller chunks.
  If that's the case you may implement the loop that processes these chunks in
  your own code, which could then have a flag for breaking out of it.

  If the external code is in fact an external process / helper tool, then the
  external process will be interrupted by the original SIGINT. The calling
  program then just has to detect that the child died because of a SIGINT and
  properly cancel the rest of whatever it was doing. The child process must
  correctly handle SIGINT itself.

  #### Interrupting MySQL Queries

  A ^C during a MySQL query is being executed is a case of external code.

  If a script is interrupted and it happens to be executing a DB operation,
  the desired outcome is that the DB operation finishes or cancels as soon as
  possible and that the script terminates.

  As for a nested interrupted DB op, it can fall in one of these subcases:

  - Query is actually killed by server (a long query being executed).
    Can be simulated with smth like SELECT * FROM arealtable WHERE sleep(10)
    Results in a error 1317
  - Query is interrupted by the server but not really killed
    Seems to happen on sleep, like SELECT sleep(10);
    The query finishes without an error, it just stops waiting.
  - The KILL QUERY fails to catch the query, for example, if the KILL was sent
    before the query actually started executing or right after it's done.
    Nothing will happen.
  - The KILL QUERY is sent after the query finished executed, but during
    transfer of the resultsete to the client (e.g. lots of rows).
    Nothing will happen.

  In the 1st case, the error would cause the script to stop, even if the script
  itself is not directly interrupted. In the other cases, the script must be
  interrupted, since the DB operation would finish. Furthermore, in these 3
  latter cases the result of the DB operation must be completely read and
  discarded internally, since the script that was going to process it will not
  execute.
*/

/** Initialize the interrupt manager

  Caller must pass a subclass of Interrupt_handler which performs platform
  specific signal-like actions.
*/

class Interrupts::Lock_guard final {
 public:
  explicit Lock_guard(Interrupts *parent) noexcept : m_parent(parent) {
    m_parent->lock();
  }

  Lock_guard(const Lock_guard &) = delete;
  Lock_guard(Lock_guard &&) = delete;

  Lock_guard &operator=(const Lock_guard &) = delete;
  Lock_guard &operator=(Lock_guard &&) = delete;

  ~Lock_guard() noexcept { m_parent->unlock(); }

 private:
  Interrupts *m_parent;
};

std::shared_ptr<Interrupts> Interrupts::create(Interrupt_helper *helper) {
  auto intr = std::shared_ptr<Interrupts>(new Interrupts(helper));
  intr->setup();
  return intr;
}

Interrupts::Interrupts(Interrupt_helper *helper) : m_helper(helper) {
  m_creator_thread_id = std::this_thread::get_id();
}

Interrupts::~Interrupts() {
  if (m_thread.joinable()) {
    terminate_thread();
  }
}

void Interrupts::setup() {
  if (m_helper) {
    m_helper->setup(this);
  }
}

bool Interrupts::in_creator_thread() const {
  return std::this_thread::get_id() == m_creator_thread_id;
}

void Interrupts::push_handler(const std::function<bool()> &signal_safe,
                              const std::function<void()> &signal_unsafe) {
  // Only allow cancellation handlers registered in the creator thread.
  // We don't want background threads to be affected directly by ^C
  // If you do want a background thread to be cancelled, the creator thread
  // should register a handler that will in turn result in the cancellation of
  // the background thread.
  if (!in_creator_thread()) {
    return;
  }

  Lock_guard guard{this};

  // Note: If you're getting a stack overflow pushing handlers, there may be
  // something wrong with how your code is working.
  // Interrupt handlers should be installed once per language. If code in a
  // scripting language can call code in the language, that should probably
  // not result in the installation of another interrupt handler.
  if (m_num_handlers >= k_max_interrupt_handlers) {
    throw std::logic_error(
        "Internal error: interrupt handler stack overflow during "
        "push_handler()");
  }

  if (!m_thread.joinable()) {
    // we cannot initialize thread in the constructor, as spawning the thread
    // copies the current interrupt handler
    init_thread();
  }

  m_safe_handlers[m_num_handlers] = signal_safe;
  m_unsafe_handlers[m_num_handlers++] = signal_unsafe;
}

void Interrupts::pop_handler() {
  if (!in_creator_thread()) {
    return;
  }

  Lock_guard guard{this};

  if (m_num_handlers == 0) {
    throw std::logic_error(
        "Internal error: interrupt handler stack underflow during "
        "pop_handler()");
  }

  m_safe_handlers[--m_num_handlers] = 0;
  m_unsafe_handlers[m_num_handlers] = 0;
}

void Interrupts::interrupt() {
  // lock this object, is can be unlocked only after asynchronous handling is
  // done
#if MYSQLSH_SYNCHRONOUS_SIGNAL_DELIVERY
  // signal is delivered from the main thread, if this happens when we're
  // locked, we're going to deadlock
  if (!try_lock()) {
    // handler is being added/removed or asynchronous callbacks are still being
    // executed, ignore the signal
    return;
  }
#else   // !MYSQLSH_SYNCHRONOUS_SIGNAL_DELIVERY
  // signal is delivered from a new thread, we can lock here
  lock();
#endif  // !MYSQLSH_SYNCHRONOUS_SIGNAL_DELIVERY

  if (m_num_handlers > 0) {
    try {
      auto stop_at = m_num_handlers;

      for (auto i = stop_at - 1; i >= 0; --i) {
        --stop_at;

        if (!m_safe_handlers[i]()) {
          break;
        }
      }

      write_to_thread(stop_at);
      return;
    } catch (const std::exception &e) {
      // logging is not signal safe, but this is an extraordinary situation
      log_error("Unexpected exception in safe interrupt handler: %s", e.what());
      assert(0);
      // this is a signal handler, we cannot throw here
    }
  }

  // there are no handlers, or abnormal situation, just unlock right away
  unlock();
}

void Interrupts::wait(uint32_t ms) { m_sigint_event.wait(ms); }

void Interrupts::lock() noexcept {
  // simple spin-lock
  while (m_lock.test_and_set(std::memory_order_acquire)) {
    while (m_lock.test(std::memory_order_relaxed)) {
    }
  }
}

bool Interrupts::try_lock() noexcept {
  if (m_lock.test_and_set(std::memory_order_acquire)) {
    return false;
  }

  return true;
}

void Interrupts::unlock() noexcept { m_lock.clear(std::memory_order_release); }

void Interrupts::init_thread() {
  if (0 != ::pipe(m_pipe)) {
    throw std::runtime_error("Interrupts: failed to initialize pipe");
  }

  m_thread = mysqlsh::spawn_scoped_thread([this]() { background_thread(); });
}

void Interrupts::terminate_thread() {
  write_to_thread(-1);
  m_thread.join();

  ::close(m_pipe[0]);
  ::close(m_pipe[1]);
}

void Interrupts::background_thread() {
  const auto fd = m_pipe[0];
  int ret;
  std::int8_t stop_at;

  const auto report_error = [](const char *msg, const char *context = nullptr) {
    log_error("Interrupts: %s: %s", msg,
              context ? context : shcore::get_last_error().c_str());
  };

  while (true) {
    ret = ::read(fd, &stop_at, sizeof(stop_at));

    if (ret < 0) {
      if (EINTR == errno) {
        continue;
      } else {
        report_error("failed to read");
        return;
      }
    } else if (0 == ret) {
      // end of file
      break;
    } else if (1 == ret) {
      if (stop_at < 0) {
        // terminate
        break;
      } else {
        try {
          for (auto i = m_num_handlers - 1; i >= stop_at; --i) {
            if (m_unsafe_handlers[i]) {
              m_unsafe_handlers[i]();
            }
          }
        } catch (const std::exception &e) {
          report_error("exception in unsafe interrupt handler", e.what());
          assert(0);
          // this is not a fatal error in release builds
        }

        // wake up any waiting threads
        m_sigint_event.interrupt();

        // asynchronous processing is done, unlock the handler
        unlock();
      }
    } else {
      report_error("read returned unexpected value",
                   std::to_string(ret).c_str());
      return;
    }
  }
}

bool Interrupts::write_to_thread(std::int8_t code) {
  return 1 == ::write(m_pipe[1], &code, 1);
}

Interrupt_handler::Interrupt_handler(
    const std::function<bool()> &signal_safe,
    const std::function<void()> &signal_unsafe) {
  current_interrupt()->push_handler(signal_safe, signal_unsafe);
}

Interrupt_handler::~Interrupt_handler() { current_interrupt()->pop_handler(); }

}  // namespace shcore
