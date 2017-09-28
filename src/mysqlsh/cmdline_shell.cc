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

#include "mysqlsh/cmdline_shell.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#else
#include <unistd.h>
#endif

#include "ext/linenoise-ng/include/linenoise.h"
#include "modules/devapi/base_resultset.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_time.h"
#include "shell_cmdline_options.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_core_options.h"  // <---
#include "shellcore/utils_help.h"

#ifdef WIN32
#undef max
#define snprintf _snprintf
#endif

extern char *mysh_get_tty_password(const char *opt_message);

#define CTRL_C_STR "\003"

namespace mysqlsh {

Command_line_shell::Command_line_shell(const Shell_options &options)
    : mysqlsh::Mysql_shell(options, &_delegate) {
  _delegate.user_data = this;
  _delegate.print = &Command_line_shell::deleg_print;
  _delegate.print_error = &Command_line_shell::deleg_print_error;
  _delegate.prompt = &Command_line_shell::deleg_prompt;
  _delegate.password = &Command_line_shell::deleg_password;
  _delegate.source = &Command_line_shell::deleg_source;
  _delegate.print_value = nullptr;

  _refresh_needed = false;
  _output_printed = false;

  observe_notification("SN_STATEMENT_EXECUTED");

  finish_init();
  observe_notification("SN_SESSION_CONNECTED");

  _history.set_limit(std::min<int64_t>(
      shcore::Shell_core_options::get()->at(SHCORE_HISTORY_MAX_SIZE).as_int(),
      (1 << 31) - 1));

  observe_notification(SN_SHELL_OPTION_CHANGED);

  const std::string cmd_help_history =
      "SYNTAX:\n"
      "   \\history                     Display history entries.\n"
      "   \\history del <num>[-<num>]   Delete entry/entries from history.\n"
      "   \\history clear               Clear history.\n"
      "   \\history save                Save history to file.\n\n"
      "EXAMPLES:\n"
      "   \\history\n"
      "   \\history del 123       Delete entry number 123\n"
      "   \\history del 10-20     Delete entries 10 to 20\n"
      "   \\history del 10-       Delete entries from 10 to last\n"
      "\n"
      "NOTE: The history.autoSave shell option may be set to true to "
      "automatically save the contents of the command history when the shell "
      "exits.";
  SET_SHELL_COMMAND("\\history", "View and edit command line history.",
                    cmd_help_history, Command_line_shell::cmd_history);
}

void Command_line_shell::load_prompt_theme(const std::string &path) {
  if (!path.empty()) {
    std::ifstream f;
    f.open(path);
    if (f.good()) {
      std::stringstream buffer;
      buffer << f.rdbuf();
      try {
        shcore::Value theme(shcore::Value::parse(buffer.str().c_str()));
        _prompt.set_theme(theme);
      } catch (std::exception &e) {
        log_warning("Error loading prompt theme '%s': %s", path.c_str(),
                    e.what());
        print_error(shcore::str_format("Error loading prompt theme '%s': %s\n",
                                       path.c_str(), e.what()));
      }
    }
  }
}

void Command_line_shell::load_state(const std::string &statedir) {
  std::string path = statedir + "/history";
  if (!_history.load(path)) {
    print_error(
        shcore::str_format("Could not load command history from %s: %s\n",
                           path.c_str(), strerror(errno)));
  }
}

void Command_line_shell::save_state(const std::string &statedir) {
  if (shcore::Shell_core_options::get()
          ->at(SHCORE_HISTORY_AUTOSAVE)
          .as_bool()) {
    std::string path = statedir + "/history";
    if (linenoiseHistorySave(path.c_str()) < 0) {
      print_error(
          shcore::str_format("Could not save command history to %s: %s\n",
                             path.c_str(), strerror(errno)));
    }
  }
}

bool Command_line_shell::cmd_history(const std::vector<std::string> &args) {
  if (args.size() == 1) {
    _history.dump([this](const std::string &s) { println(s); });
  } else if (args[1] == "clear") {
    if (args.size() == 2) {
      _history.clear();
    } else {
      println("\\history clear does not take any parameters");
    }
  } else if (args[1] == "save") {
    std::string path = shcore::get_user_config_path() + "/history";
    if (linenoiseHistorySave(path.c_str()) < 0) {
      print_error(shcore::str_format("Could not save command history to %s: %s",
                                     path.c_str(), strerror(errno)));
    } else {
      println(shcore::str_format("Command history file saved with %i entries.",
              linenoiseHistorySize()));
    }
  } else if (args[1] == "delete" || args[1] == "del") {
    if (args.size() != 3) {
      println("\\history delete requires entry number to be deleted");
    } else {
      auto sep = args[2].find('-');
      try {
        int first;
        int last;
        try {
          if (sep != std::string::npos) {
            first = std::stol(args[2].substr(0, sep), nullptr);
            if (args[2].substr(sep+1).empty()) {
              last = _history.last_entry();
            } else {
              last = std::stol(args[2].substr(sep+1), nullptr);
              if (first > last) {
                println("Invalid history range " + args[2] +
                        ". Last item must be greater than first");
                return true;
              }
            }
          } else {
            first = std::stol(args[2], nullptr);
            last = first;
          }
        } catch (...) {
          println("Invalid history entry " + args[2]);
          return true;
        }
        if (_history.size() == 0 || first < _history.first_entry() ||
            first > _history.last_entry()) {
          println("Invalid history entry " + args[2]);
        } else {
          if (last > _history.last_entry())
            last = _history.last_entry();
          _history.del(first, last);
        }
      } catch (std::invalid_argument) {
        println("\\history delete requires entry number to be deleted");
      }
    }
  } else {
    println("Invalid options for \\history. See \\help history for syntax.");
  }
  return true;
}

void Command_line_shell::deleg_print(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    std::cout << text << std::flush;
    self->_output_printed = true;
  }
}

