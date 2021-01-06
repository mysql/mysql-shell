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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "modules/mod_extensible_object.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <utility>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

REGISTER_HELP_TOPIC(Extension Objects, TOPIC, EXTENSION_OBJECTS, Contents,
                    SCRIPTING);
REGISTER_HELP_TOPIC_TEXT(EXTENSION_OBJECTS, R"*(
The MySQL Shell allows for an extension of its base functionality by using
special objects called <b>Extension Objects</b>.

Once an extension object has been created it is possible to attach different
type of object members such as:

@li Functions
@li Properties
@li Other extension objects

Once an extension object has been fully defined it can be registered as a
regular <b>Global Object</b>. This way the object and its members will be
available in the scripting languages the MySQL Shell supports, just as any other
global object such as <b>shell</b>, <b>util</b>, etc.

Extending the MySQL Shell with extension objects follows this pattern:

@li Creation of a new extension object.
@li Addition of members (properties, functions)
@li Registration of the object

Creation of a new extension object is done through the
shell.<<<createExtensionObject>>> function.

Members can be attached to the extension object by calling the
shell.<<<addExtensionObjectMember>>> function.

Registration of an extension object as a global object can be done by calling
the shell.<<<registerGlobal>>> function.

Alternatively, the extension object can be added as a member of another
extension object by calling the shell.<<<addExtensionObjectMember>>> function.

You can get more details on these functions by executing:

@li \? <<<createExtensionObject>>>
@li \? <<<addExtensionObjectMember>>>
@li \? <<<registerGlobal>>>


<b>Naming Convention</b>

The core MySQL Shell APIs follow a specific naming convention for object
members. Extension objects should follow the same naming convention for
consistency reasons.

@li JavaScript members use camelCaseNaming
@li Python members use snake_case_naming

To simplify this across languages, it is important to use camelCaseNaming when
specifying the 'name' parameter of the shell.<<<addExtensionObjectMember>>>
function.

The MySQL Shell will then automatically handle the snake_case_naming for that
member when it is switched to Python mode.

NOTE: the naming convention is only applicable for extension object members.
However, when a global object is registered, the name used to register that
object will be exactly the same in both JavaScript and Python modes.

<b>Example</b>:<br>

@code{.py}
# Sample python function to be added to an extension object
def some_python_function():
  print ("Hello world!")

# The extension object is created
obj = shell.create_extension_object()

# The sample function is added as member
# NOTE: The member name using camelCaseNaming
shell.add_extension_object_member(obj, "mySampleFunction", some_python_function)

# The extension object is registered
shell.register_global("myCustomObject", obj)
@endcode

<b>Calling in JavaScript</b>:<br>
@code{.js}
// Member is available using camelCaseNaming
mysql-js> myCustomObject.mySampleFunction()
Hello World!
mysql-js>
@endcode

<b>Calling in Python</b>:<br>
@code{.py}
# Member is available using snake_case_naming
mysql-py> myCustomObject.my_sample_function()
Hello World!
mysql-py>
@endcode

<b>Automatic Loading of Extension Objects</b>

The MySQL Shell startup logic scans for extension scripts at the following paths:

@li Windows: @%AppData@%/MySQL/mysqlsh/init.d
@li Others ~/.mysqlsh/init.d

An extension script is either a JavaScript (*.js) or Python (*.py) file which
will be automatically processed when the MySQL Shell starts.

These scripts can be used to define extension objects so there are available
right away when the MySQL Shell starts.
)*");

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

const std::set<shcore::Value_type> kAllowedMemberTypes = {
    shcore::Function, shcore::Array,   shcore::Map,      shcore::Null,
    shcore::Bool,     shcore::Integer, shcore::UInteger, shcore::Float,
    shcore::String,   shcore::Object};

const std::set<std::string> kAllowedParamTypes = {
    "string", "integer", "float", "bool", "object", "array", "dictionary"};

