/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/base_shell.h"

#include <tuple>

#include "modules/devapi/base_resultset.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_notifications.h"
#include "shellcore/shell_resultset_dumper.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"
#ifdef HAVE_V8
#include "mysqlshdk/shellcore/provider_javascript.h"
#include "shellcore/shell_jscript.h"
#endif
#ifdef HAVE_PYTHON
#include "mysqlshdk/shellcore/provider_python.h"
#include "shellcore/shell_python.h"
#endif
#include "shellcore/shell_sql.h"

namespace mysqlsh {

Base_shell::Base_shell(std::shared_ptr<Shell_options> cmdline_options,
                       shcore::Interpreter_delegate *custom_delegate)
    : m_shell_options{cmdline_options},
      _deferred_output(new std::string()),
      m_console_handler{
          std::make_shared<mysqlsh::Shell_console>(custom_delegate)} {
  shcore::Interrupts::setup();

  std::string log_path =
      shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");

  ngcommon::Logger::setup_instance(log_path.c_str(), options().log_to_stderr,
                                   options().log_level);

  _input_mode = shcore::Input_state::Ok;

  _shell.reset(new shcore::Shell_core(m_console_handler.get().get()));
  _completer_object_registry.reset(new shcore::completer::Object_registry());
}

void Base_shell::finish_init() {
  shcore::IShell_core::Mode initial_mode = options().initial_mode;
  if (initial_mode == shcore::IShell_core::Mode::None) {
#ifdef HAVE_V8
    initial_mode = shcore::IShell_core::Mode::JavaScript;
#else
#ifdef HAVE_PYTHON
    initial_mode = shcore::IShell_core::Mode::Python;
#else
    initial_mode = shcore::IShell_core::Mode::SQL;
#endif
#endif
  }
  // Final initialization that must happen outside the constructor
  switch_shell_mode(initial_mode, {}, true);

  // Pre-init the SQL completer, since it's used in places other than SQL mode
  _provider_sql.reset(new shcore::completer::Provider_sql());
  _completer.add_provider(
      shcore::IShell_core::Mode_mask(shcore::IShell_core::Mode::SQL),
      _provider_sql);

  load_plugins();
}

// load scripts for standard locations in order to be able to implement standard
// routines
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
    if (shcore::file_exists(path)) scripts_paths.push_back(path);

    // Checks existence of startup script at MYSQLSH_HOME
    // Or the binary location if not a standard installation
    path = shcore::get_mysqlx_home_path();
    if (!path.empty()) {
      path.append("/share/mysqlsh/mysqlshrc");
    } else {
      path = shcore::get_binary_folder();
      path.append("/mysqlshrc");
    }
    path.append(extension);
    if (shcore::file_exists(path)) scripts_paths.push_back(path);

    // Checks existence of user startup script
    path = shcore::get_user_config_path();
    path.append("mysqlshrc");
    path.append(extension);
    if (shcore::file_exists(path)) scripts_paths.push_back(path);

    for (std::vector<std::string>::iterator i = scripts_paths.begin();
         i != scripts_paths.end(); ++i) {
      std::ifstream stream(*i);
      if (stream && stream.peek() != std::ifstream::traits_type::eof())
        process_file(*i, {*i});
    }
  } catch (std::exception &e) {
    std::string error(e.what());
    error += "\n";
    print_diag(error);
  }
}

