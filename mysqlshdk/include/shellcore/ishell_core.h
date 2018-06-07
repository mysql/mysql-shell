/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _ISHELL_CORE_
#define _ISHELL_CORE_

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/enumset.h"
#include "scripting/lang_base.h"
#include "scripting/object_registry.h"
#include "scripting/types.h"
#include "scripting/types_common.h"

namespace mysqlsh {
class ShellBaseSession;
};

namespace shcore {
class Help_manager;
class SHCORE_PUBLIC IShell_core {
 public:
  enum class Mode { None = 0, SQL = 1, JavaScript = 2, Python = 3 };
  typedef mysqlshdk::utils::Enum_set<Mode, Mode::Python> Mode_mask;

  static Mode_mask all_scripting_modes() {
    return Mode_mask(Mode::JavaScript).set(Mode::Python);
  }

  IShell_core();
  virtual ~IShell_core();

  virtual Mode interactive_mode() const = 0;
  virtual bool switch_mode(Mode mode, bool &lang_initialized) = 0;

  // By default, globals apply to the three languages
  virtual void set_global(const std::string &name, const Value &value,
                          Mode_mask mode = Mode_mask::any()) = 0;
  virtual Value get_global(const std::string &name) = 0;

  virtual Object_registry *registry() = 0;
  virtual void handle_input(std::string &code, Input_state &state) = 0;
  virtual bool handle_shell_command(const std::string &code) = 0;
  virtual std::string get_handled_input() = 0;
  virtual int process_stream(std::istream &stream, const std::string &source,
                             const std::vector<std::string> &argv) = 0;

  // Development Session Handling
  virtual std::shared_ptr<mysqlsh::ShellBaseSession> set_dev_session(
      const std::shared_ptr<mysqlsh::ShellBaseSession> &session) = 0;
  virtual std::shared_ptr<mysqlsh::ShellBaseSession> get_dev_session() = 0;

  virtual const std::string &get_input_source() = 0;
  virtual const std::vector<std::string> &get_input_args() = 0;

  virtual Help_manager *get_helper() = 0;
  virtual std::vector<std::string> get_global_objects(Mode mode) = 0;
};

std::string to_string(const IShell_core::Mode mode);
IShell_core::Mode parse_mode(const std::string &value);

}  // namespace shcore

#endif
