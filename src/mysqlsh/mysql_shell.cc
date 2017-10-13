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

#include "mysqlsh/mysql_shell.h"
#include <mysqld_error.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "modules/devapi/mod_mysqlx.h"
#include "modules/mod_mysql.h"
#include "modules/mod_shell.h"
#include "modules/mod_mysql_session.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"
#include "src/interactive/interactive_dba_cluster.h"
#include "src/interactive/interactive_global_dba.h"
#include "src/interactive/interactive_global_shell.h"
#include "utils/debug.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "utils/utils_time.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"

DEBUG_OBJ_ENABLE(Mysql_shell);

namespace mysqlsh {

Mysql_shell::Mysql_shell(const Shell_options &options,
                         shcore::Interpreter_delegate *custom_delegate)
    : mysqlsh::Base_shell(options, custom_delegate) {
  DEBUG_OBJ_ALLOC(Mysql_shell);

  // Registers the interactive objects if required
  _global_shell =
      std::shared_ptr<mysqlsh::Shell>(new mysqlsh::Shell(this));
  _global_js_sys =
      std::shared_ptr<mysqlsh::Sys>(new mysqlsh::Sys(_shell.get()));
  _global_dba =
      std::shared_ptr<mysqlsh::dba::Dba>(new mysqlsh::dba::Dba(_shell.get()));

  if (options.wizards) {
    auto interactive_shell = std::shared_ptr<shcore::Global_shell>(
        new shcore::Global_shell(*_shell.get()));
    auto interactive_dba = std::shared_ptr<shcore::Global_dba>(
        new shcore::Global_dba(*_shell.get()));

    interactive_shell->set_target(_global_shell);
    interactive_dba->set_target(_global_dba);

    set_global_object(
        "shell",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(interactive_shell),
        shcore::IShell_core::all_scripting_modes());
    set_global_object(
        "dba",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(interactive_dba),
        shcore::IShell_core::all_scripting_modes());
  } else {
    set_global_object(
        "shell",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_shell),
        shcore::IShell_core::all_scripting_modes());
    set_global_object(
        "dba",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_dba),
        shcore::IShell_core::all_scripting_modes());
  }

  set_global_object(
      "sys",
      std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(_global_js_sys),
      shcore::IShell_core::Mode_mask(shcore::IShell_core::Mode::JavaScript));

  // dummy initialization
  _global_shell->set_session_global({});

  INIT_MODULE(mysqlsh::mysql::Mysql);
  INIT_MODULE(mysqlsh::mysqlx::Mysqlx);

  shcore::Value::Map_type_ref shcore_options =
      shcore::Shell_core_options::get();
  set_sql_safe_for_logging((*shcore_options)[SHCORE_HISTIGNORE].descr());

  // clang-format off
  std::string cmd_help_connect =
      "SYNTAX:\n"
      "   \\connect [<TYPE>] <URI>\n\n"
      "WHERE:\n"
      "   TYPE is an optional parameter to specify the session type. Accepts "
      "the following values:\n"
      "        -mc, --mysql: open a classic MySQL protocol session (default port 3306)\n"
      "        -mx, --mysqlx: open an X protocol session (default port 33060)\n"
      "        -ma: attempt automatic detection of the protocol type\n"
      "        If TYPE is omitted, -ma is assumed by default, unless the protocol is given in the URI.\n"
      "   URI format is: [user[:password]@]hostname[:port]\n\n"
      "EXAMPLE:\n"
      "   \\connect -mx root@localhost";

  std::string cmd_help_source =
      "SYNTAX:\n"
      "   \\source <sql_file_path>\n"
      "   \\. <sql_file_path>\n\n"
      "EXAMPLES:\n"
#ifdef _WIN32
      "   \\source C:\\Users\\MySQL\\sakila.sql\n"
      "   \\. C:\\Users\\MySQL\\sakila.sql\n\n"
#else
      "   \\source /home/me/sakila.sql\n"
      "   \\. /home/me/sakila.sql\n\n"