const std::map<std::string, shcore::Value_type> kTypeMapping = {
    {"string", shcore::String},  {"integer", shcore::Integer},
    {"float", shcore::Float},    {"bool", shcore::Bool},
    {"object", shcore::Object},  {"array", shcore::Array},
    {"dictionary", shcore::Map}, {"function", shcore::Function},
};

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
    : m_name(name),
      m_qualified_name(qualified_name),
      m_registered(false),
      m_detail_sequence(0) {}

Extensible_object::Extensible_object(const std::string &name,
                                     const std::string &qualified_name,
                                     bool registered)
    : m_name(name),
      m_qualified_name(qualified_name),
      m_registered(registered),
      m_detail_sequence(0) {}

Extensible_object::~Extensible_object() { disable_help(); }
bool Extensible_object::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

shcore::Value Extensible_object::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (!is_registered())
    throw shcore::Exception::runtime_error(
        "Unable to access members in an unregistered extension object.");

  const auto &child = m_children.find(prop);
  if (child != m_children.end()) {
    return shcore::Value(
        std::dynamic_pointer_cast<Object_bridge>(child->second));
  } else {
    const auto &property = m_members.find(prop);
    if (property != m_members.end()) {
      ret_val = property->second;
    } else {
      ret_val = Cpp_object_bridge::get_member(prop);
    }
  }

  return ret_val;
}

void Extensible_object::set_member(const std::string &prop,
                                   shcore::Value value) {
  if (!is_registered())
    throw shcore::Exception::runtime_error(
        "Unable to modify members in an unregistered extension object.");

  const auto &property = m_members.find(prop);
  if (property != m_members.end()) {
    property->second = value;
  } else {
    Cpp_object_bridge::set_member(prop, value);
  }
}

bool Extensible_object::has_member(const std::string &prop) const {
  return has_method(prop) || m_members.find(prop) != m_members.end() ||
         m_children.find(prop) != m_children.end();
}

shcore::Value Extensible_object::call(const std::string &name,
                                      const shcore::Argument_list &args) {
  if (!is_registered()) {
    throw shcore::Exception::runtime_error(
        "Unable to call functions in an unregistered extension object.");
  }

  return Cpp_object_bridge::call(name, args);
}

shcore::Value Extensible_object::call_advanced(
    const std::string &name, const shcore::Argument_list &args,
    const shcore::Dictionary_t &kwargs) {
  if (!is_registered()) {
    throw shcore::Exception::runtime_error(
        "Unable to call functions in an unregistered extension object.");
  }

  return Cpp_object_bridge::call_advanced(name, args, kwargs);
}

void Extensible_object::register_member(
    const std::string &name, const shcore::Value &value,
    const shcore::Dictionary_t &definition) {
  if (has_member(name)) {
    throw shcore::Exception::argument_error("The object already has a '" +
                                            name + "' member.");
  }

  std::string target_object;
  if (m_name.empty())
    target_object = "an unregistered extension object";
  else
    target_object =
        shcore::str_format("the '%s' extension object", m_name.c_str());

  if (kAllowedMemberTypes.find(value.type) == kAllowedMemberTypes.end()) {
    throw shcore::Exception::argument_error("Unsupported member type.");
  } else if (value.type == shcore::Object) {
    auto object = value.as_object<Extensible_object>();
    if (!object)
      throw shcore::Exception::argument_error(
          "Unsupported member type, object members need to be "
          "ExtensionObjects.");

    object->m_definition = parse_member_definition(definition);
    object->m_definition->name = name;
    register_object(object);
    log_debug(
        "The '%s' extension object has been registered as a member into %s.",
        name.c_str(), target_object.c_str());
  } else {
    if (value.type == shcore::Function) {
      auto fd = parse_function_definition(definition);
      fd->name = name;
      register_function(fd, value.as_function(), false);
      log_debug("The '%s' function has been registered into %s.", name.c_str(),
                target_object.c_str());
    } else {
      auto md = parse_member_definition(definition);
      md->name = name;
      register_property(md, value, false);
      log_debug("The '%s' property has been registered into %s.", name.c_str(),
                target_object.c_str());
    }
  }
}

