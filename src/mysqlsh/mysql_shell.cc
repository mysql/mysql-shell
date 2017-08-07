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

#include "mysql_shell.h"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "modules/devapi/mod_mysqlx.h"
#include "modules/mod_mysql.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"
#include "src/interactive/interactive_dba_cluster.h"
#include "src/interactive/interactive_global_dba.h"
#include "src/interactive/interactive_global_schema.h"
#include "src/interactive/interactive_global_session.h"
#include "src/interactive/interactive_global_shell.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "utils/utils_time.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
Mysql_shell::Mysql_shell(const Shell_options &options, shcore::Interpreter_delegate *custom_delegate) : mysqlsh::Base_shell(options, custom_delegate) {

  // Registers the interactive objects if required
  _global_shell = std::shared_ptr<mysqlsh::Shell>(new mysqlsh::Shell(_shell.get()));
  _global_js_sys = std::shared_ptr<mysqlsh::Sys>(new mysqlsh::Sys(_shell.get()));
  _global_dba = std::shared_ptr<mysqlsh::dba::Dba>(new mysqlsh::dba::Dba(_shell.get()));

  if (options.wizards) {
    auto interactive_db = std::shared_ptr<shcore::Global_schema>(new shcore::Global_schema(*_shell.get()));
    auto interactive_session = std::shared_ptr<shcore::Global_session>(new shcore::Global_session(*_shell.get()));
    auto interactive_shell = std::shared_ptr<shcore::Global_shell>(new shcore::Global_shell(*_shell.get()));
    auto interactive_dba = std::shared_ptr<shcore::Global_dba>(new shcore::Global_dba(*_shell.get()));

    interactive_shell->set_target(_global_shell);
    interactive_dba->set_target(_global_dba);

    set_global_object(
        "db",
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(interactive_db),
        shcore::IShell_core::all_scripting_modes());
    set_global_object("session",
                      std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(
                          interactive_session));
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

  INIT_MODULE(mysqlsh::mysql::Mysql);
  INIT_MODULE(mysqlsh::mysqlx::Mysqlx);

  shcore::Value::Map_type_ref shcore_options =
      shcore::Shell_core_options::get();
  set_sql_safe_for_logging((*shcore_options)[SHCORE_HISTIGNORE].descr());

  std::string cmd_help_connect =
    "SYNTAX:\n"
    "   \\connect [-<TYPE>] <URI>\n\n"
    "WHERE:\n"
    "   TYPE is an optional parameter to specify the session type. Accepts the next values:\n"
    "        n: to establish an Node session\n"
    "        c: to establish a Classic session\n"
    "        If the session type is not specified, an Node session will be established.\n"
    "   URI is in the format of: [user[:password]@]hostname[:port]\n\n"
    "EXAMPLES:\n"
    "   \\connect root@localhost\n";

  std::string cmd_help_source =
    "SYNTAX:\n"
    "   \\source <sql_file_path>\n"
    "   \\. <sql_file_path>\n\n"
    "EXAMPLES:\n"
    "   \\source C:\\Users\\MySQL\\sakila.sql\n"
    "   \\. C:\\Users\\MySQL\\sakila.sql\n\n"
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
    "If it is either a Node or Classic session, the current schema will be updated (affects SQL mode).\n"
    "The global db variable will be updated to hold the requested schema.\n";

  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "",
                    Mysql_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND("\\sql", "Switch to SQL processing mode.", "",
                           std::bind(&Base_shell::switch_shell_mode, this,
                                     shcore::Shell_core::Mode::SQL, _1));
  SET_CUSTOM_SHELL_COMMAND("\\js", "Switch to JavaScript processing mode.", "",
                           std::bind(&Base_shell::switch_shell_mode, this,
                                     shcore::Shell_core::Mode::JavaScript, _1));
  SET_CUSTOM_SHELL_COMMAND("\\py", "Switch to Python processing mode.", "",
                           std::bind(&Base_shell::switch_shell_mode, this,
                                     shcore::Shell_core::Mode::Python, _1));
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
}

bool Mysql_shell::connect(bool primary_session) {
  try {
    mysqlshdk::db::Connection_options connection_options;
    bool secure_password = true;
    if (!_options.uri.empty()) {
      connection_options = shcore::get_connection_options(_options.uri);
      if (connection_options.has_password())
        secure_password = false;
    }

    // If the session is being created from command line
    // Individual parameters will override whatever was defined on the URI/stored connection
    if (primary_session) {
      if (_options.password)
        secure_password = false;

      shcore::update_connection_data(&connection_options,
                                     _options.user, _options.password,
                                     _options.host, _options.port,
                                     _options.sock, _options.schema,
                                     _options.ssl_options,
                                     _options.auth_method);
      if (_options.auth_method == "PLAIN")
        println("mysqlx: [Warning] PLAIN authentication method is NOT secure!");

      if (!secure_password)
        println("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
    }

    // If a scheme is given on the URI the session type must match the URI scheme
    if (connection_options.has_scheme()) {
      std::string scheme = connection_options.get_scheme();
      std::string error;

      if (_options.session_type == mysqlsh::SessionType::Auto) {
        if (scheme == "mysqlx")
          _options.session_type = mysqlsh::SessionType::Node;
        else if (scheme == "mysql")
          _options.session_type = mysqlsh::SessionType::Classic;
      } else {
        if (scheme == "mysqlx") {
          if (_options.session_type == mysqlsh::SessionType::Classic)
            error = "Invalid URI for Classic session";
        } else if (scheme == "mysql") {
          if (_options.session_type == mysqlsh::SessionType::Node)
            error = "Invalid URI for Node session";
        }
      }

      if (!error.empty())
        throw shcore::Exception::argument_error(error);
    }

    // TODO(rennox): Analize if this should actually exist... or if it
    // should be done right before connection for the missing data
    shcore::set_default_connection_data(&connection_options);

    if (_options.interactive)
      print_connection_message(
          _options.session_type,
          connection_options.as_uri(
              mysqlshdk::db::uri::formats::full_no_password()),
          /*_options.app*/ "");

    connect_session(&connection_options, _options.session_type,
                    primary_session ? _options.recreate_database : false);
  } catch (shcore::Exception &exc) {
    _shell->print_value(shcore::Value(exc.error()), "error");
    return false;
  } catch (std::exception &exc) {
    print_error(std::string(exc.what())+"\n");
    return false;
  }

  return true;
}
// TODO(rennox): Maybe call resolve connection credentials before calling
// this function... and pass const &
// doesn't sound like the credential resolution should be done here
shcore::Value Mysql_shell::connect_session(
    mysqlshdk::db::Connection_options *connection_options,
    mysqlsh::SessionType session_type, bool recreate_schema) {
  std::string pass;
  std::string schema_name;

  // Retrieves the schema on which the session will work on
  shcore::Argument_list schema_arg;
  if (connection_options->has_schema()) {
    schema_name = connection_options->get_schema();
    schema_arg.push_back(shcore::Value(schema_name));
  }

  if (recreate_schema && schema_name.empty())
    throw shcore::Exception::runtime_error(
        "Recreate schema requested, but no schema specified");

  // Prompts for the password if needed
  if (!connection_options->has_password()) {
    if (_shell->password("Enter password: ", pass))
      connection_options->set_password(pass);
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
    new_session =
        _global_shell->connect_session(*connection_options, session_type);
    if (cancelled)
      throw shcore::cancelled("Cancelled");
  }

  _global_shell->set_dev_session(new_session);

  new_session->set_option("trace_protocol", _options.trace_protocol);

  if (recreate_schema) {
    println("Recreating schema " + schema_name + "...");
    try {
      new_session->drop_schema(schema_name);
    } catch (shcore::Exception &e) {
      if (e.is_mysql() && e.code() == 1008)
        ; // ignore DB doesn't exist error
      else
        throw;
    }
    new_session->create_schema(schema_name);

    if (new_session->class_name().compare("XSession"))
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
    if (session_type == "NodeSession")
      message += " (X protocol)";
    try {
      message += "\nServer version: " +
                 new_session->query_one_string(
                     "select concat(@@version, ' ', @@version_comment)");
    } catch (shcore::Exception &e) {
      // ignore password expired errors
      if (e.is_mysql() && e.code() == 1820)
        ;
      else
        throw;
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
        message += "Default schema `" + default_schema_name + "` accessible through db.";
    } else
      message += "No default schema selected; type \\use <schema> to set one.";

    println(message);
  }

  _update_variables_pending = 2;

  return shcore::Value::Null();
}

bool Mysql_shell::cmd_print_shell_help(const std::vector<std::string>& args) {
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
  }

  // If not specific help found, prints the generic help
  if (!printed) {
    _shell->print(_shell_command_handler.get_commands("===== Global Commands ====="));

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
        auto object = std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(object_val.as_object());
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

    if (!global_names.empty()) {
      println("===== Global Objects =====");
      for (auto name : global_names) {
        auto brief = shcore::get_help_text(name.second + "_INTERACTIVE_BRIEF");
        if (brief.empty())
          brief = shcore::get_help_text(name.second + "_BRIEF");

        if (!brief.empty())
          println(shcore::str_format("%-10s %s", name.first.c_str(), brief[0].c_str()));
      }
    }

    println();
    println("Please note that MySQL Document Store APIs are subject to change in future");
    println("releases.");
    println("");
    println("For more help on a global variable use <var>.help(), e.g. dba.help()");
    println("");
  }

  return true;
}

bool Mysql_shell::cmd_start_multiline(const std::vector<std::string>& args) {
  // This command is only available for SQL Mode
  if (args.size() == 1 && _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    _input_mode = shcore::Input_state::ContinuedBlock;

    return true;
  }

  return false;
}

bool Mysql_shell::cmd_connect(const std::vector<std::string>& args) {
  bool error = false;
  bool uri = false;
  _options.session_type = mysqlsh::SessionType::Auto;

  // Holds the argument index for the target to which the session will be established
  size_t target_index = 1;

  if (args.size() > 1 && args.size() < 4) {
    if (args.size() == 3) {
      std::string arg_2 = args[2];
      arg_2 = shcore::str_strip(arg_2);

      if (!arg_2.empty()) {
        target_index++;
        uri = true;
      }
    }

    std::string arg = args[1];
    arg = shcore::str_strip(arg);

    if (arg.empty())
      error = true;
    else if (!arg.compare("-n") || !arg.compare("-N"))
      _options.session_type = mysqlsh::SessionType::Node;
    else if (!arg.compare("-c") || !arg.compare("-C"))
      _options.session_type = mysqlsh::SessionType::Classic;
    else {
      if (args.size() == 3)
        error = true;
      else
        uri = true;
    }

    if (!error) {
      if (uri) {
        /*if (args[target_index].find("$") == 0)
          _options.app = args[target_index].substr(1);
          else {
          _options.app = "";*/
        _options.uri = args[target_index];
        //}
      }
      connect();
    }
  } else
    error = true;

  if (error)
    print_error("\\connect [-<type>] <uri>\n");

  return true;
}

bool Mysql_shell::cmd_quit(const std::vector<std::string>& UNUSED(args)) {
  _options.interactive = false;

  return true;
}

bool Mysql_shell::cmd_warnings(const std::vector<std::string>& UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = shcore::Value::True();

  println("Show warnings enabled.");

  return true;
}

bool Mysql_shell::cmd_nowarnings(const std::vector<std::string>& UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = shcore::Value::False();

  println("Show warnings disabled.");

  return true;
}

bool Mysql_shell::cmd_status(const std::vector<std::string>& UNUSED(args)) {
  std::string version_msg("MySQL Shell Version ");
  version_msg += MYSH_VERSION;
  version_msg += "\n";
  println(version_msg);

  if (_shell->get_dev_session() && _shell->get_dev_session()->is_open()) {
    auto status = _shell->get_dev_session()->get_status();
    std::string output_format = (*shcore::Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

    if (output_format.find("json") == 0)
      println(shcore::Value(status).json(output_format == "json"));
    else {
      std::string format = "%-30s%s";

      if (status->has_key("STATUS_ERROR"))
        println(shcore::str_format(format.c_str(), "Error Retrieving Status: ", (*status)["STATUS_ERROR"].descr(true).c_str()));
      else {
        if (status->has_key("SESSION_TYPE"))
          println(shcore::str_format(format.c_str(), "Session type: ", (*status)["SESSION_TYPE"].descr(true).c_str()));

        if (status->has_key("NODE_TYPE"))
          println(shcore::str_format(format.c_str(), "Server type: ", (*status)["NODE_TYPE"].descr(true).c_str()));

        if (status->has_key("CONNECTION_ID"))
          println(shcore::str_format(format.c_str(), "Connection Id: ", (*status)["CONNECTION_ID"].descr(true).c_str()));

        if (status->has_key("DEFAULT_SCHEMA"))
          println(shcore::str_format(format.c_str(), "Default schema: ", (*status)["DEFAULT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_SCHEMA"))
          println(shcore::str_format(format.c_str(), "Current schema: ", (*status)["CURRENT_SCHEMA"].descr(true).c_str()));

        if (status->has_key("CURRENT_USER"))
          println(shcore::str_format(format.c_str(), "Current user: ", (*status)["CURRENT_USER"].descr(true).c_str()));

        if (status->has_key("SSL_CIPHER") &&
            status->get_type("SSL_CIPHER") == shcore::String &&
            !status->get_string("SSL_CIPHER").empty())
          println(shcore::str_format(
              format.c_str(), "SSL: ",
              ("Cipher in use: " + (*status)["SSL_CIPHER"].descr(true))
                  .c_str()));
        else
          println(shcore::str_format(format.c_str(), "SSL: ", "Not in use."));

        if (status->has_key("SERVER_VERSION"))
          println(shcore::str_format(format.c_str(), "Server version: ", (*status)["SERVER_VERSION"].descr(true).c_str()));

        if (status->has_key("SERVER_INFO"))
          println(shcore::str_format(format.c_str(), "Server info: ", (*status)["SERVER_INFO"].descr(true).c_str()));

        if (status->has_key("PROTOCOL_VERSION"))
          println(shcore::str_format(format.c_str(), "Protocol version: ", (*status)["PROTOCOL_VERSION"].descr(true).c_str()));

        if (status->has_key("CONNECTION"))
          println(shcore::str_format(format.c_str(), "Connection: ", (*status)["CONNECTION"].descr(true).c_str()));

        if (status->has_key("SERVER_CHARSET"))
          println(shcore::str_format(format.c_str(), "Server characterset: ", (*status)["SERVER_CHARSET"].descr(true).c_str()));

        if (status->has_key("SCHEMA_CHARSET"))
          println(shcore::str_format(format.c_str(), "Schema characterset: ", (*status)["SCHEMA_CHARSET"].descr(true).c_str()));

        if (status->has_key("CLIENT_CHARSET"))
          println(shcore::str_format(format.c_str(), "Client characterset: ", (*status)["CLIENT_CHARSET"].descr(true).c_str()));

        if (status->has_key("CONNECTION_CHARSET"))
          println(shcore::str_format(format.c_str(), "Conn. characterset: ", (*status)["CONNECTION_CHARSET"].descr(true).c_str()));

        if (status->has_key("SERVER_STATS")) {
          std::string stats = (*status)["SERVER_STATS"].descr(true);
          size_t start = stats.find(" ");
          start++;
          size_t end = stats.find(" ", start);

          std::string time = stats.substr(start, end - start);
          unsigned long ltime = std::stoul(time);
          std::string str_time = MySQL_timer::format_legacy(ltime, false, true);

          println(shcore::str_format(format.c_str(), "Up time: ", str_time.c_str()));
          println("");
          println(stats.substr(end + 2));
        }
      }
    }
  } else
    print_error("Not Connected.\n");

  return true;
}

bool Mysql_shell::cmd_use(const std::vector<std::string>& args) {
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
          error = "Mistmatched quote on command parameter: " + args[0].substr(start) + "\n";
      }
    } else if (args.size() == 2)
      real_param = args[1];
    else
      error = "\\use <schema_name>\n";

    if (error.empty()) {
      try {
        _global_shell->set_current_schema(real_param);

        auto session = _shell->get_dev_session();

        if (session) {
          auto session_type = session->class_name();
          std::string message = "Schema `" + real_param + "` accessible through db.";

          if (session_type == "ClassicSession")
            message = "Schema set to `" + real_param + "`.";
          else
            message = "Schema `" + real_param + "` accessible through db.";

          println(message);
        }
      } catch (shcore::Exception &e) {
        error = e.format();
      }
    }
  } else {
    error = "Not Connected.\n";
  }
  _update_variables_pending = 1;

  if (!error.empty())
    print_error(error);

  return true;
}

bool Mysql_shell::cmd_process_file(const std::vector<std::string>& params) {
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
  // Verifies if the command can be handled by the active shell
  bool handled = _shell->handle_shell_command(line);

  // Global Command Processing (xShell specific)
  if (!handled)
    handled = _shell_command_handler.process(line);

  return handled;
}

void Mysql_shell::process_line(const std::string &line) {
  bool handled_as_command = false;
  std::string to_history;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() && _input_mode == shcore::Input_state::Ok) {
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
