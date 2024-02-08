/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/shellcore/shell_cli_mapper.h"
#include "mysqlshdk/shellcore/shell_cli_operation_provider.h"

namespace shcore {
namespace cli {

class Shell_cli_operation {
 public:
  Provider *get_provider() { return &m_provider; }
  void set_object_name(const std::string &name);
  void set_method_name(const std::string &name);
  void add_argument(const shcore::Value &argument);
  void add_cmdline_argument(const std::string &cmdline_arg);

  void parse(Options::Cmdline_iterator *cmdline_iterator);

  bool help_requested() { return m_cli_mapper.help_requested(); }

  void prepare();

  Value execute();

 protected:
  void print_help();
  Provider m_provider;
  Provider *m_current_provider;

  std::string m_object_name;
  std::string m_method_name_original;
  std::string m_method_name;
  Shell_cli_mapper m_cli_mapper;
  Argument_list m_argument_list;
};
}  // namespace cli
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_
