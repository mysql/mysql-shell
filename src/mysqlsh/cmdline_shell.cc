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

#include "mysqlsh/cmdline_shell.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <memory>
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
#include "modules/mod_shell_options.h"  // <---
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"

#ifdef WIN32
#undef max
#define fileno _fileno
#define snprintf _snprintf
#define write _write
#endif

extern char *mysh_get_tty_password(const char *opt_message);

#define CTRL_C_STR "\003"

namespace mysqlsh {

namespace {

Command_line_shell *g_instance = nullptr;

void auto_complete(const char *text, int *start_index,
                   linenoiseCompletions *completions) {
  size_t completion_offset = *start_index;
  std::vector<std::string> options(g_instance->completer()->complete(
      g_instance->shell_context()->interactive_mode(), text,
      &completion_offset));

  std::sort(options.begin(), options.end(),
            [](const std::string &a, const std::string &b) -> bool {
              return shcore::str_casecmp(a, b) < 0;
            });
  auto last = std::unique(options.begin(), options.end());
  options.erase(last, options.end());

  *start_index = completion_offset;
  for (auto &i : options) {
    linenoiseAddCompletion(completions, i.c_str());
  }
}

std::string get_pager_message(const std::string &pager) {
  if (pager.empty()) {
    return "Pager has been disabled.";
  } else {
    return "Pager has been set to '" + pager + "'.";
  }
}

static int utf8_bytes_length(unsigned char c) {
  static constexpr uint8_t lengths[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0};
  return lengths[c];
}

void write_to_console(int fd, const char *text) {
  const char *p = text;
  size_t bytes_left = strlen(text);
  while (bytes_left > 0) {
    int flush_bytes = BUFSIZ < bytes_left ? BUFSIZ : bytes_left;
    const char *flush_end = p + flush_bytes;

    // Windows Console requires all bytes of utf-8 encoded character printed at
    // once, therefore we don't cut utf-8 multi-byte character in the middle.
    // To be safe we use this behaviour on all platforms.
    while (utf8_bytes_length(static_cast<unsigned char>(*flush_end)) == 0 &&
           (p != flush_end)) {
      --flush_end;
    }

    if (p != flush_end) {
      flush_bytes = std::distance(p, flush_end);
    }

    const int written = write(fd, p, flush_bytes);
    if (written == -1) {
      const int error_no = errno;
      if ((error_no == EINTR) || (error_no == EWOULDBLOCK) ||
          (error_no == EAGAIN)) {
        continue;
      } else {
        break;
      }
    }

#ifdef _WIN32
    // On Windows platform, if stdout is redirected to console, _write returns
    // number of characters printed to console (which is not equal to bytes
    // written). We are not able to determine how many bytes were send to
    // console having information about number of printed chars, because:
    // clang-format off
    //   _write(1, "\xe2\x80\x99\n", 4); -> 2
    //   _write(1, "\xe2\x80\x99\n", 3); -> 1
    //   _write(1, "\xe2\x80\x99\n", 2); -> 1
    //   _write(1, "\xe2\x80\x99\n", 1); -> 1
    // clang-format on
    // Therefore we need to assume that number of flush_bytes was written to
    // output and advance *p and subtract bytes_left accordingly. If flush_bytes
    // is less than BUFSIZ we should be fine.
    //
    // If stdout is redirected to file _write returns number of bytes written.

    const int bytes_written = isatty(fd) ? flush_bytes : written;
#else
    const int bytes_written = written;
#endif
    bytes_left -= bytes_written;
    p += bytes_written;
  }
}
}  // namespace

REGISTER_HELP(CMD_HISTORY_BRIEF, "View and edit command line history.");
REGISTER_HELP(CMD_HISTORY_SYNTAX, "<b>\\history</b> [options].");
REGISTER_HELP(CMD_HISTORY_DETAIL,
              "The operation done by this command depends on the given "
              "options. Valid options are:");
REGISTER_HELP(
    CMD_HISTORY_DETAIL1,
    "@li <b>del</b> range         Deletes entry/entries from history.");
REGISTER_HELP(CMD_HISTORY_DETAIL2,
              "@li <b>clear</b>             Clear history.");
REGISTER_HELP(CMD_HISTORY_DETAIL3,
              "@li <b>save</b>              Save history to file.");
REGISTER_HELP(
    CMD_HISTORY_DETAIL4,
    "If no options are given the command will display the history entries.");
REGISTER_HELP(CMD_HISTORY_DETAIL5,
              "Range in the delete operation can be given in one of the "
              "following forms:");
REGISTER_HELP(CMD_HISTORY_DETAIL6,
              "@li <b>num</b> single number identifying entry to delete.");
REGISTER_HELP(CMD_HISTORY_DETAIL7,
              "@li <b>num-num</b> numbers specifying lower and upper bounds of "
              "the range.");
REGISTER_HELP(CMD_HISTORY_DETAIL8,
              "@li <b>num-</b> range from num till the end of history.");
REGISTER_HELP(CMD_HISTORY_DETAIL9, "@li <b>-num</b> last num entries.");
REGISTER_HELP(CMD_HISTORY_DETAIL10,
              "NOTE: The history.autoSave shell option must be set to true to "
              "automatically save the contents of the command history when "
              "MySQL Shell exits.");
REGISTER_HELP(CMD_HISTORY_EXAMPLE, "<b>\\history</b>");
REGISTER_HELP(CMD_HISTORY_EXAMPLE_DESC, "Displays the entire history.");
REGISTER_HELP(CMD_HISTORY_EXAMPLE1, "<b>\\history del</b> 123");
REGISTER_HELP(CMD_HISTORY_EXAMPLE1_DESC,
              "Deletes entry number 123 from the history.");
REGISTER_HELP(CMD_HISTORY_EXAMPLE2, "<b>\\history del</b> 10-20");
REGISTER_HELP(
    CMD_HISTORY_EXAMPLE2_DESC,
    "Deletes range of entries from number 10 to 20 from the history.");
REGISTER_HELP(CMD_HISTORY_EXAMPLE3, "<b>\\history del</b> 10-");
REGISTER_HELP(CMD_HISTORY_EXAMPLE3_DESC,
              "Deletes entries from number 10 and ahead from the history.");
REGISTER_HELP(CMD_HISTORY_EXAMPLE4, "<b>\\history del</b> -5");
REGISTER_HELP(CMD_HISTORY_EXAMPLE4_DESC,
              "Deletes last 5 entries from the history.");

REGISTER_HELP(CMD_PAGER_BRIEF, "Sets the current pager.");
REGISTER_HELP(CMD_PAGER_DETAIL,
              "The current pager will be automatically used to:");
REGISTER_HELP(CMD_PAGER_DETAIL1,
              "@li display results of statements executed in SQL mode,");
REGISTER_HELP(CMD_PAGER_DETAIL2, "@li display text output of \\help command,");
REGISTER_HELP(CMD_PAGER_DETAIL3,
              "@li display text output in scripting mode, after "
              "<b>shell.<<<enablePager>>>()</b> has been called,");
REGISTER_HELP(
    CMD_PAGER_DETAIL4,
    "Pager is going to be used only if shell is running in interactive mode.");
REGISTER_HELP(CMD_PAGER_SYNTAX, "<b>\\pager</b> [command]");
REGISTER_HELP(CMD_PAGER_SYNTAX1, "<b>\\P</b> [command]");
REGISTER_HELP(CMD_PAGER_EXAMPLE, "<b>\\pager</b>");
REGISTER_HELP(CMD_PAGER_EXAMPLE_DESC,
              "With no parameters this command restores the initial pager.");
REGISTER_HELP(CMD_PAGER_EXAMPLE1, "<b>\\pager</b> \"\"");
REGISTER_HELP(CMD_PAGER_EXAMPLE1_DESC, "Restores the initial pager.");
REGISTER_HELP(CMD_PAGER_EXAMPLE2, "<b>\\pager</b> more");
REGISTER_HELP(CMD_PAGER_EXAMPLE2_DESC, "Sets pager to \"more\".");
REGISTER_HELP(CMD_PAGER_EXAMPLE3, "<b>\\pager</b> \"more -10\"");
REGISTER_HELP(CMD_PAGER_EXAMPLE3_DESC, "Sets pager to \"more -10\".");
REGISTER_HELP(CMD_PAGER_EXAMPLE4, "<b>\\pager</b> more -10");
REGISTER_HELP(CMD_PAGER_EXAMPLE4_DESC, "Sets pager to \"more -10\".");

REGISTER_HELP(CMD_NOPAGER_BRIEF, "Disables the current pager.");
REGISTER_HELP(CMD_NOPAGER_SYNTAX, "<b>\\nopager</b>");

Command_line_shell::Command_line_shell(
    std::shared_ptr<Shell_options> cmdline_options,
    std::unique_ptr<shcore::Interpreter_delegate> delegate)
    : mysqlsh::Mysql_shell(cmdline_options, delegate.get()),
      _delegate(std::move(delegate)),
      m_default_pager(options().pager) {
  _output_printed = false;

  g_instance = this;

  observe_notification("SN_STATEMENT_EXECUTED");

  finish_init();

  _history.set_limit(std::min<int64_t>(options().history_max_size,
                                       std::numeric_limits<int>::max()));

  observe_notification(SN_SHELL_OPTION_CHANGED);

  linenoiseSetCompletionCallback(auto_complete);

  SET_SHELL_COMMAND("\\history", "CMD_HISTORY",
                    Command_line_shell::cmd_history);
  SET_SHELL_COMMAND("\\pager|\\P", "CMD_PAGER", Command_line_shell::cmd_pager);
  SET_SHELL_COMMAND("\\nopager", "CMD_NOPAGER",
                    Command_line_shell::cmd_nopager);

  if (!m_default_pager.empty()) {
    log_info("%s", get_pager_message(m_default_pager).c_str());
  }
}

Command_line_shell::Command_line_shell(std::shared_ptr<Shell_options> options)
    : Command_line_shell(options,
                         std::unique_ptr<shcore::Interpreter_delegate>(
                             new shcore::Interpreter_delegate{
                                 this, &Command_line_shell::deleg_print,
                                 &Command_line_shell::deleg_prompt,
                                 &Command_line_shell::deleg_password,
                                 &Command_line_shell::deleg_print_error})) {}

Command_line_shell::~Command_line_shell() {
  // global pager needs to be destroyed as it uses the delegate
  current_console()->disable_global_pager();
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
  if (options().history_autosave) {
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
      print_error("\\history clear does not take any parameters");
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
      print_error("\\history delete requires entry number to be deleted");
    } else {
      auto sep = args[2].find('-');
      if (sep != args[2].rfind('-')) {
        print_error(
            "\\history delete range argument needs to be in format first-last");
      } else {
        try {
          uint32_t first = 0;
          uint32_t last = 0;
          try {
            if (sep != std::string::npos) {
              const auto l = args[2].substr(sep + 1);
              const auto f = args[2].substr(0, sep);
              if (l.empty())
                last = _history.last_entry();
              else
                last = std::stoul(l, nullptr);

              if (f.empty()) {
                first = _history.size() > last
                            ? _history.last_entry() - last + 1
                            : _history.first_entry();
                last = _history.last_entry();
              } else {
                first = std::stoul(f, nullptr);
              }
              if (first > last && !l.empty()) {
                print_error("Invalid history range " + args[2] +
                            ". Last item must be greater than first");
                return true;
              }
            } else {
              first = std::stoul(args[2], nullptr);
              last = first;
            }
          } catch (...) {
            print_error("Invalid history entry " + args[2]);
            return true;
          }
          if (_history.size() == 0) {
            print_error("The history is already empty");
          } else if (first < _history.first_entry() ||
                     first > _history.last_entry()) {
            print_error(shcore::str_format(
                "Invalid history %s: %s - valid range is %u-%u",
                sep == std::string::npos ? "entry" : "range", args[2].c_str(),
                _history.first_entry(), _history.last_entry()));
          } else {
            if (last > _history.last_entry()) last = _history.last_entry();
            _history.del(first, last);
          }
        } catch (std::invalid_argument &) {
          print_error(
              "\\history delete requires entry number or range to be deleted");
        }
      }
    }
  } else {
    print_error(
        "Invalid options for \\history. See \\help history for syntax.");
  }
  return true;
}

