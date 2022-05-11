/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "ext/linenoise-ng/include/linenoise.h"
#include "modules/devapi/base_resultset.h"
#include "modules/mod_shell_options.h"  // <---
#include "mysqlshdk/libs/utils/log_sql.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/structured_text.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"
#include "src/mysqlsh/commands/command_edit.h"
#include "src/mysqlsh/commands/command_system.h"
#include "src/mysqlsh/prompt_handler.h"

#ifdef WIN32
#undef max
#define fileno _fileno
#define snprintf _snprintf
#define write _write

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

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

/**
 * Writes the specified text to the given file descriptor.
 *
 * @param fd - file desriptor to write to
 * @param text - text to be written
 *
 * @returns true if the specified text ends with a newline character.
 */
bool write_to_console(int fd, const char *text) {
  const char *p = text;
  size_t bytes_left = strlen(text);
  bool has_newline = (bytes_left > 0) && (text[bytes_left - 1] == '\n');

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

  return has_newline;
}

void print_diag(const std::string &s) { current_console()->print_diag(s); }

void println(const std::string &s = "") { current_console()->println(s); }

template <typename T>
void display_info(const std::shared_ptr<T> &object, const std::string &variable,
                  const std::string &context, bool print_name = true) {
  static constexpr auto var_begin = "${";
  static constexpr auto var_end = "}";
  const auto replace = [&object, &variable, &context](const std::string &var) {
    if (var == "name") {
      return object->impl()->get_name();
    } else if (var == "class") {
      return object->class_name();
    } else if (var == "var") {
      return variable;
    } else if (var == "context") {
      return context;
    }

    return std::string{"ERROR"};
  };

  if (object) {
    const auto console = current_console();

    if (print_name) {
      console->print_info(shcore::str_subvars(
          "You are connected to a member of ${context} '${name}'.", replace,
          var_begin, var_end));
    }

    console->print_info(shcore::str_subvars(
        "Variable '${var}' is set.\nUse ${var}.status() in scripting mode to "
        "get status of this ${context} or \\? ${class} for more commands.",
        replace, var_begin, var_end));
  }
}

/**
 * Checks if standard input is a TTY.
 *
 * @return true if standard input is a TTY
 */
bool is_stdin_a_tty() {
  const auto fd =
#ifdef _WIN32
      _fileno(stdin)
#else   // !_WIN32
      STDIN_FILENO
#endif  // !_WIN32
      ;
  return isatty(fd);
}

/**
 * Sends CTRL-C sequence to the controlling terminal/console.
 *
 * @return true if sequence was sent
 */
bool send_ctrl_c_to_terminal() {
  bool success = false;
#ifdef _WIN32
  const auto handle = CreateFile("CONIN$",         // lpFileName
                                 GENERIC_WRITE,    // dwDesiredAccess
                                 FILE_SHARE_READ,  // dwShareMode
                                 nullptr,          // lpSecurityAttributes
                                 OPEN_EXISTING,    // dwCreationDisposition
                                 0,                // dwFlagsAndAttributes
                                 nullptr           // hTemplateFile
  );

  if (INVALID_HANDLE_VALUE != handle) {
    constexpr DWORD count = 1;
    INPUT_RECORD ir[count];

    ir[0].EventType = KEY_EVENT;
    ir[0].Event.KeyEvent.bKeyDown = TRUE;
    ir[0].Event.KeyEvent.dwControlKeyState = 0;
    ir[0].Event.KeyEvent.uChar.UnicodeChar =
        MapVirtualKey(VK_CANCEL, MAPVK_VK_TO_CHAR);
    ir[0].Event.KeyEvent.wRepeatCount = 1;
    ir[0].Event.KeyEvent.wVirtualKeyCode = VK_CANCEL;
    ir[0].Event.KeyEvent.wVirtualScanCode =
        MapVirtualKey(VK_CANCEL, MAPVK_VK_TO_VSC);

    DWORD written = 0;
    success = WriteConsoleInput(handle, ir, count, &written);

    CloseHandle(handle);
  }
#else   // !_WIN32
  char controlling_terminal[L_ctermid];
  int fd = -1;

  // get controlling terminal name, open it only for writing, do not set that
  // terminal as a controlling if process does not have one
  if (ctermid(controlling_terminal) &&
      (fd = ::open(controlling_terminal, O_WRONLY | O_NOCTTY)) >= 0) {
    // simulate user input
    if (-1 != ioctl(fd, TIOCSTI, CTRL_C_STR)) {
      success = true;
    }

    ::close(fd);
  }
#endif  // !_WIN32

  return success;
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

REGISTER_HELP(CMD_SYSTEM_BRIEF, "Execute a system shell command.");
REGISTER_HELP(CMD_SYSTEM_SYNTAX, "<b>\\system</b> [command [arguments...]]");
REGISTER_HELP(CMD_SYSTEM_SYNTAX1, "<b>\\!</b> [command [arguments...]]");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE, "<b>\\system</b>");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE_DESC,
              "With no arguments, this command displays this help.");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE1, "<b>\\system</b> ls");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE1_DESC,
              "Executes 'ls' in the current working directory and displays the "
              "result.");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE2, "<b>\\!</b> ls > list.txt");
