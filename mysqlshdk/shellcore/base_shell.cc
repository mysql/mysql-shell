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
#include "shellcore/base_shell.h"

#include <tuple>

#include "mysqlshdk/libs/utils/fault_injection.h"
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

namespace {
const std::map<std::string, std::string> k_shell_hooks = {
    {"ClassicResult", "dump"},       {"Result", "dump"},
    {"DocResult", "dump"},           {"RowResult", "dump"},
    {"SqlResult", "dump"},           {"CollectionAdd", "execute"},
    {"CollectionFind", "execute"},   {"CollectionModify", "execute"},
    {"CollectionRemove", "execute"}, {"TableDelete", "execute"},
    {"TableInsert", "execute"},      {"TableSelect", "execute"},
    {"TableUpdate", "execute"},      {"SqlExecute", "execute"},
};
}

namespace {
void print_diag(const std::string &s) { current_console()->print_diag(s); }

void println(const std::string &s) { current_console()->println(s); }

void print(const std::string &s) { current_console()->print(s); }

void print_value(const shcore::Value &value, const std::string &s) {
  current_console()->print_value(value, s);
}

}  // namespace

Base_shell::Base_shell(const std::shared_ptr<Shell_options> &cmdline_options)
    : m_shell_options{cmdline_options},
      _deferred_output(std::make_unique<std::string>()) {
  shcore::current_interrupt()->setup();

  _input_mode = shcore::Input_state::Ok;

  _shell = std::make_shared<shcore::Shell_core>();
  _completer_object_registry =
      std::make_shared<shcore::completer::Object_registry>();
}

Base_shell::~Base_shell() {}

std::string Base_shell::get_shell_hook(const std::string &class_name) {
  auto item = k_shell_hooks.find(class_name);

  return item == k_shell_hooks.end() ? "" : item->second;
}

void Base_shell::finish_init() {
  current_console()->set_verbose(options().verbose_level);

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
}

// load scripts for standard locations in order to be able to implement standard
// routines
void Base_shell::init_scripts(shcore::Shell_core::Mode mode) {
  std::string extension;

  switch (mode) {
    case shcore::Shell_core::Mode::JavaScript:
      extension.append(".js");
      break;

    case shcore::Shell_core::Mode::Python:
      extension.append(".py");
      break;

    case shcore::Shell_core::Mode::None:
    case shcore::Shell_core::Mode::SQL:
      return;
  }

  try {
    std::vector<std::string> script_paths;
    const auto startup_script = "mysqlshrc" + extension;

    {
      // Checks existence of global startup script
      auto path = shcore::path::join_path(shcore::get_global_config_path(),
                                          startup_script);
      if (shcore::is_file(path)) script_paths.emplace_back(std::move(path));
    }

    {
      // Checks existence of startup script at MYSQLSH_HOME
      // Or the binary location if not a standard installation
      const auto home = shcore::get_mysqlx_home_path();
      std::string path;

      if (!home.empty()) {
        path =
            shcore::path::join_path(home, "share", "mysqlsh", startup_script);
      } else {
        path = shcore::path::join_path(shcore::get_binary_folder(),
                                       startup_script);
      }

      if (shcore::is_file(path)) script_paths.emplace_back(std::move(path));
    }

    {
      // Checks existence of user startup script
      auto path = shcore::path::join_path(shcore::get_user_config_path(),
                                          startup_script);
      if (shcore::is_file(path)) script_paths.emplace_back(std::move(path));
    }

    for (const auto &script : script_paths) {
      std::ifstream stream(script);
      if (stream && stream.peek() != std::ifstream::traits_type::eof()) {
        process_file(script, {script});
      }
    }
  } catch (const std::exception &e) {
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
  const auto type = clear_cache ? Prompt_variables_update_type::CLEAR_CACHE
                                : Prompt_variables_update_type::UPDATE;

  if (type > m_pending_update) {
    m_pending_update = type;
  }
}

void Base_shell::update_prompt_variables() {
  FI_SUPPRESS(mysql);
  FI_SUPPRESS(mysqlx);

  if (Prompt_variables_update_type::CLEAR_CACHE == m_pending_update) {
    _prompt_variables.clear();
  }

  const auto session = _shell->get_dev_session();
  if (_prompt_variables.empty()) {
    _prompt_variables["system_user"] = shcore::get_system_user();

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
      _prompt_variables["connection_id"] =
          std::to_string(session->get_connection_id());
    } else {
      _prompt_variables["ssl"] = "";
      _prompt_variables["uri"] = "";
      _prompt_variables["user"] = "";
      _prompt_variables["host"] = "";
      _prompt_variables["port"] = "";
      _prompt_variables["socket"] = "";
      _prompt_variables["session"] = "";
      _prompt_variables["node_type"] = "";
      _prompt_variables["connection_id"] = "";
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
                                   bool initializing,
                                   bool prompt_variables_update) {
  shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode) {
    _input_mode = shcore::Input_state::Ok;
    _input_buffer.clear();

    switch (mode) {
      case shcore::Shell_core::Mode::None:
        break;
      case shcore::Shell_core::Mode::SQL:
        if (_shell->switch_mode(mode) && !initializing)
          println("Switching to SQL mode... Commands end with ;");
        {
          auto sql =
              static_cast<shcore::Shell_sql *>(_shell->language_object(mode));
          if (!sql->result_processor()) {
            sql->set_result_processor(
                std::bind(&Base_shell::process_sql_result, this, _1, _2));
            lang_initialized = true;
          }
        }
        break;
      case shcore::Shell_core::Mode::JavaScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode) && !initializing)
          println("Switching to JavaScript mode...");
        {
          auto js = static_cast<shcore::Shell_javascript *>(
              _shell->language_object(mode));
          if (!js->result_processor()) {
            js->set_result_processor(
                std::bind(&Base_shell::process_result, this, _1, _2));
            _completer.add_provider(
                shcore::IShell_core::Mode_mask(
                    shcore::IShell_core::Mode::JavaScript),
                std::unique_ptr<shcore::completer::Provider>(
                    new shcore::completer::Provider_javascript(
                        _completer_object_registry, js->javascript_context())));
            lang_initialized = true;
          }
        }
#else
        println("JavaScript mode is not supported, command ignored.");
#endif
        break;
      case shcore::Shell_core::Mode::Python:
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode) && !initializing)
          println("Switching to Python mode...");
        {
          auto py = static_cast<shcore::Shell_python *>(
              _shell->language_object(mode));
          if (!py->result_processor()) {
            py->set_result_processor(
                std::bind(&Base_shell::process_result, this, _1, _2));
            _completer.add_provider(
                shcore::IShell_core::Mode_mask(
                    shcore::IShell_core::Mode::Python),
                std::unique_ptr<shcore::completer::Provider>(
                    new shcore::completer::Provider_python(
                        _completer_object_registry, py->python_context())));
            lang_initialized = true;
          }
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

  if (prompt_variables_update) request_prompt_variables_update();

  return lang_initialized;
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
      print_value(shcore::Value(exc.error()), "error");
      to_history = _input_buffer;
    } catch (const std::exception &exc) {
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
    const std::shared_ptr<mysqlshdk::db::IResult> &result,
    const shcore::Sql_result_info & /*info*/) {
  if (!result) {
    // Return value of undefined implies an error processing
    // TODO(alfredo) - signaling of errors should be moved down
    _shell->set_error_processing();
  }
}