bool Command_line_shell::cmd_pager(const std::vector<std::string> &args) {
  std::string new_pager;

  switch (args.size()) {
    case 1:
      // no arguments -> restore default pager
      new_pager = m_default_pager;
      break;

    case 2:
      if (args[1].empty()) {
        // one empty argument -> restore default pager
        new_pager = m_default_pager;
      } else {
        // one argument -> new pager
        new_pager = args[1];
      }
      break;

    default:
      new_pager = shcore::str_join(std::next(args.begin()), args.end(), " ");
      break;
  }

  get_options()->set_and_notify(SHCORE_PAGER, new_pager);

  return true;
}

bool Command_line_shell::cmd_nopager(const std::vector<std::string> &args) {
  if (args.size() > 1) {
    print_error(_shell->get_helper()->get_help("Shell Commands \\nopager"));
  } else {
    get_options()->set_and_notify(SHCORE_PAGER, "");
  }

  return true;
}

void Command_line_shell::deleg_print(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    write_to_console(fileno(stdout), text);
    self->_output_printed = true;
  }
}

void Command_line_shell::deleg_print_error(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    write_to_console(fileno(stderr), text);
    self->_output_printed = true;
  }
}

std::string Command_line_shell::query_variable(
    const std::string &var,
    mysqlsh::Prompt_manager::Dynamic_variable_type type) {
  if (type == mysqlsh::Prompt_manager::Shell_status) {
    if (var == "linectx") return _shell->get_continued_input_context();
    return "";
  }
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
      case mysqlsh::Prompt_manager::Shell_status:
        assert(0);
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

    if (!all_lines.empty()) std::cout << all_lines << std::flush;
  }

  // linenoise doesn't play nice with pagers, pager needs to be disabled
  // when prompt is displayed
  const auto console = current_console();
  const auto is_pager_enabled = console->is_global_pager_enabled();

  if (is_pager_enabled) {
    console->disable_global_pager();
  }

  char *tmp = linenoise(prompt_line.c_str());

  if (is_pager_enabled) {
    console->enable_global_pager();
  }

  if (!tmp) {
    switch (linenoiseKeyType()) {
      case 1:  // ^C
        return strdup(CTRL_C_STR);
      case 2:  // ^D
        return nullptr;
    }
  }
  return tmp;
}