REGISTER_HELP(CMD_SYSTEM_EXAMPLE2_DESC,
              "Executes 'ls' in the current working directory, storing the "
              "result in the 'list.txt' file.");

REGISTER_HELP(CMD_EDIT_BRIEF,
              "Launch a system editor to edit a command to be executed.");
REGISTER_HELP(CMD_EDIT_DETAIL,
              "The system editor is selected using the <b>EDITOR</b> and "
              "<b>VISUAL</b> environment variables.");
#ifdef _WIN32
REGISTER_HELP(CMD_EDIT_DETAIL1,
              "If these are not set, falls back to notepad.exe.");
#else   // !_WIN32
REGISTER_HELP(CMD_EDIT_DETAIL1, "If these are not set, falls back to vi.");
#endif  // !_WIN32
REGISTER_HELP(
    CMD_EDIT_DETAIL2,
    "It is also possible to invoke this command by pressing CTRL-X CTRL-E.");
REGISTER_HELP(CMD_EDIT_SYNTAX, "<b>\\edit</b> [arguments...]");
REGISTER_HELP(CMD_EDIT_SYNTAX1, "<b>\\e</b> [arguments...]");
REGISTER_HELP(CMD_EDIT_EXAMPLE, "<b>\\edit</b>");
REGISTER_HELP(CMD_EDIT_EXAMPLE_DESC,
              "Allows to edit the last command in history.");
REGISTER_HELP(CMD_EDIT_EXAMPLE_DESC1,
              "If there are no commands in history, editor will be blank.");
REGISTER_HELP(CMD_EDIT_EXAMPLE1, "<b>\\e</b> print('hello world!')");
REGISTER_HELP(CMD_EDIT_EXAMPLE1_DESC,
              "Allows to edit the commands given as arguments.");

