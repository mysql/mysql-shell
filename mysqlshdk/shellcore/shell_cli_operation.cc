/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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
REGISTER_HELP(cmdline,
              "Following syntax can be used to execute methods of the Shell "
              "global objects from command line:");
REGISTER_HELP(cmdline1, "  mysqlsh [options] -- <object> <method> [arguments]");
REGISTER_HELP(
    cmdline2,
    "@li object - a string identifying shell object exposed to command line:");
REGISTER_HELP(cmdline3,
              "<code>"
              "  * dba - dba global object\n"
              "  * cluster - cluster returned by calling dba.getCluster()\n"
              "  * shell - shell global object\n"
              "  * shell.options - shell.options object\n"
              "  * util - util global object\n</code>");
REGISTER_HELP(
    cmdline4,
    "@li method - a string identifying method of the object in either:");
REGISTER_HELP(
    cmdline5,
    "<code>"
    "  * Camel case form (e.g. createCluster, checkForServerUpgrade)\n"
    "  * Kebab case form (all lower case, words separated by hyphens)\n"
    "    (e.g. create-cluster, check-for-server-upgrade)\n</code>");
REGISTER_HELP(
    cmdline6,
    "@li arguments - arguments passed to the method in format described "
    "below.");
REGISTER_HELP(cmdline7,
              "Arguments syntax allows mixing positional and named arguments "
              "(similar to args and *kwargs in Python):");
REGISTER_HELP(
    cmdline8,
    "  [ positional_argument ]* [ { named_argument* } ]* [ named_argument "
    "]*");
REGISTER_HELP(
    cmdline9,
    "@li positional_argument - is a single string representing a value:");
REGISTER_HELP(
    cmdline10,
    "<code>"
    "  * positional_argument ::= value\n"
    "  * value   ::= string | integer | float | boolean | null\n"
    "  * string  ::= plain string, quoted if contains whitespace characters\n"
    "  * integer ::= plain integer\n"
    "  * float   ::= plain float\n"
    "  * boolean ::= 1 | 0 | true | false\n"
    "  * null    ::= \"-\"\n</code>");
REGISTER_HELP(
    cmdline11,
    "@li named_argument - a string in '--argument-name[=value]' format where:");
REGISTER_HELP(
    cmdline12,
    "<code>"
    "  * argument-name - is a dictionary keyword that method is expecting "
    "either\n"
    "    in the JS camel case or the command line, lowercase + hyphens, "
    "format\n"
    "    (e.g. --outputFormat or --output-format)\n"
    "  * \"--argument-name\" is interpreted as \"--argument-name=true\"\n"
    "  * value is analogical to positional argument.\n</code>");
REGISTER_HELP(
    cmdline13,
    "Such command line arguments are transformed into method parameters:");
REGISTER_HELP(
    cmdline14,
    "@li positional arguments are passed to method in the exact order they "
    "appear on the command line");
REGISTER_HELP(
    cmdline15,
    "@li named arguments grouped by curly braces are packed together into"
    " a dictionary that is passed to the method at the exact place it appears "
    "on the command line");
REGISTER_HELP(cmdline16,
              "@li named arguments that are not placed inside curly braces, "
              "independent of where they appear on command line, are packed "
              "into a single dictionary, passed as a last parameter to the "
              "method.");
REGISTER_HELP(cmdline_EXAMPLE,
              "Command line form followed by the equivalent JavaScript call:");
REGISTER_HELP(
    cmdline_EXAMPLE1,
    "<code>"
    "$ mysqlsh -- dba deploy-sandbox-instance 1234 --password=secret\n"
    "  mysql-js> dba.deploySandboxInstance(1234, {password: secret})</code>");
REGISTER_HELP(cmdline_EXAMPLE2,
              "<code>"
              "$ mysqlsh root@localhost:1234 -- dba create-cluster mycluster\n"
              "  mysql-js> dba.createCluster(\"mycluster\")</code>");
REGISTER_HELP(cmdline_EXAMPLE3,
              "<code>"
              "$ mysqlsh root@localhost:1234 -- cluster status\n"
              "  mysql-js> cluster.status()</code>");
REGISTER_HELP(
    cmdline_EXAMPLE4,
    "<code>"
    "$ mysqlsh -- shell.options set-persist history.autoSave true\n"
    "  mysql-js> shell.options.setPersist(\"history.autoSave\", true);</code>");
REGISTER_HELP(cmdline_EXAMPLE5,
              "<code>"
              "$ mysqlsh -- util checkForServerUpgrade root@localhost "
              "--outputFormat=JSON\n"
              "  mysql-js> util.checkForServerUpgrade(\"root@localhost\",\n"
              "                   {outputFormat: \"JSON\"});</code>");
REGISTER_HELP(
    cmdline_EXAMPLE6,
    "<code>"
    "$ mysqlsh -- util check-for-server-upgrade\n"
    "                   { --user=root --host=localhost } --password=\n"
    "  mysql-js> util.checkForServerUpgrade(\n"
    "                   {user:\"root\", host:\"localhost\"}, "
    "{password:\"\"})</code>");

void Shell_cli_operation::register_provider(
    const std::string &name, Shell_cli_operation::Provider provider) {
  if (m_providers.find(name) != m_providers.end())
    throw std::invalid_argument(
        "Shell operation provider already registered under name " + name);

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

    // "{" is the correct syntax, we are matching "{--" for user convenience as
    // it will be a common mistake
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
      m_argument_list.push_back(Value(local_map));
      local_map.reset();
    } else if (str_beginswith(arg, "--")) {
      if (!local_map && !m_argument_map)
        m_argument_map = std::make_shared<Value::Map_type>();
      Value::Map_type_ref target_map = local_map ? local_map : m_argument_map;

      add_option(target_map, arg);
    } else {
      if (local_map)
        throw Mapping_error(
            "Positional arguments are not allowed inside local dictionary");
      m_argument_list.push_back(arg == "-" ? Value::Null() : Value(arg));
    }
  }
  if (local_map) throw Mapping_error("Unmatched '{' character found");
  if (m_argument_map) m_argument_list.push_back(Value(m_argument_map));
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

} /* namespace shcore */
