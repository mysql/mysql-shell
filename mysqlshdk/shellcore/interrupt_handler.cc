/*
 * Copyright (c) 2014, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/interrupt_handler.h"

#include <cassert>
#include <stdexcept>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/sigint_event.h"

namespace shcore {

Interrupt_helper *Interrupts::_helper = nullptr;
std::function<bool()> Interrupts::_handlers[kMax_interrupt_handlers];
std::atomic<int> Interrupts::_num_handlers;
std::mutex Interrupts::_handler_mutex;
bool Interrupts::_propagates_interrupt = false;
std::thread::id Interrupts::_main_thread_id;
thread_local bool Interrupts::_ignore_current_thread = false;

/** User interruption (^C) handler. Called by SIGINT handler in console shell.

  ### User Interruption Handling Mechanics

  Program must provide an interrupt handling helper object (Interrupt_handler).
  In a console program, setup() would install a signal handler that calls
  Interrupts::interrupt() on SIGINT. It must also implement block() and
  unblock(), which would block and unblock SIGINT from being delivered
  (using sigprocmask). unblock() takes a flag, which tells whether pending
  signals (if any) should be flushed when unblocking.

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

  #### Blocking interruptions

  Sometimes it may be necessary to block interruptions, for example, during
  a critical operation. In that case, the Interrupts::block() function must be
  called. If a SIGINT is received during block(), it will be queued up for
  delivery on unblock(). unblock() accepts a boolean arg and if it is true, it
  will clear any pending SIGINTs.

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

  MUST be called from the main thread (the thread where user actions are
  executed, as opposed to background threads)
  */
void Interrupts::init(Interrupt_helper *helper) {
  _helper = helper;
  _main_thread_id = std::this_thread::get_id();
}

void Interrupts::ignore_thread() { _ignore_current_thread = true; }

void Interrupts::setup() {
  if (_helper) {
    _helper->setup();
  }
}

bool Interrupts::in_main_thread() {
  return std::this_thread::get_id() == _main_thread_id;
}

void Interrupts::push_handler(const std::function<bool()> &handler) {
  // The interrupt handler can be disabled by thread,
  // if that's the case this is a no-op
  if (_ignore_current_thread) return;

  // Only allow cancellation handlers registered in the main thread.
  // We don't want background threads to be affected directly by ^C
  // If you do want a background thread to be cancelled, the main thread should
  // register a handler that will in turn result in the cancellation of the
  // background thread.
  if (!in_main_thread()) {
    return;
  }

  // Note: If you're getting a stack overflow pushing handlers, there may be
  // something wrong with how your code is working.
  // Interrupt handlers should be installed once per language. If code in a
  // scripting language can call code in the language, that should probably
  // not result in the installation of another interrupt handler.
  int n = _num_handlers.load();
  if (n >= kMax_interrupt_handlers) {
    throw std::logic_error(
        "Internal error: interrupt handler stack overflow during "
        "push_interrupt_handler()");
  }
  _handlers[_num_handlers++] = handler;
}

void Interrupts::pop_handler() {
  // The interrupt handler can be disabled by thread,
  // if that's the case this is a no-op
  if (_ignore_current_thread) return;

  if (!in_main_thread()) {
    return;
  }
  std::lock_guard<std::mutex> lock(_handler_mutex);
  int n = _num_handlers.load();
  if (n == 0) {
    throw std::logic_error(
        "Internal error: interrupt handler stack underflow during "
        "pop_interrupt_handler()");
  }
  _handlers[--_num_handlers] = 0;
}

void Interrupts::interrupt() {
  // The interrupt handler can be disabled by thread,
  // if that's the case this is a no-op
  if (_ignore_current_thread) return;

  {
    Block_interrupts block_ints;
    std::lock_guard<std::mutex> lock(_handler_mutex);
    int n = _num_handlers.load();
    if (n > 0) {
      for (int i = n - 1; i >= 0; --i) {
        try {
          if (!_handlers[i]()) break;
        } catch (const std::exception &e) {
          log_error("Unexpected exception in interruption handler: %s",
                    e.what());
          assert(0);
          throw;
        }
      }
      block_ints.clear_pending(true);
    }
  }

  shcore::Sigint_event::get().interrupt();
}

void Interrupts::set_propagate_interrupt(bool flag) {
  _propagates_interrupt = flag;
}

bool Interrupts::propagates_interrupt() { return _propagates_interrupt; }

void Interrupts::block() {
  if (_helper) _helper->block();
}

void Interrupts::unblock(bool clear_pending) {
  if (_helper) _helper->unblock(clear_pending);
}

Interrupt_handler::Interrupt_handler(const std::function<bool()> &handler,
                                     bool skip)
    : m_skip(skip) {
  if (!m_skip) Interrupts::push_handler(handler);
}

Interrupt_handler::~Interrupt_handler() {
  if (!m_skip) Interrupts::pop_handler();
}

}  // namespace shcore
