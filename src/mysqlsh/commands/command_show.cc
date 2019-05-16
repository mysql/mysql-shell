/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "src/mysqlsh/commands/command_show.h"

#include <algorithm>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"

namespace mysqlsh {

bool Command_show::execute(const std::vector<std::string> &args) {
  if (args.size() == 1) {
    // no arguments -> display available reports
    list_reports();
  } else {
    const auto session = _shell->get_dev_session();
    shcore::Interrupt_handler inth([&session]() {
      if (session) {
        session->kill_query();
      }
      return true;
    });
    current_console()->print(m_reports->call_report(
        args[1], session, {args.begin() + 2, args.end()}));
  }

  return true;
}

void Command_show::list_reports() const {
  auto list = m_reports->list_reports();
  std::sort(list.begin(), list.end());
  current_console()->println(
      "Available reports: " + shcore::str_join(list, ", ") + ".");
}

}  // namespace mysqlsh
