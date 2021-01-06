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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_

#define SN_SHELL_OPTION_CHANGED "SN_SHELL_OPTION_CHANGED"

#define SHCORE_RESULT_FORMAT "resultFormat"
#define SHCORE_INTERACTIVE "interactive"
#define SHCORE_SHOW_WARNINGS "showWarnings"
#define SHCORE_BATCH_CONTINUE_ON_ERROR "batchContinueOnError"
#define SHCORE_USE_WIZARDS "useWizards"

#define SHCORE_SANDBOX_DIR "sandboxDir"
#define SHCORE_DBA_GTID_WAIT_TIMEOUT "dba.gtidWaitTimeout"
#define SHCORE_DBA_RESTART_WAIT_TIMEOUT "dba.restartWaitTimeout"
#define SHCORE_DBA_LOG_SQL "dba.logSql"

#define SHCORE_HISTORY_MAX_SIZE "history.maxSize"
#define SHCORE_HISTIGNORE "history.sql.ignorePattern"
#define SHCORE_HISTORY_SQL_SYSLOG "history.sql.syslog"
#define SHCORE_HISTORY_AUTOSAVE "history.autoSave"

#define SHCORE_DB_NAME_CACHE "autocomplete.nameCache"
#define SHCORE_DEVAPI_DB_OBJECT_HANDLES "devapi.dbObjectHandles"

#define SHCORE_PAGER "pager"

#define SHCORE_DEFAULT_COMPRESS "defaultCompress"

#define SHCORE_VERBOSE "verbose"
#define SHCORE_DEBUG "debug"

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/shellcore/shell_cli_operation.h"
#include "shellcore/ishell_core.h"

namespace mysqlsh {

class Shell_options : public shcore::Options {
 public:
  enum class Quiet_start { NOT_SET, SUPRESS_BANNER, SUPRESS_INFO };
  struct Storage {
    shcore::IShell_core::Mode initial_mode = shcore::IShell_core::Mode::None;
    std::string run_file;
    std::string run_module;

    // Individual connection parameters
    std::string user;
    std::string pwd;
    const char *password = nullptr;
    std::string host;
    int port = 0;
    std::string schema;
    // Unix socket or Windows pipe name
    mysqlshdk::utils::nullable<std::string> sock;
    bool oci_wizard = false;
    std::string oci_profile;
    std::string oci_config_file;
    std::string auth_method;
    std::string m_connect_timeout;
    std::string compress;
    std::string compress_algorithms;
    mysqlshdk::utils::nullable<int64_t> compress_level;

    std::string protocol;

    // SSL connection parameters
    mysqlshdk::db::Ssl_options ssl_options;

    std::string uri;

    std::string result_format;
    std::string wrap_json;
    mysqlsh::SessionType session_type = mysqlsh::SessionType::Auto;
    bool default_session_type = true;
    bool force = false;
    bool interactive = false;
    bool full_interactive = false;
    bool passwords_from_stdin = false;
    bool prompt_password = false;
    bool no_password = false;  //< Do not ask for password
    bool recreate_database = false;
    bool show_warnings = true;
    bool trace_protocol = false;
    bool log_to_stderr = false;
    bool devapi_schema_object_handles = true;
    bool db_name_cache = true;
    bool db_name_cache_set = false;
    std::string execute_statement;
    std::string execute_dba_statement;
    std::string sandbox_directory;
    int dba_gtid_wait_timeout;
    int dba_restart_wait_timeout;
    int dba_log_sql;
    shcore::Logger::LOG_LEVEL log_level = shcore::Logger::LOG_INFO;
    int verbose_level = 0;
    bool wizards = true;
    bool admin_mode = false;
    std::string histignore;
    int history_max_size = 1000;
    bool history_autosave = false;
    bool history_sql_syslog = false;
    enum class Redirect_to {
      None,
      Primary,
      Secondary
    } redirect_session = Redirect_to::None;
    std::string default_cluster;
    bool default_cluster_set = false;
    bool default_replicaset_set = false;
    bool get_server_public_key = false;
    std::string server_public_key_path;
    // cmdline params to be passed to script
    std::vector<std::string> script_argv;
    std::vector<std::string> import_args;
    std::vector<std::string> import_opts;
    std::string pager;
    Quiet_start quiet_start = Quiet_start::NOT_SET;
    bool show_column_type_info = false;
    bool default_compress = false;
    std::string dbug_options;

    // override default plugin search path ; separated in windows, : elsewhere
    mysqlshdk::null_string plugins_path;

    int exit_code = 0;

    bool has_connection_data() const;

    mysqlshdk::db::Connection_options connection_options() const;
  };

  explicit Shell_options(int argc = 0, char **argv = nullptr,
                         const std::string &configuration_file = "");

  void set(const std::string &option, const std::string &value) {
    Options::set(option, value);
  }
  void set(const std::string &option, const shcore::Value &value);
  void set_and_notify(const std::string &option, const std::string &value,
                      bool save_to_file = false);
  void set_and_notify(const std::string &option, const shcore::Value &value,
                      bool save_to_file = false);

  void unset(const std::string &option, bool save_to_file = false);

  shcore::Value get(const std::string &option);

  const Storage &get() { return storage; }

  bool has_key(const std::string &option) const {
    return named_options.find(option) != named_options.end();
  }

  shcore::cli::Shell_cli_operation *get_shell_cli_operation() {
    return m_shell_cli_operation.get();
  }

  void set_interactive(bool value) { storage.interactive = value; }

  void set_wizards(bool value) { storage.wizards = value; }

  void set_db_name_cache(bool value) { storage.db_name_cache = value; }

  void set_result_format(const std::string &format) {
    storage.result_format = format;
  }

  void set_oci_config_file(const std::string &path) {
    storage.oci_config_file = path;
  }

  std::vector<std::string> get_details() { return get_cmdline_help(32, 46); }

  bool action_print_help() const { return print_cmd_line_helper; }

  bool action_print_version() const { return print_cmd_line_version; }

  bool action_print_version_extra() const {
    return print_cmd_line_version_extra;
  }

  std::vector<std::string> get_named_options();

 protected:
  bool custom_cmdline_handler(Iterator *iterator);

  void override_session_type(const std::string &option, const char *value);
  void set_ssl_mode(const std::string &option, const char *value);
  void set_connection_timeout(const std::string &option, const char *value);

  void check_session_type_conflicts();
  void check_user_conflicts();
  void check_password_conflicts();
  void check_host_conflicts();
  void check_host_socket_conflicts();
  void check_port_conflicts();
  void check_socket_conflicts();
  void check_port_socket_conflicts();
  void check_result_format();
  void check_oci_conflicts();
  void check_file_execute_conflicts();

  /**
   * --import option require default schema to be provided in connection
   * options.
   */
  void check_import_options();

  Storage storage;
  std::unique_ptr<shcore::cli::Shell_cli_operation> m_shell_cli_operation;
  mysqlshdk::db::Connection_options uri_data;
  std::string session_type_arg;

  bool print_cmd_line_helper = false;
  bool print_cmd_line_version = false;
  bool print_cmd_line_version_extra = false;

 private:
  void notify(const std::string &option);
};

std::shared_ptr<Shell_options> current_shell_options(bool allow_empty = false);

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