void Base_shell::load_default_modules(shcore::Shell_core::Mode mode) {
  std::string tmp;
  if (mode == shcore::Shell_core::Mode::JavaScript) {
    tmp = "var mysqlx = require('mysqlx');";
    _shell->handle_input(tmp, _input_mode);
    tmp = "var mysql = require('mysql');";
    _shell->handle_input(tmp, _input_mode);
  } else if (mode == shcore::Shell_core::Mode::Python) {
    tmp = "from mysqlsh import mysqlx";
    _shell->handle_input(tmp, _input_mode);
    tmp = "from mysqlsh import mysql";
    _shell->handle_input(tmp, _input_mode);
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
  if (m_pending_update != Prompt_variables_update_type::NO_UPDATE) {
    update_prompt_variables();
  }

  return &_prompt_variables;
}

void Base_shell::request_prompt_variables_update(bool clear_cache) {
  m_pending_update = clear_cache ? Prompt_variables_update_type::CLEAR_CACHE
                                 : Prompt_variables_update_type::UPDATE;
}

void Base_shell::update_prompt_variables() {
  if (Prompt_variables_update_type::CLEAR_CACHE == m_pending_update) {
    _prompt_variables.clear();
  }

  const auto session = _shell->get_dev_session();
  if (_prompt_variables.empty()) {
    if (session && session->is_open()) {
      mysqlshdk::db::Connection_options options(session->uri());
      std::string socket;
      std::string port;

      _prompt_variables["ssl"] = session->get_ssl_cipher().empty() ? "" : "SSL";
      _prompt_variables["uri"] =
          session->uri(mysqlshdk::db::uri::formats::user_transport());
      _prompt_variables["user"] = options.has_user() ? options.get_user() : "";
      _prompt_variables["host"] =
          options.has_host() ? options.get_host() : "localhost";

      if (session->get_member("__connection_info").descr().find("TCP") !=
          std::string::npos) {
        port = options.has_port() ? std::to_string(options.get_port()) : "";
      } else {
        if (options.has_socket())
          socket = options.get_socket();
        else
          socket = "default";
      }
      if (session->session_type() == mysqlsh::SessionType::Classic) {
        _prompt_variables["session"] = "c";
        if (socket.empty() && port.empty()) port = "3306";
      } else {
        if (socket.empty() && port.empty()) port = "33060";
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

  m_pending_update = Prompt_variables_update_type::NO_UPDATE;
}

bool Base_shell::switch_shell_mode(shcore::Shell_core::Mode mode,
                                   const std::vector<std::string> & /*args*/,
                                   bool initializing) {
  shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode) {
    _input_mode = shcore::Input_state::Ok;
    _input_buffer.clear();

    switch (mode) {
      case shcore::Shell_core::Mode::None:
        break;
      case shcore::Shell_core::Mode::SQL:
        if (_shell->switch_mode(mode, lang_initialized) && !initializing)
          println("Switching to SQL mode... Commands end with ;");
        if (lang_initialized) {
          auto sql =
              static_cast<shcore::Shell_sql *>(_shell->language_object(mode));
          sql->set_result_processor(
              std::bind(&Base_shell::process_sql_result, this, _1, _2));
        }
        break;
      case shcore::Shell_core::Mode::JavaScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode, lang_initialized) && !initializing)
          println("Switching to JavaScript mode...");
        if (lang_initialized) {
          auto js = static_cast<shcore::Shell_javascript *>(
              _shell->language_object(mode));
          js->set_result_processor(
              std::bind(&Base_shell::process_result, this, _1, _2));
          _completer.add_provider(
              shcore::IShell_core::Mode_mask(
                  shcore::IShell_core::Mode::JavaScript),
              std::unique_ptr<shcore::completer::Provider>(
                  new shcore::completer::Provider_javascript(
                      _completer_object_registry, js->javascript_context())));
        }
#else
        println("JavaScript mode is not supported, command ignored.");
#endif
        break;
      case shcore::Shell_core::Mode::Python:
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode, lang_initialized) && !initializing)
          println("Switching to Python mode...");
        if (lang_initialized) {
          auto py = static_cast<shcore::Shell_python *>(
              _shell->language_object(mode));
          py->set_result_processor(
              std::bind(&Base_shell::process_result, this, _1, _2));
          _completer.add_provider(
              shcore::IShell_core::Mode_mask(shcore::IShell_core::Mode::Python),
              std::unique_ptr<shcore::completer::Provider>(
                  new shcore::completer::Provider_python(
                      _completer_object_registry, py->python_context())));
        }
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

  request_prompt_variables_update();

  return lang_initialized;
}

void Base_shell::print(const std::string &str) {
  m_console_handler.get()->print(str);
}

void Base_shell::println(const std::string &str) {
  m_console_handler.get()->println(str);
}

/**
 * Print output after the shell initialization is done (after Copyright info)
 * @param str text to be printed.
 */
void Base_shell::println_deferred(const std::string &str) {
  // This can't be called once the deferred output is flusehd
  assert(_deferred_output != nullptr);
  _deferred_output->append(str + "\n");
}

// This function now calls the console one, which will print the error
// but to the STDOUT, not used ATM as all the previous calls were moved
// to print_diag() for further analysis.
void Base_shell::print_error(const std::string &error) {
  m_console_handler.get()->print_error(error);
}

void Base_shell::print_diag(const std::string &error) {
  m_console_handler.get()->print_diag(error);
}

void Base_shell::print_warning(const std::string &message) {
  m_console_handler.get()->print_warning(message);
}

void Base_shell::clear_input() {
  _input_mode = shcore::Input_state::Ok;
  _input_buffer.clear();
  _shell->clear_input();
}

void Base_shell::process_line(const std::string &line) {
  std::string to_history;

  if (_input_mode == shcore::Input_state::ContinuedBlock && line.empty())
    _input_mode = shcore::Input_state::Ok;

  // Appends the line, no matter if it is an empty line
  _input_buffer.append(_shell->preprocess_input_line(line));

  // Appends the new line if anything has been added to the buffer
  if (!_input_buffer.empty()) _input_buffer.append("\n");

  if (_input_mode != shcore::Input_state::ContinuedBlock &&
      !_input_buffer.empty()) {
    try {
      _shell->handle_input(_input_buffer, _input_mode);

      // Here we analyze the input mode as it was let after executing the code
      if (_input_mode == shcore::Input_state::Ok) {
        to_history = _shell->get_handled_input();
      }
    } catch (shcore::Exception &exc) {
      m_console_handler.get()->print_value(shcore::Value(exc.error()), "error");
      to_history = _input_buffer;
    } catch (std::exception &exc) {
      std::string error(exc.what());
      error += "\n";
      print_diag(error);
      to_history = _input_buffer;
    }

    // TODO: Do we need this cleanup? i.e. in case of exceptions above??
    // Clears the buffer if OK, if continued, buffer will contain
    // the non executed code
    if (_input_mode == shcore::Input_state::Ok) _input_buffer.clear();
  }

  if (!to_history.empty()) notify_executed_statement(to_history);
}

void Base_shell::notify_executed_statement(const std::string &line) {
  shcore::Value::Map_type_ref data(new shcore::Value::Map_type());
  (*data)["statement"] = shcore::Value(line);
  shcore::ShellNotifications::get()->notify("SN_STATEMENT_EXECUTED", nullptr,
                                            data);
}

void Base_shell::process_sql_result(
    std::shared_ptr<mysqlshdk::db::IResult> result,
    const shcore::Sql_result_info &info) {
  if (!result) {
    // Return value of undefined implies an error processing
    // TODO(alfredo) - signaling of errors should be moved down
    _shell->set_error_processing();
  }
}

void Base_shell::print_result(shcore::Value result) {
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
        process_result(hook_result, false);
      }
    }

    // If the function is not found the values still needs to be printed
    if (!shell_hook) {
      // Resultset objects get printed
      if (object && object->class_name().find("Result") != std::string::npos) {
        std::shared_ptr<mysqlsh::ShellBaseResult> resultset =
            std::static_pointer_cast<mysqlsh::ShellBaseResult>(object);

        // TODO(alfredo) - dumping of SqlResults should be redirected to
        // process_sql_result()

        // Result buffering will be done ONLY if on any of the scripting
        // interfaces
        Resultset_dumper dumper(
            resultset->get_result(),
            _shell->interactive_mode() != shcore::IShell_core::Mode::SQL);

        bool is_doc_result = resultset->is_doc_result();
        bool is_query = is_doc_result || resultset->is_row_result();
        std::string item_label = is_doc_result
                                     ? "document"
                                     : resultset->is_result() ? "item" : "row";

        dumper.dump(item_label, is_query, is_doc_result);
      } else {
        // In JSON mode: the json representation is used for Object, Array and
        // Map For anything else a map is printed with the "value" key
        std::string tag;
        if (result.type != shcore::Object && result.type != shcore::Array &&
            result.type != shcore::Map)
          tag = "value";

        m_console_handler.get()->print_value(result, tag);
      }
    }
  }
}

