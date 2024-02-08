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

#ifndef _SHELLCORE_JS_H_
#define _SHELLCORE_JS_H_

#include <string>
#include <vector>

#include "shellcore/shell_core.h"

namespace shcore {
class JScript_context;

class Shell_javascript : public Shell_language {
 public:
  explicit Shell_javascript(Shell_core *shcore);

  void set_result_processor(
      std::function<void(shcore::Value, bool)> result_processor);

  std::function<void(shcore::Value, bool)> result_processor() const {
    return _result_processor;
  }

  void set_global(const std::string &name, const Value &value) override;

  void set_argv(const std::vector<std::string> &argv = {}) override;

  void handle_input(std::string &code, Input_state &state) override;

  void flush_input(const std::string &code) override;

  std::shared_ptr<JScript_context> javascript_context() { return _js; }

  std::string get_continued_input_context() override;

  bool load_plugin(const Plugin_definition &plugin) override;

 private:
  void abort() noexcept;
  std::shared_ptr<JScript_context> _js;
  std::function<void(shcore::Value, bool)> _result_processor;

  void handle_input(std::string &code, bool flush);
};
}  // namespace shcore

#endif
