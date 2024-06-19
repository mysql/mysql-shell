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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_

#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/utils_general.h"

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
#define SHCORE_DBA_CONNECTIVITY_CHECKS "dba.connectivityChecks"
#define SHCORE_DBA_VERSION_COMPATIBILITY_CHECKS "dba.versionCompatibilityChecks"
#define SHCORE_LOG_FILE_NAME "logFile"
#define SHCORE_LOG_SQL "logSql"
#define SHCORE_LOG_SQL_IGNORE "logSql.ignorePattern"
#define SHCORE_LOG_SQL_IGNORE_UNSAFE "logSql.ignorePatternUnsafe"

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
#define SHCORE_MYSQL_PLUGIN_DIR "mysqlPluginDir"

#define SHCORE_CONNECT_TIMEOUT "connectTimeout"
#define SHCORE_DBA_CONNECT_TIMEOUT "dba.connectTimeout"

#define SHCORE_PROGRESS_REPORTING "progressReporting"

#include <stdlib.h>
#include <array>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/shellcore/shell_cli_operation.h"
#include "shellcore/ishell_core.h"

struct MEM_ROOT;

namespace mysqlsh {

class Shell_options final : public shcore::Options {
 public:
  enum class Quiet_start { NOT_SET, SUPRESS_BANNER, SUPRESS_INFO };

#ifdef _WIN32
  using ssize_t = __int64;
#endif  // _WIN32

  struct Ssh_settings {
    std::string known_hosts_file;
    std::string identity_file;
    std::string config_file;
    int timeout = 10;
    unsigned int buffer_size = 10240;
    std::string uri;
    std::string pwd;
    mysqlshdk::ssh::Ssh_connection_options uri_data;
  };
  struct Storage {
    shcore::IShell_core::Mode initial_mode = shcore::IShell_core::Mode::None;
    std::string run_file;
    std::string run_module;

    // Individual connection parameters
    std::string register_factor;
    std::optional<bool> webauthn_client_preserve_privacy;
    std::string oci_profile;
    std::string oci_config_file;

    mysqlshdk::db::Connection_options connection_data;
    Ssh_settings ssh;

    std::string result_format;
    std::string wrap_json;
    bool force = false;
    bool interactive = false;
    bool full_interactive = false;
    bool passwords_from_stdin = false;
    bool show_warnings = true;
    bool trace_protocol = false;
    bool log_to_stderr = false;
    bool devapi_schema_object_handles = true;
    bool db_name_cache = true;
    bool db_name_cache_set = false;
    std::string execute_statement;
    std::string execute_dba_statement;
    std::string sandbox_directory;
    int dba_gtid_wait_timeout = 60;
    int dba_restart_wait_timeout = 60;
    int dba_log_sql = 0;
    bool dba_connectivity_checks = false;
    bool dba_version_compatibility_checks = true;
    std::string log_sql;  //< Global SQL logging level
    std::string log_sql_ignore;
    std::string log_sql_ignore_unsafe;
    shcore::Logger::LOG_LEVEL log_level = shcore::Logger::LOG_INFO;
    std::string log_file;
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
    std::string pager;
    Quiet_start quiet_start = Quiet_start::NOT_SET;
    bool show_column_type_info = false;
    bool default_compress = false;
    std::string dbug_options;

    // override default plugin search path ; separated in windows, : elsewhere
    std::optional<std::string> plugins_path;
    std::string mysql_plugin_dir;

    double connect_timeout = 10.0;
    double dba_connect_timeout = 5.0;
    bool prompt_password = false;

    // This should probably a command line option that determines how much bytes
    // should be included when returning binary data, 0 means no limits
    // Eventually this should be turned as a command line argument, i.e.
    // --binary-limit
    size_t binary_limit = 0;

    // Indicates the Shell trunning for the GUI
    bool gui_mode = false;
    bool disable_user_plugins = false;

    // TODO(anyone): Expose the option
    enum class Progress_reporting {
      DISABLED,
      SIMPLE,
      PROGRESSBAR
    } progress_reporting = isatty(STDOUT_FILENO)
                               ? Progress_reporting::PROGRESSBAR
                               : Progress_reporting::SIMPLE;

    int exit_code = 0;

    bool has_connection_data(bool require_main_options = false) const;
    mysqlshdk::db::Connection_options connection_options() const;

    bool has_multi_passwords() const {
      return (connection_data.has_mfa_password(1) ||
              connection_data.has_mfa_password(2));
    }

    void set_uri(const std::string &uri);

    mysqlsh::SessionType check_option_session_type_conflict(
        const std::string &option);

   private:
    std::list<std::string> m_session_type_options;
  };

  enum class Option_flags { CONNECTION_ONLY, READ_MYCNF };
  using Option_flags_set =
      mysqlshdk::utils::Enum_set<Option_flags, Option_flags::READ_MYCNF>;

  Shell_options(
      int argc = 0, char **argv = nullptr,
      const std::string &configuration_file = "", Option_flags_set flags = {},
      const std::function<void(const std::string &)> &on_error = {},
      const std::function<void(const std::string &)> &on_warning = {});

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

  const Storage &get() const { return storage; }

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

  void set_binary_limit(size_t value) { storage.binary_limit = value; }
  void set_gui_mode(bool value) { storage.gui_mode = value; }

  void set_oci_config_file(const std::string &path) {
    storage.oci_config_file = path;
  }

  void set_oci_profile(const std::string &profile) {
    storage.oci_profile = profile;
  }

  void set_json_output() {
    storage.wrap_json = "json/raw";
    storage.result_format = "json/raw";
  }

  std::vector<std::string> get_details() { return get_cmdline_help(33, 45); }

  bool action_print_help() const { return print_cmd_line_helper; }

  bool action_print_version() const { return print_cmd_line_version; }

  bool action_print_version_extra() const {
    return print_cmd_line_version_extra;
  }

  std::vector<std::string> get_named_options();

  bool got_cmdline_password = false;

 private:
  void handle_mycnf_options(int *argc, char ***argv, MEM_ROOT *argv_alloc);

  bool custom_cmdline_handler(Iterator *iterator);

  void override_session_type(const std::string &option, const char *value);
  void set_ssl_mode(const std::string &option, const char *value);

  void check_password_conflicts();
  void check_result_format();
  void check_file_execute_conflicts();
  void check_ssh_conflicts();

  void check_connection_options();

  Storage storage;
  std::unique_ptr<shcore::cli::Shell_cli_operation> m_shell_cli_operation;

  std::function<void(const std::string &)> m_on_error;
  std::function<void(const std::string &)> m_on_warning;

  bool print_cmd_line_helper = false;
  bool print_cmd_line_version = false;
  bool print_cmd_line_version_extra = false;

 private:
  void notify(const std::string &option);
};

std::shared_ptr<Shell_options> current_shell_options(bool allow_empty = false);

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_OPTIONS_H_
