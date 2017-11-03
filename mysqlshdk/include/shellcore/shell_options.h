/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_

#define SHCORE_OUTPUT_FORMAT "outputFormat"
#define SHCORE_INTERACTIVE "interactive"
#define SHCORE_SHOW_WARNINGS "showWarnings"
#define SHCORE_BATCH_CONTINUE_ON_ERROR "batchContinueOnError"
#define SHCORE_USE_WIZARDS "useWizards"
// AdminAPI: Gadgets path
// TODO(Miguel): which will be the path? How do we get it?
#define SHCORE_GADGETS_PATH "gadgetsPath"
#define SHCORE_SANDBOX_DIR "sandboxDir"

#define SHCORE_HISTORY_MAX_SIZE "history.maxSize"
#define SHCORE_HISTIGNORE "history.sql.ignorePattern"
#define SHCORE_HISTORY_AUTOSAVE "history.autoSave"

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/options.h"
#include "shellcore/ishell_core.h"

namespace mysqlsh {

class Shell_options : protected shcore::Options {
 public:
  struct Storage {
   public:
    Storage();

    shcore::IShell_core::Mode initial_mode;
    std::string run_file;

    // Individual connection parameters
    std::string user;
    std::string pwd;
    const char* password;
    std::string host;
    int port;
    std::string schema;
    std::string sock;  //< Unix socket or Windows pipe name
    std::string auth_method;

    std::string protocol;

    // SSL connection parameters
    mysqlshdk::db::Ssl_options ssl_options;

    std::string uri;

    std::string output_format;
    mysqlsh::SessionType session_type;
    bool default_session_type;
    bool print_cmd_line_helper;
    bool print_version;
    bool print_version_extra;
    bool force;
    bool interactive;
    bool full_interactive;
    bool passwords_from_stdin;
    bool prompt_password;
    bool recreate_database;
    bool show_warnings;
    bool trace_protocol;
    bool log_to_stderr;
    std::string execute_statement;
    std::string execute_dba_statement;
    std::string sandbox_directory;
    std::string gadgets_path;
    ngcommon::Logger::LOG_LEVEL log_level;
    bool wizards;
    bool admin_mode;
    std::string histignore;
    int history_max_size;
    bool history_autosave = false;

    // cmdline params to be passed to script
    std::vector<std::string> script_argv;

    int exit_code;

    bool has_connection_data() const;

    mysqlshdk::db::Connection_options connection_options() const;
  };

  explicit Shell_options(int argc = 0, char** argv = nullptr);

  void set(const std::string& option, const shcore::Value value);

  shcore::Value get(const std::string& option);

  const Storage& get() {
    return storage;
  }

  bool has_key(const std::string& option) const {
    return named_options.find(option) != named_options.end();
  }

  void set_interactive(bool value) {
    storage.interactive = value;
  }

  void set_wizards(bool value) {
    storage.wizards = value;
  }

  std::vector<std::string> get_details() {
    return get_cmdline_help(28, 50);
  }

  std::vector<std::string> get_named_options();

 protected:
  bool custom_cmdline_handler(char** argv, int* argi);

  void override_session_type(const std::string& option, const char* value);

  void check_session_type_conflicts();
  void check_user_conflicts();
  void check_password_conflicts();
  void check_host_conflicts();
  void check_host_socket_conflicts();
  void check_port_conflicts();
  void check_socket_conflicts();
  void check_port_socket_conflicts();

  Storage storage;
  mysqlshdk::db::Connection_options uri_data;
  std::string session_type_arg;
};
}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