#endif
      "NOTE: Can execute files from the supported types: SQL, Javascript, or Python.\n"
      "Processing is done using the active language set for processing mode.\n";

  std::string cmd_help_use =
    "SYNTAX:\n"
    "   \\use <schema>\n"
    "   \\u <schema>\n\n"
    "EXAMPLES:\n"
    "   \\use mysql"
    "   \\u 'my schema'"
    "NOTE: This command works with the active session.\n"
    "If it is either an X protocol or a Classic session, the current schema will be updated (affects SQL mode).\n"
    "The global 'db' variable will be updated to hold the requested schema.\n";
  // clang-format on

  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "",
                    Mysql_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND(
      "\\sql", "Switch to SQL processing mode.", "",
      [this](const std::vector<std::string> &args) -> bool {
        switch_shell_mode(shcore::Shell_core::Mode::SQL, args);
        return true;
      });
#ifdef HAVE_V8
  SET_CUSTOM_SHELL_COMMAND(
      "\\js", "Switch to JavaScript processing mode.", "",
      [this](const std::vector<std::string> &args) -> bool {
        switch_shell_mode(shcore::Shell_core::Mode::JavaScript, args);
        return true;
      });
#endif
#ifdef HAVE_PYTHON
  SET_CUSTOM_SHELL_COMMAND(
      "\\py", "Switch to Python processing mode.", "",
      [this](const std::vector<std::string> &args) -> bool {
        switch_shell_mode(shcore::Shell_core::Mode::Python, args);
        return true;
      });
#endif
  SET_SHELL_COMMAND("\\source|\\.",
                    "Execute a script file. Takes a file name as an argument.",
                    cmd_help_source, Mysql_shell::cmd_process_file);
  SET_SHELL_COMMAND("\\", "Start multi-line input when in SQL mode.", "",
                    Mysql_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q|\\exit", "Quit MySQL Shell.", "",
                    Mysql_shell::cmd_quit);
  SET_SHELL_COMMAND("\\connect|\\c", "Connect to a server.", cmd_help_connect,
                    Mysql_shell::cmd_connect);
  SET_SHELL_COMMAND("\\warnings|\\W", "Show warnings after every statement.",
                    "", Mysql_shell::cmd_warnings);
  SET_SHELL_COMMAND("\\nowarnings|\\w",
                    "Don't show warnings after every statement.", "",
                    Mysql_shell::cmd_nowarnings);
  SET_SHELL_COMMAND("\\status|\\s",
                    "Print information about the current global connection.",
                    "", Mysql_shell::cmd_status);
  SET_SHELL_COMMAND("\\use|\\u",
                    "Set the current schema for the active session.",
                    cmd_help_use, Mysql_shell::cmd_use);

  // clang-format off
  const std::string cmd_help_store_connection =
      "SYNTAX:\n"
      "   \\savecon [-f] <SESSION_CONFIG_NAME> <URI>\n\n"
      "   \\savecon <SESSION_CONFIG_NAME>\n\n"
      "WHERE:\n"
      "   SESSION_CONFIG_NAME is the name to be assigned to the session configuration. Must be a valid identifier\n"
      "   -f is an optional flag, when specified the store operation will override the configuration associated to SESSION_CONFIG_NAME\n"
      "   URI Optional. the connection string following the URI convention. If not provided, will use the URI of the current session.\n\n"
      "EXAMPLES:\n"
      "   \\saveconn my_config_name root:123@localhost:33060\n";
  const std::string cmd_help_delete_connection =
      "SYNTAX:\n"
      "   \\rmconn <SESSION_CONFIG_NAME>\n\n"
      "WHERE:\n"
      "   SESSION_CONFIG_NAME is the name of session configuration to be deleted.\n\n"
      "EXAMPLES:\n"
      "   \\rmconn my_config_name\n";
  // clang-format on
}

Mysql_shell::~Mysql_shell() {
  DEBUG_OBJ_DEALLOC(Mysql_shell);
}


