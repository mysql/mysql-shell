/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysh_config.h"
#include "mysqlsh/cmdline_shell.h"
#include "shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#ifdef ENABLE_SESSION_RECORDING
#include "mysqlshdk/libs/db/replay/setup.h"
#endif

#include <mysql_version.h>
#include <sys/stat.h>
#include <cstdio>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#endif

const char *g_mysqlsh_argv0;
static mysqlsh::Command_line_shell *g_shell_ptr = NULL;

#ifdef WIN32
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define snprintf _snprintf

static BOOL windows_ctrl_handler(DWORD fdwCtrlType) {
  switch (fdwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      try {
        shcore::Interrupts::interrupt();
      } catch (std::exception &e) {
        log_error("Unhandled exception in SIGINT handler: %s", e.what());
      }
      // Don't let the default handler terminate us
      return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      // TODO: Add proper exit handling if needed
      break;
  }
  /* Pass signal to the next control handler function. */
  return FALSE;
}

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  virtual void setup() {
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)windows_ctrl_handler, TRUE);
  }

  virtual void block() {
  }

  virtual void unblock(bool clear_pending) {
  }
};

#else  // !WIN32

#include <signal.h>

/**
SIGINT signal handler.

@description
This function handles SIGINT (Ctrl-C). It will call a shell function
which should handle the interruption in an appropriate fashion.
If the interrupt() function returns false, it will interret as a signal
that the shell itself should abort immediately.

@param [IN]               Signal number
*/

static void handle_ctrlc_signal(int sig) {
  int errno_save = errno;
  try {
    shcore::Interrupts::interrupt();
  } catch (std::exception &e) {
    log_error("Unhandled exception in SIGINT handler: %s", e.what());
  }
  if (shcore::Interrupts::propagates_interrupt()) {
    // propagate the ^C to the caller of the shell
    // this is the usual handling when we're running in batch mode
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
  errno = errno_save;
}

static void install_signal_handler() {
  signal(SIGINT, handle_ctrlc_signal);
  // Ignore broken pipe signal
  signal(SIGPIPE, SIG_IGN);
}

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  virtual void setup() {
    install_signal_handler();
  }

  virtual void block() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
  }

  virtual void unblock(bool clear_pending) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);

    if (clear_pending && sigpending(&sigset)) {
      struct sigaction ign, old;
      // set to ignore SIGINT
      ign.sa_handler = SIG_IGN;
      ign.sa_flags = 0;
      sigaction(SIGINT, &ign, &old);
      // unblock and let SIGINT be delivered (and ignored)
      sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
      // resore orignal SIGINT handler
      sigaction(SIGINT, &old, nullptr);
    } else {
      sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
    }
  }
};

#endif  //! WIN32

static int enable_x_protocol(mysqlsh::Command_line_shell &shell) {
  static const char *script =
      "function enableXProtocol()\n"
      "{\n"
      "  try\n"
      "  {\n"
      "    var mysqlx_port = session.runSql('select "
      "@@mysqlx_port').fetchOne();\n"
      "    print('enableXProtocol: X Protocol plugin is already enabled and "
      "listening for connections on port '+mysqlx_port[0]+'\\n');\n"
      "    return 0;\n"
      "  }\n"
      "  catch (error)\n"
      "  {\n"
      "    if (error[\"code\"] != 1193) // unknown system variable\n"
      "    {\n"
      "      print('enableXProtocol: Error checking for X Protocol plugin: "
      "'+error[\"message\"]+'\\n');\n"
      "      return 1;\n"
      "    }\n"
      "  }\n"
      "  print('enableXProtocol: Installing plugin mysqlx...\\n');\n"
      "  var os = session.runSql('select @@version_compile_os').fetchOne();\n"
      "  try {\n"
      "    if (os[0] == \"Win32\" || os[0] == \"Win64\")\n"
      "    {\n"
      "      var r = session.runSql(\"install plugin mysqlx soname "
      "'mysqlx.dll';\");\n"
      "    }\n"
      "    else\n"
      "    {\n"
      "      var r = session.runSql(\"install plugin mysqlx soname "
      "'mysqlx.so';\")\n"
      "    }\n"
      "    print(\"enableXProtocol: done\\n\");\n"
      "  } catch (error) {\n"
      "    print('enableXProtocol: Error installing the X Plugin: "
      "'+error['message']+'\\n');\n"
      "  }\n"
      "}\n"
      "enableXProtocol(); print('');\n"
      "\n";
  std::stringstream stream(script);
  return shell.process_stream(stream, "(command line)", {});
}

