/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "mysqlshdk/shellcore/shell_cli_operation.h"

#include <regex>
#include <stdexcept>
#include "modules/mod_extensible_object.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace cli {

const std::regex k_api_general_error(
    "^([a-z|A-Z|_][a-z|A-Z|0-9|_]*)\\.([a-z|A-Z|_][a-z|A-Z|0-9|_]*):\\s("
    "Argument #(\\d+):\\s)?(.+)$$");

REGISTER_HELP_TOPIC(Command Line, TOPIC, cmdline, Contents, ALL);
REGISTER_HELP_TOPIC_TEXT(cmdline, R"*(
The MySQL Shell functionality is generally available as API calls, this is
objects containing methods to perform specific tasks, they can be called either
in JavaScript or Python modes.

The most important functionality (available on the shell global objects) has
been integrated in a way that it is possible to call it directly from the
command line, by following a specific syntax.

<b>SYNTAX</b>:

  mysqlsh [options] -- <object> <operation> [arguments]

<b>WHERE</b>:

@li object - a string identifying shell object to be called.
@li operation - a string identifying method to be called on the object.
@li arguments - arguments passed to the called method.

<b>DETAILS</b>:

The following objects can be called using this format:

@li dba - dba global object.
@li cluster - cluster returned by calling dba.getCluster().
@li shell - shell global object.
@li shell.options - shell.options object.
@li util - util global object

The operation name can be given in the following naming styles:

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
              "$ mysqlsh -- util check-for-server-upgrade --user=root "
              "--host=localhost --password=");
REGISTER_HELP(cmdline_EXAMPLE5_DESC,
              "mysql-js> util.checkForServerUpgrade({user:\"root\", "
              "host:\"localhost\"}, "
              "{password:\"\"})");

void Shell_cli_operation::add_cmdline_argument(const std::string &cmdline_arg) {
  m_cli_mapper.add_cmdline_argument(cmdline_arg);
}

void Shell_cli_operation::set_object_name(const std::string &name) {
  m_object_name = name;
  m_cli_mapper.set_object_name(name);
}

void Shell_cli_operation::set_method_name(const std::string &name) {
  m_method_name_original = name;
  m_cli_mapper.set_operation_name(name);
}

/**
 * Parses the command line to identify the operation to be executed as well as
 * to aggregate the received arguments into a list for further processing.
 *
 * For backward compatibility it ignores the presense of { and } as they are
 * no longer needed to group options.
 *
 * GRAMMAR For a Command Line Call:
 * ================================
 * CMDLINE_CALL ::= '-' '-' 'space' (HELP | (OBJECT_NAME (OBJECT_NAME)*
 *            (HELP | COMMAND (HELP | (ARGUMENT)*))))
 * HELP ::= '-' '-' 'help' | '-' 'h'
 * ARGUMENT ::= VALUE | '-' '-' NAME (('=' NAME )? (':' TYPE)? '=' VALUE)?
 * VALUE ::= JSON_NO_WHITESPACES | UNQUOTED_STRING | RAW_LIST
 * RAW_LIST ::= VALUE (',' VALUE)*
 * TYPE ::= 'int'|'uint'|'float'|'json'|'bool'|'null'|'list'|'dict'
 * ================================
 * OBJECT_NAME: Defined by the global objects and nested objects exposing
 *              operations for CLI.
 * COMMAND: Defined by the functions exposed for CLI in every single object
 * JSON_NO_WHITESPACES: Refers to any sequence valid JSON specification
 *                      sequence except that being command line calls,
 *                      whitespace TOKENS would be taken by the terminal as
 *                      beggining of new argument.
 * UNQUOTED_STRING: ANY value not matching JSON_NO_WITESPACES will be processed
 *                      as a string.
 */
void Shell_cli_operation::parse(Options::Cmdline_iterator *cmdline_iterator) {
  //  argument_list;
  while (cmdline_iterator->valid() && !help_requested()) {
    std::string arg = cmdline_iterator->get();

    // "{" is the correct syntax, we are matching "{--" for user
    // convenience as it will be a common mistake
    if (arg == "{" || str_beginswith(arg, "{--")) {
      if (arg.length() == 1)
        continue;
      else
        arg = arg.substr(1);
    }

    if (arg != "}") {
      add_cmdline_argument(arg);
    }
  }
}

/**
 * This function is called before the actual execution takes place.
 *
 * Its purpose is to:
 *
 * - Ensure the target object.function is valid for CLI integration.
 * - Associate each named argument to it's corresponding parameter.
 * - Process positional arguments and create the final argument list to be
 * used on the function call.
 */
void Shell_cli_operation::prepare() {
  m_current_provider = m_cli_mapper.identify_operation(&m_provider);
  m_object_name = m_cli_mapper.object_name();
  m_method_name_original = m_cli_mapper.operation_name();
  m_method_name = shcore::to_camel_case(m_method_name_original);

  if (!m_object_name.empty()) {
    auto object = m_current_provider->get_object(help_requested());

    if (!object)
      throw std::invalid_argument(
          "Unable to retrieve object instance for name " + m_object_name);

    if (!m_method_name.empty()) {
      auto md = object->get_function_metadata(m_method_name, true);
      if (md) {
        m_cli_mapper.set_metadata(object->class_name(), *md);
      } else {
        throw std::invalid_argument("Invalid operation for " + m_object_name +
                                    " object: " + m_method_name_original);
      }
    }
  }

  if (!help_requested()) m_cli_mapper.process_arguments(&m_argument_list);
}

