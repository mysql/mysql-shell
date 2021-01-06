/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CLI_MAPPER_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CLI_MAPPER_H_

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/shellcore/shell_cli_operation_provider.h"

namespace shcore {
namespace cli {
enum class Help_type { NONE, FUNCTION, OBJECT, GLOBAL };
/**
 * Definition of a command line arg as defined by the user.
 * Possible formats:
 * - --option[type]=value
 * - --option value
 * - -ovalue
 * - value
 * - -
 */
struct Cmd_line_arg_definition {
  // Holds the definition as specified by the user
  std::string definition;

  // Holds the option: --option or -o, empty for anonymous args
  std::string option;

  // Value as specified by the user, if type is specified the value is stored
  // using that type
  shcore::Value value;

  // Type specified by the user (if any)
  mysqlshdk::utils::nullable<shcore::Value_type> user_type;
};

/**
 * Holds the mapping of a cmdline option
 */
struct Cli_option {
  // Name of the option: --name
  std::string name;

  // Short name of the option
  std::string short_option;

  // Associated parameter
  std::string param;

  // Associated API option
  std::string option;

  // Required and defined flags
  bool required;
  bool defined;
};

class Shell_cli_mapper {
 public:
  void add_cmdline_argument(const std::string &cmdline_arg);
  Provider *identify_operation(Provider *base_provider);
  void set_metadata(const std::string &object_class,
                    const Cpp_function::Metadata &metadata);
  void process_arguments(shcore::Argument_list *argument_list);
  bool help_requested() const { return m_help_type != Help_type::NONE; }
  Help_type help_type() const { return m_help_type; }
  void clear();
  bool has_argument_options(const std::string &name) const;
  bool empty() const;

  void set_object_name(const std::string &name) {
    m_object_chain.push_back(name);
  }
  void set_operation_name(const std::string &name) { m_operation_name = name; }

  // Functions for the help system
  std::string object_name() const;
  const Cpp_function::Metadata *metadata() const { return m_metadata; }

  const std::vector<Cli_option> &options() const {
    return m_cli_option_registry;
  }

  const std::vector<Cmd_line_arg_definition> &get_cmdline_args() const {
    return m_cmdline_args;
  }

  std::string operation_name() const { return m_operation_name; }

  const std::vector<std::string> &get_object_chain() const {
    return m_object_chain;
  }

 private:
  void add_argument(const shcore::Value &argument);
  void define_named_arg(const Cli_option &option);
  void define_named_arg(const std::string &param, const std::string &option,
                        const std::string &short_option, bool required);

  void process_named_args();
  void process_positional_args();

  const shcore::Value &get_argument_options(const std::string &name);
  std::map<std::string, std::vector<const Cli_option *>> get_param_options();
  void define_named_args();

  /**
   * Adds a named option into it's corresponding parameter doing the data type
   * conversion so the API function receives the parameter type it expects.
   */
  void set_named_option(const std::string &param_name,
                        const std::string &option,
                        const Cmd_line_arg_definition &arg);

  Cli_option *cli_option(const std::string &name);

  std::string m_operation_name;
  std::string m_object_class;

  // Registry of all the named options supported on the operation
  std::vector<Cli_option> m_cli_option_registry;
  std::map<std::string, size_t> m_normalized_cli_options;

  // Keeps record of options that become ambiguous and the associated parameters
  std::map<std::string, std::vector<std::string>> m_ambiguous_cli_options;

  // First parameter accepting ANY option (in case it exists)
  std::string m_any_options_param;

  // Registry of the values associated to each argument through named options
  shcore::Value::Map_type m_argument_options;

  // Stores the position of each parameter to avoid having to search every time
  // a named option is specified
  std::map<std::string, size_t> m_param_index;

  // Function metadata and argument list to be filled by the mapper
  const Cpp_function::Metadata *m_metadata;
  shcore::Argument_list *m_argument_list;

  // Definition of the command line arguments specified by the user
  std::vector<Cmd_line_arg_definition> m_cmdline_args;

  // Counter of optional arguments that has been skipped before a new argument
  // is specified, used to fill the missing args with NULLs
  size_t m_missing_optionals;

  // Used when adding cmdline_args, for the case when the value for a named
  // argument comes in a separate cmd line arg.
  // i.e. --named-arg value
  bool m_waiting_value = false;

  Help_type m_help_type = Help_type::NONE;

  std::vector<std::string> m_object_chain;
};
}  // namespace cli
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_SHELL_CLI_MAPPER_H_
