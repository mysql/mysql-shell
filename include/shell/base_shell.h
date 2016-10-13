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

#include "shell/shell_options.h"
#include "shellcore/types.h"
#include "shellcore/shell_core.h"

namespace mysh {
class SHCORE_PUBLIC Base_shell {
public:
  Base_shell(const Shell_options &options, shcore::Interpreter_delegate *custom_delegate);
  int process_stream(std::istream & stream, const std::string& source,
                     const std::vector<std::string> &argv);
  int process_file(const std::string& file, const std::vector<std::string> &argv);

  void init_environment();
  void init_scripts(shcore::Shell_core::Mode mode);

  bool cmd_process_file(const std::vector<std::string>& params);
  bool connect(bool primary_session = false);

  void println(const std::string &str = "");

  void print_error(const std::string &error);

  bool cmd_print_shell_help(const std::vector<std::string>& args);
  bool cmd_start_multiline(const std::vector<std::string>& args);
  bool cmd_connect(const std::vector<std::string>& args);
  bool cmd_quit(const std::vector<std::string>& args);
  bool cmd_warnings(const std::vector<std::string>& args);
  bool cmd_nowarnings(const std::vector<std::string>& args);
  bool cmd_store_connection(const std::vector<std::string>& args);
  bool cmd_delete_connection(const std::vector<std::string>& args);
  bool cmd_list_connections(const std::vector<std::string>& args);
  bool cmd_status(const std::vector<std::string>& args);
  bool cmd_use(const std::vector<std::string>& args);

  void print_connection_message(mysh::SessionType type, const std::string& uri, const std::string& sessionid);
  shcore::IShell_core::Mode interactive_mode() const { return _shell->interactive_mode(); }

  void set_log_level(ngcommon::Logger::LOG_LEVEL level) { if (_logger) _logger->set_log_level(level); }

  void process_line(const std::string &line);
  void abort();
  std::string prompt();

  std::shared_ptr<shcore::Shell_core> shell_context() const { return _shell; }

protected:
  mysh::Shell_options _options;
  std::shared_ptr<shcore::Shell_core> _shell;

private:
  void process_result(shcore::Value result);
  ngcommon::Logger* _logger;

  bool switch_shell_mode(shcore::Shell_core::Mode mode, const std::vector<std::string> &args);
  std::function<void(shcore::Value)> _result_processor;

private:
  shcore::Value connect_session(const shcore::Argument_list &args, mysh::SessionType session_type, bool recreate_schema);

private:
  bool do_shell_command(const std::string &command);
private:
  shcore::Interpreter_delegate _delegate;

  std::string _input_buffer;
  shcore::Input_state _input_mode;

  shcore::Shell_command_handler _shell_command_handler;
};
}
#endif