void Command_line_shell::deleg_print_error(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    std::cerr << text;
    self->_output_printed = true;
  }
}

std::string Command_line_shell::query_variable(
    const std::string &var,
    mysqlsh::Prompt_manager::Dynamic_variable_type type) {
  auto session = _shell->get_dev_session();

  if (session) {
    const char *q = "";
    switch (type) {
      case mysqlsh::Prompt_manager::Mysql_system_variable:
        q = "show global variables like ?";
        break;
      case mysqlsh::Prompt_manager::Mysql_session_variable:
        q = "show session variables like ?";
        break;
      case mysqlsh::Prompt_manager::Mysql_status:
        q = "show global status like ?";
        break;
      case mysqlsh::Prompt_manager::Mysql_session_status:
        q = "show session status like ?";
        break;
    }
    return session->query_one_string((shcore::sqlstring(q, 0) << var), 1);
  }
  return "";
}

std::string Command_line_shell::prompt() {
  // The continuation prompt should be used if state != Ok
  if (input_state() != shcore::Input_state::Ok) {
    _prompt.set_is_continuing(true);
  } else {
    _prompt.set_is_continuing(false);
  }
  if (_refresh_needed) {
    _refresh_needed = false;
  }
  if (_update_variables_pending > 0) {
    update_prompt_variables(_update_variables_pending > 1);
  }
  return _prompt.get_prompt(
      prompt_variables(),
      std::bind(&Command_line_shell::query_variable, this,
                std::placeholders::_1, std::placeholders::_2));
}

char *Command_line_shell::readline(const char *prompt) {
  std::string prompt_line(prompt);

  size_t pos = prompt_line.rfind("\n");
  if (pos != std::string::npos) {
    auto all_lines = prompt_line.substr(0, pos + 1);
    prompt_line = prompt_line.substr(pos + 1);

    if (!all_lines.empty())
      std::cout << all_lines << std::flush;
  }

  return linenoise(prompt_line.c_str());
}

void Command_line_shell::handle_interrupt() {
  _interrupted = true;
}

shcore::Prompt_result Command_line_shell::deleg_prompt(void *cdata,
                                                       const char *prompt,
                                                       std::string *ret) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_interrupted = false;
  char *tmp = Command_line_shell::readline(prompt);
  if (tmp && strcmp(tmp, CTRL_C_STR) == 0)
    self->_interrupted = true;
  if (!tmp || self->_interrupted) {
    if (tmp) free(tmp);
    *ret = "";
    if (self->_interrupted)
      return shcore::Prompt_result::Cancel;
    else
      return shcore::Prompt_result::CTRL_D;
  }
  *ret = tmp;
  free(tmp);
  return shcore::Prompt_result::Ok;
}

shcore::Prompt_result Command_line_shell::deleg_password(void *cdata,
                                                         const char *prompt,
                                                         std::string *ret) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_interrupted = false;
  shcore::Interrupt_handler inth(
    [self]() {
      self->handle_interrupt();
      return true;
    });
  char *tmp = self->_options.passwords_from_stdin
                  ? shcore::mysh_get_stdin_password(prompt)
                  : mysh_get_tty_password(prompt);
  if (tmp && strcmp(tmp, CTRL_C_STR) == 0)
    self->_interrupted = true;
  if (!tmp || self->_interrupted) {
    if (tmp) free(tmp);
    *ret = "";
    if (self->_interrupted)
      return shcore::Prompt_result::Cancel;
    else
      return shcore::Prompt_result::CTRL_D;
  }
  *ret = tmp;
  free(tmp);
  return shcore::Prompt_result::Ok;
}

void Command_line_shell::deleg_source(void *cdata, const char *module) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->process_file(module, {});
}

