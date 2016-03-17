/*
 * Copyright (c) 2014, 2016 Oracle and/or its affiliates. All rights reserved.
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
#ifndef _INTERACTIVE_SHELL_
#define _INTERACTIVE_SHELL_

#ifndef WIN32
#  include "editline/readline.h"
#endif

#include "shell_cmdline_options.h"
#include "shellcore/types.h"
#include "shellcore/shell_core.h"

using namespace shcore;

class Interactive_shell
{
public:
  Interactive_shell(const Shell_command_line_options &options, Interpreter_delegate *custom_delegate = NULL);
  void command_loop();
  int process_stream(std::istream & stream, const std::string& source);
  int process_file();

  void init_environment();
  void init_scripts(Shell_core::Mode mode);

  bool cmd_process_file(const std::vector<std::string>& params);
  bool connect(bool primary_session = false);

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);
  void print_json_info(const std::string &info, const std::string& label = "info");

  bool cmd_print_shell_help(const std::vector<std::string>& args);
  bool cmd_start_multiline(const std::vector<std::string>& args);
  bool cmd_connect(const std::vector<std::string>& args);
  bool cmd_connect_node(const std::vector<std::string>& args);
  bool cmd_connect_classic(const std::vector<std::string>& args);
  bool cmd_quit(const std::vector<std::string>& args);
  bool cmd_warnings(const std::vector<std::string>& args);
  bool cmd_nowarnings(const std::vector<std::string>& args);
  bool cmd_store_connection(const std::vector<std::string>& args);
  bool cmd_delete_connection(const std::vector<std::string>& args);
  bool cmd_update_connection(const std::vector<std::string>& args);
  bool cmd_list_connections(const std::vector<std::string>& args);
  bool cmd_status(const std::vector<std::string>& args);

  void print_banner();
  void print_connection_message(mysh::SessionType type, const std::string& uri, const std::string& sessionid);
  void print_cmd_line_helper();
  IShell_core::Mode interactive_mode() const { return _shell->interactive_mode(); }

  void set_log_level(ngcommon::Logger::LOG_LEVEL level) { if (_logger) _logger->set_log_level(level); }

  void process_line(const std::string &line);
  void abort();
  std::string prompt();

private:
  Shell_command_line_options _options;
  static char *readline(const char *prompt);

  void process_result(shcore::Value result);
  ngcommon::Logger* _logger;

  bool switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);
  boost::function<void(shcore::Value)> _result_processor;

private:
  shcore::Value connect_session(const shcore::Argument_list &args, mysh::SessionType session_type, bool recreate_schema);

private:
  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_prompt(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

  bool do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::ShellBaseSession> _session;
  //boost::shared_ptr<mysh::mysqlx::Schema> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  Interactive_input_state _input_mode;

  Shell_command_handler _shell_command_handler;
};
#endif
