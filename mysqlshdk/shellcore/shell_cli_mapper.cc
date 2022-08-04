/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/shellcore/shell_cli_mapper.h"

#include <algorithm>
#include <regex>
#include <stdexcept>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace cli {
namespace {
shcore::Value interpret_string_value(const std::string &input,
                                     bool only_json = false) {
  try {
    auto ret_val = shcore::Value::parse(input);

    // If the values was properly parsed as a string we let the value intact
    if (ret_val.type == shcore::Value_type::String)
      ret_val = shcore::Value(input);

    return ret_val;
  } catch (...) {
    if (only_json) {
      throw shcore::Exception::runtime_error("Invalid JSON value: " + input);
    }
  }
  return shcore::Value(input);
}

shcore::Value get_value_of_user_type(const std::string &definition,
                                     const std::string &value,
                                     shcore::Value_type type) {
  shcore::Value temp;
  // When parsing values form command line, values that fail interpretation
  // are interpreted as strings, they get quoted on this case
  temp = interpret_string_value(value, type == shcore::Value_type::Undefined);

  // If a type was specified by the user...
  try {
    switch (type) {
      case shcore::Value_type::Bool:
        return shcore::Value(temp.as_bool());
      case shcore::Value_type::Float:
        return shcore::Value(temp.as_double());
      case shcore::Value_type::Null:
        if (temp.type != shcore::Value_type::Null) {
          throw shcore::Exception::type_error(
              "Unexpected value defined for NULL");
        }
        break;
      case shcore::Value_type::UInteger:
        return shcore::Value(temp.as_uint());
      case shcore::Value_type::Integer:
        return shcore::Value(temp.as_int());
      case shcore::Value_type::Array:
        return shcore::Value(temp.as_array());
      case shcore::Value_type::Map:
        return shcore::Value(temp.as_map());
      case shcore::Value_type::String:
        return shcore::Value(value);
      case shcore::Value_type::Undefined:
        // The JSON format
        return temp;
      default:
        throw std::logic_error("This was not meant to happen!");
    }
  } catch (const shcore::Exception &ex) {
    if (ex.is_type()) {
      std::string requested = shcore::type_name(type);

      throw std::invalid_argument(
          "Argument error at '" + definition + "': " + requested +
          " indicated, but value is " + shcore::type_name(temp.type));

    } else {
      throw;
    }
  }

  // By default returns the parsed value...
  return temp;
}

std::string unescape_string(const std::string &input) {
  std::vector<std::pair<std::string, std::string>> escaped_sequences = {
      {"\\,", ","}, {"\\\"", "\""}, {"\\'", "'"}, {"\\=", "="}};

  auto output = input;
  for (const auto &sequence : escaped_sequences) {
    output = shcore::str_replace(output, std::get<0>(sequence),
                                 std::get<1>(sequence));
  }

  return output;
}

void validate_list_items(const Cmd_line_arg_definition &arg,
                         shcore::Value_type expected_type) {
  assert(arg.value.type == shcore::Value_type::Array);
  if (expected_type != shcore::Value_type::Undefined) {
    for (const auto &item : *arg.value.as_array()) {
      if (item.type != expected_type) {
        throw std::invalid_argument("Argument error at '" + arg.definition +
                                    "': " + shcore::type_name(expected_type) +
                                    " expected, but value is " +
                                    shcore::type_name(item.type));
      }
    }
  }
}

shcore::Value get_value_of_expected_type(const Cmd_line_arg_definition &arg,
                                         shcore::Value_type type) {
  // If the user defined a value type and the expected type is defined, we must
  // ensure they match
  if (type != shcore::Undefined && !arg.user_type.is_null() &&
      *arg.user_type != type) {
    throw std::invalid_argument("Argument error at '" + arg.definition +
                                "': " + shcore::type_name(type) +
                                " expected, but value is " +
                                shcore::type_name(*arg.user_type));
  }

  try {
    switch (type) {
      case shcore::Value_type::Bool:
        return shcore::Value(arg.value.as_bool());
      case shcore::Value_type::Float:
        return shcore::Value(arg.value.as_double());
      case shcore::Value_type::Integer:
        return shcore::Value(arg.value.as_int());
      case shcore::Value_type::String:
        return shcore::Value(unescape_string(arg.value.as_string()));
      case shcore::Value_type::UInteger:
        return shcore::Value(arg.value.as_uint());
      case shcore::Value_type::Undefined:
        return arg.value;
      default:
        throw std::logic_error(
            "get_value_of_expected_type: this was not meant to happen!");
    }
  } catch (const shcore::Exception &ex) {
    if (ex.is_type()) {
      throw std::invalid_argument("Argument error at '" + arg.definition +
                                  "': " + shcore::type_name(type) +
                                  " expected, but value is " +
                                  shcore::type_name(arg.value.type));
    } else {
      throw;
    }
  }
  return shcore::Value();
}

const std::regex k_cmdline_arg_assignment("^((--)?(.+?))(:(.*?))?(=(.*))?$");
// Constants for match index
const int OPTION_MATCH_IDX = 1;
const int COLON_MATCH_IDX = 4;
const int USER_TYPE_MATCH_IDX = 5;
const int EQUAL_MATCH_IDX = 6;
const int VALUE_MATCH_IDX = 7;

Cmd_line_arg_definition parse_named_argument(const std::string &definition) {
  std::smatch results;
  if (std::regex_match(definition, results, k_cmdline_arg_assignment)) {
    if ((results[COLON_MATCH_IDX].matched &&
         !results[USER_TYPE_MATCH_IDX].matched) ||  // : without type
        (results[EQUAL_MATCH_IDX].matched &&
         !results[VALUE_MATCH_IDX].matched) ||  // = without value
        (results[COLON_MATCH_IDX].matched &&
         !results[EQUAL_MATCH_IDX].matched)) {  // : without =

      throw std::invalid_argument(
          "Invalid format for command line argument. Valid formats are:\n"
          "\t<arg>\n"
          "\t--<arg>[=<value>]\n"
          "\t--<arg>:<type>=<value>");
    }

    // If either type or value were specified the value can be determined (even
    // if it is null), otherwise we treat as if no match were found to allow
    // value be defined in coming command line arg
    // i.e. --attribute value
    if (results[USER_TYPE_MATCH_IDX].matched ||
        results[VALUE_MATCH_IDX].matched) {
      if (results[USER_TYPE_MATCH_IDX].matched) {
        shcore::Value_type user_type = shcore::Value_type::Undefined;
        if (results[USER_TYPE_MATCH_IDX] == "str") {
          user_type = shcore::String;
        } else if (results[USER_TYPE_MATCH_IDX] == "int") {
          user_type = shcore::Integer;
        } else if (results[USER_TYPE_MATCH_IDX] == "uint") {
          user_type = shcore::UInteger;
        } else if (results[USER_TYPE_MATCH_IDX] == "float") {
          user_type = shcore::Float;
        } else if (results[USER_TYPE_MATCH_IDX] == "json") {
          // We use undefined as the result will be shcore::Value::parse(...)
          // which means any value is mostly correct
        } else if (results[USER_TYPE_MATCH_IDX] == "bool") {
          user_type = shcore::Bool;
        } else if (results[USER_TYPE_MATCH_IDX] == "null") {
          user_type = shcore::Null;
        } else if (results[USER_TYPE_MATCH_IDX] == "list") {
          user_type = shcore::Array;
        } else if (results[USER_TYPE_MATCH_IDX] == "dict") {
          user_type = shcore::Map;
        } else {
          throw std::invalid_argument(
              "Unsupported data type specified in option: " + definition);
        }
        return {definition, results[OPTION_MATCH_IDX],
                get_value_of_user_type(definition, results[VALUE_MATCH_IDX],
                                       user_type),
                user_type};
      } else {
        return {definition,
                results[OPTION_MATCH_IDX],
                interpret_string_value(results[VALUE_MATCH_IDX]),
                {}};
      }
    }
  }
  throw std::logic_error("No Match Found");
}

/**
 * Adds a list of values into the specified array.
 *
 * This function is used to :
 * - Set the values of an array parameter.
 * - Add values to an array option. (This can happen several times).
 */
void add_value_to_list(shcore::Array_t *target,
                       const Cmd_line_arg_definition &arg,
                       shcore::Value_type expected_type) {
  if (arg.value.type != shcore::String || !arg.user_type.is_null()) {
    (*target)->push_back(get_value_of_expected_type(arg, expected_type));
  } else {
    auto string_value = arg.value.as_string();
    size_t start = 0;
    size_t end = 0;

    std::string value;
    bool done = string_value.empty();
    while (!done) {
      bool next_quoted = true;
      if (string_value[start] == '"') {
        end = mysqlshdk::utils::span_quoted_string_dq(string_value, start);
      } else if (string_value[start] == '\'') {
        end = mysqlshdk::utils::span_quoted_string_sq(string_value, start);
      } else {
        next_quoted = false;
        // Searches for the next comma that is not escaped
        size_t next_start = start;
        do {
          end = string_value.find(',', next_start);
          next_start = end + 1;
        } while (end != std::string::npos && end > 0 &&
                 string_value[end - 1] == '\\');
      }

      if (next_quoted) {
        if (end != std::string::npos) {
          (*target)->emplace_back(
              string_value.substr(start + 1, end - start - 2));

          // If the closing quote is the last character...
          done = end == string_value.size();
        } else {
          throw std::invalid_argument("Missing closing quote");
        }
      } else {
        shcore::Value item_value;
        if (end != std::string::npos) {
          item_value = interpret_string_value(
              unescape_string(string_value.substr(start, end - start)));
        } else {
          item_value = interpret_string_value(
              unescape_string(string_value.substr(start)));
          done = true;
        }
        auto item_arg = arg;
        item_arg.value = item_value;
        (*target)->push_back(
            get_value_of_expected_type(item_arg, expected_type));
      }

      if (!done) {
        if (string_value[end] != ',') {
          throw std::invalid_argument("Missing comma after list value");
        }

        // Starts on the next element
        start = end + 1;
      }
    }
  }
}

void add_value_to_dict(shcore::Dictionary_t *target_map,
                       shcore::Option_validator *validator,
                       const Cmd_line_arg_definition &arg) {
  shcore::Dictionary_t arg_map = *target_map;

  // If no options are defined, all the options are passed right away
  if (validator->allowed().empty()) {
    (*arg_map)[arg.option] = arg.value;
  } else {
    for (const auto &option_md : validator->allowed()) {
      if (option_md->name == arg.option) {
        // A value is being added to an option on this dictionary, however it
        // will be skipped in the following cases:
        // - If target option is yet another dictionary: in such case the
        // validation will be done when the option is added into the target
        // dictionary.
        // - If the target option is a list: in such case the validation will be
        // done when added into the target list.
        // - If the target option has type Undefined, which means it is not a
        // fixed type.
        if (option_md->type() == shcore::Value_type::Map) {
          const auto &inner_option =
              parse_named_argument(arg.value.as_string());

          // Ensures the target dict option is available
          if (!arg_map->has_key(arg.option)) {
            (*arg_map)[arg.option] = shcore::Value(shcore::make_dict());
          }

          // Gets the target dictionary and sets the key inside
          auto inner_map = arg_map->get_map(arg.option);

          add_value_to_dict(&inner_map,
                            option_md->validator<shcore::Option_validator>(),
                            inner_option);
        } else if (option_md->type() == shcore::Value_type::Array) {
          if (!arg_map->has_key(arg.option)) {
            (*arg_map)[arg.option] = shcore::Value(shcore::make_array());
          }

          auto array = arg_map->get_array(arg.option);

          auto list_validator = option_md->validator<List_validator>();

          if (arg.value.type == shcore::Value_type::Array) {
            validate_list_items(arg, list_validator->element_type());
            auto source = arg.value.as_array();
            std::move(source->begin(), source->end(),
                      std::back_inserter(*array));
          } else {
            add_value_to_list(&array, arg, list_validator->element_type());
          }

        } else {
          (*arg_map)[arg.option] =
              get_value_of_expected_type(arg, option_md->type());
        }
      }
    }
  }
}
}  // namespace

