/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "src/mysqlsh/commands/command_edit.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "src/mysqlsh/history.h"

namespace mysqlsh {

namespace {

std::string select_editor() {
  const char *editor = nullptr;

  if (!(editor = getenv("EDITOR")) && !(editor = getenv("VISUAL"))) {
    editor =
#ifdef _WIN32
        "notepad.exe";
#else   // !_WIN32
        "vi";
#endif  // !_WIN32
  }

  return editor;
}

}  // namespace

bool Command_edit::execute(const std::vector<std::string> &args) {
  std::string command;
  const auto &full_command = args[0];
  auto pos = full_command.find(' ');

  if (std::string::npos != pos) {
    while (std::isspace(full_command[pos])) {
      ++pos;
    }

    // we're using the full command to avoid problems with quoting
    command = full_command.substr(pos);
  }

  return execute(command);
}

bool Command_edit::execute(const std::string &command) {
  const auto final_command = get_command(command);

  const auto temp_file = shcore::get_tempfile_path(
      shcore::path::join_path(shcore::path::tmpdir(), "mysqlsh.edit"));

  const auto finally = shcore::on_leave_scope([&temp_file]() {
    if (shcore::is_file(temp_file)) {
      shcore::delete_file(temp_file);
    }
  });

  if (!shcore::create_file(temp_file, "")) {
    throw std::runtime_error("Failed to create temporary file: " + temp_file);
  }

  if (0 != shcore::set_user_only_permissions(temp_file)) {
    throw std::runtime_error("Failed to set permissions of temporary file: " +
                             temp_file);
  }

  if (!shcore::create_file(temp_file, final_command)) {
    throw std::runtime_error("Failed to write to temporary file: " + temp_file);
  }

  const auto editor = select_editor();
  const auto editor_command = editor + " " + temp_file;

  const int status = system(editor_command.c_str());
  std::string error;

  if (!shcore::verify_status_code(status, &error)) {
    current_console()->print_error("Editor \"" + editor + "\" " + error + ".");
  } else {
    m_cmdline->set_next_input(shcore::get_text_file(temp_file));
  }

  return true;
}

std::string Command_edit::get_command(const std::string &command) const {
  if (command.empty() && m_history->size() > 0) {
    return m_history->get_entry(m_history->last_entry());
  } else {
    return command;
  }
}

}  // namespace mysqlsh