void Base_shell::print_result(const shcore::Value &result) {
  if (result) {
    if (result.type == shcore::Object) {
      auto object = result.as_object();
      auto shell_hook = get_shell_hook(object->class_name());
      if (!shell_hook.empty()) {
        if (object->has_member(shell_hook)) {
          shcore::Value hook_result = object->call(shell_hook, {});

          // Recursive call to continue processing shell hooks if any
          process_result(hook_result, false);

          return;
        }
      }
    }

    // In JSON mode: the json representation is used for Object, Array and
    // Map For anything else a map is printed with the "value" key
    std::string tag;
    if (result.type != shcore::Object && result.type != shcore::Array &&
        result.type != shcore::Map)
      tag = "value";

    print_value(result, tag);
  }
}

void Base_shell::process_result(const shcore::Value &result, bool got_error) {
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
  } else {
    std::string file = shcore::path::expand_user(path);
    if (shcore::is_folder(file)) {
      print_diag(
          shcore::str_format("Failed to open file: '%s' is a "
                             "directory\n",
                             file.c_str()));
      return ret_val;
    }

#ifdef _WIN32
    std::ifstream s(shcore::utf8_to_wide(file));
#else
    std::ifstream s(file.c_str());
#endif

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

int Base_shell::run_module(const std::string &module,
                           const std::vector<std::string> &argv) {
  if (module.empty()) {
    print_diag("Invalid module name");
    return 1;
  }

  _shell->execute_module(module, argv);

  return 0;
}

int Base_shell::process_stream(std::istream &stream, const std::string &source,
                               const std::vector<std::string> &argv,
                               bool force_batch) {
  // If interactive is set, it means that the shell was started with the option
  // to Emulate interactive mode while processing the stream
  if (!force_batch && options().interactive) {
    if (options().full_interactive) print(prompt());

    bool comment_first_js_line =
        _shell->interactive_mode() == shcore::IShell_core::Mode::JavaScript;
    while (!stream.eof()) {
      std::string line;

      shcore::getline(stream, line);

      // When processing JavaScript files, validates the very first line to
      // start with #! If that's the case, it is replaced by a comment indicator
      // //
      if (comment_first_js_line && line.size() > 1 && line[0] == '#' &&
          line[1] == '!')
        line.replace(0, 2, "//");

      comment_first_js_line = false;

      if (options().full_interactive) println(line);

      process_line(line);

      if (options().full_interactive) print(prompt());
    }

    // Being interactive, we do not care about the return value
    return 0;
  } else {
    return _shell->process_stream(stream, source, argv);
  }
}

void Base_shell::set_global_object(
    const std::string &name,
    const std::shared_ptr<shcore::Cpp_object_bridge> &object,
    shcore::IShell_core::Mode_mask modes) {
  if (object) {
    _shell->set_global(name, shcore::Value(object), modes);
  } else {
    _shell->set_global(name, shcore::Value(), modes);
  }
}

}  // namespace mysqlsh