std::string Shell_cli_mapper::object_name() const {
  if (!m_object_chain.empty()) return m_object_chain.back();
  return "";
}

bool Shell_cli_mapper::empty() const {
  return m_object_chain.empty() && m_cmdline_args.empty();
}
/**
 * Processes command line arguments which could come in the format of:
 * - -     : Indicates a positional parameter with value NULL
 * - VALUE : Indicates a positional parameter with the given value
 * - --NAME=VALUE : Indicates a named option with the given value
 * - --NAME=-     : Indicates a named option with NULL value
 * - --NAME       : Indicates a named boolean option to be set to TRUE
 *
 * For each argument, an str,str pair will be added into m_cmdline_arg as
 * follows:
 * - First str is the arg name, or empty for positional ones.
 * - Second str is the value, which could be NULL or TRUE
 *
 * These args will be processed later in the mapping process.
 */
void Shell_cli_mapper::add_cmdline_argument(const std::string &cmdline_arg) {
  // If previous named arg did not get a value it could be one of:
  // - Boolean arg set to TRUE (default behavior already handled)
  // - Named arg expecting a value, which is this new cmdline_arg unless it is
  //   a new argument (starts with -)
  bool handled_as_value = false;
  if (m_waiting_value) {
    if (cmdline_arg.at(0) != '-' || cmdline_arg.size() == 1) {
      m_cmdline_args.back().definition.append(" " + cmdline_arg);
      m_cmdline_args.back().value = interpret_string_value(cmdline_arg);
      m_cmdline_args.back().user_type.reset();
      handled_as_value = true;
    }

    m_waiting_value = false;
  }

  if (!handled_as_value) {
    if (shcore::str_beginswith(cmdline_arg, "--")) {
      try {
        m_cmdline_args.push_back(parse_named_argument(cmdline_arg));
      } catch (const std::invalid_argument &ex) {
        throw std::invalid_argument("Error at '" + cmdline_arg + "'.\n" +
                                    ex.what());
      } catch (const std::logic_error &) {
        m_waiting_value = true;
        m_cmdline_args.push_back({cmdline_arg, cmdline_arg,
                                  shcore::Value::True(),
                                  shcore::Value_type::Bool});
      }
    } else if (cmdline_arg[0] == '-' && cmdline_arg.size() > 1) {
      // Short named option
      if (cmdline_arg.size() > 2) {
        shcore::Value temp = interpret_string_value(cmdline_arg.substr(2));
        m_cmdline_args.push_back(
            {cmdline_arg, cmdline_arg.substr(0, 2), temp, {}});
      } else {
        // Boolean
        m_cmdline_args.push_back(
            {cmdline_arg, cmdline_arg, shcore::Value::True(), {}});
      }
    } else {
      // Positional argument
      m_cmdline_args.push_back(
          {cmdline_arg, "", interpret_string_value(cmdline_arg), {}});
    }
  }
}