// Execute a Administrative DB command passed from the command line via the
// --dba option Currently, only the enableXProtocol command is supported.
int execute_dba_command(mysqlsh::Command_line_shell *shell,
                        const std::string &command) {
  if (command != "enableXProtocol") {
    shell->print_error("Unsupported dba command " + command);
    return 1;
  }

  // this is a temporary solution, ideally there will be a dba object/module
  // that implements all commands are the requested command will be invoked as
  // dba.command() with param handling and others, from both interactive shell
  // and cmdline

  return enable_x_protocol(*shell);
}

// Detects whether the shell will be running in interactive mode or not
// Non interactive mode is used when:
// - A file is processed using the --file option
// - A file is processed through the OS redirection mechanism
// - stdin is not a tty
//
// Interactive mode is used when:
// - stdin is a tty
// - --interactive option is passed
//
// An error occurs when both --file and STDIN redirection are used
std::string detect_interactive(mysqlsh::Shell_options *options,
                               bool *stdin_is_tty, bool *stdout_is_tty) {
  assert(options);

  bool is_interactive = true;
  std::string error;

  *stdin_is_tty = false;
  *stdout_is_tty = false;

  int __stdin_fileno;
  int __stdout_fileno;

#if defined(WIN32)
  __stdin_fileno = _fileno(stdin);
  __stdout_fileno = _fileno(stdout);
#else
  __stdin_fileno = STDIN_FILENO;
  __stdout_fileno = STDOUT_FILENO;
#endif
  *stdin_is_tty = isatty(__stdin_fileno) != 0;
  *stdout_is_tty = isatty(__stdout_fileno) != 0;

  if (!*stdin_is_tty || !*stdout_is_tty)
    is_interactive = false;
  else
    is_interactive = options->get().run_file.empty() &&
                     options->get().execute_statement.empty();

  // The --interactive option forces the shell to work emulating the
  // interactive mode no matter if:
  // - Input is being redirected from file
  // - Input is being redirected from STDIN
  // - It is not running on a terminal
  if (options->get().interactive)
    is_interactive = true;

  options->set_interactive(is_interactive);
  return error;
}

static mysqlshdk::textui::Color_capability detect_color_capability() {
  mysqlshdk::textui::Color_capability color_mode = mysqlshdk::textui::Color_256;
#ifdef _WIN32
  // Try to enable VT100 escapes... if it doesn't work,
  // then it disables the ansi escape sequences
  // Supported in Windows 10 command window and some other terminals
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(handle, &mode);

  // ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(handle, mode | 0x004)) {
    if (getenv("ANSICON")) {
      // ConEmu
      color_mode = mysqlshdk::textui::Color_rgb;
    } else {
      color_mode = mysqlshdk::textui::No_color;
    }
  } else {
    color_mode = mysqlshdk::textui::Color_rgb;
  }
#else
  const char *term = getenv("TERM");
  if (term) {
    if (shcore::str_endswith(term, "-256color") == 0) {
      color_mode = mysqlshdk::textui::Color_256;
    }
  } else {
    color_mode = mysqlshdk::textui::Color_16;
  }
#endif
  return color_mode;
}

