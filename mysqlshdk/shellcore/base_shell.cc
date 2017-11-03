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
#include <mysql.h>
#include "shellcore/base_shell.h"
#include "shellcore/base_session.h"
#include "utils/utils_file.h"
#include "utils/utils_path.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_notifications.h"
#include "modules/devapi/base_resultset.h"
#include "shellcore/shell_resultset_dumper.h"
#include "utils/utils_time.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlsh {

std::shared_ptr<mysqlsh::Shell_options> Base_shell::shell_options =
    std::make_shared<mysqlsh::Shell_options>();

Base_shell::Base_shell(std::shared_ptr<Shell_options> cmdline_options,
                       shcore::Interpreter_delegate *custom_delegate) {
  Base_shell::shell_options =  cmdline_options;
  shcore::Interrupts::setup();

  std::string log_path = shcore::get_user_config_path();
  log_path += "mysqlsh.log";

  ngcommon::Logger::setup_instance(log_path.c_str(), options().log_to_stderr,
                                   options().log_level);

  _input_mode = shcore::Input_state::Ok;

  if (options().output_format.empty())
    shell_options->set(SHCORE_OUTPUT_FORMAT, shcore::Value("table"));

  _shell.reset(new shcore::Shell_core(custom_delegate));

  bool lang_initialized;
  _shell->switch_mode(shell_options->get().initial_mode, lang_initialized);
  _update_variables_pending = 1;

  _result_processor = std::bind(&Base_shell::process_result, this, _1);

  // This call is to initialize the SSL engine
  auto handle = mysql_init(NULL);
  mysql_close(handle);
}

void Base_shell::finish_init() {
  load_default_modules(_shell->interactive_mode());
  init_scripts(_shell->interactive_mode());
}

void Base_shell::print_connection_message(mysqlsh::SessionType type, const std::string& uri, const std::string& sessionid) {
  std::string stype;

  switch (type) {
    case mysqlsh::SessionType::X:
      stype = "an X protocol";
      break;
    case mysqlsh::SessionType::Classic:
      stype = "a Classic";
      break;
    case mysqlsh::SessionType::Auto:
      stype = "a";
      break;
  }

  std::string message;
  message += "Creating " + stype + " session to '" + uri + "'";

  println(message);
}

// load scripts for standard locations in order to be able to implement standard routines
void Base_shell::init_scripts(shcore::Shell_core::Mode mode) {
  std::string extension;

  if (mode == shcore::Shell_core::Mode::JavaScript)
    extension.append(".js");
  else if (mode == shcore::Shell_core::Mode::Python)
    extension.append(".py");
  else
    return;

  std::vector<std::string> scripts_paths;

  std::string user_file = "";

  try {
    // Checks existence of gobal startup script
    std::string path = shcore::get_global_config_path();
    path.append("shellrc");
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of startup script at MYSQLSH_HOME
    // Or the binary location if not a standard installation
    path = shcore::get_mysqlx_home_path();
    if (!path.empty())
      path.append("/share/mysqlsh/mysqlshrc");
    else {
      path = shcore::get_binary_folder();
      path.append("/mysqlshrc");
    }
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of user startup script
    path = shcore::get_user_config_path();
    path.append("mysqlshrc");
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    for (std::vector<std::string>::iterator i = scripts_paths.begin(); i != scripts_paths.end(); ++i) {
      std::ifstream stream(*i);
      if (stream && stream.peek() != std::ifstream::traits_type::eof())
        process_file(*i, {*i});
    }
  } catch (std::exception &e) {
    std::string error(e.what());
    error += "\n";
    print_error(error);
  }
}

void Base_shell::load_default_modules(shcore::Shell_core::Mode mode) {
  std::string tmp;
  if (mode == shcore::Shell_core::Mode::JavaScript) {
    tmp = "var mysqlx = require('mysqlx');";
    _shell->handle_input(tmp, _input_mode, _result_processor);
    tmp = "var mysql = require('mysql');";
    _shell->handle_input(tmp, _input_mode, _result_processor);
  } else if (mode == shcore::Shell_core::Mode::Python) {
    tmp = "from mysqlsh import mysqlx";
    _shell->handle_input(tmp, _input_mode, _result_processor);
    tmp = "from mysqlsh import mysql";
    _shell->handle_input(tmp, _input_mode, _result_processor);
  }
}

/**
 * Simple, default prompt handler.
 *
 * @return prompt string
 */
std::string Base_shell::prompt() {
  std::string ret_val;

  switch (_shell->interactive_mode()) {
    case shcore::IShell_core::Mode::None:
      break;
    case shcore::IShell_core::Mode::SQL:
      ret_val = "mysql-sql> ";
      break;
    case shcore::IShell_core::Mode::JavaScript:
      ret_val = "mysql-js> ";
      break;
    case shcore::IShell_core::Mode::Python:
      ret_val = "mysql-py> ";
      break;
  }
  // The continuation prompt should be used if state != Ok
  if (_input_mode != shcore::Input_state::Ok) {
    if (ret_val.length() > 4)
      ret_val = std::string(ret_val.length() - 4, ' ');
    else
      ret_val.clear();

    ret_val.append("... ");
  }

  return ret_val;
}