Provider *Shell_cli_mapper::identify_operation(Provider *base_provider) {
  Provider *current_provider = base_provider;

  if (empty()) {
    throw std::invalid_argument("Empty operations are not allowed");
  }

  // Useful to see what the shell actually receives from the terminal
  if (current_logger()->get_log_level() >= shcore::Logger::LOG_DEBUG) {
    std::vector<std::string> cli_args;
    cli_args.push_back("CLI Arguments:");
    for (const auto &arg : m_cmdline_args) {
      cli_args.push_back("\t" + arg.definition);
    }
    log_debug("%s", shcore::str_join(cli_args, "\n").c_str());
  }

  /**
   * Helper function to identify whether HELP has been requested, either
   * because was already requested or because the next argument is a help
   * argument.
   *
   * @param type The type of help that would be requested if the next arg is a
   * help one.
   */
  auto is_help_required = [this](Help_type type) {
    if (help_requested()) return true;
    if (m_cmdline_args.empty()) return false;
    const auto &next_arg = m_cmdline_args.front().definition;
    if (next_arg == "--help" || next_arg == "-h") {
      m_help_type = type;
      m_cmdline_args.erase(m_cmdline_args.begin());
      return true;
    }
    return false;
  };

  if (!m_object_chain.empty()) {
    // The target object name is pre-defined when using the --import function,
    // on this case the target provider is just taken from the base providers.
    current_provider = base_provider->get_provider(m_object_chain.back()).get();
  } else if (!is_help_required(Help_type::GLOBAL)) {
    // This block will identify the target object provider to be used on the
    // operation, it will also analyze nested objects.
    const auto advance_provider = [this](Provider *p) {
      assert(p);
      auto next_arg = m_cmdline_args.front().definition;
      auto next = p->get_provider(next_arg).get();
      if (next) {
        m_cmdline_args.erase(m_cmdline_args.begin());
        m_object_chain.push_back(std::move(next_arg));
      }
      return next;
    };

    // Identifies the top level object
    current_provider = advance_provider(base_provider);
    if (!current_provider) {
      throw std::invalid_argument("There is no object registered under name '" +
                                  m_cmdline_args.front().definition + "'");
    }

    // Verifies if following args identify nested objects
    auto provider = current_provider;
    while (!m_cmdline_args.empty() && (provider = advance_provider(provider))) {
      current_provider = provider;
    }
  }

  // Handles the first argument after the object chain to determine if it is a
  // help request or a function.
  if (m_operation_name.empty() && !is_help_required(Help_type::OBJECT)) {
    if (m_cmdline_args.empty())
      throw std::invalid_argument("No operation specified for object '" +
                                  m_object_chain.back() + "'");

    // Handles the case where help is requested for the object
    m_operation_name = m_cmdline_args.front().definition;
    m_cmdline_args.erase(m_cmdline_args.begin());
  }

  // Identifies if it is a help request for a function and if so, if
  // additional arguments (which is an error) were specified
  if (is_help_required(Help_type::FUNCTION) && !m_cmdline_args.empty()) {
    throw std::invalid_argument(
        "No additional arguments are allowed when requesting CLI help");
  }

  return current_provider;
}

