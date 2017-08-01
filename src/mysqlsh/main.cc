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

#include <mysql_version.h>
#include <sys/stat.h>
#include <cstdio>
#include <sstream>
#include <iostream>

#include "mysh_config.h"
#include "mysqlsh/cmdline_shell.h"
#include "mysqlsh/shell_cmdline_options.h"
#include "shellcore/interrupt_handler.h"

#ifndef WIN32
#include <unistd.h>
#endif

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

  virtual void block() {}

  virtual void unblock(bool clear_pending) {}
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
}

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  virtual void setup() { install_signal_handler(); }

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
  static const char *script = "function enableXProtocol()\n"\
"{\n"\
"  try\n"\
"  {\n"\
"    var mysqlx_port = session.runSql('select @@mysqlx_port').fetchOne();\n"\
"    print('enableXProtocol: X Protocol plugin is already enabled and listening for connections on port '+mysqlx_port[0]+'\\n');\n"\
"    return 0;\n"\
"  }\n"\
"  catch (error)\n"\
"  {\n"\
"    if (error[\"code\"] != 1193) // unknown system variable\n"\
"    {\n"\
"      print('enableXProtocol: Error checking for X Protocol plugin: '+error[\"message\"]+'\\n');\n"\
"      return 1;\n"\
"    }\n"\
"  }\n"\
"  print('enableXProtocol: Installing plugin mysqlx...\\n');\n"\
"  var os = session.runSql('select @@version_compile_os').fetchOne();\n"\
"  try {\n"\
"    if (os[0] == \"Win32\" || os[0] == \"Win64\")\n"\
"    {\n"\
"      var r = session.runSql(\"install plugin mysqlx soname 'mysqlx.dll';\");\n"\
"    }\n"\
"    else\n"\
"    {\n"\
"      var r = session.runSql(\"install plugin mysqlx soname 'mysqlx.so';\")\n"\
"    }\n"\
"    print(\"enableXProtocol: done\\n\");\n"\
"  } catch (error) {\n"\
"    print('enableXProtocol: Error installing the X Plugin: '+error['message']+'\\n');\n"\
"  }\n"\
"}\n"\
"enableXProtocol(); print('');\n"\
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
                               bool *from_stdin, bool *stdout_is_tty) {
  assert(options);
  assert(from_stdin);

  bool is_interactive = true;
  std::string error;

  *from_stdin = false;
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
  *from_stdin = !isatty(__stdin_fileno);

  if (!isatty(__stdin_fileno)) {
    // Here we know the input comes from stdin
    *from_stdin = true;
  }

  *stdout_is_tty = isatty(__stdout_fileno) != 0;

  if (!isatty(__stdin_fileno) || !isatty(__stdout_fileno))
    is_interactive = false;
  else
    is_interactive =
        options->run_file.empty() && options->execute_statement.empty();

  // The --interactive option forces the shell to work emulating the
  // interactive mode no matter if:
  // - Input is being redirected from file
  // - Input is being redirected from STDIN
  // - It is not running on a terminal
  if (options->interactive)
    is_interactive = true;

  options->interactive = is_interactive;
  return error;
}

int main(int argc, char **argv) {
  int ret_val = 0;
  Interrupt_helper sighelper;

  shcore::Interrupts::init(&sighelper);

  Shell_command_line_options cmd_line_options(argc,
                                              const_cast<const char **>(argv));
  mysqlsh::Shell_options options = cmd_line_options.get_options();

  if (options.exit_code != 0)
    return options.exit_code;

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  try {
    bool from_stdin = false;
    bool stdout_is_tty = false;
    std::string error =
        detect_interactive(&options, &from_stdin, &stdout_is_tty);

    // Usage of wizards will be disabled if running in non interactive mode
    if (!options.interactive)
      options.wizards = false;
    // Switch default output format to tab separated instead of table
    if (!options.interactive && options.output_format.empty() && !stdout_is_tty)
      options.output_format = "tabbed";

    bool interrupted = false;
    if (!options.interactive) {
      shcore::Interrupts::push_handler([&interrupted]() {
        interrupted = true;
        return false;
      });
    }

    mysqlsh::Command_line_shell shell(options);

    if (!error.empty()) {
      shell.print_error(error);
      ret_val = 1;
    } else if (options.print_version) {
      char version_msg[1024];
      if (*MYSH_BUILD_ID && options.print_version_extra) {
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
      shell.println(version_msg);
      ret_val = options.exit_code;
    } else if (options.print_cmd_line_helper) {
      shell.print_cmd_line_helper();
      ret_val = options.exit_code;
    } else {
      // Performs the connection
      if (options.has_connection_data()) {
        try {
          if (!shell.connect(true))
            return 1;
        } catch (shcore::Exception &e) {
          shell.print_error(e.format());
          return 1;
        } catch (std::exception &e) {
          shell.print_error(e.what());
          return 1;
        }
      }

      // This must be set after all initialization is done, otherwise
      // a signal could be caught at a time that the shell is in an
      // inconsistent state
      g_shell_ptr = &shell;

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
        if (!from_stdin)
          shell.print_banner();
        shell.command_loop();
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
  return ret_val;
}
