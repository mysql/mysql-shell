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

#include "mysqlshdk/include/shellcore/shell_options.h"

#include <stdlib.h>
#include <cstdio>

#include <iostream>
#include <limits>

#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/shellcore/credential_manager.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_notifications.h"
#include "shellcore/shell_resultset_dumper.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

namespace mysqlsh {

using mysqlshdk::db::Transport_type;

bool Shell_options::Storage::has_connection_data() const {
  return !uri.empty() || !user.empty() || !host.empty() || !schema.empty() ||
         !sock.is_null() || port != 0 || password != NULL || prompt_password ||
         !m_connect_timeout.empty() || ssl_options.has_data() ||
         !compress.empty() || !compress_algorithms.empty() ||
         !compress_level.is_null();
}

/**
 * Builds Connection_options from options given by user through command line
 */
mysqlshdk::db::Connection_options Shell_options::Storage::connection_options()
    const {
  mysqlshdk::db::Connection_options target_server;
  if (!uri.empty()) {
    target_server = shcore::get_connection_options(uri);
  }

  // If a scheme is given on the URI the session type must match the URI scheme
  if (target_server.has_scheme()) {
    mysqlsh::SessionType uri_session_type = target_server.get_session_type();
    std::string error;

    if (session_type != mysqlsh::SessionType::Auto &&
        session_type != uri_session_type) {
      if (session_type == mysqlsh::SessionType::Classic)
        error = "The given URI conflicts with the --mysql session type option.";
      else if (session_type == mysqlsh::SessionType::X)
        error =
            "The given URI conflicts with the --mysqlx session type "
            "option.";
    }
    if (!error.empty()) throw shcore::Exception::argument_error(error);
  } else {
    switch (session_type) {
      case mysqlsh::SessionType::Auto:
        target_server.clear_scheme();
        break;
      case mysqlsh::SessionType::X:
        target_server.set_scheme("mysqlx");
        break;
      case mysqlsh::SessionType::Classic:
        target_server.set_scheme("mysql");
        break;
    }
  }

  // other options need to be set after scheme, as some of them require specific
  // scheme type (i.e. pipe)
  shcore::update_connection_data(
      &target_server, user, password, host, port, sock, schema, ssl_options,
      auth_method, get_server_public_key, server_public_key_path,
      m_connect_timeout, compress, compress_algorithms, compress_level);

  if (no_password && !target_server.has_password()) {
    target_server.set_password("");
  }

  return target_server;
}

static std::string hide_password_in_uri(std::string uri,
                                        const std::string &username) {
  std::size_t pwd_start = uri.find(username) + username.length() + 1;
  std::size_t pwd_size = uri.find('@', pwd_start) - pwd_start;
  return uri.replace(pwd_start, pwd_size, pwd_size, '*');
}

static std::string get_session_type_name(mysqlsh::SessionType type) {
  std::string label;
  switch (type) {
    case mysqlsh::SessionType::X:
      label = "X protocol";
      break;
    case mysqlsh::SessionType::Classic:
      label = "Classic";
      break;
    case mysqlsh::SessionType::Auto:
      label = "";
      break;
  }

  return label;
}

using mysqlshdk::db::Ssl_options;
using shcore::opts::assign_value;
using shcore::opts::cmdline;
using shcore::opts::deprecated;
using shcore::opts::Source;
using std::placeholders::_1;
using std::placeholders::_2;

Shell_options::Shell_options(int argc, char **argv,
                             const std::string &configuration_file)
    : Options(configuration_file) {
  std::string home = shcore::get_home_dir();
#ifdef WIN32
  home += ("MySQL\\mysql-sandboxes");
#else
  home += ("mysql-sandboxes");
#endif

  const auto create_result_format_handler = [this](const char *value) {
    return [this, value](const std::string &opt_name, const char *) {
      get_option(SHCORE_RESULT_FORMAT)
          .handle_command_line_input(opt_name, value);
    };
  };

  // clang-format off
  add_startup_options()
    (&print_cmd_line_helper, false,
        cmdline("-?", "--help"), "Display this help and exit.")
    (cmdline("--"),
        "Triggers API Command Line integration, which allows execution of "
        "methods of the Shell global objects from command line using syntax:\n"
        "  -- <object> <method> [arguments]\n"
        "For more details execute '\\? cmdline' inside of the Shell.")
    (&storage.execute_statement, "", cmdline("-e", "--execute=<cmd>"),
        "Execute command and quit.")
    (cmdline("-f", "--file=<file>"), "Specify a file to process in batch mode. "
        "Any options specified after this are used as arguments of the "
        "processed file.")
    (cmdline("--uri=<value>"), "Connect to Uniform Resource Identifier. "
        "Format: [user[:pass]@]host[:port][/db]")
    (&storage.host, "", cmdline("-h", "--host=<name>"),
        "Connect to host.")
    (&storage.port, 0, cmdline("-P", "--port=<#>"),
        "Port number to use for connection.")
    (cmdline("--connect-timeout=<ms>"), "Connection timeout in milliseconds.",
        std::bind(&Shell_options::set_connection_timeout, this, _1, _2))
#ifndef _WIN32
    (cmdline("-S", "--socket[=<sock>]"), "Socket name to use. "
        "If no value is provided will use default UNIX socket path.",
#else
    (cmdline("-S", "--socket=<sock>"),
        "Pipe name to use (only classic sessions).",
#endif
        [this](const std::string&, const char* value) {
          storage.sock = value == nullptr ? "" : value;
        }
      )
    (&storage.user, "", cmdline("-u", "--user=<name>"),
        "User for the connection to the server.")
    (cmdline("--dbuser=<name>"),
      deprecated("--user", [this](const std::string &, const char *value) {
        storage.user = value;
      }))
    (cmdline("--password=[<pass>]"), "Password to use when connecting to server. "
      "If password is empty, connection will be made without using a password.")
    (cmdline("--dbpassword[=<pass>]"), deprecated("--password"))
    (cmdline("-p", "--password"), "Request password prompt to set the password")
    (cmdline("-C", "--compress[=<value>]"),
        "Use compression in client/server protocol. Valid values: 'REQUIRED', "
        "'PREFFERED', 'DISABLED', 'True', 'False', '1', and '0'. Boolean values"
        " map to REQUIRED and DISABLED respectively. By default compression is "
        "set to DISABLED in classic protocol and PREFERRED in X protocol "
        "connections.",
        [this](const std::string&, const char* value) {
          storage.compress = value == nullptr ? "REQUIRED" : value;
        })
    (&storage.compress_algorithms, "",
        cmdline("--compression-algorithms=<list>"),
        "Use compression algorithm in server/client protocol. Expects comma "
        "separated list of algorithms. Supported algorithms include "
        "'zstd', 'zlib', 'lz4' (X protocol only), and 'uncompressed', "
        "which if appears in a list, causes connection to "
        "succeed even if compression negotiation fails.")
    (cmdline("--compression-level=<int>", "--zstd-compression-level=<int>"),
        "Use this compression level in the client/server protocol. "
        "Supported by X protocol and zstd compression in classic protocol.",
        [this](const std::string& opt, const char* value) {
          try {
            storage.compress_level = shcore::lexical_cast<int64_t>(value);
            if (opt == "--zstd-compression-level"
                && storage.compress_algorithms.empty())
              storage.compress_algorithms = "zstd";
          } catch(...) {
            throw std::invalid_argument(
                "The value of 'compression-level' must be an integer.");
          }
        })
    (cmdline("--import <file> <collection>", "--import <file> <table> <col>"),
        "Import JSON documents from file to collection or table in MySQL"
        " Server. Set file to - if you want to read the data from stdin."
        " Requires a default schema on the connection options.")
    (&storage.schema, "", "schema",
        cmdline("-D", "--schema=<name>", "--database=<name>"), "Schema to use.")
    (&storage.recreate_database, false, "recreateDatabase",
        cmdline("--recreate-schema"), "Drop and recreate the specified schema. "
        "Schema will be deleted if it exists!")
    (cmdline("--mx", "--mysqlx"),
        "Uses connection data to create an X protocol session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--mc", "--mysql"),
        "Uses connection data to create a classic session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--redirect-primary"), "Ensure that the target server is part of "
        "an InnoDB cluster or ReplicaSet and if it is not a primary, find the "
        "primary and connect to it.",
        assign_value(&storage.redirect_session,
          Shell_options::Storage::Redirect_to::Primary))
    (cmdline("--redirect-secondary"), "Ensure that the target server is part "
        "of an InnoDB cluster or ReplicaSet and if it is not a secondary, find "
        "a secondary and connect to it.",
        assign_value(&storage.redirect_session,
          Shell_options::Storage::Redirect_to::Secondary))
    (cmdline("--cluster"), "Ensure that the target server is part of an InnoDB "
        "cluster and if so, set the cluster global variable.",
        [this](const std::string&, const char* value) {
          storage.default_cluster = value ? value : "";
          storage.default_cluster_set = true;
        })
    (cmdline("--replicaset"), "Ensure that the target server is part of an "
        "InnoDB ReplicaSet and if so, set the rs global variable.",
        assign_value(&storage.default_replicaset_set, true))
    (cmdline("--sql"), "Start in SQL mode, auto-detecting the protocol to use "
        "if it is not specified as part of the connection information.",
        assign_value(
            &storage.initial_mode, shcore::IShell_core::Mode::SQL))
    (cmdline("--sqlc"), "Start in SQL mode using a classic session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--sqlx"),
        "Start in SQL mode using an X protocol session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--js", "--javascript"), "Start in JavaScript mode.",
