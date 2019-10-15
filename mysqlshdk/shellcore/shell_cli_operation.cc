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

#include "mysqlshdk/shellcore/shell_cli_operation.h"
#include <stdexcept>
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

REGISTER_HELP_TOPIC(Command Line, TOPIC, cmdline, Contents, ALL);
REGISTER_HELP_TOPIC_TEXT(cmdline, R"*(
The MySQL Shell functionality is generally available as API calls, this is
objects containing methods to perform specific tasks, they can be called either
in JavaScript or Python modes.

The most important functionality (available on the shell global objects) has
been integrated in a way that it is possible to call it directly from the
command line, by following a specific syntax.

<b>SYNTAX</b>:

  mysqlsh [options] -- <object> <method> [arguments]

<b>WHERE</b>:

@li object - a string identifying shell object to be called.
@li method - a string identifying method to be called on the object.
@li arguments - arguments passed to the called method.

<b>DETAILS</b>:

The following objects can be called using this format:

@li dba - dba global object.
@li cluster - cluster returned by calling dba.getCluster().
@li shell - shell global object.
@li shell.options - shell.options object.
@li util - util global object

The method name can be given in the following naming styles:

@li Camel case: (e.g. createCluster, checkForServerUpgrade)
@li Kebab case: (e.g. create-cluster, check-for-server-upgrade)

The arguments syntax allows mixing positional and named arguments (similar to
args and *kwargs in Python):

<b>ARGUMENT SYNTAX</b>:

  [ positional_argument ]* [ { named_argument* } ]* [ named_argument ]*

<b>WHERE</b>:

@li positional_argument - is a single string representing a value.
@li named_argument - a string in the form of --argument-name[=value]

A positional_argument is defined by the following rules:

@li positional_argument ::= value
@li value   ::= string | integer | float | boolean | null
@li string  ::= plain string, quoted if contains whitespace characters
@li integer ::= plain integer
@li float   ::= plain float
@li boolean ::= 1 | 0 | true | false
@li null    ::= "-"

Positional arguments are passed to the function call in the order they are
defined.

Named arguments represent a dictionary option that the method is expecting. It
is created by prepending the option name with double dash and can also be
specified either using camelCaseNaming or kebab-case-naming, examples:<br>

@code
--anOptionName[=value]
--an-option-name[=value]
@endcode

If the value is not provided the option name will be handled as a boolean
option with a default value of TRUE.

<b>Grouping Named Arguments</b>

It is possible to create a group of named arguments by enclosing them between
curly braces. This group would be handled as a positional argument at the
position where the opening curly brace was found, it will be passed to the
function as a dictionary containing all the named arguments defined on the
group.

Named arguments that are not placed inside curly braces (independently of the
position on the command line), are packed a single dictionary, passed as a last
parameter on the method call.

Following are some examples of command line calls as well as how they are mapped
to the API method call.
)*");

REGISTER_HELP(
    cmdline_EXAMPLE,
    "$ mysqlsh -- dba deploy-sandbox-instance 1234 --password=secret");
REGISTER_HELP(cmdline_EXAMPLE_DESC,
              "mysql-js> dba.deploySandboxInstance(1234, {password: secret})");
REGISTER_HELP(cmdline_EXAMPLE1,
              "$ mysqlsh root@localhost:1234 -- dba create-cluster mycluster");
REGISTER_HELP(cmdline_EXAMPLE1_DESC,
              "mysql-js> dba.createCluster(\"mycluster\")");
REGISTER_HELP(cmdline_EXAMPLE2,
              "$ mysqlsh root@localhost:1234 -- cluster status");
REGISTER_HELP(cmdline_EXAMPLE2_DESC, "mysql-js> cluster.status()");
REGISTER_HELP(cmdline_EXAMPLE3,
              "$ mysqlsh -- shell.options set-persist history.autoSave true");
REGISTER_HELP(cmdline_EXAMPLE3_DESC,
              "mysql-js> shell.options.setPersist(\"history.autoSave\", true)");
REGISTER_HELP(cmdline_EXAMPLE4,
              "$ mysqlsh -- util checkForServerUpgrade root@localhost "
              "--outputFormat=JSON");
REGISTER_HELP(
    cmdline_EXAMPLE4_DESC,
    "mysql-js> util.checkForServerUpgrade(\"root@localhost\",{outputFormat: "
    "\"JSON\"})");
REGISTER_HELP(cmdline_EXAMPLE5,
              "$ mysqlsh -- util check-for-server-upgrade { --user=root "
              "--host=localhost } --password=");
REGISTER_HELP(
    cmdline_EXAMPLE5_DESC,
    "mysql-js> util.checkForServerUpgrade({user:\"root\", host:\"localhost\"}, "
    "{password:\"\"})");

void Shell_cli_operation::register_provider(
    const std::string &name, Shell_cli_operation::Provider provider) {
  if (m_providers.find(name) != m_providers.end())
    throw std::invalid_argument(
        "Shell operation provider already registered under "
        "name " +
        name);

  m_providers.emplace(name, provider);
}