void Extensible_object::set_registered(const std::string &name) {
  // The caller must ensure this function is not called for a registered
  // object
  assert(!m_registered);

  if (!m_registered) {
    auto parent = m_parent.lock();
    m_name = name.empty() ? m_definition->name : name;
    m_registered = true;

    if (parent)
      m_qualified_name = parent->m_qualified_name + "." + m_name;
    else
      m_qualified_name = m_name;

    // When parent is shellapi then it is a global object
    register_help(m_definition, !parent);

    for (const auto &mdef : m_property_definition) register_property_help(mdef);

    for (const auto &fdef : m_function_definition) register_function_help(fdef);

    for (auto &child : m_children) {
      child.second->set_registered();
    }
  }
}

/**
 * Registers an existing object definition on this object.
 *
 * A new topic will be created for the object on the shell help system.
 *
 * @param definition a Member definition instance
 * @param object The object being registered.
 *
 * - The module name must be a valid identifier.
 */
void Extensible_object::register_object(
    const std::shared_ptr<Extensible_object> &object) {
  if (object.get() == this) {
    throw shcore::Exception::argument_error(
        "An extension object can not be member of itself.");
  }
  // The order of these validations matters, the first one
  // tests for the object being part of another object
  if (object->get_parent()) {
    std::string error =
        "The provided extension object is already registered as part of "
        "another extension object";

    if (object->is_registered())
      error += " at: " + object->get_qualified_name();

    error += ".";

    throw shcore::Exception::argument_error(error);
  }

  // And this one tests for a registered object (global)
  if (object->is_registered()) {
    std::string error =
        "The provided extension object is already registered as: " +
        object->get_qualified_name() + ".";

    throw shcore::Exception::argument_error(error);
  }

  object->m_parent = shared_from_this();

  if (object->cli_enabled()) enable_cli();

  m_children.emplace(object->m_definition->name, object);

  add_property(object->m_definition->name);

  if (m_registered) object->set_registered();
}

std::shared_ptr<Extensible_object> Extensible_object::register_object(
    const std::string &name, const std::string &brief,
    const std::vector<std::string> &details, const std::string &type_label) {
  std::shared_ptr<Extensible_object> object;

  if (!shcore::is_valid_identifier(name))
    throw shcore::Exception::argument_error(
        "The " + type_label + " name must be a valid identifier.");

  if (has_member(name)) {
    throw shcore::Exception::argument_error("The object already has a '" +
                                            name + "' member.");
  }

  std::string qualified_name =
      m_registered ? m_qualified_name + "." + name : "";

  object = std::make_shared<Extensible_object>(name, qualified_name);

  object->m_definition =
      std::make_shared<Member_definition>(name, brief, details);

  register_object(object);

  return object;
}

std::shared_ptr<Member_definition> Extensible_object::parse_member_definition(
    const shcore::Dictionary_t &definition) {
  auto def = std::make_shared<Member_definition>();

  if (definition) {
    shcore::Option_unpacker unpacker(definition);
    unpacker.optional("brief", &def->brief);
    unpacker.optional("details", &def->details);
    unpacker.end("at member definition");
  }

  return def;
}

std::shared_ptr<Function_definition>
Extensible_object::parse_function_definition(
    const shcore::Dictionary_t &definition) {
  auto def = std::make_shared<Function_definition>();
  shcore::Array_t params;

  if (definition) {
    shcore::Option_unpacker unpacker(definition);
    unpacker.optional("parameters", &params);
    unpacker.optional("brief", &def->brief);
    unpacker.optional("details", &def->details);
    unpacker.optional("cli", &def->cli_enabled);
    unpacker.end("at function definition");
  }

  // Builds the parameter list
  shcore::Parameter_context context{"", {{"parameter", {}}}};
  def->parameters =
      parse_parameters(params, &context, kAllowedParamTypes, true);

  return def;
}

void Extensible_object::register_property(
    const std::shared_ptr<Member_definition> &definition,
    const shcore::Value &value, bool custom_names) {
  validate_name(definition->name, "member", custom_names);

  add_property(definition->name);

  m_members[_properties.back().base_name()] = value;

  if (is_registered())
    register_property_help(definition);
  else
    m_property_definition.push_back(definition);
}