#ifdef HAVE_V8
        [this](const std::string&, const char*) {
          storage.initial_mode = shcore::IShell_core::Mode::JavaScript;
        }
#else
        [](const std::string&, const char*) {
          throw std::invalid_argument("JavaScript is not supported.");
        }
#endif
    )
    (cmdline("--py", "--python"), "Start in Python mode.",
#ifdef HAVE_PYTHON
        [this](const std::string&, const char*) {
          storage.initial_mode = shcore::IShell_core::Mode::Python;
        }
#else
        [](const std::string&, const char*) {
          throw std::invalid_argument("Python is not supported.");
        }
#endif
    )
    (cmdline("--pym <module>"),
       "Run Python library module as a script. Remaining args are forwarded to it.")
    (&storage.wrap_json, "off", cmdline("--json[=<format>]"),
        "Produce output in JSON format. Allowed values: raw, pretty, and off. "
        "If no format is specified pretty format is produced.",
        [](const std::string &val, Source) {
          if (val == "off") return "off";
          if (val.empty() || val == "pretty") return "json";
          if (val == "raw") return "json/raw";
          throw std::invalid_argument(
              "Value for --json must be either pretty, raw or off.");
        })
    (cmdline("--table"),
        "Produce output in table format (default for interactive mode). This "
        "option can be used to force that format when running in batch mode.",
        create_result_format_handler("table"))
    (cmdline("--tabbed"),
        "Produce output in tab separated format (default for batch mode). This "
        "option can be used to force that format when running in interactive "
        "mode.",
        create_result_format_handler("tabbed"))
    (cmdline("-E", "--vertical"),
        "Print the output of a query (rows) vertically.",
        create_result_format_handler("vertical"));

  add_named_options()
    (&storage.result_format, "table", "outputFormat",
        "outputFormat option has been deprecated, "
        "please use " SHCORE_RESULT_FORMAT " to set result format and --json "
        "command line option to wrap output in JSON instead.\n",
        [this](const std::string &val, Source s) {
          std::cout << "WARNING: outputFormat option has been deprecated, "
              "please use " SHCORE_RESULT_FORMAT " to set result format and"
              " --json command line option to wrap output in JSON instead.\n";
          get_option(SHCORE_RESULT_FORMAT).set(val, s);
          storage.wrap_json = shcore::str_beginswith(val, "json") ? val : "off";
          return storage.result_format;
        })
    (&storage.result_format, "table", SHCORE_RESULT_FORMAT,
        cmdline("--result-format=<value>"),
        "Determines format of results. Allowed values:"
        " [" RESULTSET_DUMPER_FORMATS "].",
        [](const std::string &val, Source) {
          if (!Resultset_dumper_base::is_valid_format(val))
            throw std::invalid_argument(
                "The acceptable values for the option " SHCORE_RESULT_FORMAT
                " are: " RESULTSET_DUMPER_FORMATS);
          return val;
        })
    (&storage.interactive, false, SHCORE_INTERACTIVE,
        "Enables interactive mode", shcore::opts::Read_only<bool>())
    (&storage.db_name_cache, -1, SHCORE_DB_NAME_CACHE,
        "Enable database name caching for autocompletion.")
    (&storage.devapi_schema_object_handles, -1,
        SHCORE_DEVAPI_DB_OBJECT_HANDLES,
        "Enable table and collection name handles for the DevAPI db object.");

  add_startup_options()
    (cmdline("--get-server-public-key"), "Request public key from the server "
        "required for RSA key pair-based password exchange. Use when connecting"
        " to MySQL 8.0 servers with classic sessions with SSL mode DISABLED.",
        assign_value(&storage.get_server_public_key, true))
    (&storage.server_public_key_path, "",
        cmdline("--server-public-key-path=<p>"), "The path name to a file "
        "containing a client-side copy of the public key required by the "
        "server for RSA key pair-based password exchange. Use when connecting "
        "to MySQL 8.0 servers with classic sessions with SSL mode DISABLED.")
    (cmdline("-i", "--interactive[=full]"),
      "To use in batch mode. "
      "It forces emulation of interactive mode processing. Each "
      "line on the batch is processed as if it were in interactive mode.",
      [this](const std::string&, const char* value) {
        if (!value || !value[0]) {
          storage.interactive = true;
          storage.full_interactive = false;
        } else if (strcmp(value, "full") == 0) {
          storage.interactive = true;
          storage.full_interactive = true;
        } else {
          throw std::invalid_argument(
                    "Value for --interactive if any, must be full\n");
        }
      });

  // make sure hack for accessing log_level via Value works
  static_assert(
      std::is_integral<
          std::underlying_type<shcore::Logger::LOG_LEVEL>::type>::value,
      "Invalid underlying type of shcore::Logger::LOG_LEVEL enum");
  add_named_options()
    (&storage.force, false, SHCORE_BATCH_CONTINUE_ON_ERROR, cmdline("--force"),
        "In SQL batch mode, forces processing to continue if an error "
        "is found.", shcore::opts::Read_only<bool>())
    (reinterpret_cast<int*>(&storage.log_level),
        shcore::Logger::LOG_INFO, "logLevel", cmdline("--log-level=<value>"),
        std::string("Set logging level. ") +
        shcore::Logger::get_level_range_info(),
        [this](const std::string &val, Source) {
          const char* value = val.c_str();

          if (*value == '@') {
            storage.log_to_stderr = true;
            value++;
          } else {
            storage.log_to_stderr = false;
          }

          const auto level = shcore::Logger::parse_log_level(value);

          if (const auto logger = shcore::current_logger(true)) {
            if (storage.log_to_stderr) {
              logger->log_to_stderr();
            } else {
              logger->stop_log_to_stderr();
            }

            logger->set_log_level(level);
          }

          return level;
        })
    (&storage.dba_log_sql, 0, SHCORE_DBA_LOG_SQL,
        cmdline("--dba-log-sql[={0|1|2}]"),
        "Log SQL statements executed by AdminAPI operations: "
        "0 - logging disabled; 1 - log statements other than SELECT and SHOW; "
        "2 - log all statements.", shcore::opts::Range<int>(0, 2))
    (&storage.history_sql_syslog, false, SHCORE_HISTORY_SQL_SYSLOG,
        cmdline("--syslog"),
        "Log filtered interactive commands to the system log. Filtering of "
        "commands depends on the patterns supplied via histignore option.")
    (&storage.verbose_level, 0, SHCORE_VERBOSE,
        cmdline("--verbose[={0|1|2|3|4}]"),
        "Enable diagnostic message output to the console: 0 - display no "
        "messages; 1 - display error, warning and informational messages; 2, 3,"
        " 4 - include higher levels of debug messages. If level is not given, 1"
        " is assumed.", shcore::opts::Range<int>(0, 4))
    (&storage.passwords_from_stdin, false, "passwordsFromStdin",
        cmdline("--passwords-from-stdin"),
        "Read passwords from stdin instead of the console.")
    (&storage.show_warnings, true, SHCORE_SHOW_WARNINGS,
        cmdline("--show-warnings={true|false}"),
        "Automatically display SQL warnings on SQL mode if available.")
    (&storage.show_column_type_info, false, "showColumnTypeInfo",
        cmdline("--column-type-info"),
        "Display column type information in SQL mode. Please be aware that "
        "output may depend on the protocol you are using to connect to the "
        "server, e.g. DbType field is approximated when using X protocol.")
    (&storage.history_max_size, 1000, SHCORE_HISTORY_MAX_SIZE,
        "Shell's history maximum size",
        shcore::opts::Range<int>(0, std::numeric_limits<int>::max()))
    (&storage.histignore, "*IDENTIFIED*:*PASSWORD*", SHCORE_HISTIGNORE,
        cmdline("--histignore=<filters>"), "Shell's history ignore list.")
    (&storage.history_autosave, false, SHCORE_HISTORY_AUTOSAVE,
        "Shell's history autosave.")
    (&storage.sandbox_directory, home, SHCORE_SANDBOX_DIR,
        "Default sandbox directory")
    (&storage.dba_gtid_wait_timeout, 60, SHCORE_DBA_GTID_WAIT_TIMEOUT,
        "Timeout value in seconds to wait for GTIDs to be synchronized.",
        shcore::opts::Range<int>(0, std::numeric_limits<int>::max()))
    (&storage.dba_restart_wait_timeout, 60, SHCORE_DBA_RESTART_WAIT_TIMEOUT,
        "Timeout in seconds to wait for MySQL server to come back after a "
        "restart during clone recovery.",
        shcore::opts::Range<int>(0, std::numeric_limits<int>::max()))
    (&storage.wizards, true, SHCORE_USE_WIZARDS, "Enables wizard mode.")
    (&storage.initial_mode, shcore::IShell_core::Mode::None,
        "defaultMode", "Specifies the shell mode to use when shell is started "
        "- one of sql, js or py.", std::bind(&shcore::parse_mode, _1),
        [](shcore::IShell_core::Mode mode) {
          return shcore::to_string(mode);
        })
    (&storage.pager, "", SHCORE_PAGER, "PAGER", cmdline("--pager=<value>"),
        "Pager used to display text output of statements executed in SQL mode "
        "as well as some other selected commands. Pager can be manually "
        "enabled in scripting modes. If you don't supply an "
        "option, the default pager is taken from your ENV variable PAGER. "
        "This option only works in interactive mode. This option is disabled "
        "by default.")
    (&storage.default_compress, false, SHCORE_DEFAULT_COMPRESS,
        "Enable compression in client/server protocol by default "
        "in global shell sessions.")
    (&storage.oci_config_file,
        shcore::path::join_path(shcore::path::home(), ".oci", "config"),
        "oci.configFile",
        "Path to Oracle Cloud Infrastructure (OCI) configuration file.")
    (&storage.oci_profile, std::string{"DEFAULT"}, "oci.profile",
        "Oracle Cloud Infrastructure (OCI) configuration file profile name.");

  add_startup_options()
    (cmdline("--name-cache"),
      "Enable database name caching for autocompletion and DevAPI (default).",
      [this](const std::string &, const char *) {
        storage.db_name_cache = true;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = true;
      })
    (cmdline("-A", "--no-name-cache"),
      "Disable automatic database name caching for autocompletion and DevAPI. "
      "Use \\rehash to load DB names manually.",
      [this](const std::string &, const char *) {
        storage.db_name_cache = false;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = false;
      })
    (cmdline("--nw", "--no-wizard"), "Disables wizard mode.",
        assign_value(&storage.wizards, false))
    (&storage.no_password, false, cmdline("--no-password"),
        "Sets empty password and disables prompt for password.")
    (cmdline("-V", "--version"),
        "Prints the version of MySQL Shell.", [this](const std::string&, const char*) {
        if (print_cmd_line_version) {
          print_cmd_line_version_extra = true;
        }

        print_cmd_line_version = true;
    })
    (cmdline("--ssl-key=<file_name>"),
        "The path to the SSL private key file in PEM format.",
        std::bind(&Ssl_options::set_key, &storage.ssl_options, _2))
    (cmdline("--ssl-cert=<file_name>"),
        "The path to the SSL public key certificate file in PEM format.",
        std::bind(&Ssl_options::set_cert, &storage.ssl_options, _2))
    (cmdline("--ssl-ca=<file_name>"),
        "The path to the certificate authority file in PEM format.",
        std::bind(&Ssl_options::set_ca, &storage.ssl_options, _2))
    (cmdline("--ssl-capath=<dir_name>"),
        "The path to the directory that contains the certificate authority "
        "files in PEM format.",
        std::bind(&Ssl_options::set_capath, &storage.ssl_options, _2))
    (cmdline("--ssl-cipher=<cipher_list>"),
        "The list of permissible encryption ciphers for connections that use "
        "TLS protocols up through TLSv1.2.",
        std::bind(&Ssl_options::set_cipher, &storage.ssl_options, _2))
    (cmdline("--ssl-crl=<file_name>"),
        "The path to the file containing certificate revocation lists in PEM "
        "format.",
        std::bind(&Ssl_options::set_crl, &storage.ssl_options, _2))
    (cmdline("--ssl-crlpath=<dir_name>"),
        "The path to the directory that contains certificate revocation-list "
        "files in PEM format.",
        std::bind(&Ssl_options::set_crlpath, &storage.ssl_options, _2))
    (cmdline("--ssl-mode=<mode>"), "SSL mode to use. Allowed values: DISABLED,"
        "PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
        std::bind(&Shell_options::set_ssl_mode, this, _1, _2))
    (cmdline("--tls-version=<version>"),
        "TLS version to use. Allowed values: TLSv1, TLSv1.1, TLSv1.2, TLSv1.3."
        " TLS versions TLSv1 and TLSv1.1 are now deprecated and will be"
        " removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.",
        std::bind(&Ssl_options::set_tls_version, &storage.ssl_options, _2))
    (cmdline("--tls-ciphersuites=<name>"), "TLS v1.3 cipher to use.",
        std::bind(&Ssl_options::set_tls_ciphersuites, &storage.ssl_options, _2))
    (&storage.auth_method, "", cmdline("--auth-method=<method>"),
        "Authentication method to use. In case of classic session, this is the "
        "name of the authentication plugin to use, i.e. caching_sha2_password. "
        "In case of X protocol session, it should be one of: AUTO, "
        "FROM_CAPABILITIES, FALLBACK, MYSQL41, PLAIN, SHA256_MEMORY.")
    (&storage.execute_dba_statement, "",
        cmdline("--dba=enableXProtocol"), "Enable the X protocol in the target "
        "server. Requires a connection using classic session.")
    (cmdline("--trace-proto"),
        assign_value(&storage.trace_protocol, true))
    (cmdline("--ssl[=<opt>]"), deprecated("--ssl-mode",
      std::bind(&Shell_options::set_ssl_mode, this, _1, _2), "REQUIRED",
     {
       {"1", "REQUIRED"},
       {"YES", "REQUIRED"},
       {"0", "DISABLED"},
       {"NO", "DISABLED"}
     }))
    (cmdline("--node"), deprecated("--mysqlx", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("--classic"), deprecated("--mysql", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("--sqln"), deprecated("--sqlx", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (
      cmdline("--quiet-start[={1|2}]"),
      "Avoids printing information when the shell is started. A value of "
      "1 will prevent printing the shell version information. A value of "
      "2 will prevent printing any information unless it is an error. "
      "If no value is specified uses 1 as default.",
      [this](const std::string&, const char* value) {
        if (!value || !value[0] || !strcmp(value, "1")) {
          storage.quiet_start = Shell_options::Quiet_start::SUPRESS_BANNER;
          storage.full_interactive = false;
        } else if (strcmp(value, "2") == 0) {
          storage.quiet_start = Shell_options::Quiet_start::SUPRESS_INFO;
        } else {
          throw std::invalid_argument("Value for --quiet-start if any, must be any of 1 or 2");
        }
      })
      (cmdline("--debug=<control>"),
      [this](const std::string &, const char* value) {
#ifdef NDEBUG
        // If DBUG is disabled, we just print a warning saying the option won't
        // do anything. This is to keep options compatible between build types
        std::cout << "WARNING: This build of mysqlsh has the DBUG feature "
          "disabled. --debug option ignored." << std::endl;
#endif
        storage.dbug_options = value ? value : "";
      })
;  // <-- Note this is on purpose: Mark the termination of the option definition.
  // clang-format on

  shcore::Credential_manager::get().register_options(this);

  try {
    handle_environment_options();
    if (!configuration_file.empty()) handle_persisted_options();
    handle_cmdline_options(
        argc, argv, false,
        std::bind(&Shell_options::custom_cmdline_handler, this, _1));

    check_session_type_conflicts();
    check_user_conflicts();
    check_password_conflicts();
    check_host_conflicts();
    check_host_socket_conflicts();
    check_port_conflicts();
    check_socket_conflicts();
    check_port_socket_conflicts();
    check_file_execute_conflicts();
    check_import_options();
    check_result_format();
    shcore::Logger::set_stderr_output_format(storage.wrap_json);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    storage.exit_code = 1;
  }
}

static inline std::string value_to_string(const shcore::Value &value) {
  return value.type == shcore::Value_type::String ? value.get_string()
                                                  : value.repr();
}

void Shell_options::set(const std::string &option, const shcore::Value &value) {
  try {
    shcore::Options::set(option, value_to_string(value));
  } catch (const std::exception &e) {
    throw shcore::Exception::argument_error(e.what());
  }
}

void Shell_options::notify(const std::string &option) {
  shcore::Value::Map_type_ref info = shcore::Value::new_map().as_map();
  (*info)["option"] = shcore::Value(option);
  (*info)["value"] = get(option);
  shcore::ShellNotifications::get()->notify(SN_SHELL_OPTION_CHANGED, nullptr,
                                            info);
}

void Shell_options::set_and_notify(const std::string &option,
                                   const std::string &value,
                                   bool save_to_file) {
  try {
    Options::set(option, value);
    notify(option);
    if (save_to_file) save(option);
  } catch (const std::exception &e) {
    throw shcore::Exception::argument_error(e.what());
  }
}

void Shell_options::set_and_notify(const std::string &option,
                                   const shcore::Value &value,
                                   bool save_to_file) {
  set_and_notify(option, value_to_string(value), save_to_file);
}

void Shell_options::unset(const std::string &option, bool save_to_file) {
  if (save_to_file) unsave(option);
  get_option(option).reset_to_default_value();
  notify(option);
}

shcore::Value Shell_options::get(const std::string &option) {
  using shcore::opts::Concrete_option;
  auto it = named_options.find(option);
  if (it == named_options.end())
    throw std::invalid_argument("No option registered under name: " + option);
  Concrete_option<bool> *opt_bool =
      dynamic_cast<Concrete_option<bool> *>(it->second);
  if (opt_bool != nullptr) return shcore::Value(opt_bool->get());

  Concrete_option<int> *opt_int =
      dynamic_cast<Concrete_option<int> *>(it->second);
  if (opt_int != nullptr) return shcore::Value(opt_int->get());

  auto opt_set_string =
      dynamic_cast<Concrete_option<std::vector<std::string>> *>(it->second);

  if (opt_set_string != nullptr) {
    auto value = shcore::Value::new_array();
    auto array = value.as_array();

    for (const auto &val : opt_set_string->get()) {
      array->emplace_back(val);
    }

    return value;
  }

  return shcore::Value(get_value_as_string(option));
}

std::vector<std::string> Shell_options::get_named_options() {
  std::vector<std::string> res;
  for (const auto &op : named_options) res.push_back(op.first);
  return res;
}

static void handle_missing_value(shcore::Options::Iterator *it) {
  if (nullptr == it->value() || '\0' == it->value()[0]) {
    throw std::invalid_argument(std::string(it->iterator()->first()) +
                                ": option " + it->option() +
                                " requires an argument");
  }
}

bool Shell_options::custom_cmdline_handler(Iterator *iterator) {
  const auto option = iterator->option();

  if ("--" == option) {
    if (m_shell_cli_operation)
      throw std::invalid_argument(
          "MySQL Shell can handle only one operation at a time");
    iterator->next_no_value();
    m_shell_cli_operation.reset(new shcore::cli::Shell_cli_operation());
    m_shell_cli_operation->parse(iterator->iterator());
  } else if ("--file" == option || "-f" == option) {
    handle_missing_value(iterator);

    const std::string file = iterator->value();
    iterator->next();

    storage.run_file = file;

    if (shcore::str_endswith(file, ".js")) {
      storage.initial_mode = shcore::IShell_core::Mode::JavaScript;
    } else if (shcore::str_endswith(file, ".py")) {
      storage.initial_mode = shcore::IShell_core::Mode::Python;
    } else if (shcore::str_endswith(file, ".sql")) {
      storage.initial_mode = shcore::IShell_core::Mode::SQL;
    }

    // the rest of the cmdline options, starting from here are all passed
    // through to the script
    storage.script_argv.push_back(file);
    const auto cmdline = iterator->iterator();
    while (cmdline->valid()) storage.script_argv.push_back(cmdline->get());
  } else if ("--pym" == option) {
#ifndef HAVE_PYTHON
    throw std::invalid_argument("Python is not supported.");
#endif
    handle_missing_value(iterator);

    const std::string module = iterator->value();
    iterator->next();

    storage.run_module = module;
    storage.initial_mode = shcore::IShell_core::Mode::Python;

    // the rest of the cmdline options, starting from here are all passed
    // through to the script
    storage.script_argv.push_back(module);
    const auto cmdline = iterator->iterator();
    while (cmdline->valid()) storage.script_argv.push_back(cmdline->get());
  } else if ("--uri" == option || '-' != option[0]) {
    handle_missing_value(iterator);

    storage.uri = iterator->value();
    uri_data = shcore::get_connection_options(storage.uri, false);

    if (uri_data.has_password()) {
      const auto value = iterator->value();
      const auto nopwd_uri = hide_password_in_uri(value, uri_data.get_user());
      strncpy(const_cast<char *>(value), nopwd_uri.c_str(), strlen(value) + 1);
    }

    iterator->next();
  } else if ("--password" == option || "-p" == option ||
             "--dbpassword" == option) {
    // Note that in any connection attempt, password prompt will be done if
    // the password is missing.
    // The behavior of the password cmd line argument is as follows:

    // ARGUMENT           EFFECT
    // --password         forces password prompt no matter it was already
    // provided
    // --password value   forces password prompt no matter it was already
    // provided (value is not taken as password)
    // --password=        sets password to empty (password is available but
    // empty so it will not be prompted)
    // -p<value> sets the password to <value>
    // --password=<value> sets the password to <value>

    {
      if ("--dbpassword" == option) {
        // Deprecated handler is not going to be invoked, need to inform the
        // user that --dbpassword should not longer be used.
        // Console is not available lat this time, need to use stdout.
        std::cout << "WARNING: The --dbpassword option has been deprecated, "
                     "please use --password instead."
                  << std::endl;
      }
    }

    if (shcore::Options::Iterator::Type::NO_VALUE == iterator->type()) {
      storage.prompt_password = true;
      iterator->next_no_value();
    } else if (shcore::Options::Iterator::Type::SEPARATE_VALUE !=
               iterator->type()) {
      // --password=value || -pvalue
      const auto value = iterator->value();

      storage.pwd.assign(value);
      storage.password = storage.pwd.c_str();

      const std::string stars(storage.pwd.length(), '*');

      strncpy(const_cast<char *>(value), stars.c_str(), strlen(value) + 1);

      iterator->next();
    } else {  // --password value (value is ignored)
      storage.prompt_password = true;
      iterator->next_no_value();
    }
  } else if ("--import" == option) {
    const auto cmdline = iterator->iterator();

    storage.import_args.push_back(cmdline->get());  // omit --import

    // Gets the positional arguments for --import
    // As they define the target database object for the data
    while (cmdline->valid()) {
      // We append --import arguments until next shell option in program
      // argument list, i.e. -* or --*. Single char '-' is a --import argument.
      const char *arg = cmdline->peek();
      if (arg[0] == '-' && arg[1] != '\0') {
        break;
      }
      storage.import_args.push_back(cmdline->get());
    }

    if (storage.import_args[1] == "-") {
      // STDIN import will be handled directly in main
      // Here we store the import options
      while (cmdline->valid()) storage.import_opts.push_back(cmdline->get());
    } else {
      // Non STDIN import is handled as a CLI API call
      if (m_shell_cli_operation)
        throw std::invalid_argument(
            "MySQL Shell can handle only one operation at a time");
      m_shell_cli_operation =
          std::make_unique<shcore::cli::Shell_cli_operation>();
      m_shell_cli_operation->set_object_name("util");
      m_shell_cli_operation->set_method_name("importJson");

      m_shell_cli_operation->add_cmdline_argument(storage.import_args.at(1));

      // Parses the positional arguments to set them on the CLI operation
      switch (storage.import_args.size()) {
        case 4: {
          const std::string &target = storage.import_args.at(2);
          const std::string &column = storage.import_args.at(3);
          m_shell_cli_operation->add_cmdline_argument("--table=" + target);
          m_shell_cli_operation->add_cmdline_argument("--tableColumn=" +
                                                      column);
          break;
        }
        case 3: {
          const std::string &target = storage.import_args.at(2);
          m_shell_cli_operation->add_cmdline_argument("--collection=" + target);
          break;
        }
        case 2:
          break;
        default:
          throw std::runtime_error(
              "Usage: --import <filename> [<collection>|<table> <column>] "
              "[options]");
      }

      // All of the options above are valid
      m_shell_cli_operation->parse(cmdline);
    }
  } else if ("-m" == option) {
    bool handled = false;

    if (iterator->value()) {
      const std::string value = option + iterator->value();
      const char *replacement = nullptr;

      if ("-ma" == value) {
        handled = true;
      } else if ("-mc" == value) {
        replacement = "--mc";
      } else if ("-mx" == value) {
        replacement = "--mx";
      }

      if (nullptr != replacement) {
        handled = true;
      }

      if (handled) {
        deprecated(replacement, std::bind(&Shell_options::override_session_type,
                                          this, _1, _2))(value, nullptr);
        iterator->next();
      }
    }

    if (!handled) {
      throw std::invalid_argument(iterator->iterator()->first() +
                                  std::string(": unknown option -m"));
    }
  } else {
    return false;
  }
  return true;
}

void Shell_options::override_session_type(const std::string &option,
                                          const char *value) {
  if (value != nullptr)
    throw std::runtime_error("Option " + option + " does not support value");

  if (option.find("--sql") != std::string::npos)
    storage.initial_mode = shcore::IShell_core::Mode::SQL;

  mysqlsh::SessionType new_type = mysqlsh::SessionType::Auto;
  if (option == "-mc" || option == "--mc" || option == "--mysql" ||
      option == "--sqlc")
    new_type = mysqlsh::SessionType::Classic;
  else if (option == "-mx" || option == "--mx" || option == "--mysqlx" ||
           option == "--sqlx")
    new_type = mysqlsh::SessionType::X;

  if (new_type != storage.session_type) {
    if (!storage.default_session_type) {
      std::stringstream ss;
      if (storage.session_type == mysqlsh::SessionType::Auto) {
        ss << "Automatic protocol detection is enabled, unable to change to "
           << get_session_type_name(new_type) << " with option " << option;
      } else if (new_type == mysqlsh::SessionType::Auto) {
        ss << "Session type already configured to "
           << get_session_type_name(storage.session_type)
           << ", automatic protocol detection (-ma) can't be enabled.";
      } else {
        ss << "Session type already configured to "
           << get_session_type_name(storage.session_type)
           << ", unable to change to " << get_session_type_name(new_type)
           << " with option " << option;
      }
      throw std::invalid_argument(ss.str());
    }

    storage.session_type = new_type;
    session_type_arg = option;
  }

  storage.default_session_type = false;
}

void Shell_options::set_ssl_mode(const std::string &option, const char *value) {
  int mode = mysqlshdk::db::MapSslModeNameToValue::get_value(value);

  if (mode == 0) {
    throw std::invalid_argument(option +
                                " must be any of [DISABLED, PREFERRED, "
                                "REQUIRED, VERIFY_CA, VERIFY_IDENTITY]");
  }

  storage.ssl_options.set_mode(static_cast<mysqlshdk::db::Ssl_mode>(mode));
}

void Shell_options::set_connection_timeout(const std::string & /*option*/,
                                           const char *value) {
  // Creates a temporary connection options object so the established
  // validations are performed on the data
  mysqlshdk::db::Connection_options{}.set(mysqlshdk::db::kConnectTimeout,
                                          value);
  storage.m_connect_timeout.assign(value);
}

void Shell_options::check_session_type_conflicts() {
  if (storage.session_type != mysqlsh::SessionType::Auto &&
      uri_data.has_scheme()) {
    if ((storage.session_type == mysqlsh::SessionType::Classic &&
         uri_data.get_scheme() != "mysql") ||
        (storage.session_type == mysqlsh::SessionType::X &&
         uri_data.get_scheme() != "mysqlx")) {
      auto error = "The given URI conflicts with the " + session_type_arg +
                   " session type option.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_user_conflicts() {
  if (!storage.user.empty() && uri_data.has_user()) {
    if (storage.user != uri_data.get_user()) {
      auto error =
          "Conflicting options: provided user name differs from the "
          "user in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_password_conflicts() {
  if (storage.password != NULL && uri_data.has_password()) {
    if (storage.password != uri_data.get_password()) {
      const auto error =
          "Conflicting options: provided password differs from the "
          "password in the URI.";
      throw std::runtime_error(error);
    }
  }

  if (storage.no_password && storage.prompt_password) {
    const auto error =
        "Conflicting options: --password and --no-password option cannot be "
        "used together.";
    throw std::runtime_error(error);
  }

  if (storage.no_password && uri_data.has_password() &&
      !uri_data.get_password().empty()) {
    const auto error =
        "Conflicting options: --no-password cannot be used if password is "
        "provided in URI.";
    throw std::runtime_error(error);
  }

  if (storage.no_password && storage.password && strlen(storage.password) > 0) {
    const auto error =
        "Conflicting options: --no-password cannot be used if password is "
        "provided.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_host_conflicts() {
  if (uri_data.has_host() && !storage.host.empty()) {
    if (storage.host != uri_data.get_host()) {
      auto error =
          "Conflicting options: provided host differs from the "
          "host in the URI.";
      throw std::runtime_error(error);
    }
  }
}

#ifdef _WIN32
#define SOCKET_NAME "named pipe"
#else  // !_WIN32
#define SOCKET_NAME "socket"
#endif  // !_WIN32

void Shell_options::check_host_socket_conflicts() {
  if (!storage.sock.is_null()) {
    if ((!storage.host.empty() && storage.host != "localhost"
#ifdef _WIN32
         && storage.host != "."
#endif  // _WIN32
         ) ||
        (uri_data.has_host() && uri_data.get_host() != "localhost"
#ifdef _WIN32
         && uri_data.get_host() != "."
#endif  // _WIN32
         )) {
      auto error = "Conflicting options: " SOCKET_NAME
                   " cannot be used if host is "
#ifdef _WIN32
                   "neither '.' nor 'localhost'.";
#else   // !_WIN32
                   "not 'localhost'.";
#endif  // !_WIN32
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_port_conflicts() {
  if (storage.port != 0 && uri_data.has_transport_type() &&
      uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Tcp &&
      uri_data.has_port() && uri_data.get_port() != storage.port) {
    auto error =
        "Conflicting options: provided port differs from the "
        "port in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_socket_conflicts() {
  if (!storage.sock.is_null() && uri_data.has_transport_type() &&
      uri_data.get_transport_type() ==
#ifdef _WIN32
          mysqlshdk::db::Transport_type::Pipe
#else   // !_WIN32
          mysqlshdk::db::Transport_type::Socket
#endif  // !_WIN32
      && uri_data.get_socket() != *storage.sock) {
    auto error = "Conflicting options: provided " SOCKET_NAME
                 " differs from the " SOCKET_NAME " in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_port_socket_conflicts() {
  if (storage.port != 0 && !storage.sock.is_null()) {
    auto error = "Conflicting options: port and " SOCKET_NAME
                 " cannot be used together.";
    throw std::runtime_error(error);
  } else if (storage.port != 0 && uri_data.has_transport_type() &&
             uri_data.get_transport_type() ==
#ifdef _WIN32
                 mysqlshdk::db::Transport_type::Pipe
#else   // !_WIN32
                 mysqlshdk::db::Transport_type::Socket
#endif  // !_WIN32
  ) {
    auto error =
        "Conflicting options: port cannot be used if the URI "
        "contains a " SOCKET_NAME ".";
    throw std::runtime_error(error);
  } else if (!storage.sock.is_null() && uri_data.has_port()) {
    auto error = "Conflicting options: " SOCKET_NAME
                 " cannot be used if the URI contains a port.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_file_execute_conflicts() {
  if (!storage.run_file.empty() && !storage.execute_statement.empty()) {
    throw std::runtime_error(
        "Conflicting options: --execute and --file cannot be used at the same "
        "time.");
  } else if (!storage.run_module.empty() &&
             !storage.execute_statement.empty()) {
    throw std::runtime_error(
        "Conflicting options: --execute and --pym cannot be used at the same "
        "time.");
  }
  // note: --file and --pym are impossible because both consume remaining args
}

void Shell_options::check_result_format() {
  if (storage.wrap_json != "off" &&
      get_option_source(SHCORE_RESULT_FORMAT) == Source::Command_line &&
      storage.wrap_json != storage.result_format)
    throw std::invalid_argument(shcore::str_format(
        "Conflicting options: " SHCORE_RESULT_FORMAT
        " cannot be set to '%s' when "
        "--json option implying '%s' value is used.",
        storage.result_format.c_str(), storage.wrap_json.c_str()));
}

void Shell_options::check_import_options() {
  bool is_schema_set = !storage.schema.empty() || uri_data.has_schema();
  if (!storage.import_args.empty()) {
    if (!storage.has_connection_data()) {
      const auto error = "Error: --import requires an active session.";
      throw std::runtime_error(error);
    }
    if (!is_schema_set) {
      const auto error =
          "Error: --import requires a default schema on the active session.";
      throw std::runtime_error(error);
    }
  }
}

}  // namespace mysqlsh