Command_line_shell::Command_line_shell(
    std::shared_ptr<Shell_options> cmdline_options,
    std::unique_ptr<shcore::Interpreter_delegate> delegate,
    bool suppress_output)
    : mysqlsh::Mysql_shell(cmdline_options, delegate.get()),
      _delegate(std::move(delegate)),
      m_suppressed_handler(this, &Command_line_shell::deleg_delayed_print,
                           &Command_line_shell::deleg_delayed_print_error,
                           nullptr),
      m_suppress_output(suppress_output),
      m_default_pager(options().pager) {
  // The default command line shell initializes printing in delayed mode
  // so everything printed before/during the initialization process is cached
  if (m_suppress_output) {
    m_enable_toggle_print = true;
    current_console()->add_print_handler(&m_suppressed_handler);
  }

  g_instance = this;

  observe_notification("SN_STATEMENT_EXECUTED");

  finish_init();

  _history.set_limit(std::min<int64_t>(options().history_max_size,
                                       std::numeric_limits<int>::max()));

  m_syslog.enable(options().history_sql_syslog);

  observe_notification(SN_SHELL_OPTION_CHANGED);

  linenoiseSetCompletionCallback(auto_complete);

  SET_SHELL_COMMAND("\\history", "CMD_HISTORY",
                    Command_line_shell::cmd_history);
  SET_SHELL_COMMAND("\\pager|\\P", "CMD_PAGER", Command_line_shell::cmd_pager);
  SET_SHELL_COMMAND("\\nopager", "CMD_NOPAGER",
                    Command_line_shell::cmd_nopager);
  SET_CUSTOM_SHELL_COMMAND("\\system|\\!", "CMD_SYSTEM",
                           [this](const std::vector<std::string> &args) {
                             return Command_system(_shell)(args);
                           });
  SET_CUSTOM_SHELL_COMMAND("\\edit|\\e", "CMD_EDIT",
                           [this](const std::vector<std::string> &args) {
                             return Command_edit(_shell, this, &_history)(args);
                           });

  linenoiseRegisterCustomCommand(
      "\x18\x05",  // CTRL-X, CTRL-E
      [](const char *, void *data, char **) {
        const auto that = static_cast<Command_line_shell *>(data);

        if (that->set_pending_command([that](const std::string &line) {
              Command_edit(that->_shell, that, &that->_history)
                  .execute(that->_input_buffer + line);
            })) {
          // command was registered, simulate user pressing enter
          return 0x0A;
        } else {
          // it was not possible to register the edit command, continue
          return -1;
        }
      },
      this);

  if (!m_default_pager.empty()) {
    log_info("%s", get_pager_message(m_default_pager).c_str());
  }
}

/**
 * Backups the print function on the _delegate to disable
 * information printing.
 */
void Command_line_shell::quiet_print() {
  if (m_suppress_output) {
    current_console()->remove_print_handler(&m_suppressed_handler);
  }

  m_suppress_output = true;
  m_suppressed_handler = shcore::Interpreter_print_handler(
      this, &Command_line_shell::deleg_disable_print,
      &Command_line_shell::deleg_disable_print_error, nullptr);

  current_console()->add_print_handler(&m_suppressed_handler);
}

/**
 * Used to temporarily enable printing when a global session is being created
 * from command line arguments.
 *
 * This is required because when using the FIDO
 * authentication plugin, the instructions to use the FIDO device will be
 * printed by the authentication plugin, but at the connection time the printing
 * is disabled (or delayed), using this function it can be enabled properly so
 * the instruction is printed with the right timing.
 */
void Command_line_shell::toggle_print() {
  if (m_enable_toggle_print) {
    if (m_suppress_output) {
      current_console()->remove_print_handler(&m_suppressed_handler);
      m_suppress_output = false;
    } else {
      current_console()->add_print_handler(&m_suppressed_handler);
      m_suppress_output = true;
    }
  }
}

/**
 * Enables back information printing.
 * If info was not being suppressed with --quiet-start=2
 * Then the cached information will be printed.
 */
void Command_line_shell::restore_print() {
  if (m_suppress_output) {
    current_console()->remove_print_handler(&m_suppressed_handler);
    m_suppress_output = false;

    // If printing information is not disabled. prints cached information
    if (options().quiet_start != Shell_options::Quiet_start::SUPRESS_INFO) {
      for (const auto &s : _full_output) {
        if (s.first == STDOUT_FILENO)
          _delegate->print(s.second.c_str());
        else
          _delegate->print_error(s.second.c_str());
      }
      _full_output.clear();

      for (const auto &s : _delayed_output) {
        if (s.first == STDOUT_FILENO)
          _delegate->print(s.second.c_str());
        else
          _delegate->print_error(s.second.c_str());
      }
      _delayed_output.clear();
    }
  }

  // Once the printing is restored, the toggle function should do nothing
  m_enable_toggle_print = false;
}

