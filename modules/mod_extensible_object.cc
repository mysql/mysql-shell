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
#include "modules/mod_extensible_object.h"

#include <functional>
#include <memory>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
Extensible_object::Extensible_object(const std::string &name,
                                     const std::string &qualified_name)
    : m_name(name), m_qualified_name(qualified_name) {}

bool Extensible_object::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

shcore::Value Extensible_object::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  const auto &child = m_children.find(prop);
  if (child != m_children.end()) {
    return shcore::Value(
        std::dynamic_pointer_cast<Object_bridge>(child->second));
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}

void Extensible_object::register_object(const std::string &name,
                                        const std::string &brief,
                                        const std::vector<std::string> &details,
                                        const std::string &type_label) {
  if (!shcore::is_valid_identifier(name))
    throw shcore::Exception::argument_error(
        "The " + type_label + " name must be a valid identifier.");

  if (m_children.find(name) != m_children.end()) {
    auto error = shcore::str_format("The %s '%s' already exists at '%s'",
                                    type_label.c_str(), name.c_str(),
                                    m_qualified_name.c_str());
    throw shcore::Exception::argument_error(error);
  }

  // Creates the new object as a member of this object
  std::string qname = m_qualified_name + "." + name;
  m_children.emplace(name, std::make_shared<Extensible_object>(name, qname));
  add_property(name + "|" + name);

  auto help = shcore::Help_registry::get()->get_topic(qname, true);

  if (!help) {
    // Registers the new object on the shell help system
    shcore::Help_registry::get()->add_help_topic(
        name, shcore::Topic_type::OBJECT, name, m_qualified_name,
        shcore::IShell_core::all_scripting_modes());

    // Now registers the brief and description help data if provided
    if (!brief.empty())
      shcore::Help_registry::get()->add_help(name, "BRIEF", brief);

    shcore::Help_registry::get()->add_help(name, "DETAIL", details);
  } else {
    mysqlsh::current_console()->print_warning("Help for " + qname +
                                              " already exists.");
  }
}

void Extensible_object::register_function(
    const std::string &name, const shcore::Function_base_ref &function,
    const shcore::Dictionary_t &definition) {
  shcore::Array_t params;
  std::string brief;
  std::vector<std::string> details;
  shcore::Option_unpacker unpacker(definition);
  unpacker.optional("parameters", &params);
  unpacker.optional("brief", &brief);
  unpacker.optional("details", &details);
  unpacker.end("at function definition");

  auto names = shcore::str_split(name, "|");
  if (names.size() > 2) {
    throw shcore::Exception::argument_error(
        "Invalid function name. When using custom names only two names should "
        "be provided.");
  }

  for (const auto &cname : names) {
    if (!shcore::is_valid_identifier(cname))
      throw shcore::Exception::argument_error("The function name '" + cname +
                                              "'is not a valid identifier.");
  }

  // Builds the parameter list
  std::vector<shcore::Parameter> parameters;
  std::vector<std::pair<std::string, shcore::Value_type>> ptypes;

  parameters = parse_parameters(params, {"parameter"}, true);

  expose(name, function, std::move(parameters));

  register_function_help(m_name, name, definition);
}

std::shared_ptr<Extensible_object> Extensible_object::search_object(
    std::vector<std::string> *name_chain) {
  assert(name_chain);
  size_t length = name_chain->size();
  assert(length);

  if (name_chain->at(0) == m_name) {
    if (length == 1) {
      return shared_from_this();
    } else {
      if (m_children.find(name_chain->at(1)) != m_children.end()) {
        name_chain->erase(name_chain->begin());
        return m_children[name_chain->at(0)]->search_object(name_chain);
      }
    }
  }

  return nullptr;
}

std::shared_ptr<Extensible_object> Extensible_object::search_object(
    const std::string &name_chain) {
  auto tokens = shcore::str_split(name_chain, ".");
  return search_object(&tokens);
}

shcore::Value_type Extensible_object::map_type(const std::string &value) {
  if (value == "string") {
    return shcore::Value_type::String;
  } else if (value == "integer") {
    return shcore::Value_type::Integer;
  } else if (value == "float") {
    return shcore::Value_type::Float;
  } else if (value == "bool") {
    return shcore::Value_type::Bool;
  } else if (value == "object") {
    return shcore::Value_type::Object;
  } else if (value == "array") {
    return shcore::Value_type::Array;
  } else if (value == "dictionary") {
    return shcore::Value_type::Map;
  }

  return shcore::Value_type::Undefined;
}

