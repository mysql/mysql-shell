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

#include "mysql-secret-store/login-path/config_editor_invoker.h"

#include "mysql-secret-store/include/helper.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysql {
namespace secret_store {
namespace login_path {

using mysql::secret_store::common::Helper_exception;

namespace {

constexpr auto k_config_editor = "mysql_config_editor";

}  // namespace

void Config_editor_invoker::validate() {
  try {
    invoke({"--help"});
  } catch (const std::exception &ex) {
    throw Helper_exception{"Failed to execute \"" +
                           std::string{k_config_editor} +
                           "\", make sure it is installed and available in the "
                           "PATH environment variable. Internal error: " +
                           ex.what()};
  }
}

std::string Config_editor_invoker::list() { return invoke({"print", "--all"}); }

void Config_editor_invoker::store(const Entry &entry,
                                  const std::string &password) {
  std::vector<std::string> args = {"set", "--skip-warn", "--password"};
  args.emplace_back("--login-path=" + entry.name);

  if (!entry.user.empty()) {
    args.emplace_back("--user=" + entry.user);
  }

  if (!entry.host.empty()) {
    args.emplace_back("--host=" + entry.host);
  }

  if (!entry.port.empty()) {
    args.emplace_back("--port=" + entry.port);
  }

  if (!entry.socket.empty()) {
    args.emplace_back("--socket=" + entry.socket);
  }

  invoke(args, true, password);
}

void Config_editor_invoker::erase(const Entry &entry) {
  std::vector<std::string> args = {"remove"};
  args.emplace_back("--login-path=" + entry.name);

  invoke(args);
}

void Config_editor_invoker::erase_port(const Entry &entry) {
  std::vector<std::string> args = {"remove", "--port"};
  args.emplace_back("--login-path=" + entry.name);

  invoke(args);
}

void Config_editor_invoker::erase_socket(const Entry &entry) {
  std::vector<std::string> args = {"remove", "--socket"};
  args.emplace_back("--login-path=" + entry.name);

  invoke(args);
}

std::string Config_editor_invoker::invoke(const std::vector<std::string> &args,
                                          bool uses_terminal,
                                          const std::string &password) const {
  std::vector<const char *> process_args;
  process_args.emplace_back(k_config_editor);

  for (const auto &arg : args) {
    process_args.emplace_back(arg.c_str());
  }

  process_args.emplace_back(nullptr);

  shcore::Process_launcher app{&process_args[0]};

  if (uses_terminal) {
    app.enable_child_terminal();
  }

  app.start();

  if (uses_terminal) {
    // wait for password prompt
    app.read_from_terminal();
    app.write_to_terminal(password.c_str(), password.length());
    app.write_to_terminal("\n", 1);
  }

  std::string output = shcore::str_strip(app.read_all());
  const auto exit_code = app.wait();

  if (0 != exit_code) {
    throw Helper_exception{"Process \"" + std::string{process_args[0]} +
                           "\" finished with exit code " +
                           std::to_string(exit_code) + ": " + output};
  }

  return output;
}

}  // namespace login_path
}  // namespace secret_store
}  // namespace mysql