void Command_line_shell::handle_interrupt() { _interrupted = true; }

shcore::Prompt_result Command_line_shell::deleg_prompt(void *cdata,
                                                       const char *prompt,
                                                       std::string *ret) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_interrupted = false;
  char *tmp = Command_line_shell::readline(prompt);
  if (tmp && strcmp(tmp, CTRL_C_STR) == 0) self->_interrupted = true;
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
  shcore::Interrupt_handler inth([self]() {
    self->handle_interrupt();
    return true;
  });
  char *tmp = self->options().passwords_from_stdin
                  ? shcore::mysh_get_stdin_password(prompt)
                  : mysh_get_tty_password(prompt);
  if (tmp && strcmp(tmp, CTRL_C_STR) == 0) self->_interrupted = true;
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

void Command_line_shell::command_loop() {
  bool using_tty = false;
#if defined(WIN32)
  using_tty = isatty(_fileno(stdin));
#else
  using_tty = isatty(STDIN_FILENO);
#endif

  if (_deferred_output && !_deferred_output->empty()) {
    println(*_deferred_output);
    _deferred_output.reset();
  }
  if (options().full_interactive && using_tty) {
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

  m_current_session_uri = get_current_session_uri();

  while (options().interactive) {
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
          mysqlsh::current_console()->raw_print(
              "\n", mysqlsh::Output_stream::STDOUT, false);
          _output_printed = false;
        }
        char *tmp = Command_line_shell::readline(prompt().c_str());
        if (tmp && strcmp(tmp, CTRL_C_STR) != 0) {
          cmd = tmp;
          free(tmp);
        } else {
          if (tmp) {
            if (strcmp(tmp, CTRL_C_STR) == 0) _interrupted = true;
            free(tmp);
          }
          if (_interrupted) {
            clear_input();
            _interrupted = false;
            continue;
          }
          break;
        }
      } else {
        if (options().full_interactive)
          mysqlsh::current_console()->raw_print(
              prompt(), mysqlsh::Output_stream::STDOUT, false);
        if (!std::getline(std::cin, cmd)) {
          if (_interrupted || !std::cin.eof()) {
            _interrupted = false;
            continue;
          }
          break;
        }
      }
      if (options().full_interactive)
        mysqlsh::current_console()->raw_print(
            cmd + "\n", mysqlsh::Output_stream::STDOUT, false);
    }
    process_line(cmd);
    reconnect_if_needed();
    detect_session_change();
  }

  std::cout << "Bye!\n";
}