static mysqlsh::SessionType get_session_type(
    const mysqlshdk::db::Connection_options &opt) {
  if (!opt.has_scheme()) {
    return mysqlsh::SessionType::Auto;
  } else {
    std::string scheme = opt.get_scheme();
    if (scheme == "mysqlx")
      return mysqlsh::SessionType::X;
    else if (scheme == "mysql")
      return mysqlsh::SessionType::Classic;
    else
      throw std::invalid_argument("Unknown MySQL URI type "+scheme);
  }
}

std::shared_ptr<mysqlsh::ShellBaseSession> Mysql_shell::create_session(
    const mysqlshdk::db::Connection_options &connection_options) {
  std::shared_ptr<mysqlsh::ShellBaseSession> ret_val;
  std::string connection_error;

  SessionType type = get_session_type(connection_options);

  // Automatic protocol detection is ON
  // Attempts X Protocol first, then Classic
  if (type == mysqlsh::SessionType::Auto) {
    ret_val.reset(new mysqlsh::mysqlx::Session());
    try {
      ret_val->connect(connection_options);

      shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED",
                                                ret_val);

      return ret_val;
    } catch (shcore::Exception &e) {
      // Unknown message received from server indicates an attempt to create
      // And X Protocol session through the MySQL protocol
      int code = 0;
      if (e.error()->has_key("code"))
        code = e.error()->get_int("code");

      if (code == 2027 ||  // Unknown message received from server 10
          code == 2002 ||  // No connection could be made because the target
                           // machine actively refused it connecting to
                           // host:port
          code == 2006) {  // MySQL server has gone away (randomly sent by
                           // libmysqlx)
        type = mysqlsh::SessionType::Classic;

        // Since this is an unexpected error, we store the message to be logged
        // in case the classic session connection fails too
        if (code == 2006)
          connection_error = "X protocol error: " + e.format();
      } else {
        throw;
      }
    }
  }

  switch (type) {
    case mysqlsh::SessionType::X:
      ret_val.reset(new mysqlsh::mysqlx::Session());
      break;
    case mysqlsh::SessionType::Classic:
      ret_val.reset(new mysql::ClassicSession());
      break;
    default:
      throw shcore::Exception::argument_error(
          "Invalid session type specified for MySQL connection.");
      break;
  }

  try {
    ret_val->connect(connection_options);
  } catch (shcore::Exception &e) {
    if (connection_error.empty()) {
      throw;
    } else {
      // If an error was cached for the X protocol connection
      // it is included on a new exception
      connection_error.append("\nClassic protocol error: ");
      connection_error.append(e.format());
      throw shcore::Exception::argument_error(connection_error);
    }
  }

  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", ret_val);

  return ret_val;
}