Command_line_shell::Command_line_shell(std::shared_ptr<Shell_options> options)
    : Command_line_shell(options,
                         std::unique_ptr<shcore::Interpreter_delegate>(
                             new shcore::Interpreter_delegate{
                                 this, &Command_line_shell::deleg_print,
                                 &Command_line_shell::deleg_prompt,
                                 &Command_line_shell::deleg_print_error,
                                 &Command_line_shell::deleg_print_diag}),
                         true) {}

Command_line_shell::Command_line_shell(
    std::shared_ptr<Shell_options> options,
    std::unique_ptr<shcore::Interpreter_delegate> delegate)
    : Command_line_shell(options, std::move(delegate), false) {}

Command_line_shell::~Command_line_shell() {
  // global pager needs to be destroyed as it uses the delegate
  current_console()->disable_global_pager();
  // disable verbose output, since it also uses the delegate
  current_console()->set_verbose(0);

  linenoiseRemoveCustomCommand("\x18\x05");
}

void Command_line_shell::load_prompt_theme(const std::string &path) {
  if (!path.empty()) {
#ifdef _WIN32
    std::ifstream f(shcore::utf8_to_wide(path));
#else
    std::ifstream f(path);
#endif

    if (f.good()) {
      std::stringstream buffer;
      buffer << f.rdbuf();

      try {
        const auto data = buffer.str();

        if (!shcore::is_valid_utf8(data)) {
          throw std::invalid_argument("File contains invalid UTF-8 sequence.");
        }

        _prompt.set_theme(shcore::Value::parse(data));
      } catch (const std::exception &e) {
        log_warning("Error loading prompt theme '%s': %s", path.c_str(),
                    e.what());
        print_diag(shcore::str_format("Error loading prompt theme '%s': %s\n",
                                      path.c_str(), e.what()));
      }
    }
  }
}

void Command_line_shell::load_state(shcore::Shell_core::Mode mode) {
  std::string path = history_file(mode);

  // Copy old history from before the split
  if (!shcore::is_file(path)) {
    std::string old_hist_file = path.substr(0, path.rfind('.'));
    if (shcore::is_file(old_hist_file)) {
      shcore::copy_file(old_hist_file,
                        history_file(shcore::IShell_core::Mode::SQL));
#ifdef HAVE_V8
      shcore::copy_file(old_hist_file,
                        history_file(shcore::IShell_core::Mode::JavaScript));
#endif
#ifdef HAVE_PYTHON
      shcore::copy_file(old_hist_file,
                        history_file(shcore::IShell_core::Mode::Python));
#endif
      shcore::delete_file(old_hist_file);
    }
  }

  if (!_history.load(path)) {
    print_diag(
        shcore::str_format("Could not load command history from %s: %s\n",
                           path.c_str(), strerror(errno)));
  }
}

void Command_line_shell::save_state(shcore::Shell_core::Mode mode) {
  if (options().history_autosave) {
    std::string path = history_file(mode);
    if (!_history.save(path)) {
      print_diag(
          shcore::str_format("Could not save command history to %s: %s\n",
                             path.c_str(), strerror(errno)));
    }
  }
}

bool Command_line_shell::switch_shell_mode(shcore::Shell_core::Mode mode,
                                           const std::vector<std::string> &args,
                                           bool initializing,
                                           bool /* prompt_variables_update */) {
  shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
  bool ret = Mysql_shell::switch_shell_mode(mode, args, initializing);

  // if mode changed back to original one before statement got fully executed
  // avoid history switch
  if (m_previous_mode == interactive_mode())
    m_previous_mode = shcore::Shell_core::Mode::None;
  // if mode changed switch history after statement is executed
  else if (old_mode != interactive_mode())
    m_previous_mode = old_mode;

  return ret;
}