std::string pick_prompt_theme(const char *argv0) {
  mysqlshdk::textui::Color_capability mode = detect_color_capability();
  if (const char *force_mode = getenv("MYSQLSH_TERM_COLOR_MODE")) {
    if (strcmp(force_mode, "rgb") == 0) {
      mode = mysqlshdk::textui::Color_rgb;
    } else if (strcmp(force_mode, "256") == 0) {
      mode = mysqlshdk::textui::Color_256;
    } else if (strcmp(force_mode, "16") == 0) {
      mode = mysqlshdk::textui::Color_16;
    } else if (strcmp(force_mode, "nocolor") == 0) {
      mode = mysqlshdk::textui::No_color;
    } else if (strcmp(force_mode, "") != 0) {
      std::cout << "NOTE: MYSQLSH_TERM_COLOR_MODE environment variable set to "
                   "invalid value. Must be one of rgb, 256, 16, nocolor\n";
      return "";
    }
  }
  log_debug("Using color mode %i", static_cast<int>(mode));
  mysqlshdk::textui::set_color_capability(mode);

  // check environment variable to override prompt theme
  if (char *theme = getenv("MYSQLSH_PROMPT_THEME")) {
    if (*theme) {
      if (!shcore::file_exists(theme)) {
        std::cout << "NOTE: MYSQLSH_PROMPT_THEME prompt theme file '" << theme
                  << "' does not exist.\n";
        return "";
      }
      log_debug("Using prompt theme file %s", theme);
    }
    return theme;
  }

  // check user overriden prompt theme
  std::string path = shcore::get_user_config_path();
  path.append("prompt.json");

  if (shcore::file_exists(path)) {
    log_debug("Using prompt theme file %s", path.c_str());
    return path;
  }

#ifdef _WIN32
  path = shcore::get_binary_folder();
  path += "/../prompt/";
#else
  path = shcore::get_binary_folder();
  path += "/../";
  path += INSTALL_SHAREDIR;
  path += "/prompt/";
#endif
  switch (mode) {
    case mysqlshdk::textui::Color_rgb:
    case mysqlshdk::textui::Color_256:
      path += "prompt_256.json";
      break;
    case mysqlshdk::textui::Color_16:
      path += "prompt_16.json";
      break;
    case mysqlshdk::textui::No_color:
      path += "prompt_nocolor.json";
      break;
    default:
      path += "prompt_classic.json";
      break;
  }
  log_debug("Using prompt theme file %s", path.c_str());
  return path;
}