std::map<std::string, std::string> *Base_shell::prompt_variables() {
  if (_update_variables_pending > 0)
    update_prompt_variables(_update_variables_pending > 1);
  return &_prompt_variables;
}

void Base_shell::update_prompt_variables(bool reconnected) {
  _update_variables_pending = 0;

  if (reconnected)
    _prompt_variables.clear();

  auto session = _shell->get_dev_session();
  if (reconnected || _prompt_variables.empty()) {
    if (session) {
      mysqlshdk::db::Connection_options options(session->uri());
      std::string socket;
      std::string port;

      _prompt_variables["ssl"] = session->get_ssl_cipher().empty() ? "" : "SSL";
      _prompt_variables["uri"] =
          session->uri(mysqlshdk::db::uri::formats::scheme_user_transport());
      _prompt_variables["user"] = options.has_user() ? options.get_user() : "";
      _prompt_variables["host"] =
          options.has_host() ? options.get_host() : "localhost";

      if (session->get_member("__connection_info").descr().find("TCP") !=
          std::string::npos) {
        port =
            options.has_port() ? std::to_string(options.get_port()) : "";
      } else {
        if (options.has_socket())
          socket = options.get_socket();
        else
          socket = "default";
      }
      if (session->session_type() == mysqlsh::SessionType::Classic) {
        _prompt_variables["session"] = "c";
        if (socket.empty() && port.empty())
          port = "3306";
      } else {
        if (socket.empty() && port.empty())
          port = "33060";
        _prompt_variables["session"] = "x";
      }
      _prompt_variables["port"] = port;
      _prompt_variables["socket"] = socket;
      _prompt_variables["node_type"] = session->get_node_type();
    } else {
      _prompt_variables["ssl"] = "";
      _prompt_variables["uri"] = "";
      _prompt_variables["user"] = "";
      _prompt_variables["host"] = "";
      _prompt_variables["port"] = "";
      _prompt_variables["socket"] = "";
      _prompt_variables["session"] = "";
      _prompt_variables["node_type"] = "";
    }
  }
  if (session && session->is_open()) {
    try {
      _prompt_variables["schema"] = session->get_current_schema();
    } catch (...) {
      _prompt_variables["schema"] = "";
    }
  } else {
    _prompt_variables["schema"] = "";
  }
  switch (_shell->interactive_mode()) {
    case shcore::IShell_core::Mode::None:
      break;
    case shcore::IShell_core::Mode::SQL:
      _prompt_variables["mode"] = "sql";
      _prompt_variables["Mode"] = "SQL";
      break;
    case shcore::IShell_core::Mode::JavaScript:
      _prompt_variables["mode"] = "js";
      _prompt_variables["Mode"] = "JS";
      break;
    case shcore::IShell_core::Mode::Python:
      _prompt_variables["mode"] = "py";
      _prompt_variables["Mode"] = "Py";
      break;
  }
}

bool Base_shell::switch_shell_mode(shcore::Shell_core::Mode mode, const std::vector<std::string> &UNUSED(args)) {
  shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode) {
    _input_mode = shcore::Input_state::Ok;
    _input_buffer.clear();

    switch (mode) {
      case shcore::Shell_core::Mode::None:
        break;
      case shcore::Shell_core::Mode::SQL:
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to SQL mode... Commands end with ;");
        break;
      case shcore::Shell_core::Mode::JavaScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to JavaScript mode...");
#else
        println("JavaScript mode is not supported, command ignored.");
#endif
        break;
      case shcore::Shell_core::Mode::Python:
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to Python mode...");
#else
        println("Python mode is not supported, command ignored.");
#endif
        break;
    }

    // load scripts for standard locations
    if (lang_initialized) {
      load_default_modules(mode);
      init_scripts(mode);
    }
  }

  _update_variables_pending = 1;

  return true;
}

void Base_shell::println(const std::string &str) {
  _shell->println(str);
}

void Base_shell::print_error(const std::string &error) {
  _shell->print_error(error);
}

void Base_shell::process_line(const std::string &line) {
  std::string to_history;

  if (_input_mode == shcore::Input_state::ContinuedBlock && line.empty())
    _input_mode = shcore::Input_state::Ok;

  // Appends the line, no matter if it is an empty line
  _input_buffer.append(_shell->preprocess_input_line(line));

  // Appends the new line if anything has been added to the buffer
  if (!_input_buffer.empty())
    _input_buffer.append("\n");

  if (_input_mode != shcore::Input_state::ContinuedBlock && !_input_buffer.empty()) {
    try {
      _shell->handle_input(_input_buffer, _input_mode, _result_processor);

      // Here we analyze the input mode as it was let after executing the code
      if (_input_mode == shcore::Input_state::Ok) {
        to_history = _shell->get_handled_input();
      }
    } catch (shcore::Exception &exc) {
      _shell->print_value(shcore::Value(exc.error()), "error");
      to_history = _input_buffer;
    } catch (std::exception &exc) {
      std::string error(exc.what());
      error += "\n";
      print_error(error);
      to_history = _input_buffer;
    }

    // TODO: Do we need this cleanup? i.e. in case of exceptions above??
    // Clears the buffer if OK, if continued, buffer will contain
    // the non executed code
    if (_input_mode == shcore::Input_state::Ok)
      _input_buffer.clear();
  }

  if (!to_history.empty())
    notify_executed_statement(to_history);
}

