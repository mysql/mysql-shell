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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace shcore {

// Max number of SIGINT handlers to allow at a time in the stack
static constexpr int kMax_interrupt_handlers = 8;

class Interrupt_helper {
 public:
  virtual ~Interrupt_helper() = default;
  virtual void setup() = 0;
  virtual void block() = 0;
  virtual void unblock(bool clean_pending) = 0;
};

class Interrupts {
 public:
  static void init(Interrupt_helper *helper);

  static void setup();

  static bool in_main_thread();

  static void push_handler(std::function<bool()> handler);
  static void pop_handler();

  static void set_propagate_interrupt(bool flag);
  static bool propagates_interrupt();

  static void block();
  static void unblock(bool clear_pending = false);

  static void interrupt();
  static void ignore_thread();

 private:
  static Interrupt_helper *_helper;
  static std::function<bool()> _handlers[kMax_interrupt_handlers];
  static std::atomic<int> _num_handlers;
  static std::mutex _handler_mutex;
  static bool _propagates_interrupt;
  static std::thread::id _main_thread_id;
  static thread_local bool _ignore_current_thread;
};

struct Block_interrupts {
  explicit Block_interrupts(bool clear_on_unblock = false)
      : clear_on_unblock_(clear_on_unblock) {
    Interrupts::block();
  }

  ~Block_interrupts() { Interrupts::unblock(clear_on_unblock_); }

  void clear_pending(bool flag = true) { clear_on_unblock_ = flag; }

  bool clear_on_unblock_;
};

struct Interrupt_handler {
  explicit Interrupt_handler(std::function<bool()> handler, bool skip = false) {
    _skip = skip;
    if (!skip) Interrupts::push_handler(handler);
  }

  ~Interrupt_handler() {
    if (!_skip) Interrupts::pop_handler();
  }

  bool _skip;
};

std::shared_ptr<Interrupt_helper> current_interrupt_helper();
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HANDLER_H_