void Extensible_object::register_function(
    const std::shared_ptr<Function_definition> &definition,
    const shcore::Function_base_ref &function, bool custom_names) {
  validate_name(definition->name, "function", custom_names);

  validate_function(definition);

  expose(definition->name, function, to_raw_signature(definition->parameters))
      ->cli(definition->cli_enabled);

  if (definition->cli_enabled) enable_cli();

  if (is_registered())
    register_function_help(definition);
  else
    m_function_definition.push_back(definition);
}

shcore::Value_type Extensible_object::map_type(
    const std::string &type, const std::set<std::string> &allowed_types) const {
  if (allowed_types.find(type) != allowed_types.end())
    return kTypeMapping.at(type);
  else
    return shcore::Value_type::Undefined;
}

Function_definition::Parameters Extensible_object::parse_parameters(
    const shcore::Array_t &data, shcore::Parameter_context *context,
    const std::set<std::string> &allowed_types, bool as_parameters) {
  Function_definition::Parameters parameters;

  std::string optional_parameter;
  std::vector<std::string> item_names;
  if (data) {
    for (size_t index = 0; index < data->size(); index++) {
      auto &item = data->at(index);

      context->levels.back().position = static_cast<int>(index + 1);

      if (item.type == shcore::Value_type::Map) {
        parameters.push_back(parse_parameter(item.as_map(), context,
                                             allowed_types, as_parameters));

        std::string item_name = parameters.back()->parameter->name;
        auto item_index =
            std::find_if(item_names.begin(), item_names.end(),
                         [item_name](const std::string &existing) {
                           return shcore::str_caseeq(item_name, existing);
                         });

        // Validates duplicates
        std::string item_label = context->levels.back().name;
        if (item_index != item_names.end()) {
          std::string error =
              item_label + " '" + item_name + "' is already defined.";
          throw shcore::Exception::argument_error(error);
        } else {
          item_names.push_back(item_name);
        }

        // When parsing parameters as parameters there can't be required
        // parameters after an optional parameter has been found
        if (as_parameters) {
          bool new_required = parameters.back()->is_required();
          if (new_required && !optional_parameter.empty()) {
            std::string error = item_label + " '" + item_name +
                                "' can not be required after optional " +
                                item_label + " '" + optional_parameter + "'";
            throw shcore::Exception::argument_error(error);
          } else if (optional_parameter.empty() && !new_required) {
            optional_parameter = item_name;
          }
        }
      } else {
        std::string error = "Invalid definition at " + context->str();
        throw shcore::Exception::argument_error(error);
      }
    }

    context->levels.back().position = nullptr;
  }

  return parameters;
}

std::shared_ptr<Parameter_definition>
Extensible_object::start_parsing_parameter(const shcore::Dictionary_t &,
                                           shcore::Option_unpacker *) const {
  return std::make_shared<Parameter_definition>();
}