void Command_line_shell::command_loop() {
  bool using_tty = false;
#if defined(WIN32)
  using_tty = isatty(_fileno(stdin));
#else
  using_tty = isatty(STDIN_FILENO);
#endif

  if (_options.full_interactive && using_tty) {
    std::string message;
    auto session = _shell->get_dev_session();

    message = " Use \\sql to switch to SQL mode and execute queries.";

    switch (_shell->interactive_mode()) {
      case shcore::Shell_core::Mode::SQL:
#ifdef HAVE_V8
        message =
            "Currently in SQL mode. Use \\js or \\py to switch the shell to a "
            "scripting language.";
#else
        message =
            "Currently in SQL mode. Use \\py to switch the shell to python "
            "scripting.";
#endif
        break;
      case shcore::Shell_core::Mode::JavaScript:
        message = "Currently in JavaScript mode." + message;
        break;
      case shcore::Shell_core::Mode::Python:
        message = "Currently in Python mode." + message;
        break;
      default:
        break;
    }

    println(message);
  }

  while (_options.interactive) {
    std::string cmd;
    {
      shcore::Interrupt_handler handler([this]() {
        handle_interrupt();
        return true;
      });
      if (using_tty) {
        // Ensure prompt is in its own line, in case there was any output
        // without a \n
        if (_output_printed) {
          std::cout << "\n";
          _output_printed = false;
        }
        char *tmp = Command_line_shell::readline(prompt().c_str());
        if (tmp && strcmp(tmp, CTRL_C_STR) != 0) {
          cmd = tmp;
          free(tmp);
        } else {
          if (tmp) {
            if (strcmp(tmp, CTRL_C_STR) == 0)
              _interrupted = true;
            free(tmp);
          }
          if (_interrupted) {
            println("^C");
            clear_input();
            _interrupted = false;
            continue;
          }
          break;
        }
      } else {
        if (_options.full_interactive)
          std::cout << prompt() << std::flush;
        if (!std::getline(std::cin, cmd)) {
          if (_interrupted || !std::cin.eof()) {
            _interrupted = false;
            continue;
          }
          break;
        }
      }
      if (_options.full_interactive)
        std::cout << cmd << "\n";
    }
    process_line(cmd);
  }

  std::cout << "Bye!\n";
}

void Command_line_shell::clear_input() {
  _input_mode = shcore::Input_state::Ok;
  _input_buffer.clear();
  _shell->clear_input();
}

void Command_line_shell::print_banner() {
  std::string welcome_msg("MySQL Shell ");
  welcome_msg += MYSH_VERSION;
  welcome_msg += "\n\n";
  welcome_msg +=
      "Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights "
      "reserved.\n\n"
      "Oracle is a registered trademark of Oracle Corporation and/or its\n"
      "affiliates. Other names may be trademarks of their respective\n"
      "owners.";
  println(welcome_msg);
  println();
  println("Type '\\help' or '\\?' for help; '\\quit' to exit.");
  println();
}

void Command_line_shell::print_cmd_line_helper() {
  std::string help_msg("MySQL Shell ");
  help_msg += MYSH_VERSION;
  println(help_msg);
  println("");
  println("Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Usage: mysqlsh [OPTIONS] [URI]");
  println("Usage: mysqlsh [OPTIONS] [URI] -f <path> [script args...]");
  std::vector<std::string> details = Shell_command_line_options::get_details();
  for (std::string line : details)
    println(line);

  println("");
  println("Usage examples:");
  println("$ mysqlsh root@localhost/schema");
  println("$ mysqlsh mysqlx://root@some.server:3307/world_x");
  println("$ mysqlsh --uri root@localhost --py -f sample.py sample param");
  println("$ mysqlsh root@targethost:33070 -s world_x -f sample.js");
  println("");
}

bool Command_line_shell::cmd_process_file(
    const std::vector<std::string> &params) {
  pause_history(true);
  try {
    bool r = Mysql_shell::cmd_process_file(params);
    pause_history(false);
    return r;
  } catch (...) {
    pause_history(false);
    throw;
  }
}

void Command_line_shell::handle_notification(
    const std::string &name, const shcore::Object_bridge_ref &sender,
    shcore::Value::Map_type_ref data) {
  if (name == "SN_STATEMENT_EXECUTED") {
    std::string executed = data->get_string("statement");
    while (executed.back() == '\n' || executed.back() == '\r')
      executed.pop_back();
    if (_shell->interactive_mode() != shcore::Shell_core::Mode::SQL ||
        sql_safe_for_logging(executed)) {
      _history.add(executed);
    }
  } else if (name == "SN_SESSION_CONNECTED") {
    _refresh_needed = true;
  } else if (name == SN_SHELL_OPTION_CHANGED) {
    if (data->get_string("option") == SHCORE_HISTORY_MAX_SIZE) {
      _history.set_limit(data->get_int("value"));
    } else if (data->get_string("option") == SHCORE_HISTIGNORE) {
      set_sql_safe_for_logging(data->get_string("value"));
    }
  }
}
}  // namespace mysqlsh
