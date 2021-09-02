/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#ifndef SRC_MYSQLSH_JSON_SHELL_H_
#define SRC_MYSQLSH_JSON_SHELL_H_

#include <memory>
#include <string>
#include <utility>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "src/mysqlsh/cmdline_shell.h"

namespace mysqlsh {

class Json_shell : public Command_line_shell {
 public:
  explicit Json_shell(std::shared_ptr<Shell_options> options);
  Json_shell(std::shared_ptr<Shell_options> cmdline_options,
             std::unique_ptr<shcore::Interpreter_delegate> delegate);

  ~Json_shell() override = default;

  void process_line(const std::string &line) override;
};
}  // namespace mysqlsh
#endif  // SRC_MYSQLSH_JSON_SHELL_H_