void Shell_cli_operation::print_help() {
  auto help_type = m_cli_mapper.help_type();

  Help_manager help;
  help.set_mode(shcore::IShell_core::Mode::JavaScript);
  auto get_help_topic = [&help](
                            const std::shared_ptr<Cpp_object_bridge> &object,
                            const std::string &function = "",
                            const std::string &qualified_parent = "") {
    auto extension_object =
        std::dynamic_pointer_cast<mysqlsh::Extensible_object>(object);

    std::string search_item;
    if (extension_object) {
      search_item = extension_object->get_qualified_name();
    } else if (qualified_parent.empty()) {
      search_item = object->class_name();
    } else {
      search_item = qualified_parent + "." + object->class_name();
    }

    if (!function.empty()) search_item += "." + function;

    auto topics = help.search_topics(search_item);

    return topics.empty() ? nullptr : topics[0];
  };

  std::string help_text;
  auto object_chain = m_cli_mapper.get_object_chain();
  if (help_type == Help_type::GLOBAL || help_type == Help_type::OBJECT) {
    std::string target_object;
    if (!object_chain.empty()) {
      target_object += " at '";
      target_object += shcore::str_join(object_chain, " ");
      target_object += "'";
    }

    std::vector<shcore::Help_topic *> cli_members;
    size_t provider_count = m_current_provider->get_providers().size();
    if (provider_count > 0) {
      help_text = shcore::str_format(
          "The following object%s provide%s command line operations%s:\n\n",
          provider_count == 1 ? "" : "s", provider_count == 1 ? "s" : "",
          target_object.empty() ? "" : target_object.c_str());
    }

    std::map<Help_topic *, std::string> object_names;

    // Objects members require the fully qualified object chain
    std::string qualified_parent = shcore::str_join(object_chain, ".");
    for (const auto &provider : m_current_provider->get_providers()) {
      auto object = provider.second->get_object(true);

      auto topic = get_help_topic(object, "", qualified_parent);

      if (topic) {
        cli_members.push_back(topic);
        object_names[topic] = provider.first;
      }
    }

    help_text.append(help.format_member_list(
        cli_members, 0UL, false, [object_names](shcore::Help_topic *topic) {
          return "   " + object_names.at(topic);
        }));

    std::shared_ptr<Cpp_object_bridge> object =
        m_current_provider->get_object(true);

    if (object && help_type == Help_type::OBJECT) {
      auto members = object->get_cli_members();
      cli_members.clear();
      for (const auto &method : members) {
        auto md = object->get_function_metadata(method, true);
        if (md) {
          auto topic = get_help_topic(object, md->name[0]);
          if (topic) {
            cli_members.push_back(topic);
          }
        }
      }

      if (!cli_members.empty()) {
        if (!help_text.empty()) help_text += "\n\n";
        help_text +=
            "The following operations are available" + target_object + ":\n\n";
        help_text.append(help.format_member_list(
            cli_members, 0UL, false, [](shcore::Help_topic *topic) {
              return "   " + shcore::from_camel_case_to_dashes(topic->get_name(
                                 shcore::IShell_core::Mode::JavaScript));
            }));
      }
    }
  } else {
    std::shared_ptr<Cpp_object_bridge> object =
        m_current_provider->get_object(true);

    auto md = object->get_function_metadata(m_method_name, true);
    auto topic = get_help_topic(object, md->name[0]);

    if (topic) {
      help_text = help.get_help(*topic, Help_options::any(), &m_cli_mapper);
    }
  }

  if (!help_text.empty()) {
    mysqlsh::current_console()->println(help_text);
  }
}

Value Shell_cli_operation::execute() {
  try {
    prepare();
    if (help_requested()) {
      print_help();
      return shcore::Value();
    } else {
      std::shared_ptr<Cpp_object_bridge> object =
          m_current_provider->get_object(false);

      // Useful to see what the shell actually receives from the terminal
      if (current_logger()->get_log_level() >= shcore::Logger::LOG_DEBUG) {
        std::vector<std::string> api_call;
        api_call.push_back("CLI-API Mapping:");
        api_call.push_back("\tObject: " + m_object_name);
        api_call.push_back("\tMethod: " + m_method_name);
        api_call.push_back("\tArguments:");
        for (const auto &arg : m_argument_list) {
          api_call.push_back("\t\t: " + arg.json(false));
        }
        log_debug("%s", shcore::str_join(api_call, "\n").c_str());
      }

      return object->call(m_method_name, m_argument_list);
    }
  } catch (const std::exception &ex) {
    std::smatch results;
    std::string error(ex.what());
    if (std::regex_match(error, results, k_api_general_error)) {
      if (results[3].matched) {
        size_t arg_num = std::atoi(results[4].str().c_str());
        std::string new_error;
        new_error.append("Argument ");
        new_error.append(m_cli_mapper.metadata()->signature[arg_num - 1]->name +
                         ": ");
        new_error.append(results[5]);
        throw shcore::Exception::argument_error(new_error);
      } else {
        throw shcore::Exception::runtime_error(results[5]);
      }
    } else {
      throw;
    }
  }
}
}  // namespace cli
}  // namespace shcore