void Shell_cli_mapper::add_argument(const shcore::Value &argument) {
  for (size_t index = 0; index < m_missing_optionals; index++) {
    m_argument_list->push_back(shcore::Value::Null());
  }
  m_argument_list->push_back(argument);
  m_missing_optionals = 0;
}

/**
 * Traverses the command line arguments to store the named options on the
 * parameter dictionary where they belong.
 *
 * Named options may belong to:
 * - A specific parameter (options defined in an Option_pack)
 * - A connection data parameter
 * - A dictionary parameter that accepts ANY option (parameter does not have
 * options defined in an Option_pack)
 *
 * If a named option is not identified as any of the three groups above, it
 * will be considered invalid and an error will be raised.
 */
void Shell_cli_mapper::process_named_args() {
  std::vector<std::string> invalid_options;

  for (size_t index = 0; index < m_cmdline_args.size(); index++) {
    const auto &arg = m_cmdline_args[index];
    if (!arg.option.empty()) {
      // This is required to support options as --optionName in addition to
      // --option-name
      auto opt_name = shcore::from_camel_case_to_dashes(arg.option);
      auto ambiguous = m_ambiguous_cli_options.find(opt_name);
      if (ambiguous == m_ambiguous_cli_options.end()) {
        // Verifies if the option belongs to a parameter
        auto item = cli_option(opt_name);
        if (item != nullptr) {
          set_named_option(item->param, item->option, arg);
          item->defined = true;
          // Now verifies if there's a parameter accepting ANY option
        } else if (!m_any_options_param.empty()) {
          if (opt_name.size() > 2) {
            // Full named option
            set_named_option(m_any_options_param,
                             shcore::to_camel_case(opt_name.substr(2)), arg);
          } else {
            // Short named option
            set_named_option(m_any_options_param, opt_name.substr(1), arg);
          }
        } else {
          // Otherwise it is an invalid option
          invalid_options.push_back(arg.option);
        }
      } else {
        std::vector<std::string> valid_options;
        for (const auto &param : ambiguous->second) {
          valid_options.push_back("--" +
                                  shcore::from_camel_case_to_dashes(param) +
                                  ambiguous->first.substr(1));
        }

        throw std::invalid_argument(shcore::str_format(
            "Option %s is ambiguous as it is valid for the following "
            "parameters: %s. Please use any of the following options to "
            "disambiguate: "
            "%s",
            arg.option.c_str(),
            shcore::str_join(ambiguous->second, ", ").c_str(),
            shcore::str_join(valid_options, ", ").c_str()));
      }
    }
  }

  std::vector<std::string> missing_options;
  for (const auto &cli_option : m_cli_option_registry) {
    if (cli_option.required && !cli_option.defined) {
      missing_options.push_back(cli_option.name);
    }
  }

  if (!missing_options.empty()) {
    throw std::runtime_error(
        shcore::str_format("The following required option%s missing: %s",
                           missing_options.size() == 1 ? " is" : "s are",
                           shcore::str_join(missing_options, ", ").c_str()));
  }

  if (!invalid_options.empty()) {
    throw std::invalid_argument(
        shcore::str_format("The following option%s invalid: %s",
                           invalid_options.size() == 1 ? " is" : "s are",
                           shcore::str_join(invalid_options, ", ").c_str()));
  }
}