void Base_shell::notify_executed_statement(const std::string& line) {
    shcore::Value::Map_type_ref data(new shcore::Value::Map_type());
    (*data)["statement"] = shcore::Value(line);
    shcore::ShellNotifications::get()->notify("SN_STATEMENT_EXECUTED", nullptr, data);
}

void Base_shell::process_result(shcore::Value result) {
  if (options().interactive ||
      _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    if (result) {
      shcore::Value shell_hook;
      std::shared_ptr<shcore::Object_bridge> object;
      if (result.type == shcore::Object) {
        object = result.as_object();
        if (object && object->has_member("__shell_hook__"))
          shell_hook = object->get_member("__shell_hook__");

        if (shell_hook) {
          shcore::Argument_list args;
          shcore::Value hook_result = object->call("__shell_hook__", args);

          // Recursive call to continue processing shell hooks if any
          process_result(hook_result);
        }
      }

      // If the function is not found the values still needs to be printed
      if (!shell_hook) {
        // Resultset objects get printed
        if (object && object->class_name().find("Result") != std::string::npos) {
          std::shared_ptr<mysqlsh::ShellBaseResult> resultset = std::static_pointer_cast<mysqlsh::ShellBaseResult> (object);

          // Result buffering will be done ONLY if on any of the scripting interfaces
          ResultsetDumper dumper(resultset, _shell->get_delegate(), _shell->interactive_mode() != shcore::IShell_core::Mode::SQL);
          dumper.dump();
        } else {
          // In JSON mode: the json representation is used for Object, Array and Map
          // For anything else a map is printed with the "value" key
          std::string tag;
          if (result.type != shcore::Object && result.type != shcore::Array && result.type != shcore::Map)
            tag = "value";

          _shell->print_value(result, tag);
        }
      }
    }
  }

  // Return value of undefined implies an error processing
  if (result.type == shcore::Undefined)
  _shell->set_error_processing();
}

int Base_shell::process_file(const std::string &path, const std::vector<std::string> &argv) {
  // Default return value will be 1 indicating there were errors
  int ret_val = 1;

  if (path.empty())
    print_error("Invalid filename");
  else if (_shell->is_module(path))
    _shell->execute_module(path, _result_processor, argv);
  else
  {
    std::string file = shcore::path::expand_user(path);
    if (shcore::is_folder(file)) {
        print_error(shcore::str_format("Failed to open file: '%s' is a "
            "directory\n", file.c_str()));
        return ret_val;
    }

    std::ifstream s(file.c_str());
    if (!s.fail()) {

      // The return value now depends on the stream processing
      ret_val = process_stream(s, file, argv);

      // When force is used, we do not care of the processing
      // errors
      if (options().force)
        ret_val = 0;

      s.close();
    } else {
      // TODO: add a log entry once logging is
      print_error(shcore::str_format("Failed to open file '%s', error: %s\n",
        file.c_str(), std::strerror(errno)));
    }
  }

  return ret_val;
}

int Base_shell::process_stream(std::istream & stream, const std::string& source,
    const std::vector<std::string> &argv, bool force_batch) {
  // If interactive is set, it means that the shell was started with the option to
  // Emulate interactive mode while processing the stream
  if (!force_batch && options().interactive) {
    if (options().full_interactive)
      _shell->print(prompt());

    bool comment_first_js_line =
        _shell->interactive_mode() == shcore::IShell_core::Mode::JavaScript;
    while (!stream.eof()) {
      std::string line;

      std::getline(stream, line);

      // When processing JavaScript files, validates the very first line to start with #!
      // If that's the case, it is replaced by a comment indicator //
      if (comment_first_js_line && line.size() > 1 && line[0] == '#' && line[1] == '!')
        line.replace(0, 2, "//");

      comment_first_js_line = false;

      if (options().full_interactive)
        println(line);

      process_line(line);

      if (options().full_interactive)
        _shell->print(prompt());
    }

    // Being interactive, we do not care about the return value
    return 0;
  } else {
    return _shell->process_stream(stream, source, _result_processor, argv);
  }
}

void Base_shell::set_global_object(
    const std::string &name, std::shared_ptr<shcore::Cpp_object_bridge> object,
    shcore::IShell_core::Mode_mask modes) {
  if (object) {
    _shell->set_global(
        name,
        shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(object)),
        modes);
  } else {
    _shell->set_global(name, shcore::Value(), modes);
  }
}
}