void Mysql_shell::connect(
    const mysqlshdk::db::Connection_options &connection_options_,
    bool recreate_schema) {
  mysqlshdk::db::Connection_options connection_options(connection_options_);
  std::string pass;
  std::string schema_name;

  if (_options.interactive)
    print_connection_message(
        get_session_type(connection_options),
        connection_options.as_uri(
            mysqlshdk::db::uri::formats::no_scheme_no_password()),
        /*_options.app*/ "");

  // Retrieves the schema on which the session will work on
  shcore::Argument_list schema_arg;
  if (connection_options.has_schema()) {
    schema_name = connection_options.get_schema();
    schema_arg.push_back(shcore::Value(schema_name));
  }

  if (recreate_schema && schema_name.empty())
    throw shcore::Exception::runtime_error(
        "Recreate schema requested, but no schema specified");

  // Prompts for the password if needed
  if (!connection_options.has_password()) {
    if (_shell->password("Enter password: ", pass))
      connection_options.set_password(pass);
    else
      throw shcore::cancelled("Cancelled");
  }

  auto old_session(_shell->get_dev_session());

  std::shared_ptr<mysqlsh::ShellBaseSession> new_session;
  {
    // allow SIGINT to interrupt the connect()
    bool cancelled = false;
    shcore::Interrupt_handler intr([&cancelled]() {
      cancelled = true;
      return true;
    });
    new_session = create_session(connection_options);

    if (cancelled)
      throw shcore::cancelled("Cancelled");
  }

  _shell->set_dev_session(new_session);
  _global_shell->set_session_global(new_session);

  _target_server = connection_options;

  new_session->set_option("trace_protocol", _options.trace_protocol);

  if (recreate_schema) {
    println("Recreating schema " + schema_name + "...");
    try {
      new_session->drop_schema(schema_name);
    } catch (shcore::Exception &e) {
      if (e.is_mysql() && e.code() == 1008) {
        // ignore DB doesn't exist error
      } else {
        throw;
      }
    }
    new_session->create_schema(schema_name);

    new_session->call("setCurrentSchema", schema_arg);
  }
  if (_options.interactive) {
    if (old_session && old_session.unique() && old_session->is_open()) {
      if (_options.interactive)
        println("Closing old connection...");

      old_session->close();
    }

    std::string session_type = new_session->class_name();
    std::string message;

    message = "Your MySQL connection id is " +
              std::to_string(new_session->get_connection_id());
    if (session_type == "Session")
      message += " (X protocol)";
    try {
      message += "\nServer version: " +
                 new_session->query_one_string(
                     "select concat(@@version, ' ', @@version_comment)", 0);
    } catch (mysqlshdk::db::Error &e) {
      // ignore password expired errors
      if (e.code() == ER_MUST_CHANGE_PASSWORD) {
      } else {
        throw;
      }
    } catch (shcore::Exception &e) {
      // ignore password expired errors
      if (e.is_mysql() && e.code() == ER_MUST_CHANGE_PASSWORD) {
      } else {
        throw;
      }
    }
    message += "\n";
    // Any session could have a default schema after connection is done
    std::string default_schema_name = new_session->get_default_schema();

    // This will cause two things to happen
    // 1) Validation that the default schema is real
    // 2) Triggers the auto loading of the schema cache

    shcore::Object_bridge_ref default_schema;

    if (!default_schema_name.empty())
      default_schema = new_session->get_schema(default_schema_name);

    if (default_schema) {
      if (session_type == "ClassicSession")
        message += "Default schema set to `" + default_schema_name + "`.";
      else
        message += "Default schema `" + default_schema_name +
                   "` accessible through db.";
    } else {
      message += "No default schema selected; type \\use <schema> to set one.";
    }
    println(message);
  }
  _update_variables_pending = 2;
}

bool Mysql_shell::cmd_print_shell_help(const std::vector<std::string> &args) {
  bool printed = false;

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (args.size() > 1) {
    printed = _shell->print_help(args[1]);

    if (!printed) {
      std::string help;
      if (_shell_command_handler.get_command_help(args[1], help)) {
        _shell->println(help);
        printed = true;
      }
    }

    if (!printed) {
      std::string topic = "TOPIC_" + args[1];
      auto data = shcore::get_help_text(topic);

      if (!data.empty()) {
        _shell->println(shcore::format_markup_text(data, 80, 0));
        printed = true;
      }
    }
  }

  // If not specific help found, prints the generic help
  if (!printed) {
    _shell->print(
        _shell_command_handler.get_commands("===== Global Commands ====="));

    // Prints the active shell specific help
    _shell->print_help("");

    println("");
    println("For help on a specific command use the command as \\? <command>");
    println("");
    auto globals = _shell->get_global_objects(interactive_mode());
    std::vector<std::pair<std::string, std::string> > global_names;

    if (globals.size()) {
      for (auto name : globals) {
        auto object_val = _shell->get_global(name);
        auto object = std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(
            object_val.as_object());
        global_names.push_back({name, object->class_name()});
      }
    }

    // Inserts the default modules
    if (shcore::IShell_core::all_scripting_modes().is_set(
            interactive_mode())) {
      global_names.push_back({"mysqlx", "mysqlx"});
      global_names.push_back({"mysql", "mysql"});
    }

    std::sort(global_names.begin(), global_names.end());

    if (!global_names.empty() &&
        interactive_mode() != shcore::IShell_core::Mode::SQL) {
      println("===== Global Objects =====");
      for (auto name : global_names) {
        auto brief = shcore::get_help_text(name.second + "_INTERACTIVE_BRIEF");
        if (brief.empty())
          brief = shcore::get_help_text(name.second + "_BRIEF");

        if (!brief.empty())
          println(shcore::str_format("%-10s %s", name.first.c_str(),
                                     brief[0].c_str()));
      }
    }

    println();
    println(
        "Please note that MySQL Document Store APIs are subject to change in "
        "future");
    println("releases.");
    println("");
    println(
        "For more help on a global variable use <var>.help(), e.g. dba.help()");
    println("");
  }

  return true;
}

