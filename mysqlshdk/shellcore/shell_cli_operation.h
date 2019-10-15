/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/options.h"

namespace shcore {

class Shell_cli_operation {
 public:
  using Provider =
      std::function<std::shared_ptr<shcore::Cpp_object_bridge>(void)>;

  class Mapping_error : public std::invalid_argument {
   public:
    explicit Mapping_error(const std::string &description)
        : std::invalid_argument(description) {}
  };

  void register_provider(const std::string &name, Provider provider);
  void remove_provider(const std::string &name);

  void set_object_name(const std::string &name);
  void set_method_name(const std::string &name);
  void add_argument(const shcore::Value &argument);
  void add_option(const std::string &option_definition);
  static void add_option(Value::Map_type_ref target_map,
                         const std::string &option_definition);

  void parse(Options::Cmdline_iterator *cmdline_iterator);

  bool empty() { return m_object_name.empty(); }

  Value execute();

 protected:
  Value::Map_type_ref fix_connection_options(Value::Map_type_ref dict);

  std::unordered_map<std::string, Provider> m_providers;

  std::string m_object_name;
  std::string m_method_name_original;
  std::string m_method_name;
  Argument_list m_argument_list;
  Value::Map_type_ref m_argument_map;
};

} /* namespace shcore */

#endif /* MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_H_ */