void Shell_cli_mapper::process_positional_args() {
  // Holds the list in case a list_argument is found.
  for (size_t index = 0; index < m_metadata->param_codes.size(); index++) {
    const auto &param_name = m_metadata->signature[index]->name;
    switch (m_metadata->param_codes[index]) {
      case 'C': {  // ConnectionData is expected
        const auto &connection_data = get_argument_options(param_name).as_map();
        if (!m_cmdline_args.empty() && m_cmdline_args[0].option.empty()) {
          // Next argument is anonymous, so it is the URI, if named parameters
          // defined connection data then it is an error, functions accept
          // either URI or Connection Data Dictionary, not both.
          if (!connection_data->empty()) {
            std::vector<std::string> options;
            for (const auto &item : *connection_data) {
              options.push_back("--" +
                                shcore::from_camel_case_to_dashes(item.first));
            }
            throw std::invalid_argument(shcore::str_format(
                "Either a URI or connection data should be defined for "
                "argument %s, you have specified both, URI: '%s' connection "
                "data: %s",
                m_cmdline_args[0].value.as_string().c_str(), param_name.c_str(),
                shcore::str_join(options, ", ").c_str()));
          }

          add_argument(m_cmdline_args[0].value);

          m_cmdline_args.erase(m_cmdline_args.begin());
        } else {
          if (connection_data->empty()) {
            // If the parameter is empty, inserts a null if not optional,
            // otherwise counts it as parameters to be added in case a further
            // parameter has value
            if (m_metadata->signature[index]->flag ==
                shcore::Param_flag::Optional)
              m_missing_optionals++;
            else
              add_argument(shcore::Value::Null());
          } else {
            add_argument(shcore::Value(connection_data));
          }
        }
      } break;
      case 'D': {
        const auto &options = get_argument_options(param_name).as_map();

        if (options->empty()) {
          // If the parameter is empty, inserts a null if not optional,
          // otherwise counts it as parameters to be added in case a further
          // parameter has value
          if (m_metadata->signature[index]->flag ==
              shcore::Param_flag::Optional)
            m_missing_optionals++;
          else
            add_argument(shcore::Value::Null());
        } else {
          add_argument(shcore::Value(options));
        }
        break;
      }
      case 'A': {
        shcore::Array_t list_argument;
        if (has_argument_options(param_name)) {
          list_argument = get_argument_options(param_name).as_array();
        } else if (!m_cmdline_args.empty()) {
          list_argument = shcore::make_array();
          auto validator =
              m_metadata->signature[index]->validator<List_validator>();

          // All remaining anonymous arguments are added to the list parameter
          while (!m_cmdline_args.empty()) {
            // Only anonymous arguments are added to the list
            if (m_cmdline_args[0].option.empty()) {
              if (m_cmdline_args[0].value.type == shcore::Array) {
                validate_list_items(m_cmdline_args[0],
                                    validator->element_type());
                auto source = m_cmdline_args[0].value.as_array();
                std::move(source->begin(), source->end(),
                          std::back_inserter(*list_argument));
              } else {
                add_value_to_list(&list_argument, m_cmdline_args[0],
                                  validator->element_type());
              }
            }

            m_cmdline_args.erase(m_cmdline_args.begin());
          }
        }

        if ((!list_argument || list_argument->empty()) &&
            m_metadata->signature[index]->flag ==
                shcore::Param_flag::Optional) {
          m_missing_optionals++;
        } else {
          add_argument(shcore::Value(list_argument));
        }
      } break;
      default:
        shcore::Value value;
        if (has_argument_options(param_name)) {
          value = get_argument_options(param_name);
        } else if (!m_cmdline_args.empty() &&
                   m_cmdline_args[0].option.empty()) {
          value = get_value_of_expected_type(
              m_cmdline_args[0], m_metadata->signature[index]->type());
          m_cmdline_args.erase(m_cmdline_args.begin());
        }

        if (value.type == shcore::Undefined) {
          if (m_metadata->signature[index]->flag ==
              shcore::Param_flag::Optional) {
            m_missing_optionals++;
          }
        } else {
          add_argument(value);
        }
        break;
    }

    // Deletes consecutive named args
    while (!m_cmdline_args.empty() && !m_cmdline_args[0].option.empty()) {
      m_cmdline_args.erase(m_cmdline_args.begin());
    }
  }

  // Remaining args are invalid
  if (!m_cmdline_args.empty()) {
    std::vector<std::string> args;
    for (const auto &arg : m_cmdline_args) {
      if (arg.option.empty()) {
        args.push_back(arg.value.descr());
      }
    }
    throw std::invalid_argument(
        shcore::str_format("The following argument%s invalid: %s",
                           args.size() == 1 ? " is" : "s are",
                           shcore::str_join(args, ", ").c_str()));
  }
}