std::shared_ptr<Parameter_definition> Extensible_object::parse_parameter(
    const shcore::Dictionary_t &definition, shcore::Parameter_context *context,
    const std::set<std::string> &allowed_types, bool as_parameters) {
  mysqlshdk::utils::nullable<std::string> type;

  // By default, parameters are required, unless required=false is specified
  // By default, options are not required, unless required=true is specified
  bool required = as_parameters;
  shcore::Option_unpacker unpacker(definition);

  auto param_definition = start_parsing_parameter(definition, &unpacker);
  const auto &param = param_definition->parameter;

  unpacker.required("name", &param->name);
  unpacker.optional("type", &type);
  unpacker.optional("required", &required);
  unpacker.optional("default", &param->def_value);
  unpacker.optional("brief", &param_definition->brief);
  unpacker.optional("details", &param_definition->details);

  if (!type.is_null()) {
    param->set_type(map_type(*type, allowed_types));
  } else {
    param->set_type(shcore::Value_type::Undefined);
  }
  param->cmd_line_enabled = true;

  if (param->type() == shcore::Value_type::String) {
    std::vector<std::string> allowed;
    unpacker.optional("values", &allowed);
    param->validator<shcore::String_validator>()->set_allowed(
        std::move(allowed));
  }

  if (param->type() == shcore::Value_type::Array) {
    std::string itemtype;
    unpacker.optional("itemtype", &itemtype);
    param->validator<shcore::List_validator>()->set_element_type(
        map_type(itemtype, kAllowedParamTypes));
  }

  if (param->type() == shcore::Value_type::Object) {
    std::string error;
    mysqlshdk::utils::nullable<std::vector<std::string>> allowed_classes;
    unpacker.optional("classes", &allowed_classes);

    mysqlshdk::utils::nullable<std::string> allowed;
    unpacker.optional("class", &allowed);

    if (!allowed.is_null()) {
      if (allowed_classes.is_null())
        allowed_classes = {*allowed};
      else
        allowed_classes->push_back(*allowed);
    }

    std::vector<std::string> callowed;
    if (!allowed_classes.is_null()) {
      if (allowed_classes->empty()) {
        error = "An empty array is not valid for the classes option.";
      } else {
        callowed = *allowed_classes;

        std::set<std::string> classes;

        for (const auto allowed_type :
             {shcore::Topic_type::CLASS, shcore::Topic_type::GLOBAL_OBJECT,
              shcore::Topic_type::OBJECT}) {
          auto topics =
              shcore::Help_registry::get()->get_help_topics(allowed_type);

          for (const auto &topic : topics) {
            classes.insert(topic->get_base_name());
          }
        }

        std::set<std::string> invalids;
        for (auto &a_class : callowed) {
          std::string trimmed = shcore::str_strip(a_class);

          if (trimmed.empty()) {
            invalids.insert("'" + a_class + "'");
          } else if (classes.find(a_class) == classes.end()) {
            invalids.insert(a_class);
          }
        }

        if (!invalids.empty()) {
          std::string custom = invalids.size() == 1
                                   ? " is not recognized, it "
                                   : "es are not recognized, they ";
          error = "The following class" + custom +
                  "can not be used on the validation of an object parameter: " +
                  shcore::str_join(invalids, ", ") + ".";
        }
      }
    }

    if (!error.empty()) {
      throw shcore::Exception::argument_error(error);
    } else {
      param->validator<shcore::Object_validator>()->set_allowed(
          std::move(callowed));
    }
  }

  shcore::Array_t options;
  if (param->type() == shcore::Value_type::Map)
    unpacker.optional("options", &options);

  std::string current_ctx;
  auto ctx_name_backup = context->levels.back().name;
  auto ctx_position_backup = context->levels.back().position;

  if (shcore::is_valid_identifier(param->name)) {
    context->levels.back().name = (type.get_safe().empty() ? "" : *type + " ") +
                                  ctx_name_backup + " '" + param->name + "'";
    context->levels.back().position = nullptr;
  }

  current_ctx = context->str();
  unpacker.end("at " + current_ctx);

  context->levels.back().name = ctx_name_backup + " '" + param->name + "'";

  // If type was specified, it can't be Undefined
  if (!type.is_null() && param->type() == shcore::Value_type::Undefined) {
    const auto error =
        "Unsupported type used at " + context->str() +
        ". Allowed types: " + shcore::str_join(allowed_types, ", ");
    throw shcore::Exception::argument_error(error);
  }

  if (param->type() == shcore::Value_type::Map) {
    if (options && !options->empty()) {
      context->levels.push_back({"option", {}});
      param_definition->set_options(
          parse_parameters(options, context, allowed_types, false));
      context->levels.pop_back();
    }
  }

  // Restores the context to it's original values
  context->levels.back().name = ctx_name_backup;
  context->levels.back().position = ctx_position_backup;

  param->flag =
      required ? shcore::Param_flag::Mandatory : shcore::Param_flag::Optional;

  return param_definition;
}  // namespace mysqlsh

void Extensible_object::register_help(
    const std::shared_ptr<Member_definition> &def, bool is_global) {
  if (is_registered())
    register_help(def->brief, def->details, is_global);
  else
    m_definition = def;
}

