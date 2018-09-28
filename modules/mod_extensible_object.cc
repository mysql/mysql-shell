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

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

std::string to_string(shcore::Value_type t) {
  if (shcore::Value_type::Map == t) {
    return "Dictionary";
  } else {
    return shcore::type_name(t);
  }
}

shcore::Cpp_function::Raw_signature to_raw_signature(
    const Function_definition::Parameters &in) {
  shcore::Cpp_function::Raw_signature out;

  for (const auto &p : in) {
    out.emplace_back(p->parameter);
  }

  return out;
}

}  // namespace

Parameter_definition::Parameter_definition()
    : parameter{std::make_shared<shcore::Parameter>()} {}

Parameter_definition::Parameter_definition(const std::string &n,
                                           shcore::Value_type t,
                                           shcore::Param_flag f)
    : parameter{std::make_shared<shcore::Parameter>(n, t, f)} {}

bool Parameter_definition::is_required() const {
  return shcore::Param_flag::Mandatory == parameter->flag;
}

void Parameter_definition::set_options(const Options &options) {
  validate_options();

  m_options = options;
  parameter->validator<shcore::Option_validator>()->set_allowed(
      to_raw_signature(options));
}

const Parameter_definition::Options &Parameter_definition::options() const {
  validate_options();

  return m_options;
}

void Parameter_definition::validate_options() const {
  if (parameter->type() != shcore::Value_type::Map) {
    throw std::logic_error("Only map type can have options.");
  }
}

Extensible_object::Extensible_object(const std::string &name,
                                     const std::string &qualified_name)
    : m_name(name), m_qualified_name(qualified_name) {}

Extensible_object::~Extensible_object() { disable_help(); }
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

  m_children[name]->register_object_help(brief, details);
}

void Extensible_object::register_function(
    const std::string &name, const shcore::Function_base_ref &function,
    const shcore::Dictionary_t &definition) {
  Function_definition description;
  shcore::Array_t params;

  shcore::Option_unpacker unpacker(definition);
  unpacker.optional("parameters", &params);
  unpacker.optional("brief", &description.brief);
  unpacker.optional("details", &description.details);
  unpacker.end("at function definition");

  // Builds the parameter list
  description.parameters = parse_parameters(params, {"parameter"}, true);

  register_function(name, function, description);
}