bool Command_line_shell::cmd_history(const std::vector<std::string> &args) {
  if (args.size() == 1) {
    _history.dump([](const std::string &s) { println(s); });
  } else if (args[1] == "clear") {
    if (args.size() == 2) {
      _history.clear();
    } else {
      print_diag("\\history clear does not take any parameters");
    }
  } else if (args[1] == "save") {
    std::string path = history_file();
    if (linenoiseHistorySave(path.c_str()) < 0) {
      print_diag(shcore::str_format("Could not save command history to %s: %s",
                                    path.c_str(), strerror(errno)));
    } else {
      println(shcore::str_format("Command history file saved with %i entries.",
                                 linenoiseHistorySize()));
    }
  } else if (args[1] == "delete" || args[1] == "del") {
    if (args.size() != 3) {
      print_diag("\\history delete requires entry number to be deleted");
    } else {
      auto sep = args[2].find('-');
      if (sep != args[2].rfind('-')) {
        print_diag(
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
                print_diag("Invalid history range " + args[2] +
                           ". Last item must be greater than first");
                return true;
              }
            } else {
              first = std::stoul(args[2], nullptr);
              last = first;
            }
          } catch (...) {
            print_diag("Invalid history entry " + args[2]);
            return true;
          }
          if (_history.size() == 0) {
            print_diag("The history is already empty");
          } else if (first < _history.first_entry() ||
                     first > _history.last_entry()) {
            print_diag(shcore::str_format(
                "Invalid history %s: %s - valid range is %u-%u",
                sep == std::string::npos ? "entry" : "range", args[2].c_str(),
                _history.first_entry(), _history.last_entry()));
          } else {
            if (last > _history.last_entry()) last = _history.last_entry();
            _history.del(first, last);
          }
        } catch (const std::invalid_argument &) {
          print_diag(
              "\\history delete requires entry number or range to be deleted");
        }
      }
    }
  } else {
    print_diag("Invalid options for \\history. See \\help history for syntax.");
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
    print_diag(_shell->get_helper()->get_help("Shell Commands \\nopager"));
  } else {
    get_options()->set_and_notify(SHCORE_PAGER, "");
  }

  return true;
}

bool Command_line_shell::deleg_disable_print(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_full_output.emplace_back(STDOUT_FILENO, text);
  return true;
}

bool Command_line_shell::deleg_disable_print_error(void *cdata,
                                                   const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_full_output.emplace_back(STDERR_FILENO, text);
  return true;
}

bool Command_line_shell::deleg_delayed_print(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_delayed_output.emplace_back(STDOUT_FILENO, text);
  return true;
}

bool Command_line_shell::deleg_delayed_print_error(void *cdata,
                                                   const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  self->_delayed_output.emplace_back(STDERR_FILENO, text);
  return true;
}

bool Command_line_shell::deleg_print(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    self->m_prompt_requires_newline = !write_to_console(fileno(stdout), text);
  }
  return true;
}

bool Command_line_shell::deleg_print_error(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    self->m_prompt_requires_newline = !write_to_console(fileno(stderr), text);
  }
  return true;
}