const shcore::Value &Shell_cli_mapper::get_argument_options(
    const std::string &name) {
  return m_argument_options.at(name);
}

bool Shell_cli_mapper::has_argument_options(const std::string &name) const {
  return m_argument_options.has_key(name);
}

std::map<std::string, std::vector<const Cli_option *>>
Shell_cli_mapper::get_param_options() {
  std::map<std::string, std::vector<const Cli_option *>> options;
  for (const auto &option : m_cli_option_registry) {
    options[option.param].push_back(&option);
  }
  return options;
}

void Shell_cli_mapper::clear() {
  m_operation_name.clear();
  m_object_chain.clear();
  m_argument_options.clear();
  m_cli_option_registry.clear();
  m_normalized_cli_options.clear();
  m_missing_optionals = 0;
  m_waiting_value = false;
  m_help_type = Help_type::NONE;
  m_cmdline_args.clear();
  m_ambiguous_cli_options.clear();
}

void Shell_cli_mapper::define_named_arg(const Cli_option &option) {
  // Adds to the registry
  m_cli_option_registry.push_back(option);

  m_normalized_cli_options[m_cli_option_registry.back().name] =
      m_cli_option_registry.size() - 1;

  if (!m_cli_option_registry.back().short_option.empty() &&
      cli_option(m_cli_option_registry.back().short_option) == nullptr) {
    m_normalized_cli_options[m_cli_option_registry.back().short_option] =
        m_cli_option_registry.size() - 1;
  }
}
void Shell_cli_mapper::define_named_arg(const std::string &param,
                                        const std::string &option,
                                        const std::string &short_option,
                                        bool required) {
  auto cmd_line_option =
      "--" + shcore::from_camel_case_to_dashes(option.empty() ? param : option);

  auto ambiguous = m_ambiguous_cli_options.find(cmd_line_option);
  // If the option is already ambiguous...(already registered for >1 param)
  if (ambiguous != m_ambiguous_cli_options.end()) {
    (*ambiguous).second.push_back(param);
    define_named_arg(
        {cmd_line_option + "-" + shcore::from_camel_case_to_dashes(option),
         short_option.empty() ? "" : "-" + short_option, param, option,
         required, false});
  } else {
    auto existing_option = cli_option(cmd_line_option);
    // The option is new....(not yet registered for any param)
    if (existing_option == nullptr) {
      define_named_arg({cmd_line_option,
                        short_option.empty() ? "" : "-" + short_option, param,
                        option, required, false});
    } else {
      // The option now becomes ambiguous...(already registered for other
      // param)
      m_ambiguous_cli_options[cmd_line_option] = {existing_option->param,
                                                  param};

      existing_option->name =
          "--" + shcore::from_camel_case_to_dashes(existing_option->param) +
          "-" + shcore::from_camel_case_to_dashes(existing_option->option);

      m_normalized_cli_options[existing_option->name] =
          m_normalized_cli_options.at(cmd_line_option);

      define_named_arg(
          {"--" + shcore::from_camel_case_to_dashes(param) + "-" +
               shcore::from_camel_case_to_dashes(existing_option->option),
           short_option.empty() ? "" : "-" + short_option, param, option,
           required, false});

      m_normalized_cli_options.erase(cmd_line_option);
    }
  }
}