void Extensible_object::register_function(
    const std::string &name, const shcore::Function_base_ref &function,
    const Function_definition &definition) {
  validate_function(name, definition.parameters);

  expose(name, function, to_raw_signature(definition.parameters));

  register_function_help(name, definition);
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

Function_definition::Parameters Extensible_object::parse_parameters(
    const shcore::Array_t &data, const shcore::Parameter_context &context,
    bool default_require) {
  Function_definition::Parameters parameters;

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

std::shared_ptr<Parameter_definition>
Extensible_object::start_parsing_parameter(const shcore::Dictionary_t &,
                                           shcore::Option_unpacker *) const {
  return std::make_shared<Parameter_definition>();
}

std::shared_ptr<Parameter_definition> Extensible_object::parse_parameter(
    const shcore::Dictionary_t &definition,
    const shcore::Parameter_context &context, bool default_require) {
  std::string type;
  bool required = default_require;
  shcore::Option_unpacker unpacker(definition);

  auto param_definition = start_parsing_parameter(definition, &unpacker);
  const auto &param = param_definition->parameter;

  unpacker.required("name", &param->name);
  unpacker.required("type", &type);
  unpacker.optional("required", &required);
  unpacker.optional("brief", &param_definition->brief);
  unpacker.optional("details", &param_definition->details);

  param->set_type(map_type(type));

  if (param->type() == shcore::Value_type::String) {
    std::vector<std::string> allowed;
    unpacker.optional("values", &allowed);
    param->validator<shcore::String_validator>()->set_allowed(
        std::move(allowed));
  }

  if (param->type() == shcore::Value_type::Object) {
    std::vector<std::string> allowed_classes;
    unpacker.optional("classes", &allowed_classes);

    std::string allowed;
    unpacker.optional("class", &allowed);
    if (!allowed.empty()) allowed_classes.push_back(allowed);

    param->validator<shcore::Object_validator>()->set_allowed(
        std::move(allowed_classes));
  }

  shcore::Array_t options;
  if (param->type() == shcore::Value_type::Map)
    unpacker.required("options", &options);

  std::string current_ctx;
  if (shcore::is_valid_identifier(param->name))
    current_ctx = context.title + " '" + param->name + "'";
  else
    current_ctx = context.str();

  unpacker.end("at " + current_ctx);

  if (param->type() == shcore::Value_type::Map) {
    if (options && !options->empty()) {
      param_definition->set_options(
          parse_parameters(options, {current_ctx + " option"}, false));
    }
  }

  param->flag =
      required ? shcore::Param_flag::Mandatory : shcore::Param_flag::Optional;

  return param_definition;
}
void Extensible_object::register_object_help(
    const std::string &brief, const std::vector<std::string> &details) {
  auto help = shcore::Help_registry::get();

  // Attempts getting the fully qualified object
  shcore::Help_topic *topic = help->get_topic(m_qualified_name, true);

  if (topic) {
    mysqlsh::current_console()->print_warning("Help for " + m_qualified_name +
                                              " already exists.");
    enable_help();
  } else {
    // Defines the modes where the help will be available
    auto mask = shcore::IShell_core::all_scripting_modes();

    auto tokens = shcore::str_split(m_qualified_name, ".");
    tokens.erase(tokens.end() - 1);
    auto parent = shcore::str_join(tokens, ".");

    // Creates the help topic for the object
    help->add_help_topic(m_name, shcore::Topic_type::OBJECT, m_name, parent,
                         mask);

    // Now registers the object brief, parameters and details
    auto prefix = shcore::str_upper(m_name);
    help->add_help(prefix, "BRIEF", brief);
    help->add_help(prefix, "DETAIL", details);
  }
}

void Extensible_object::register_function_help(
    const std::string &name, const std::string &brief,
    const std::vector<std::string> &params,
    const std::vector<std::string> &details) {
  auto help = shcore::Help_registry::get();

  auto names = shcore::str_split(name, "|");

  shcore::Help_topic *topic =
      help->get_topic(m_qualified_name + "." + names[0], true);

  if (!topic) {
    // Defines the modes where the help will be available
    auto mask = shcore::IShell_core::all_scripting_modes();

    // Creates the help topic for the function
    help->add_help_topic(name, shcore::Topic_type::FUNCTION, names[0],
                         m_qualified_name, mask);

    // Now registers the function brief, parameters and details
    auto prefix = shcore::str_upper(m_name + "_" + names[0]);
    if (!brief.empty()) help->add_help(prefix, "BRIEF", brief);
    help->add_help(prefix, "PARAM", params);
    help->add_help(prefix, "DETAIL", details);
  } else {
    topic->set_enabled(true);
  }
}

void Extensible_object::register_function_help(
    const std::string &name, const Function_definition &definition) {
  // Param details are inserted at the end of the function details by default.
  // If @PARAMDETAILS entry is found on the function details, the parameter
  // details wil be inserted right there.
  bool param_details = true;

  std::vector<std::string> params;
  std::vector<std::string> details;

  for (const auto &detail : definition.details) {
    if (!detail.empty()) {
      if (detail == "@PARAMDETAILS") {
        for (const auto &param : definition.parameters) {
          get_param_help_detail(*param, true, &params);
        }

        param_details = false;
      } else {
        details.emplace_back(detail);
      }
    }
  }

  for (const auto &param : definition.parameters) {
    get_param_help_brief(*param, true, &params);

    if (param_details) {
      get_param_help_detail(*param, true, &details);
    }
  }

  register_function_help(name, definition.brief, params, details);
}

void Extensible_object::get_param_help_brief(
    const Parameter_definition &param_definition, bool as_parameter,
    std::vector<std::string> *target) {
  std::string param_help(as_parameter ? "@param " : "@li ");
  const auto &param = param_definition.parameter;

  param_help += param->name;

  if (!param_definition.is_required()) param_help += " Optional";

  param_help += " " + to_string(param->type()) + ".";

  if (!param_definition.brief.empty())
    param_help.append(" " + param_definition.brief);

  target->emplace_back(std::move(param_help));
}

void Extensible_object::get_param_help_detail(
    const Parameter_definition &param_definition, bool as_parameter,
    std::vector<std::string> *details) {
  details->insert(details->end(), param_definition.details.begin(),
                  param_definition.details.end());

  const auto &param = param_definition.parameter;

  std::string help_entry =
      "The " + param->name + (as_parameter ? " parameter" : " option");

  switch (param->type()) {
    case shcore::Value_type::Object: {
      const auto &classes =
          param->validator<shcore::Object_validator>()->allowed();

      if (classes.size() == 1)
        help_entry += " must be a " + classes[0] + " object.";
      else
        help_entry +=
            " must be any of " + shcore::str_join(classes, ", ") + ".";

      details->emplace_back(std::move(help_entry));

      break;
    }

    case shcore::Value_type::String: {
      const auto &values =
          param->validator<shcore::String_validator>()->allowed();

      if (!values.empty()) {
        help_entry += " accepts the following values: ";
        details->emplace_back(std::move(help_entry));

        for (const auto &value : values) details->emplace_back("@li " + value);
      }

      break;
    }

    case shcore::Value_type::Map: {
      help_entry += " accepts the following options:";
      details->emplace_back(std::move(help_entry));

      const auto &options = param_definition.options();

      for (const auto &option : options)
        get_param_help_brief(*option, false, details);

      for (const auto &option : options)
        get_param_help_detail(*option, false, details);

      break;
    }

    default:
      // no action
      break;
  }
}

void Extensible_object::validate_function(
    const std::string &name,
    const Function_definition::Parameters &parameters) {
  const auto names = shcore::str_split(name, "|");

  if (names.size() > 2) {
    throw shcore::Exception::argument_error(
        "Invalid function name. When using custom names only two names should "
        "be provided.");
  }

  for (const auto &cname : names) {
    if (!shcore::is_valid_identifier(cname))
      throw shcore::Exception::argument_error("The function name '" + cname +
                                              "' is not a valid identifier.");
  }

  int index = 0;

  for (const auto &param : parameters) {
    validate_parameter(*param->parameter, {"parameter", ++index});
  }
}

void Extensible_object::validate_parameter(
    const shcore::Parameter &param, const shcore::Parameter_context &context) {
  bool valid_identifier = shcore::is_valid_identifier(param.name);

  std::string current_ctx;

  if (valid_identifier) {
    current_ctx = context.title + " '" + param.name + "'";
  } else {
    current_ctx = context.str();
  }

  if (!valid_identifier) {
    const auto error =
        shcore::str_format("%s is not a valid identifier: '%s'.",
                           current_ctx.c_str(), param.name.c_str());
    throw shcore::Exception::argument_error(error);
  }

  if (param.type() == shcore::Value_type::Undefined) {
    const auto error =
        shcore::str_format("Unsupported data type at %s.", current_ctx.c_str());
    throw shcore::Exception::argument_error(error);
  }

  if (param.type() == shcore::Value_type::Map) {
    const auto &options =
        param.validator<shcore::Option_validator>()->allowed();
    // Options must be defined
    if (options.empty()) {
      auto error =
          shcore::str_format("Missing 'options' at %s.", current_ctx.c_str());
      throw shcore::Exception::argument_error(error);
    } else {
      int index = 0;

      for (const auto &option : options) {
        validate_parameter(*option, {current_ctx + " option", ++index});
      }
    }
  }
}

namespace {

struct Compare_help_topic_and_string {
  bool operator()(const shcore::Help_topic *left, const std::string &right) {
    return left->m_help_tag < right;
  }

  bool operator()(const std::string &left, const shcore::Help_topic *right) {
    return left < right->m_help_tag;
  }
};

}  // namespace

void Extensible_object::enable_help() {
  auto topic = shcore::Help_registry::get()->get_topic(m_qualified_name, true);

  if (topic && !topic->is_enabled()) {
    topic->set_enabled(true);

    const auto members = get_members();
    std::vector<shcore::Help_topic *> to_enable;

    // find topics for currently existing members
    std::set_intersection(topic->m_childs.begin(), topic->m_childs.end(),
                          members.begin(), members.end(),
                          std::back_inserter(to_enable),
                          Compare_help_topic_and_string());

    // enable them
    for (const auto &member : to_enable) {
      member->set_enabled(true);
    }

    // make sure registered children enable help for their ancestors
    for (auto &child : m_children) child.second->enable_help();
  }
}

void Extensible_object::disable_help() {
  auto topic = shcore::Help_registry::get()->get_topic(m_qualified_name, true);

  if (topic && topic->is_enabled()) {
    topic->set_enabled(false);

    // disable help for all the children, including methods
    for (const auto &child : topic->m_childs) {
      child->set_enabled(false);
    }

    // make sure registered children disable help for their ancestors
    for (auto &child : m_children) child.second->disable_help();
  }
}

}  // namespace mysqlsh
