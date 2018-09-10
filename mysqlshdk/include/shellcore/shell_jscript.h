/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELLCORE_JS_H_
#define _SHELLCORE_JS_H_

#include <string>

#include "shellcore/shell_core.h"

namespace shcore {
class JScript_context;

class Shell_javascript : public Shell_language {
 public:
  explicit Shell_javascript(Shell_core *shcore);

  void set_result_processor(
      std::function<void(shcore::Value, bool)> result_processor);

  virtual void set_global(const std::string &name, const Value &value);

  virtual void handle_input(std::string &code, Input_state &state);

  std::shared_ptr<JScript_context> javascript_context() { return _js; }

  virtual void clear_input();
  virtual std::string get_continued_input_context();

 private:
  void abort() noexcept;
  std::shared_ptr<JScript_context> _js;
  std::function<void(shcore::Value, bool)> _result_processor;
  Input_state m_last_input_state;
};
};  // namespace shcore

#endif
