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
#ifndef _INTERACTIVE_SHELL_
#define _INTERACTIVE_SHELL_

#include "shellcore/shell_options.h"
#include "scripting/types.h"
#include "shellcore/shell_core.h"

namespace mysqlsh {
class SHCORE_PUBLIC Base_shell {
public:
  Base_shell(const Shell_options &options, shcore::Interpreter_delegate *custom_delegate);
  int process_stream(std::istream & stream, const std::string& source,
                     const std::vector<std::string> &argv, bool force_batch = false);
  int process_file(const std::string& file, const std::vector<std::string> &argv);

  /** Finalize initialization steps after basic init of the shell is already done
      Does things like loading init scripts.
   */
  virtual void finish_init();

  void init_environment();
  void init_scripts(shcore::Shell_core::Mode mode);
  void load_default_modules(shcore::Shell_core::Mode mode);

  void println(const std::string &str = "");

  void print_error(const std::string &error);

  void print_connection_message(mysqlsh::SessionType type, const std::string& uri, const std::string& sessionid);
  shcore::IShell_core::Mode interactive_mode() const { return _shell->interactive_mode(); }

  virtual void process_line(const std::string &line);
  void notify_executed_statement(const std::string& line);
  std::string prompt();

  std::shared_ptr<shcore::Shell_core> shell_context() const { return _shell; }

  void set_global_object(const std::string &name,
                         std::shared_ptr<shcore::Cpp_object_bridge> object,
                         shcore::IShell_core::Mode_mask modes =
                             shcore::IShell_core::Mode_mask::any());
  bool switch_shell_mode(shcore::Shell_core::Mode mode, const std::vector<std::string> &args);

protected:
  mysqlsh::Shell_options _options;
  std::shared_ptr<shcore::Shell_core> _shell;
  shcore::Input_state _input_mode;
  std::string _input_buffer;

private:
  void process_result(shcore::Value result);

  std::function<void(shcore::Value)> _result_processor;

  shcore::Interpreter_delegate _delegate;
};
}
#endif