bool Mysql_shell::cmd_start_multiline(const std::vector<std::string> &args) {
  // This command is only available for SQL Mode
  if (args.size() == 1 &&
      _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    _input_mode = shcore::Input_state::ContinuedBlock;

    return true;
  }

  return false;
}

bool Mysql_shell::cmd_connect(const std::vector<std::string> &args) {
  bool error = false;
  Shell_options options;

  // Holds the argument index for the target to which the session will be
  // established
  size_t target_index = 1;

  if (args.size() > 1 && args.size() < 4) {
    if (args.size() == 3) {
      std::string arg_2 = args[2];
      arg_2 = shcore::str_strip(arg_2);

      if (!arg_2.empty()) {
        target_index++;
        options.uri = args[target_index];
      }
    }

    std::string arg = args[1];
    arg = shcore::str_strip(arg);

    if (arg.empty())
      error = true;
    else if (!arg.compare("-n") || !arg.compare("-N")) {
      options.session_type = mysqlsh::SessionType::X;
      print_error("The -n option is deprecated, please use --mysqlx or -mx instead\n");
    } else if (!arg.compare("-c") || !arg.compare("-C")) {
      options.session_type = mysqlsh::SessionType::Classic;
      print_error("The -c option is deprecated, please use --mysql or -mc instead\n");
    } else if (!arg.compare("-mx") || !arg.compare("--mysqlx")) {
      options.session_type = mysqlsh::SessionType::X;
    } else if (!arg.compare("-mc") || !arg.compare("--mysql")) {
      options.session_type = mysqlsh::SessionType::Classic;
    } else {
      if (args.size() == 3 && arg.compare("-ma"))
        error = true;
      else
        options.uri = args[target_index];
    }

    if (!error && !options.uri.empty()) {
      try {
        connect(options.connection_options());
      } catch (shcore::Exception &e) {
        print_error(std::string(e.format())+"\n");
      } catch (std::exception &e) {
        print_error(std::string(e.what())+"\n");
      }
    }
  } else {
    error = true;
  }
  if (error)
    print_error("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");

  return true;
}

bool Mysql_shell::cmd_quit(const std::vector<std::string> &UNUSED(args)) {
  _options.interactive = false;

  return true;
}

bool Mysql_shell::cmd_warnings(const std::vector<std::string> &UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] =
      shcore::Value::True();

  println("Show warnings enabled.");

  return true;
}

bool Mysql_shell::cmd_nowarnings(const std::vector<std::string> &UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] =
      shcore::Value::False();

  println("Show warnings disabled.");

  return true;
}