/**
 * Creates the list of valid named options for the given API function by
 * looking at the metadata definition of the parameters and options.
 *
 * Named options come from 3 different sources:
 * - ConnectionData: in which case every connection attribute becomes a valid
 * named option.
 * - Option Dictionaries: which can define the list of valid options by using
 * an Option_pack, or can support ANY option if no Option_pack is used to
 * define the supported options.
 * - Any parameter defined after a list parameter also becomes a named
 * argument.
 *
 * This function creates a temporary dictionary for each of the parameter
 * types mentioned above, these will be used to hold the options that belong
 * to each parameter.
 */
void Shell_cli_mapper::define_named_args() {
  bool found_list = false;
  for (size_t index = 0; index < m_metadata->param_codes.size(); index++) {
    const auto &param_name = m_metadata->signature[index]->name;
    m_param_index[param_name] = index;
    switch (m_metadata->param_codes[index]) {
      case 'C':
        for (const auto &attribute : mysqlshdk::db::connection_attributes()) {
          // Skips dbUser and dbPassword
          if (attribute[0] != 'd' && attribute[1] != 'b') {
            define_named_arg(param_name, attribute, "", false);
          }
        }
        m_argument_options[param_name] = shcore::Value(shcore::make_dict());
        break;
      case 'D': {
        const auto &validator =
            m_metadata->signature[index]->validator<shcore::Option_validator>();
        if (!validator->allowed().empty()) {
          for (const auto &option : validator->allowed()) {
            // Only adds the options that are enabled for command line
            // operation i.e. in many cases password is allowed for connection
            // data and options but in both cases it is used for the same
            // purpose (connection data), so we disable the options one to
            // avoid creating unnecesary ambiguity.
            if (option->cmd_line_enabled)
              define_named_arg(param_name, option->name, option->short_name,
                               option->flag == shcore::Param_flag::Mandatory);
          }
        } else if (m_any_options_param.empty()) {
          m_any_options_param = param_name;
        }
        m_argument_options[param_name] = shcore::Value(shcore::make_dict());
      } break;
      case 'A':
        if (!found_list) {
          found_list = true;
        } else {
          define_named_arg(param_name, "", "",
                           m_metadata->signature[index]->flag ==
                               shcore::Param_flag::Mandatory);
          m_argument_options[param_name] = shcore::Value(shcore::Array_t());
        }
        break;
      default:
        if (found_list) {
          // A parameter defined after a list shold be required
          define_named_arg(param_name, "", "",
                           m_metadata->signature[index]->flag ==
                               shcore::Param_flag::Mandatory);
          m_argument_options[param_name] = shcore::Value();
        }
        break;
    }
  }
}