void Shell_cli_operation::remove_provider(const std::string &name) {
  auto it = m_providers.find(name);
  if (it != m_providers.end()) m_providers.erase(it);
}

void Shell_cli_operation::set_object_name(const std::string &name) {
  m_object_name = name;
}

void Shell_cli_operation::set_method_name(const std::string &name) {
  m_method_name = name;
  m_method_name_original = name;
}

void Shell_cli_operation::add_argument(const shcore::Value &argument) {
  m_argument_list.push_back(argument);
}

void Shell_cli_operation::add_option(Value::Map_type_ref target_map,
                                     const std::string &option_definition) {
  std::string::size_type offset = option_definition.find('=');
  // named argument without value means boolean with true value
  if (offset == std::string::npos) {
    target_map->emplace(to_camel_case(option_definition.substr(2)),
                        Value::True());
  } else {
    std::string arg_name =
        to_camel_case(option_definition.substr(2, offset - 2));
    std::string val = option_definition.substr(offset + 1);
    target_map->emplace(arg_name, val == "-" ? Value::Null() : Value(val));
  }
}

void Shell_cli_operation::add_option(const std::string &option_definition) {
  if (!m_argument_map) m_argument_map = std::make_shared<Value::Map_type>();

  add_option(m_argument_map, option_definition);
}

void Shell_cli_operation::parse(Options::Cmdline_iterator *cmdline_iterator) {
  if (m_object_name.empty() && m_method_name_original.empty() &&
      !cmdline_iterator->valid()) {
    throw Mapping_error("Empty operations are not allowed");
  }

  // Object name is only read if not already provided
  if (m_object_name.empty()) m_object_name = cmdline_iterator->get();

  // Object name is only read if not already provided
  if (m_method_name_original.empty()) {
    if (!cmdline_iterator->valid())
      throw Mapping_error("No operation specified for object " + m_object_name);

    m_method_name_original = cmdline_iterator->get();
    m_method_name = to_camel_case(m_method_name_original);
  }

  Value::Map_type_ref local_map;

  //  argument_list;
  while (cmdline_iterator->valid()) {
    std::string arg = cmdline_iterator->get();

    // "{" is the correct syntax, we are matching "{--" for user
    // convenience as it will be a common mistake
    if (arg == "{" || str_beginswith(arg, "{--")) {
      if (local_map)
        throw Mapping_error("Nested dictionaries are not supported");
      local_map = std::make_shared<Value::Map_type>();
      if (arg.length() == 1)
        continue;
      else
        arg = arg.substr(1);
    }

    if (arg == "}") {
      if (!local_map) throw Mapping_error("Unmatched character '}' found");
      m_argument_list.push_back(Value(fix_connection_options(local_map)));
      local_map.reset();
    } else if (str_beginswith(arg, "--")) {
      if (!local_map && !m_argument_map)
        m_argument_map = std::make_shared<Value::Map_type>();
      Value::Map_type_ref target_map = local_map ? local_map : m_argument_map;

      add_option(target_map, arg);
    } else {
      if (local_map)
        throw Mapping_error(
            "Positional arguments are not allowed inside local "
            "dictionary");
      m_argument_list.push_back(arg == "-" ? Value::Null() : Value(arg));
    }
  }
  if (local_map) throw Mapping_error("Unmatched '{' character found");
  if (m_argument_map)
    m_argument_list.push_back(Value(fix_connection_options(m_argument_map)));
}

Value Shell_cli_operation::execute() {
  auto provider = m_providers.find(m_object_name);
  if (provider == m_providers.end())
    throw Mapping_error("There is no global object registered under name " +
                        m_object_name);

  std::shared_ptr<Object_bridge> object = provider->second();
  if (!object)
    throw Mapping_error("Unable to retrieve object instance for name " +
                        m_object_name);

  if (!object->has_method(m_method_name))
    throw Mapping_error("Invalid method for " + m_object_name +
                        " object: " + m_method_name);

  return object->call(m_method_name, m_argument_list);
}

Value::Map_type_ref Shell_cli_operation::fix_connection_options(
    Value::Map_type_ref dict) {
  shcore::Value::Map_type_ref ret = std::make_shared<shcore::Value::Map_type>();

  for (const auto &i : *dict) {
    std::string new_name = shcore::str_beginswith(i.first, "db")
                               ? shcore::to_camel_case(i.first)
                               : shcore::from_camel_case_to_dashes(i.first);
    if (mysqlshdk::db::connection_attributes.find(new_name) ==
        mysqlshdk::db::connection_attributes.end())
      return dict;
    auto r = ret->emplace(new_name, i.second);
    if (!r.second)
      throw std::invalid_argument("Connection option '" + i.first +
                                  "' is already defined as '" +
                                  r.first->second.as_string() + "'.");
  }

  return ret;
}

} /* namespace shcore */