void Command_line_shell::print_banner() {
  std::string welcome_msg("MySQL Shell ");
  welcome_msg += MYSH_FULL_VERSION;
  welcome_msg += "\n\n";
  welcome_msg +=
      "Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights "
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
  // clang-format off
  std::string help_msg("MySQL Shell ");
  help_msg += MYSH_FULL_VERSION;
  println(help_msg);
  println("");
  println("Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Usage: mysqlsh [OPTIONS] [URI]");
  println("       mysqlsh [OPTIONS] [URI] -f <path> [script args...]");
  println("       mysqlsh [OPTIONS] [URI] --dba [command]");
  println("       mysqlsh [OPTIONS] [URI] --cluster");
  println("       mysqlsh [OPTIONS] [URI] -- <object> <method> [method args...]");
  println("       mysqlsh [OPTIONS] [URI] --import file|- [collection] | [table [, column]");
  println("");
  // clang-format on
  std::vector<std::string> details = Shell_options(0, nullptr).get_details();
  for (std::string line : details) println("  " + line);

  println("");
  println("Usage examples:");
  println("$ mysqlsh root@localhost/schema");
  println("$ mysqlsh mysqlx://root@some.server:3307/world_x");
  println("$ mysqlsh --uri root@localhost --py -f sample.py sample param");
  println("$ mysqlsh root@targethost:33070 -s world_x -f sample.js");
  println(
      "$ mysqlsh -- util check-for-server-upgrade root@localhost "
      "--output-format=JSON");
  println("$ mysqlsh mysqlx://user@host/db --import ~/products.json shop");
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
  } else if (name == SN_SHELL_OPTION_CHANGED) {
    const auto option = data->get_string("option");

    if (SHCORE_HISTORY_MAX_SIZE == option) {
      _history.set_limit(data->get_int("value"));
    } else if (SHCORE_HISTIGNORE == option) {
      set_sql_safe_for_logging(data->get_string("value"));
    } else if (SHCORE_PAGER == option) {
      const auto console = current_console();

      console->print_info(get_pager_message(data->get_string("value")));

      if (console->is_global_pager_enabled()) {
        // if global pager is enabled, disable and re-enable it to use new pager
        console->disable_global_pager();
        console->enable_global_pager();
      }
    }
  }
}

std::string Command_line_shell::get_current_session_uri() const {
  std::string session_uri;

  const auto session = _shell->get_dev_session();

  if (session) {
    const auto core_session = session->get_core_session();
    if (core_session && core_session->is_open()) {
      session_uri = core_session->uri();
    }
  }

  return session_uri;
}

void Command_line_shell::detect_session_change() {
  const auto session_uri = get_current_session_uri();

  if (session_uri != m_current_session_uri) {
    m_current_session_uri = session_uri;
    request_prompt_variables_update(true);
  }
}

}  // namespace mysqlsh