/**
 * Adds a named option into it's corresponding parameter doing the data type
 * conversion so the API function receives the parameter type it expects.
 */
void Shell_cli_mapper::set_named_option(const std::string &param_name,
                                        const std::string &option,
                                        const Cmd_line_arg_definition &arg) {
  auto index = m_param_index[param_name];

  // The value here comes as a string, we do the required conversion here
  if (m_metadata->param_codes[index] == 'C') {
    auto arg_map = m_argument_options[param_name].as_map();
    if (option == mysqlshdk::db::kPort) {
      (*arg_map)[option] = shcore::Value(arg.value.as_int());
    } else {
      (*arg_map)[option] = arg.value;
    }
  } else if (m_metadata->param_codes[index] == 'D') {
    auto arg_map = m_argument_options[param_name].as_map();
    add_value_to_dict(
        &arg_map,
        m_metadata->signature[index]->validator<shcore::Option_validator>(),
        {arg.definition, option, arg.value, arg.user_type});
  } else if (m_metadata->param_codes[index] == 'A') {
    auto arg_list = m_argument_options[param_name].as_array();
    if (!arg_list) {
      arg_list = shcore::make_array();
      m_argument_options[param_name] = shcore::Value(arg_list);
    }
    auto validator = m_metadata->signature[index]->validator<List_validator>();
    add_value_to_list(&arg_list, arg, validator->element_type());
  } else {
    auto new_val =
        get_value_of_expected_type(arg, m_metadata->signature[index]->type());
    m_argument_options[param_name] = new_val;
  }
}

Cli_option *Shell_cli_mapper::cli_option(const std::string &name) {
  auto index = m_normalized_cli_options.find(name);
  if (index != m_normalized_cli_options.end())
    return &m_cli_option_registry[index->second];
  else
    return nullptr;
}

void Shell_cli_mapper::set_metadata(const std::string &object_class,
                                    const Cpp_function::Metadata &metadata) {
  m_object_class = object_class;
  m_metadata = &metadata;

  define_named_args();
}

void Shell_cli_mapper::process_arguments(shcore::Argument_list *argument_list) {
  m_argument_list = argument_list;

  process_named_args();
  process_positional_args();
}

}  // namespace cli
}  // namespace shcore