void Base_shell::process_result(shcore::Value result, bool got_error) {
  assert(_shell->interactive_mode() != shcore::Shell_core::Mode::SQL);

  if (options().interactive) print_result(result);

  if (got_error) _shell->set_error_processing();
}

int Base_shell::process_file(const std::string &path,
                             const std::vector<std::string> &argv) {
  // Default return value will be 1 indicating there were errors
  int ret_val = 1;

  if (path.empty()) {
    print_diag("Invalid filename");
  } else if (_shell->is_module(path)) {
    _shell->execute_module(path, argv);
  } else {
    std::string file = shcore::path::expand_user(path);
    if (shcore::is_folder(file)) {
      print_diag(
          shcore::str_format("Failed to open file: '%s' is a "
                             "directory\n",
                             file.c_str()));
      return ret_val;
    }

    std::ifstream s(file.c_str());
    if (!s.fail()) {
      // The return value now depends on the stream processing
      ret_val = process_stream(s, file, argv);

      // When force is used, we do not care of the processing
      // errors
      if (options().force) ret_val = 0;

      s.close();
    } else {
      // TODO: add a log entry once logging is
      print_diag(shcore::str_format("Failed to open file '%s', error: %s\n",
                                    file.c_str(), std::strerror(errno)));
    }
  }

  return ret_val;
}

