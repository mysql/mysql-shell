/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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
#ifndef SRC_MYSQLSH_COMMANDS_COMMAND_WATCH_H_
#define SRC_MYSQLSH_COMMANDS_COMMAND_WATCH_H_

#include <memory>
#include <string>
#include <vector>

#include "src/mysqlsh/commands/command_show.h"

namespace mysqlsh {

class Command_watch : public Command_show {
 public:
  Command_watch(const std::shared_ptr<shcore::IShell_core> &shell,
                const std::shared_ptr<Shell_reports> &reports)
      : Command_show(shell, reports) {}

  bool execute(const std::vector<std::string> &args) override;

 private:
  std::vector<std::string> parse_arguments(
      const std::vector<std::string> &args);

  // default values of options handled by \watch
  bool m_clear_screen = true;

  float m_refresh_interval = 2.0f;
};

}  // namespace mysqlsh

#endif  // SRC_MYSQLSH_COMMANDS_COMMAND_WATCH_H_
