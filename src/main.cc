/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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

#include "interactive_shell.h"
#include <sys/stat.h>
#include <sstream>

#ifdef WIN32
#  include <io.h>
#  include <windows.h>
#  define isatty _isatty

BOOL windows_ctrl_handler(DWORD fdwCtrlType)
{
  switch (fdwCtrlType)
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      // TODO: Add proper Ctrl+C handling if needed
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
#endif

// Detects whether the shell will be running in interactive mode or not
// Non interactive mode is used when:
// - A file is processed using the --file option
// - A file is processed through the OS redirection mechanism
//
// Interactive mode is used when:
// - A file is processed using the --interactive option
// - No file is processed
//
// An error occurs when both --file and STDIN redirection are used
std::string detect_interactive(Shell_command_line_options &options, bool &from_stdin)
{
  bool is_interactive = true;
  std::string error;

  from_stdin = false;

  int __stdin_fileno;
  int __stdout_fileno;

#if defined(WIN32)
  __stdin_fileno = _fileno(stdin);
  __stdout_fileno = _fileno(stdout);
#else
  __stdin_fileno = STDIN_FILENO;
  __stdout_fileno = STDOUT_FILENO;
#endif

  if (!isatty(__stdin_fileno))
  {
    // Here we know the input comes from stdin
    from_stdin = true;
  }
  if (!isatty(__stdin_fileno) || !isatty(__stdout_fileno))
    is_interactive = false;
  else
    is_interactive = options.run_file.empty() && options.execute_statement.empty();

  // The --interactive option forces the shell to work emulating the
  // interactive mode no matter if:
  // - Input is being redirected from file
  // - Input is being redirected from STDIN
  // - It is not running on a terminal
  if (options.interactive)
    is_interactive = true;

  options.interactive = is_interactive;
  return error;
}

int main(int argc, char **argv)
{
#ifdef WIN32
  // Sets console handler (Ctrl+C)
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)windows_ctrl_handler, TRUE);
#endif

  int ret_val = 0;

  Shell_command_line_options options(argc, argv);

  if (options.exit_code != 0)
    return options.exit_code;

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  {
    bool from_stdin = false;
    std::string error = detect_interactive(options, from_stdin);

    Interactive_shell shell(options);

    if (!error.empty())
    {
      shell.print_error(error);
      ret_val = 1;
    }
    else if (options.print_version)
    {
      std::string version_msg("MySQL X Shell Version ");
      version_msg += MYSH_VERSION;
      version_msg += "\n";
      shell.print(version_msg);
      ret_val = options.exit_code;
    }
    else if (options.print_cmd_line_helper)
    {
      shell.print_cmd_line_helper();
      ret_val = options.exit_code;
    }
    else
    {
      // Performs the connection
      if (options.has_connection_data())
      {
        try
        {
          if (!shell.connect(true))
            return 1;
        }
        catch (shcore::Exception &e)
        {
          shell.print_error(e.format());
          return 1;
        }
        catch (std::exception &e)
        {
          shell.print_error(e.what());
          return 1;
        }
      }

      if (!options.execute_statement.empty())
      {
        std::stringstream stream(options.execute_statement);
        ret_val = shell.process_stream(stream, "STDIN");
      }
      else if (!options.run_file.empty())
        ret_val = shell.process_file();
      else if (from_stdin)
      {
        ret_val = shell.process_stream(std::cin, "STDIN");
      }
      else if (options.interactive)
      {
        shell.print_banner();
        shell.command_loop();
        ret_val = 0;
      }
    }
  }

  return ret_val;
}