int Base_shell::process_stream(std::istream &stream, const std::string &source,
                               const std::vector<std::string> &argv,
                               bool force_batch) {
  // If interactive is set, it means that the shell was started with the option
  // to Emulate interactive mode while processing the stream
  if (!force_batch && options().interactive) {
    if (options().full_interactive) m_console_handler.get()->print(prompt());

    bool comment_first_js_line =
        _shell->interactive_mode() == shcore::IShell_core::Mode::JavaScript;
    while (!stream.eof()) {
      std::string line;

      std::getline(stream, line);

      // When processing JavaScript files, validates the very first line to
      // start with #! If that's the case, it is replaced by a comment indicator
      // //
      if (comment_first_js_line && line.size() > 1 && line[0] == '#' &&
          line[1] == '!')
        line.replace(0, 2, "//");

      comment_first_js_line = false;

      if (options().full_interactive) println(line);

      process_line(line);

      if (options().full_interactive) m_console_handler.get()->print(prompt());
    }

    // Being interactive, we do not care about the return value
    return 0;
  } else {
    return _shell->process_stream(stream, source, argv);
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

void Base_shell::load_plugins() {
  const auto initial_mode = _shell->interactive_mode();
  const std::string plugin_directories[] = {
      shcore::path::join_path(shcore::get_user_config_path(), "reporters")};
  // mode, extension, files to load
  std::vector<std::tuple<shcore::IShell_core::Mode, const char *,
                         std::vector<std::string>>>
      languages;

#ifdef HAVE_V8
  languages.emplace_back(shcore::IShell_core::Mode::JavaScript, ".js",
                         std::vector<std::string>());
#endif  // HAVE_V8
#ifdef HAVE_PYTHON
  languages.emplace_back(shcore::IShell_core::Mode::Python, ".py",
                         std::vector<std::string>());
#endif  // HAVE_PYTHON

  // iterate over directories, find files to load
  for (const auto &dir : plugin_directories) {
    if (shcore::is_folder(dir)) {
      shcore::iterdir(dir, [&languages, &dir](const std::string &name) {
        const auto full_path = shcore::path::join_path(dir, name);

        // make sure it's a file, not a directory
        if (shcore::is_file(full_path)) {
          for (auto &language : languages) {
            // check extension, ignoring case
            if (shcore::str_iendswith(name, std::get<1>(language))) {
              std::get<2>(language).emplace_back(full_path);
            }
          }
        }

        return true;
      });
    }
  }

  // if plugins are found, switch to the appropriate mode and load all files
  for (const auto &language : languages) {
    const auto &mode = std::get<0>(language);
    const auto &files_to_load = std::get<2>(language);

    if (!files_to_load.empty()) {
      switch_shell_mode(mode, {}, true);

      for (const auto &file : files_to_load) {
        log_debug("--------> loading %s", file.c_str());
        _shell->load_plugin(file);
      }
    }
  }

  // switch back to the initial mode
  switch_shell_mode(initial_mode, {}, true);
}

}  // namespace mysqlsh