bool Mysql_shell::cmd_status(const std::vector<std::string>& UNUSED(args)) {
  std::string version_msg("MySQL Shell version ");
  version_msg += MYSH_VERSION;
  version_msg += "\n";
  println(version_msg);
  auto session = _shell->get_dev_session();

  if (session && session->is_open()) {
    auto status = session->get_status();
    (*status)["DELIMITER"] = shcore::Value(_shell->get_main_delimiter());
    std::string output_format =
        (*shcore::Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

    if (output_format.find("json") == 0)
      println(shcore::Value(status).json(output_format == "json"));
    else {
      std::string format = "%-30s%s";

      if (status->has_key("STATUS_ERROR")) {
        println(shcore::str_format(format.c_str(), "Error Retrieving Status: ", (*status)["STATUS_ERROR"].descr(true).c_str()));
      } else {
        if (status->has_key("SESSION_TYPE"))
          println(shcore::str_format(
              format.c_str(),
              "Session type: ", (*status)["SESSION_TYPE"].descr(true).c_str()));

        if (status->has_key("CONNECTION_ID"))
          println(shcore::str_format(
              format.c_str(), "Connection Id: ",
              (*status)["CONNECTION_ID"].descr(true).c_str()));

        if (status->has_key("DEFAULT_SCHEMA"))
          println(shcore::str_format(
              format.c_str(), "Default schema: ",
              (*status)["DEFAULT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_SCHEMA"))
          println(shcore::str_format(
              format.c_str(), "Current schema: ",
              (*status)["CURRENT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_USER"))
          println(shcore::str_format(
              format.c_str(),
              "Current user: ", (*status)["CURRENT_USER"].descr(true).c_str()));

        if (status->has_key("SSL_CIPHER") &&
            status->get_type("SSL_CIPHER") == shcore::String &&
            !status->get_string("SSL_CIPHER").empty())
          println(shcore::str_format(
              format.c_str(), "SSL: ",
              ("Cipher in use: " + (*status)["SSL_CIPHER"].descr(true))
                  .c_str()));
        else
          println(shcore::str_format(format.c_str(), "SSL: ", "Not in use."));

        if (status->has_key("DELIMITER"))
          println(shcore::str_format(format.c_str(), "Using delimiter: ", (*status)["DELIMITER"].descr(true).c_str()));
        if (status->has_key("SERVER_VERSION"))
          println(shcore::str_format(
              format.c_str(), "Server version: ",
              (*status)["SERVER_VERSION"].descr(true).c_str()));

        if (status->has_key("PROTOCOL_VERSION"))
          println(shcore::str_format(
              format.c_str(), "Protocol version: ",
              (*status)["PROTOCOL_VERSION"].descr(true).c_str()));

        if (status->has_key("CLIENT_LIBRARY"))
          println(shcore::str_format(format.c_str(), "Client library: ", (*status)["CLIENT_LIBRARY"].descr(true).c_str()));

        if (status->has_key("CONNECTION"))
          println(shcore::str_format(
              format.c_str(),
              "Connection: ", (*status)["CONNECTION"].descr(true).c_str()));

        if (status->has_key("SERVER_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Server characterset: ",
              (*status)["SERVER_CHARSET"].descr(true).c_str()));

        if (status->has_key("SCHEMA_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Schema characterset: ",
              (*status)["SCHEMA_CHARSET"].descr(true).c_str()));

        if (status->has_key("CLIENT_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Client characterset: ",
              (*status)["CLIENT_CHARSET"].descr(true).c_str()));

        if (status->has_key("CONNECTION_CHARSET"))
          println(shcore::str_format(
              format.c_str(), "Conn. characterset: ",
              (*status)["CONNECTION_CHARSET"].descr(true).c_str()));

        if (status->has_key("TCP_PORT"))
          println(shcore::str_format(format.c_str(), "TCP port: ", (*status)["TCP_PORT"].descr(true).c_str()));

        if (status->has_key("UNIX_SOCKET"))
          println(shcore::str_format(format.c_str(), "Unix socket: ", (*status)["UNIX_SOCKET"].descr(true).c_str()));

        if (status->has_key("UPTIME"))
          println(shcore::str_format(format.c_str(), "Uptime: ", (*status)["UPTIME"].descr(true).c_str()));

        if (status->has_key("SERVER_STATS")) {
          std::string stats = (*status)["SERVER_STATS"].descr(true);
          size_t start = stats.find(" ");
          start++;
          size_t end = stats.find(" ", start);

          std::string time = stats.substr(start, end - start);
          unsigned long ltime = std::stoul(time);
          std::string str_time = MySQL_timer::format_legacy(ltime, false, true);

          println(shcore::str_format(format.c_str(), "Uptime: ", str_time.c_str()));
          println("");
          println(stats.substr(end + 2));
        }
      }
    }
  } else {
    print_error("Not Connected.\n");
  }
  return true;
}

bool Mysql_shell::cmd_use(const std::vector<std::string> &args) {
  std::string error;
  if (_shell->get_dev_session() && _shell->get_dev_session()->is_open()) {
    std::string real_param;

    // If quoted, takes as param what's inside of the quotes
    auto start = args[0].find_first_of("\"'`");
    if (start != std::string::npos) {
      std::string quote = args[0].substr(start, 1);

      if (args[0].size() >= start) {
        auto end = args[0].find(quote, start + 1);

        if (end != std::string::npos)
          real_param = args[0].substr(start + 1, end - start - 1);
        else
          error = "Mistmatched quote on command parameter: " +
                  args[0].substr(start) + "\n";
      }
    } else if (args.size() == 2) {
      real_param = args[1];
    } else {
      error = "\\use <schema_name>\n";
    }
    if (error.empty()) {
      try {
        _global_shell->set_current_schema(real_param);

        auto session = _shell->get_dev_session();

        if (session) {
          auto session_type = session->class_name();
          std::string message =
              "Default schema `" + real_param + "` accessible through db.";

          if (session_type == "ClassicSession")
            message = "Default schema set to `" + real_param + "`.";
          else
            message =
                "Default schema `" + real_param + "` accessible through db.";

          println(message);

          _update_variables_pending = 1;
        }
      } catch (shcore::Exception &e) {
        error = e.format();
      } catch (mysqlshdk::db::Error &e) {
        error = shcore::Exception::mysql_error_with_code(e.what(), e.code())
                    .format();
      }
    }
  } else {
    error = "Not Connected.\n";
  }

  if (!error.empty())
    print_error(error);

  return true;
}

bool Mysql_shell::cmd_process_file(const std::vector<std::string> &params) {
  std::string file;

  if (params.size() < 2)
    throw shcore::Exception::runtime_error("Filename not specified");

  // The parameter 0 contains the somplete command as submitted by the user
  // File name would be on parameter 1
  file = shcore::str_strip(params[1]);

  // Adds support for quoted files in case there are spaces in the path
  if ((file[0] == '\'' && file[file.size() - 1] == '\'') ||
      (file[0] == '"' && file[file.size() - 1] == '"'))
    file = file.substr(1, file.size() - 2);

  if (file.empty()) {
    print_error("Usage: \\. <filename> | \\source <filename>\n");
    return true;
  }

  std::vector<std::string> args(params);

  // Deletes the original command
  args.erase(args.begin());
  args[0] = file;

  Base_shell::process_file(file, args);

  return true;
}

bool Mysql_shell::do_shell_command(const std::string &line) {
  // Special handling for use <db>, which in the classic client was overriden
  // as a built-in command and thus didn't need ; at the end
  if (_options.interactive &&
     _shell->interactive_mode() == shcore::IShell_core::Mode::SQL) {
    std::string tmp = shcore::str_rstrip(shcore::str_strip(line), ";");
    if (shcore::str_ibeginswith(tmp, "use ")) {
      return _shell_command_handler.process("\\" + tmp);
    }
  }

  // Verifies if the command can be handled by the active shell
  bool handled = _shell->handle_shell_command(line);

  // Global Command Processing (xShell specific)
  if (!handled) {
    handled = _shell_command_handler.process(line);
  }

  return handled;
}

void Mysql_shell::process_line(const std::string &line) {
  bool handled_as_command = false;
  std::string to_history;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() &&
      _input_mode == shcore::Input_state::Ok) {
    try {
      handled_as_command = do_shell_command(line);
    } catch (std::exception &exc) {
      std::string error(exc.what());
      error += "\n";
      print_error(error);
    }
  }

  if (handled_as_command)
    notify_executed_statement(line);
  else
    Base_shell::process_line(line);

  _shell->reconnect_if_needed();
}

static std::vector<std::string> g_patterns;

bool Mysql_shell::sql_safe_for_logging(const std::string &sql) {
  for (auto &pat : g_patterns) {
    if (shcore::match_glob(pat, sql)) {
      return false;
    }
  }
  return true;
}

void Mysql_shell::set_sql_safe_for_logging(const std::string &patterns) {
  g_patterns = shcore::split_string(patterns, ":");
}

}  // namespace mysqlsh