int main(int argc, char **argv) {
#ifdef WIN32
  UINT origcp = GetConsoleCP();
  UINT origocp = GetConsoleOutputCP();

  // Enable UTF-8 input and output
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
#endif
  int ret_val = 0;
  Interrupt_helper sighelper;
  g_mysqlsh_argv0 = argv[0];
  shcore::Interrupts::init(&sighelper);

  std::shared_ptr<mysqlsh::Shell_options> shell_options =
      std::make_shared<mysqlsh::Shell_options>(argc, argv);
  const mysqlsh::Shell_options::Storage& options = shell_options->get();

  if (options.exit_code != 0)
    return options.exit_code;

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif
#ifdef ENABLE_SESSION_RECORDING
  mysqlshdk::db::replay::setup_from_env();
#endif

  try {
    bool stdin_is_tty = false;
    bool stdout_is_tty = false;
    std::string error =
        detect_interactive(shell_options.get(), &stdin_is_tty, &stdout_is_tty);

    // If not a tty, then autocompletion can't be used, so we disable
    // name cache for autocompletion... but keep it for db object from DevAPI
    if (!options.db_name_cache_set &&
        (!options.interactive || !stdin_is_tty || !stdout_is_tty))
      shell_options->set_db_name_cache(false);

    // Usage of wizards will be disabled if running in non interactive mode
    if (!options.interactive)
      shell_options->set_wizards(false);
    // Switch default output format to tab separated instead of table
    if (!options.interactive && options.output_format.empty() && !stdout_is_tty)
      shell_options->set(SHCORE_OUTPUT_FORMAT, shcore::Value("tabbed"));

    bool interrupted = false;
    if (!options.interactive) {
      shcore::Interrupts::push_handler([&interrupted]() {
        interrupted = true;
        return false;
      });
    }

    mysqlsh::Command_line_shell shell(shell_options);

    if (!error.empty()) {
      shell.print_error(error);
      ret_val = 1;
    } else if (shell_options->action_print_version()) {
      char version_msg[1024];
      if (*MYSH_BUILD_ID && shell_options->action_print_version_extra()) {
        snprintf(version_msg, sizeof(version_msg),
                 "%s   Ver %s for %s on %s - for MySQL %s (%s) - build %s",
                 argv[0], MYSH_VERSION, SYSTEM_TYPE, MACHINE_TYPE,
                 LIBMYSQL_VERSION, MYSQL_COMPILATION_COMMENT, MYSH_BUILD_ID);
      } else {
        snprintf(version_msg, sizeof(version_msg),
                 "%s   Ver %s for %s on %s - for MySQL %s (%s)", argv[0],
                 MYSH_VERSION, SYSTEM_TYPE, MACHINE_TYPE, LIBMYSQL_VERSION,
                 MYSQL_COMPILATION_COMMENT);
      }
#ifdef ENABLE_SESSION_RECORDING
      shell.println(std::string(version_msg) + " + session_recorder");
#else
      shell.println(version_msg);
#endif
      ret_val = options.exit_code;
    } else if (shell_options->action_print_help()) {
      shell.print_cmd_line_helper();
      ret_val = options.exit_code;
    } else {
      // Performs the connection
      if (options.has_connection_data()) {
        try {
          mysqlshdk::db::Connection_options target =
              options.connection_options();

          if (target.has(mysqlshdk::db::kAuthMethod) &&
              target.get(mysqlshdk::db::kAuthMethod) == "PLAIN") {
            std::cerr << "mysqlx: [Warning] PLAIN authentication method is NOT "
                         "secure!\n";
          }
          if (target.has_password()) {
            std::cerr << "mysqlx: [Warning] Using a password on the command "
                         "line interface can be insecure.\n";
          }

          shell.connect(target, options.recreate_database);
        } catch (shcore::Exception &e) {
          std::cerr << e.format() << "\n";
          ret_val = 1;
          goto end;
        } catch (std::exception &e) {
          std::cerr << e.what() << "\n";
          ret_val = 1;
          goto end;
        }
      }

      g_shell_ptr = &shell;
      shell.load_prompt_theme(pick_prompt_theme(argv[0]));

      if (!options.execute_statement.empty()) {
        std::stringstream stream(options.execute_statement);
        ret_val = shell.process_stream(stream, "(command line)", {});
      } else if (!options.execute_dba_statement.empty()) {
        if (options.initial_mode != shcore::IShell_core::Mode::JavaScript) {
          shell.print_error(
              "The --dba option can only be used in JavaScript mode\n");
          ret_val = 1;
        } else {
          ret_val = execute_dba_command(&shell, options.execute_dba_statement);
        }
      } else if (!options.run_file.empty()) {
        ret_val = shell.process_file(options.run_file, options.script_argv);
      } else if (options.interactive) {
        shell.load_state(shcore::get_user_config_path());
        if (stdin_is_tty)
          shell.print_banner();

        shell.command_loop();

        shell.save_state(shcore::get_user_config_path());

        ret_val = 0;
      } else {
        ret_val = shell.process_stream(std::cin, "STDIN", {});
      }
    }
    if (interrupted) {
#ifdef _WIN32
      ret_val = 130;
#else
      signal(SIGINT, SIG_DFL);
      kill(getpid(), SIGINT);
#endif
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    ret_val = 1;
  }

  end:
#ifdef _WIN32
  // Restore original codepage
  SetConsoleCP(origcp);
  SetConsoleOutputCP(origocp);
#endif
  return ret_val;
}
