/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "mysqlshdk/include/shellcore/sigint_event.h"

namespace shcore {

class Interrupt_helper {
 public:
  virtual ~Interrupt_helper() = default;
  virtual void setup() = 0;
  virtual void block() = 0;
  virtual void unblock(bool clean_pending) = 0;
};

class Interrupts final {
  // Max number of SIGINT handlers to allow at a time in the stack
  static constexpr int k_max_interrupt_handlers = 8;

 public:
  static std::shared_ptr<Interrupts> create(Interrupt_helper *helper);
  void setup();

  bool in_creator_thread();

  void push_handler(const std::function<bool()> &handler);
  void pop_handler();

  void set_propagate_interrupt(bool flag);
  bool propagates_interrupt();

  void block();
  void unblock(bool clear_pending = false);

  void interrupt();

  void wait(uint32_t ms);

 private:
  explicit Interrupts(Interrupt_helper *helper);

  Interrupt_helper *m_helper;
  std::function<bool()> m_handlers[k_max_interrupt_handlers];
  int m_num_handlers;
  std::mutex m_handler_mutex;
  bool m_propagates_interrupt;
  std::thread::id m_creator_thread_id;
  Sigint_event s_sigint_event;
};

std::shared_ptr<Interrupts> current_interrupt(bool allow_empty = false);

struct Block_interrupts {
  explicit Block_interrupts(bool clear_on_unblock = false)
      : m_clear_on_unblock(clear_on_unblock) {
    current_interrupt()->block();
  }

  ~Block_interrupts() { current_interrupt()->unblock(m_clear_on_unblock); }

  void clear_pending(bool flag = true) { m_clear_on_unblock = flag; }

  bool m_clear_on_unblock;
};

class Interrupt_handler final {
 public:
  Interrupt_handler() = delete;

  explicit Interrupt_handler(const std::function<bool()> &handler,
                             bool skip = false);

  Interrupt_handler(const Interrupt_handler &) = delete;
  Interrupt_handler(Interrupt_handler &&) = delete;

  Interrupt_handler &operator=(const Interrupt_handler &) = delete;
  Interrupt_handler &operator=(Interrupt_handler &&) = delete;

  ~Interrupt_handler();

 private:
  bool m_skip;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