void Extensible_object::register_help(const std::string &brief,
                                      const std::vector<std::string> &details,
                                      bool is_global) {
  if (is_registered()) {
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
      auto type = is_global ? shcore::Topic_type::GLOBAL_OBJECT
                            : shcore::Topic_type::OBJECT;
      help->add_help_topic(m_name, type, m_qualified_name, parent, mask,
                           shcore::Keyword_location::LOCAL_CTX);

      // Now registers the object brief, parameters and details
      auto prefix = shcore::str_upper(m_qualified_name);

      help->add_help(prefix, "BRIEF", brief,
                     shcore::Keyword_location::LOCAL_CTX);
      help->add_help(prefix, "DETAIL", &m_detail_sequence, details,
                     shcore::Keyword_location::LOCAL_CTX);

      if (is_global)
        help->add_help(prefix, "GLOBAL_BRIEF", brief,
                       shcore::Keyword_location::LOCAL_CTX);
    }
  } else {
    m_definition.reset(new Member_definition("", brief, details));
  }
}

void Extensible_object::register_property_help(
    const std::shared_ptr<Member_definition> &def) {
  auto help = shcore::Help_registry::get();

  // Defines the modes where the help will be available
  auto mask = shcore::IShell_core::all_scripting_modes();

  // Creates the help topic for the object
  help->add_help_topic(def->name, shcore::Topic_type::PROPERTY, def->name,
                       m_qualified_name, mask,
                       shcore::Keyword_location::LOCAL_CTX);

  // Now registers the object brief, parameters and details
  auto object = shcore::str_upper(m_name);
  auto prefix = object + "_" + shcore::str_upper(def->name);
  if (!def->brief.empty()) {
    help->add_help(prefix, "BRIEF", def->brief,
                   shcore::Keyword_location::LOCAL_CTX);
  }

  size_t member_sequence = 0;

  if (!def->details.empty()) {
    help->add_help(prefix, "DETAIL", &member_sequence, def->details,
                   shcore::Keyword_location::LOCAL_CTX);
  }
}

void Extensible_object::register_function_help(
    const std::string &name, const std::string &brief,
    const std::vector<std::string> &params,
    const std::vector<std::string> &details,
    const Function_definition::Examples &examples) {
  auto help = shcore::Help_registry::get();

  auto names = shcore::str_split(name, "|");

  shcore::Help_topic *topic =
      help->get_topic(m_qualified_name + "." + names[0], true);

  if (!topic) {
    // Defines the modes where the help will be available
    auto mask = shcore::IShell_core::all_scripting_modes();

    // Creates the help topic for the function
    help->add_help_topic(name, shcore::Topic_type::FUNCTION, names[0],
                         m_qualified_name, mask,
                         shcore::Keyword_location::LOCAL_CTX);

    // Now registers the function brief, parameters and details
    auto prefix = shcore::str_upper(m_name + "_" + names[0]);
    if (!brief.empty()) help->add_help(prefix, "BRIEF", brief);
    help->add_help(prefix, "PARAM", params,
                   shcore::Keyword_location::LOCAL_CTX);
    help->add_help(prefix, "DETAIL", details,
                   shcore::Keyword_location::LOCAL_CTX);
    help->add_help(prefix, examples, shcore::Keyword_location::LOCAL_CTX);
  } else {
    topic->set_enabled(true);
  }
}

void Extensible_object::register_function_help(
    const std::shared_ptr<Function_definition> &definition) {
  // Param details are inserted at the end of the function details by default.
  // If @PARAMDETAILS entry is found on the function details, the parameter
  // details wil be inserted right there.
  bool param_details = true;

  std::vector<std::string> params;
  std::vector<std::string> details;

  for (const auto &detail : definition->details) {
    if (!detail.empty()) {
      if (detail == "@PARAMDETAILS") {
        for (const auto &param : definition->parameters) {
          get_param_help_detail(*param, true, &params);
        }

        param_details = false;
      } else {
        details.emplace_back(detail);
      }
    }
  }

  for (const auto &param : definition->parameters) {
    get_param_help_brief(*param, true, &params);

    if (param_details) {
      get_param_help_detail(*param, true, &details);
    }
  }

  register_function_help(definition->name, definition->brief, params, details,
                         definition->examples);
}

