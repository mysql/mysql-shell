/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_POLYGLOT_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_POLYGLOT_H_

#include <condition_variable>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/scripting/polyglot/polyglot_context.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_utils.h"
#include "shellcore/shell_core.h"

namespace shcore {

class Shell_polyglot : public Shell_language {
 public:
  explicit Shell_polyglot(Shell_core *shcore, polyglot::Language type);
  ~Shell_polyglot() override;

  void set_result_processor(
      std::function<void(shcore::Value, bool)> result_processor);

  const std::function<void(shcore::Value, bool)> &result_processor() const {
    return m_result_processor;
  }

  void set_global(const std::string &name, const Value &value) override;

  void set_argv(const std::vector<std::string> &argv = {}) override;

  void handle_input(std::string &code, Input_state &state) override;

  void flush_input(const std::string &code) override;

  std::shared_ptr<polyglot::Polyglot_context> polyglot_context() {
    return m_polyglot;
  }

  std::string get_continued_input_context() override;

  bool load_plugin(const Plugin_definition &plugin) override;

 private:
  void abort() noexcept;
  void handle_input(std::string &code, bool flush);

  // The following conditions should be met for a proper interruption in
  // graalvm:
  // - Interruption should be done from a thread other than main
  // - The interruption should not block the main thread
  // - A poly_context reference is needed so it is available on both threads
  //
  // The solution is to start and sleep the graal interruption handler thread
  // which will be only activated in 2 scenarios:
  // - When an interruption occurs while handling input
  // - When finalizing the Shell_polyglot
  std::shared_ptr<polyglot::Polyglot_context> m_polyglot;
  std::function<void(shcore::Value, bool)> m_result_processor;
  std::mutex m_interrupt_handler_mutex;
  std::condition_variable m_interrupt_handler_condition;
  std::thread m_interrupt_handler_thread;
  bool m_tear_down = false;
  bool m_interrupt = false;
};
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_POLYGLOT_H_