bool Command_line_shell::deleg_print_diag(void *cdata, const char *text) {
  Command_line_shell *self = reinterpret_cast<Command_line_shell *>(cdata);
  if (text && *text) {
    self->m_prompt_requires_newline = !write_to_console(fileno(stderr), text);
  }
  return true;
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

shcore::Prompt_result Command_line_shell::deleg_prompt(
    void *cdata, const char *prompt,
    const shcore::prompt::Prompt_options &options, std::string *ret) {
  const auto self = reinterpret_cast<Command_line_shell *>(cdata);

  return Prompt_handler(
             [self](bool is_password, const char *text, std::string *reply) {
               return self->do_prompt(is_password, text, reply);
             })
      .handle_prompt(prompt, options, ret);
}

shcore::Prompt_result Command_line_shell::do_prompt(bool is_password,
                                                    const char *text,
                                                    std::string *ret) {
  char *tmp = nullptr;

  const auto passwords_from_stdin = this->options().passwords_from_stdin;

  char *(*get_response)(const char *) =
      is_password ? (passwords_from_stdin ? shcore::mysh_get_stdin_password
                                          : mysh_get_tty_password)
                  : Command_line_shell::readline;

  shcore::on_leave_scope cleanup([&tmp, is_password]() {
    if (!tmp) return;

    if (is_password) {
      // BUG#28915716: Cleans up the memory containing the password
      shcore::clear_buffer(tmp, ::strlen(tmp));
    }

    free(std::exchange(tmp, nullptr));
  });

  shcore::Interrupt_handler second_handler([get_response]() {
#ifdef _WIN32
    // this handler is not called if shell is running interactively
    // this handler is called when shell is running interactively with
    // --passwords-from-stdin option and a password prompt is canceled
    // this handler is called when shell is running in non-interactive mode
    // (i.e. launched by another process)

    // cancel any pending I/O, if signal was sent using
    // GenerateConsoleCtrlEvent() these are not going to be interrupted on its
    // own
    CancelIoEx(GetStdHandle(STD_INPUT_HANDLE), nullptr);

    // mysh_get_tty_password() reads directly from the console, CancelIoEx()
    // will not interrupt this, need to send CTRL-C
    if (mysh_get_tty_password == get_response) {
#else   // !_WIN32
    // if signal is sent to us from another process and we're reading from
    // terminal, we need to generate CTRL-C sequence to interrupt read loops
    if (mysh_get_tty_password == get_response || is_stdin_a_tty()) {
#endif  // !_WIN32
      send_ctrl_c_to_terminal();
    }

    // always suppress further processing of the interrupt handlers, if a call
    // to prompt*() throws in response to Prompt_result::Cancel, we want to
    // make sure that this exception is properly propagated
    return false;
  });

  shcore::Interrupt_handler first_handler([this]() {
    // we need another Interrupt_handler, which is going to be executed first
    // and returns true, to make sure that any interrupts generated by the
    // handler registered first (and executed second) are suppressed
    handle_interrupt();
    return true;
  });

  _interrupted = false;

  tmp = get_response(text);

#ifdef _WIN32
  if (!tmp && !_interrupted) {
    // If prompt returned no result but it was not interrupted, it means that
    // either user pressed CTRL-D/CTRL-Z or there was a signal which
    // interrupted read(), but was not yet delivered (signals are asynchronous
    // on Windows) - this may happen i.e. if shell was started with
    // --passwords-from-stdin and password prompt is interrupted with CTRL-C.
    // Wait a bit here, if signal arrives, sleep is going to be interrupted.
    // Worst case scenario: CTRL-D/CTRL-Z was pressed or signal arrived after
    // flag was checked but before we went to sleep.
    shcore::sleep_ms(100);
  }
#endif  // _WIN32

  if (tmp && strcmp(tmp, CTRL_C_STR) == 0) _interrupted = true;

  if (!tmp || _interrupted) {
    *ret = "";

    if (_interrupted)
      return shcore::Prompt_result::Cancel;
    else
      return shcore::Prompt_result::CTRL_D;
  }

  *ret = tmp;

  return shcore::Prompt_result::Ok;
}

std::string Command_line_shell::history_file(shcore::Shell_core::Mode mode) {
  return shcore::path::join_path(
      shcore::get_user_config_path(),
      "history." + to_string(mode == shcore::Shell_core::Mode::None
                                 ? interactive_mode()
                                 : mode));
}

void Command_line_shell::pre_command_loop() {
  display_info(m_default_cluster, "cluster", "cluster");
  display_info(m_default_clusterset, "clusterset", "clusterset");

  // information about replicaset name is displayed by the dba
  display_info(m_default_replicaset, "rs", "replicaset", false);
}

void Command_line_shell::command_loop() {
  bool using_tty = is_stdin_a_tty();

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
            "Currently in SQL mode. Use \\js or \\py to switch the shell to "
            "a scripting language.";
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
        if (m_prompt_requires_newline) {
          mysqlsh::current_console()->raw_print(
              "\n", mysqlsh::Output_stream::STDOUT, false);
        }

        m_set_pending_command = true;

        char *tmp = Command_line_shell::readline(prompt().c_str());
        const std::unique_ptr<char, decltype(&free)> tmp_deleter{tmp, free};

        m_set_pending_command = false;

        if (tmp && strcmp(tmp, CTRL_C_STR) != 0) {
          cmd = tmp;
        } else {
          if (tmp) {
            if (strcmp(tmp, CTRL_C_STR) == 0) _interrupted = true;
          }
          if (_interrupted) {
            clear_input();
            _interrupted = false;
            continue;
          }
          break;
        }
      } else {
        if (options().full_interactive) {
          if (m_prompt_requires_newline) {
            mysqlsh::current_console()->raw_print(
                "\n", mysqlsh::Output_stream::STDOUT, false);
          }

          mysqlsh::current_console()->raw_print(
              prompt(), mysqlsh::Output_stream::STDOUT, false);
        }
        if (!shcore::getline(std::cin, cmd)) {
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
  welcome_msg += "Copyright (c) 2016, " PACKAGE_YEAR
                 ", Oracle and/or its affiliates.\n"
                 "Oracle is a registered trademark of Oracle Corporation "
                 "and/or its affiliates.\n"
                 "Other names may be trademarks of their respective owners.";
  println(welcome_msg);
  println();
  println("Type '\\help' or '\\?' for help; '\\quit' to exit.");
}

void Command_line_shell::print_cmd_line_helper() {
  // clang-format off
  std::string help_msg("MySQL Shell ");
  help_msg += MYSH_FULL_VERSION;
  println(help_msg);
  println("");
  // Splitting line in two so the git hook does not complain
  println("Copyright (c) 2016, " PACKAGE_YEAR ", "
          "Oracle and/or its affiliates.");
  println("Oracle is a registered trademark of Oracle Corporation and/or its affiliates.");
  println("Other names may be trademarks of their respective owners.");
  println("");
  println("Usage: mysqlsh [OPTIONS] [URI]");
  println("       mysqlsh [OPTIONS] [URI] -f <path> [<script-args>...]");
  println("       mysqlsh [OPTIONS] [URI] --dba enableXProtocol");
  println("       mysqlsh [OPTIONS] [URI] --cluster");
  println("       mysqlsh [OPTIONS] [URI] -- <object> <method> [<method-args>...]");
  println("       mysqlsh [OPTIONS] [URI] --import {<file>|-} [<collection>|<table> <column>]");
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
  shcore::on_leave_scope resume_history{[this]() { pause_history(false); }};

  return Mysql_shell::cmd_process_file(params);
}

void Command_line_shell::handle_notification(
    const std::string &name, const shcore::Object_bridge_ref & /* sender */,
    shcore::Value::Map_type_ref data) {
  if (name == "SN_STATEMENT_EXECUTED") {
    const auto executed = shcore::str_strip(data->get_string("statement"));
    auto mode = interactive_mode();
    auto sql = executed;
    if (shcore::str_beginswith(executed, "\\sql ") && executed.length() > 5) {
      mode = shcore::Shell_core::Mode::SQL;
      sql = executed.substr(5);
    }
    if (mode != shcore::Shell_core::Mode::SQL || sql_safe_for_logging(sql)) {
      _history.add(executed);

      if (shcore::Shell_core::Mode::SQL == mode) {
        syslog(sql);
      }
    } else {
      // add but delete after the next command and
      // don't let it get saved to disk either
      _history.add_temporary(executed);
    }
    if (m_previous_mode != shcore::Shell_core::Mode::None) {
      save_state(m_previous_mode);
      load_state();
      m_previous_mode = shcore::Shell_core::Mode::None;
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
        // if global pager is enabled, disable and re-enable it to use new
        // pager
        console->disable_global_pager();
        console->enable_global_pager();
      }
    } else if (SHCORE_VERBOSE == option) {
      const auto console = current_console();

      console->set_verbose(data->get_int("value"));
    } else if (SHCORE_HISTORY_SQL_SYSLOG == option) {
      m_syslog.enable(data->get_bool("value"));
    }
  }
}

std::string Command_line_shell::get_current_session_uri() const {
  if (const auto session = _shell->get_dev_session(); session) {
    if (const auto core_session = session->get_core_session();
        core_session && core_session->is_open())
      return core_session->get_connection_options().as_uri();
  }

  return {};
}

void Command_line_shell::detect_session_change() {
  const auto session_uri = get_current_session_uri();

  if (session_uri != m_current_session_uri) {
    m_current_session_uri = session_uri;
    request_prompt_variables_update(true);
  }
}

void Command_line_shell::set_next_input(const std::string &input) {
  if (!input.empty()) {
    auto lines = shcore::str_split(input, "\n");

    // trim whitespace from the right side of the lines
    std::transform(
        lines.begin(), lines.end(), lines.begin(),
        [](const std::string &line) { return shcore::str_rstrip(line); });

    // remove consecutive empty lines at the end
    while (lines.size() > 1 && lines.back().empty() &&
           std::prev(lines.end(), 2)->empty()) {
      lines.pop_back();
    }

    // if there are only two lines, and the last one is empty, remove it as
    // well
    if (lines.size() == 2 && lines.back().empty()) {
      lines.pop_back();
    }

    // the last line goes to the linenoise and can be edited
    const auto last_line = lines.back();
    lines.pop_back();

    const auto console = current_console();

    // emulate user writing the input line by line
    _input_buffer = "";
    _input_mode = shcore::Input_state::Ok;

    for (const auto &line : lines) {
      console->raw_print(prompt() + line + "\n", mysqlsh::Output_stream::STDOUT,
                         false);

      _input_buffer += line + "\n";
      _input_mode = shcore::Input_state::ContinuedBlock;
    }

    // feed the last line to the linenoise
    if (!last_line.empty()) {
      linenoisePreloadBuffer(last_line.c_str());
    }
  }
}

bool Command_line_shell::set_pending_command(const Pending_command &command) {
  if (m_set_pending_command) {
    m_pending_command = command;
  }

  return m_set_pending_command;
}

void Command_line_shell::process_line(const std::string &line) {
  if (m_pending_command) {
    m_pending_command(line);
    m_pending_command = nullptr;
  } else {
    auto mode = _shell->interactive_mode();
    const char *mode_name = nullptr;
    switch (mode) {
      case shcore::IShell_core::Mode::None:
        mode_name = "none";
        break;
      case shcore::IShell_core::Mode::SQL:
        mode_name = "sql";
        break;
      case shcore::IShell_core::Mode::JavaScript:
        mode_name = "js";
        break;
      case shcore::IShell_core::Mode::Python:
        mode_name = "py";
        break;
    }
    shcore::Log_sql_guard g(mode_name);
    Mysql_shell::process_line(line);
  }
}

void Command_line_shell::syslog(const std::string &statement) {
  // log SQL statements and \source commands
  if (m_syslog.active() &&
      ('\\' != statement[0] || shcore::str_beginswith(statement, "\\source ") ||
       shcore::str_beginswith(statement, "\\. "))) {
    m_syslog.log(shcore::syslog::Level::INFO, syslog_format(statement));
  }
}

std::string Command_line_shell::syslog_format(const std::string &statement) {
  const std::size_t max_log_length = 1024;
  const auto variables = prompt_variables();

  std::vector<std::string> data;

  const auto add_data = [&data](const char *key, const std::string &value) {
    data.emplace_back(shcore::make_kvp(key, value.empty() ? "--" : value));
  };

  const auto add_data_from_prompt =
      [&variables, &add_data](const char *key, const std::string &var) {
        const auto value = variables->find(var);

        add_data(key, variables->end() != value ? value->second : "");
      };

  add_data_from_prompt("SYSTEM_USER", "system_user");
  add_data_from_prompt("MYSQL_USER", "user");
  add_data_from_prompt("CONNECTION_ID", "connection_id");
  add_data_from_prompt("DB_SERVER", "host");
  add_data_from_prompt("DB", "schema");
  add_data("QUERY", statement);

  return shcore::truncate(shcore::str_join(data, " "), max_log_length);
}

void Command_line_shell::pause_history(bool flag) {
  _history.pause(flag);
  m_syslog.pause(flag);
}

}  // namespace mysqlsh