std::vector<shcore::Parameter> Extensible_object::parse_parameters(
    shcore::Array_t data, const shcore::Parameter_context &context,
    bool default_require) {
  std::vector<shcore::Parameter> parameters;

  if (data) {
    for (size_t index = 0; index < data->size(); index++) {
      auto &item = data->at(index);

      shcore::Parameter_context new_ctx = {context.str(),
                                           static_cast<int>(index + 1)};
      if (item.type == shcore::Value_type::Map) {
        parameters.push_back(
            parse_parameter(item.as_map(), new_ctx, default_require));
      } else {
        std::string error = "Invalid definition at " + new_ctx.str();
        throw shcore::Exception::argument_error(error);
      }
    }
  }

  return parameters;
}

shcore::Parameter Extensible_object::parse_parameter(
    const shcore::Dictionary_t &definition,
    const shcore::Parameter_context &context, bool default_require) {
  shcore::Parameter param;
  std::string type;
  bool required = default_require;
  shcore::Option_unpacker unpacker(definition);
  unpacker.required("name", &param.name);
  unpacker.required("type", &type);
  unpacker.optional("required", &required);

  // Pulling these two is for parameter validation purposes only
  std::string brief;
  std::vector<std::string> details;
  unpacker.optional("brief", &brief);
  unpacker.optional("details", &details);

  param.type = map_type(type);

  if (param.type == shcore::Value_type::String) {
    auto validator = std::make_shared<shcore::String_validator>();
    unpacker.optional("values", &validator->allowed);
    param.validator = validator;
  }

  if (param.type == shcore::Value_type::Object) {
    auto validator = std::make_shared<shcore::Object_validator>();
    unpacker.optional("classes", &validator->allowed);

    std::string allowed;
    unpacker.optional("class", &allowed);
    if (!allowed.empty()) validator->allowed.push_back(allowed);

    param.validator = validator;
  }

  shcore::Array_t options;
  if (param.type == shcore::Value_type::Map)
    unpacker.required("options", &options);

  bool valid_identifier = shcore::is_valid_identifier(param.name);

  std::string current_ctx;
  if (valid_identifier)
    current_ctx = context.title + " '" + param.name + "'";
  else
    current_ctx = context.str();

  unpacker.end("at " + current_ctx);

  if (!valid_identifier) {
    auto error = shcore::str_format("%s is not a valid identifier: '%s'.",
                                    current_ctx.c_str(), param.name.c_str());
    throw shcore::Exception::argument_error(error);
  }

  if (param.type == shcore::Value_type::Undefined) {
    auto error = shcore::str_format("Unsupported data type '%s' at %s.",
                                    type.c_str(), current_ctx.c_str());
    throw shcore::Exception::argument_error(error);
  }

  if (param.type == shcore::Value_type::Map) {
    // Options must be defined
    if (!(options && !options->empty())) {
      auto error =
          shcore::str_format("Missing 'options' at %s.", current_ctx.c_str());
      throw shcore::Exception::argument_error(error);
    }

    auto validator = std::make_shared<shcore::Option_validator>();
    validator->allowed =
        parse_parameters(options, {current_ctx + " option"}, false);

    param.validator = std::move(validator);
  }

  param.flag =
      required ? shcore::Param_flag::Mandatory : shcore::Param_flag::Optional;

  return param;
}

void Extensible_object::register_function_help(
    const std::string &parent, const std::string &name,
    const std::string &brief, const std::vector<std::string> &params,
    const std::vector<std::string> &details) {
  auto help = shcore::Help_registry::get();

  // Defines the modes where the help will be available
  auto mask = shcore::IShell_core::all_scripting_modes();

  // Creates the help topi for the function
  help->add_help_topic(name, shcore::Topic_type::FUNCTION, name, parent, mask);

  // Rest of the help is registered using the JS name
  auto names = shcore::str_split(name, "|");

  // Now registers the function brief, parameters and details
  auto prefix = shcore::str_upper(parent + "_" + names[0]);
  help->add_help(prefix, "BRIEF", brief);
  help->add_help(prefix, "PARAM", params);
  help->add_help(prefix, "DETAIL", details);
}