void Extensible_object::get_param_help_brief(
    const Parameter_definition &param_definition, bool as_parameter,
    std::vector<std::string> *target) {
  std::string param_help(as_parameter ? "@param " : "@li ");
  const auto &param = param_definition.parameter;

  param_help += param->name;

  if (!as_parameter) {
    param_help += ":";
  } else {
    if (!param_definition.is_required()) param_help += " Optional";
  }

  size_t help_size = param_help.size();
  if (param->type() != shcore::Value_type::Undefined)
    param_help += " " + to_string(param->type());

  if (!as_parameter && param_definition.is_required())
    param_help += " (required)";

  if (!param_definition.brief.empty()) {
    if (param_help.size() > help_size) param_help.append(" -");
    param_help.append(" " + param_definition.brief);
  }

  if (!shcore::str_endswith(param_help, ".")) param_help.append(".");

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
        help_entry +=
            " accepts the following values: " + shcore::str_join(values, ", ") +
            ".";
        details->emplace_back(std::move(help_entry));
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

void Extensible_object::validate_name(const std::string &name,
                                      const std::string &label,
                                      bool custom_names) {
  std::vector<std::string> names;
  if (custom_names)
    names = shcore::str_split(name, "|");
  else
    names = {name};

  if (names.size() > 2) {
    throw shcore::Exception::argument_error(
        "Invalid function name. When using custom names only two names "
        "should "
        "be provided.");
  }

  for (const auto &cname : names) {
    if (!shcore::is_valid_identifier(cname)) {
      throw shcore::Exception::argument_error(
          "The " + label + " name '" + cname + "' is not a valid identifier.");
    }
  }
}

void Extensible_object::validate_function(
    const std::shared_ptr<Function_definition> &definition) {
  int index = 0;
  shcore::Parameter_context context{"", {{"", {}}}};
  for (const auto &param : definition->parameters) {
    context.levels[0].name = "parameter";
    context.levels[0].position = ++index;
    validate_parameter(*param->parameter, &context);
  }
}

void Extensible_object::validate_parameter(const shcore::Parameter &param,
                                           shcore::Parameter_context *context) {
  bool valid_identifier = shcore::is_valid_identifier(param.name);

  std::string current_ctx;

  if (valid_identifier) {
    context->levels.back().name += " '" + param.name + "'";
    context->levels.back().position = nullptr;
  }

  current_ctx = context->str();

  if (!valid_identifier) {
    const auto error =
        shcore::str_format("%s is not a valid identifier: '%s'.",
                           current_ctx.c_str(), param.name.c_str());
    throw shcore::Exception::argument_error(error);
  }

  if (param.type() == shcore::Value_type::Map) {
    const auto &options =
        param.validator<shcore::Option_validator>()->allowed();
    // Options must be defined
    if (!options.empty()) {
      int index = 0;

      for (const auto &option : options) {
        context->levels.push_back({"option", ++index});
        validate_parameter(*option, context);
        context->levels.pop_back();
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

std::string Extensible_object::get_help_id() const {
  auto tokens = shcore::split_string(m_qualified_name, ".");
  std::vector<std::string> id_tokens;
  for (const auto &token : tokens) {
    id_tokens.push_back(
        shcore::get_member_name(token, shcore::current_naming_style()));
  }
  return shcore::str_join(id_tokens, ".");
}

}  // namespace mysqlsh

namespace shcore {
namespace detail {

std::shared_ptr<mysqlsh::Extensible_object>
Type_info<std::shared_ptr<mysqlsh::Extensible_object>>::to_native(
    const shcore::Value &in) {
  std::shared_ptr<mysqlsh::Extensible_object> object;

  if (in.type == shcore::Object)
    object = in.as_object<mysqlsh::Extensible_object>();

  if (!object) {
    throw shcore::Exception::type_error(
        "is expected to be " +
        Type_info<std::shared_ptr<mysqlsh::Extensible_object>>::desc());
  }

  return object;
}

}  // namespace detail
}  // namespace shcore
