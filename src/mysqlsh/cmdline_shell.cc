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

#include "cmdline_shell.h"
#include "shellcore/base_session.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h" // <---
#include "modules/base_resultset.h"
#include "utils/utils_time.h"
#include "shellcore/utils_help.h"
#include "logger/logger.h"

// TODO: This should be ported from the server, not used from there (see comment below)
//const int MAX_READLINE_BUF = 65536;
extern char *mysh_get_tty_password(const char *opt_message);

namespace mysqlsh {
Command_line_shell::Command_line_shell(const Shell_options &options) : mysqlsh::Mysql_shell(options, &_delegate) {
#ifndef WIN32
  rl_initialize();
#endif

  _delegate.user_data = this;
  _delegate.print = &Command_line_shell::deleg_print;
  _delegate.print_error = &Command_line_shell::deleg_print_error;
  _delegate.prompt = &Command_line_shell::deleg_prompt;
  _delegate.password = &Command_line_shell::deleg_password;
  _delegate.source = &Command_line_shell::deleg_source;
  _delegate.print_value = nullptr;

  observe_notification("SN_STATEMENT_EXECUTED");

  finish_init();
}

void Command_line_shell::deleg_print(void *cdata, const char *text) {
  std::cout << text << std::flush;
}

void Command_line_shell::deleg_print_error(void *cdata, const char *text) {
  std::cerr << text;
}

char *Command_line_shell::readline(const char *prompt) {
  char *tmp = NULL;
  // TODO: This should be ported from the server, not used from there
  /*
  tmp = (char *)malloc(MAX_READLINE_BUF);
  if (!tmp)
  throw Exception::runtime_error("Cannot allocate memory for Command_line_shell::readline buffer.");
  my_win_console_fputs(&my_charset_latin1, prompt);
  tmp = my_win_console_readline(&my_charset_latin1, tmp, MAX_READLINE_BUF);
  */

#ifndef WIN32
  std::string prompt_line(prompt);

  size_t pos = prompt_line.rfind("\n");
  if (pos != std::string::npos) {
    auto all_lines = prompt_line.substr(0, pos+1);
    prompt_line = prompt_line.substr(pos+1);

    if (!all_lines.empty())
      std::cout << all_lines << std::flush;
  }

  tmp = ::readline(prompt_line.c_str());
#else
  std::string line;
  std::cout << prompt << std::flush;

  std::getline(std::cin, line);

  if (!std::cin.fail())
    tmp = strdup(line.c_str());
#endif

  return tmp;
}

bool Command_line_shell::deleg_prompt(void *UNUSED(cdata), const char *prompt, std::string &ret) {
  char *tmp = Command_line_shell::readline(prompt);
  if (!tmp)
    return false;

  ret = tmp;
  free(tmp);

  return true;
}

bool Command_line_shell::deleg_password(void *cdata, const char *prompt, std::string &ret) {
  Command_line_shell *self = (Command_line_shell*)cdata;
  char *tmp = self->_options.passwords_from_stdin ? shcore::mysh_get_stdin_password(prompt) : mysh_get_tty_password(prompt);
  if (!tmp)
    return false;
  ret = tmp;
  free(tmp);
  return true;
}

void Command_line_shell::deleg_source(void *cdata, const char *module) {
  Command_line_shell *self = (Command_line_shell*)cdata;
  self->process_file(module, {});
}

void Command_line_shell::command_loop() {
  if (_options.interactive) // check if interactive
  {
    std::string message;
    auto session = _shell->get_dev_session();

    if (!session || (session && session->class_name() != "XSession"))
      message = " Use \\sql to switch to SQL mode and execute queries.";

    switch (_shell->interactive_mode()) {
      case shcore::Shell_core::Mode::SQL:
#ifdef HAVE_V8
        message = "Currently in SQL mode. Use \\js or \\py to switch the shell to a scripting language.";
#else
        message = "Currently in SQL mode. Use \\py to switch the shell to python scripting.";
#endif
        break;
      case shcore::Shell_core::Mode::JScript:
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
    char *cmd = Command_line_shell::readline(prompt().c_str());
    if (!cmd)
      break;

    process_line(cmd);
    free(cmd);
  }

  std::cout << "Bye!\n";
}

void Command_line_shell::print_banner() {
  std::string welcome_msg("Welcome to MySQL Shell ");
  welcome_msg += MYSH_VERSION;
  welcome_msg += "\n\n";
  welcome_msg += "Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.\n\n"\
                 "Oracle is a registered trademark of Oracle Corporation and/or its\n"\
                 "affiliates. Other names may be trademarks of their respective\n"\
                 "owners.";
  println(welcome_msg);
  println();
  println("Type '\\help', '\\h' or '\\?' for help, type '\\quit' or '\\q' to exit.");
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
  println("  --help                   Display this help and exit.");
  println("  -f, --file=file          Process file.");
  println("  -e, --execute=<cmd>      Execute command and quit.");
  println("  --uri                    Connect to Uniform Resource Identifier.");
  println("                           Format: [user[:pass]]@host[:port][/db]");
  println("                           or user[:pass]@::socket[/db] .");
  println("  -h, --host=name          Connect to host.");
  println("  -P, --port=#             Port number to use for connection.");
  println("  -S, --socket=sock        Socket name to use in UNIX, pipe name to use in Windows (only classic sessions).");
  println("  -u, --dbuser=name        User for the connection to the server.");
  println("  --user=name              An alias for dbuser.");
  println("  --dbpassword=name        Password to use when connecting to server");
  println("  --password=name          An alias for dbpassword.");
  println("  -p                       Request password prompt to set the password");
  println("  -D --schema=name         Schema to use.");
  println("  --recreate-schema        Drop and recreate the specified schema. Schema will be deleted if it exists!");
  println("  --database=name          An alias for --schema.");
  println("  --node                   Uses connection data to create a Node Session.");
  println("  --classic                Uses connection data to create a Classic Session.");
  println("  --sql                    Start in SQL mode.");
  println("  --sqlc                   Start in SQL mode using a classic session.");
  println("  --sqln                   Start in SQL mode using a node session.");
  println("  --js                     Start in JavaScript mode.");
  println("  --py                     Start in Python mode.");
  println("  --json                   Produce output in JSON format.");
  println("  --table                  Produce output in table format (default for interactive mode).");
  println("                           This option can be used to force that format when running in batch mode.");
  println("  -E, --vertical           Print the output of a query (rows) vertically.");
  println("  -i, --interactive[=full] To use in batch mode, it forces emulation of interactive mode processing.");
  println("                           Each line on the batch is processed as if it were in interactive mode.");
  println("  --force                  To use in SQL batch mode, forces processing to continue if an error is found.");
  println("  --log-level=value        The log level." + ngcommon::Logger::get_level_range_info());
  println("  --version                Prints the version of MySQL Shell.");
  println("  --ssl                    Enable SSL for connection(automatically enabled with other flags).");
  println("  --ssl-key=name           X509 key in PEM format.");
  println("  --ssl-cert=name          X509 cert in PEM format.");
  println("  --ssl-ca=name            CA file in PEM format.");
  println("  --ssl-capath=dir         CA directory.");
  println("  --ssl-cipher=name        SSL Cipher to use.");
  println("  --ssl-crl=name           Certificate revocation list.");
  println("  --ssl-crlpath=dir        Certificate revocation list path.");
  println("  --tls-version=version    TLS version to use, permitted values are : TLSv1, TLSv1.1.");
  println("  --passwords-from-stdin   Read passwords from stdin instead of the tty.");
  println("  --auth-method=method     Authentication method to use.");
  println("  --show-warnings          Automatically display SQL warnings on SQL mode if available.");
  println("  --dba enableXProtocol    Enable the X Protocol in the server connected to. Must be used with --classic.");
  println("  --no-wizard              Disables wizard mode.");

  println("");
  println("Usage examples:");
  println("$ mysqlsh root@localhost/schema");
  println("$ mysqlsh mysqlx://root@some.server:3307/world_x");
  println("$ mysqlsh --uri root@localhost --py -f sample.py sample param");
  println("$ mysqlsh root@targethost:33070 -s world_x -f sample.js");
  println("");
}

void Command_line_shell::handle_notification(const std::string &name, const shcore::Object_bridge_ref& sender, shcore::Value::Map_type_ref data){
#ifndef WIN32
  if(name=="SN_STATEMENT_EXECUTED"){
    std::string executed = data->get_string("statement");
    add_history(executed.c_str());
  }
#endif
}

}