void Extensible_object::register_function_help(
    const std::string &parent, const std::string &name,
    const shcore::Dictionary_t &function) {
  auto help = shcore::Help_registry::get();

  // Defines the modes where the help will be available
  auto mask = shcore::IShell_core::all_scripting_modes();

  // The help is registered using the base name as tag
  auto names = shcore::str_split(name, "|");

  help->add_help_topic(name, shcore::Topic_type::FUNCTION, names[0], parent,
                       mask);

  // Now registers the brief abd description help data if provided
  std::string help_prefix = shcore::str_upper(parent + "_" + names[0]);
  shcore::Option_unpacker unpacker(function);
  std::string brief;
  unpacker.optional("brief", &brief);
  if (!brief.empty()) help->add_help(help_prefix, "BRIEF", brief);

  size_t detail_count = 0;
  size_t param_count = 0;

  // Param details are inserted at the end of the function details by defaule.
  // If @PARAMDETAILS entry is found on the function details, the parameter
  // details wil be inserted right there.
  std::vector<std::string> details;
  shcore::Array_t parameters;
  unpacker.optional("details", &details);
  unpacker.optional("parameters", &parameters);
  bool param_details = true;
  for (const auto &detail : details) {
    if (!detail.empty()) {
      if (detail == "@PARAMDETAILS") {
        if (parameters) {
          for (const auto &param : *parameters) {
            register_param_help_detail(param.as_map(), help_prefix,
                                       &detail_count, true);
          }
        }
        param_details = false;
      } else {
        help->add_help(help_prefix, "DETAIL", &detail_count, {detail});
      }
    }
  }

  if (parameters) {
    for (const auto &param : *parameters) {
      register_param_help_brief(param.as_map(), help_prefix, &param_count,
                                true);

      if (param_details)
        register_param_help_detail(param.as_map(), help_prefix, &detail_count,
                                   true);
    }
  }
}

void Extensible_object::register_param_help_brief(
    const shcore::Dictionary_t &param, const std::string &prefix,
    size_t *sequence, bool as_parameter) {
  std::string param_help(as_parameter ? "@param " : "@li ");
  std::string suffix(as_parameter ? "PARAM" : "DETAIL");

  shcore::Option_unpacker unpacker(param);

  std::string name;
  std::string type;
  std::string brief;
  bool required = as_parameter;
  unpacker.required("name", &name);
  unpacker.required("type", &type);
  unpacker.optional("required", &required);
  unpacker.optional("brief", &brief);

  param_help += name;

  if (!required) param_help += " Optional";

  param_help += " " + type + ".";

  if (!brief.empty()) param_help.append(" " + brief);

  shcore::Help_registry::get()->add_help(prefix, suffix, sequence,
                                         {param_help});
}

void Extensible_object::register_param_help_detail(
    const shcore::Dictionary_t &param, const std::string &prefix,
    size_t *sequence, bool as_parameter) {
  shcore::Option_unpacker unpacker(param);
  std::vector<std::string> details;
  std::string type, name;
  unpacker.required("name", &name);
  unpacker.required("type", &type);
  unpacker.optional("details", &details);

  auto help = shcore::Help_registry::get();

  help->add_help(prefix, "DETAIL", sequence, details);

  std::string help_entry =
      "The " + name + (as_parameter ? " parameter" : " option");
  if (type == "object") {
    std::vector<std::string> classes;
    unpacker.optional("classes", &classes);

    std::string aclass;
    unpacker.optional("class", &aclass);
    if (!aclass.empty()) classes.push_back(aclass);

    if (classes.size() == 1)
      help_entry += " must be a " + classes[0] + " object.";
    else
      help_entry += " must be any of " + shcore::str_join(classes, ", ") + ".";

    help->add_help(prefix, "DETAIL", sequence, {help_entry});
  } else if (type == "string") {
    std::vector<std::string> values;
    unpacker.optional("values", &values);

    if (!values.empty()) {
      help_entry += " accepts the following values: ";
      help->add_help(prefix, "DETAIL", sequence, {help_entry});

      for (const auto &value : values)
        help->add_help(prefix, "DETAIL", sequence, {"@li " + value});
    }
  } else if (type == "dictionary") {
    help_entry += " accepts the following options:";
    help->add_help(prefix, "DETAIL", sequence, {help_entry});

    shcore::Array_t options;
    unpacker.required("options", &options);

    if (options) {
      for (const auto &option : *options)
        register_param_help_brief(option.as_map(), prefix, sequence, false);

      for (const auto &option : *options)
        register_param_help_detail(option.as_map(), prefix, sequence, false);
    }
  }
}

}  // namespace mysqlsh
