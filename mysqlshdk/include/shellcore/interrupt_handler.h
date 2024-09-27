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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

#include "mysqlshdk/include/shellcore/sigint_event.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"

namespace shcore {

class Interrupts;

class Interrupt_helper {
 public:
  virtual ~Interrupt_helper() = default;
  virtual void setup(Interrupts *) = 0;
};

class Interrupts final {
 public:
  static std::shared_ptr<Interrupts> create(Interrupt_helper *helper);

  ~Interrupts();

  void interrupt();

  void wait(uint32_t ms);

 private:
  friend class Interrupt_handler;

#ifdef FRIEND_TEST
  FRIEND_TEST(Interrupt, basics);
#endif

  class Lock_guard;

  explicit Interrupts(Interrupt_helper *helper);

  void push_handler(const std::function<bool()> &signal_safe,
                    const std::function<void()> &signal_unsafe);
  void pop_handler();

  bool in_creator_thread() const;

  void setup();

  void lock() noexcept;
  bool try_lock() noexcept;
  void unlock() noexcept;

  void init_thread();
  void terminate_thread();
  void background_thread();
  bool write_to_thread(std::int8_t code);

  // Max number of SIGINT handlers to allow at a time in the stack
  static constexpr std::uint8_t k_max_interrupt_handlers = 8;

  Interrupt_helper *m_helper;
  std::thread::id m_creator_thread_id;

  std::function<bool()> m_safe_handlers[k_max_interrupt_handlers];
  std::function<void()> m_unsafe_handlers[k_max_interrupt_handlers];
  // this counts the number of handlers in both arrays above
  std::int8_t m_num_handlers = 0;

  shcore::atomic_flag m_lock;

  Sigint_event m_sigint_event;

  std::thread m_thread;
  int m_pipe[2] = {0};
};

std::shared_ptr<Interrupts> current_interrupt(bool allow_empty = false);

class Interrupt_handler final {
 public:
  Interrupt_handler() = delete;

  /**
   * The first callback may only execute async-signal-safe code, i.e. read from
   * and write to lock-free atomic objects and variables of type `volatile
   * std::sig_atomic_t`, call async-safe functions (man 7 signal-safety). Note
   * that std::atomic is not guaranteed to be lock-free (only std::atomic_flag
   * is, but unfortunately std::atomic_flag::test() is not available in GCC10).
   *
   * The second callback may execute any code, it's going to be asynchronously
   * called once the signal-safe callbacks are done.
   */
  explicit Interrupt_handler(const std::function<bool()> &signal_safe,
                             const std::function<void()> &signal_unsafe = {});

  Interrupt_handler(const Interrupt_handler &) = delete;
  Interrupt_handler(Interrupt_handler &&) = delete;

  Interrupt_handler &operator=(const Interrupt_handler &) = delete;
  Interrupt_handler &operator=(Interrupt_handler &&) = delete;

  ~Interrupt_handler();
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
