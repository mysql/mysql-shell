/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_MOD_EXTENSIBLE_OBJECT_H_
#define MODULES_MOD_EXTENSIBLE_OBJECT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/utils_help.h"

namespace mysqlsh {
/**
 * Member information to be used when exposing a member in an extensible
 * object. It contains the following information:
 *
 * name: should be given in camelCase format, the member will be exposed
 * using camelCase in JavaScript and snake_case in Python to honor naming
 * conventions, however, specific names can be given for both languages in
 * the format of jsname|pyname.
 *
 * brief: a brief description for the member (WHAT the member is).
 * details: additional information about the member.
 *
 * The information on the brief and details will be integrated into the
 * help system to be available when:
 *
 * - Querying for help on the member itself.
 * - Querying for help on the object containing the member.
 */
struct Member_definition {
  Member_definition() {}
  Member_definition(const std::string &n, const std::string &b,
                    const std::vector<std::string> &d)
      : name(n), brief(b), details(d) {}

  std::string name;
  std::string brief;
  std::vector<std::string> details;
};

/**
 * This structure holds parameter metadata to be used when registering a
 * function into an extensible object, this information includes:
 *
 * parameter: the parameter definition itself which includes:
 *
 *   - name
 *   - whether it is required or not
 *   - data validators
 *
 * brief: a brief description for the parameter (WHAT it is).
 * details: additional information about the parameter.
 *
 * The information on this structure will be integrated into the help
 * system to be available when querying information about the function
 * where the parameter is defined.
 */
struct Parameter_definition {
  using Options = std::vector<std::shared_ptr<Parameter_definition>>;

  Parameter_definition();

  Parameter_definition(const std::string &n, shcore::Value_type t,
                       shcore::Param_flag f);

  virtual ~Parameter_definition() = default;

  bool is_required() const;

  void set_options(const Options &options);
  const Options &options() const;

  std::shared_ptr<shcore::Parameter> parameter;
  std::string brief;
  std::vector<std::string> details;

 private:
  void validate_options() const;

  Options m_options;
};

/**
 * This structure plays the same role as the Member_definition struct, it holds
 * the information for all the parameters on a function exposed in an
 * extensible object.
 *
 * For more details look at Member_definition and Parameter_definition.
 */
struct Function_definition : public Member_definition {
  using Parameters = std::vector<std::shared_ptr<Parameter_definition>>;
  using Examples = std::vector<shcore::Help_registry::Example>;
  Function_definition() {}
  Function_definition(const std::string &n, const Parameters &p,
                      const std::string &b, const std::vector<std::string> &d,
                      const Examples &e = {})
      : Member_definition(n, b, d), parameters(p), examples(e) {}
  Parameters parameters;

  Examples examples;
  bool cli_enabled = false;
};

/**
 * Base class for extensible objects to be exposed on the API.
 *
 * This adds dynamic behavior where:
 *
 * - Additional objects exposed as properties can be added to this object.
 * - Additional functions can be added to this object.
 *
 * Any object that can be extended dynamically should inherit from this class.
 */
class Extensible_object
    : public std::enable_shared_from_this<Extensible_object>,
      public shcore::Cpp_object_bridge {
 public:
  Extensible_object(const std::string &name = "",
                    const std::string &qualified_name = "");
  virtual ~Extensible_object();
  std::string class_name() const override {
    return m_name.empty() ? "ExtensionObject" : m_name;
  }
  std::string get_help_id() const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value get_member(const std::string &prop) const override;
  void set_member(const std::string &prop, shcore::Value) override;
  bool has_member(const std::string &prop) const override;
  shcore::Value call(const std::string &name,
                     const shcore::Argument_list &args) override;
  shcore::Value call_advanced(const std::string &name,
                              const shcore::Argument_list &args,
                              const shcore::Dictionary_t &kwargs = {}) override;

  /**
   * Registers a new member on this object.
   *
   * @param name The name of the object to be created.
   * @param member The value to be registered.
   * @param definition Help data for the member being registered
   *
   * The member parameter can be any of s_allowed_member_types
   *
   * - If it is a function, then the member will be registered as a function.
   * - Otherwise will be registered as a property in the object.
   *
   * NOTE: Members registered through this function strictly follow naming
   *       convention, this is, members are exposed in camelCase in JavaScript
   *       in snake_case for Python.
   *
   *       In some situations it is required to break the naming convention,
   *       to achieve that, expose the member by calling the following
   *       functions:
   *
   *       - register_function(definition, function)
   *       - register_member(definition, member)
   *
   * The definition parameter is a dictionary that can contain the following
   * attributes for any member type:
   *
   * - brief: brief description of the function being registered.
   * - details: array of strings with details about the function being
   *   registered. Each entry on this list is turned into a paragraph
   *   in the help system.
   *
   * If the member being registered is a function, the following attribute is
   * also allowed:
   *
   * - parameters: list of parameters that the function accepts.
   *
   * Each parameter is defined as another dictionary where the following
   * options are allowed:
   *
   * - name: required, the name of the parameter.
   * - type: required, the parameter data type.
   * - required: boolean indicating if the parameter is required or not.
   * - brief: brief description of the parameter.
   * - details: list of strings with additional details about the parameter.
   *   Each entry in the list becomes a paragraph on the help system.
   *
   * Supported data types include:
   *
   * - string
   * - integer
   * - float
   * - bool
   * - array
   * - dictionary
   * - object
   *
   * A string parameter may also include a 'values' option with the list of
   * values allowed for the parameter.
   *
   * An object parameter may also include a 'class' optoin with the name of
   * the class that the object must be. Also may include a 'classes' option
   * with a list of allowed object types.
   *
   * A dictionary parameter must include an 'options' option which is a list
   * of options allowed on the parameter.
   *
   * The definition of each option follows the same rules as the definition
   * for a parameter.   *
   */
  void register_member(const std::string &name, const shcore::Value &member,
                       const shcore::Dictionary_t &definition);

  /**
   * Registers a new child object on this object.
   *
   * A new topic will be created for the object on the shell help system.
   *
   * @param name The name of the object to be created.
   * @param brief A brief description of the new object.
   * @param details Detailed description of the new object.
   * @param type_label: String identifying the type of object being registered.
   * @returns Shared pointer to the extensible object created within the
   * function.
   *
   * Each entry in the details list is registered in the help system, this means
   * it follows the same rules as registering the help with the REGISTER_HELP
   * macro, except that this help will NOT be rendered on the Doxygen docs.
   *
   * The type_label can be used to get custom messages related to the object
   * registration, i.e.
   *
   * - The <type_label> must be a valid identifier
   * - The module name must be a valid identifier.
   * - The report name must be a valid identifier.
   *
   * NOTE: This function is for usage from C++ only, i.e. object addition
   *       from Python/JavaScript is currently not supported and when enabled
   *       will be through the register_member(...) function.
   */
  std::shared_ptr<Extensible_object> register_object(
      const std::string &name, const std::string &brief,
      const std::vector<std::string> &details, const std::string &type_label);

  /**
   * Registers a new function on this object.
   *
   * This function is to be used internally within C++ to dynamically expose
   * a function and register help information into the help system.
   *
   * @param definition Container for help and metadata for the function
   * @param function The callback that will be registered under the specified
   * name.
   * @param custom_names determines whether the exposed function should follow
   * naming convention strictly or if it allows custom names for JavaScript and
   * Python.
   *
   * In general when coming from the user, strict naming convention should be
   * followed, not necessarily the case when coming from C++.
   */
  void register_function(const std::shared_ptr<Function_definition> &definition,
                         const shcore::Function_base_ref &function,
                         bool custom_names = true);

  /**
   * Registers a new member on this object.
   *
   * This function is to be used internally within C++ to dynamically expose
   * a member and register help information into the help system.
   *
   * @param definition Container for help and metadata for the function
   * @param member The value to be used as the exposed member.
   * @param custom_names determines whether the exposed function should follow
   * naming convention strictly or if it allows custom names for JavaScript and
   * Python.
   *
   * In general when coming from the user, strict naming convention should be
   * followed, not necessarily the case when coming from C++.
   */
  void register_property(const std::shared_ptr<Member_definition> &definition,
                         const shcore::Value &value, bool custom_names = true);

  /**
   * Utility function to register help data for this object.
   *
   * @param details The Member_definition containing the help data.
   */
  void register_help(const std::shared_ptr<Member_definition> &details,
                     bool is_global);

  /**
   * Utility function to ease the help registration from C++.
   *
   * @param brief A brief descrption for this object.
   * @param details Detailed information about this object.
   *
   * This function just creates the Member_definition structure with
   * the received data and calls the function above.
   */
  void register_help(const std::string &brief,
                     const std::vector<std::string> &details, bool is_global);

  /**
   * Utility function register help about a property registered on this object.
   *
   * @param details The member definition that contains the help data
   */
  void register_property_help(
      const std::shared_ptr<Member_definition> &details);

  /**
   * Utility function to ease the registration of help data for a function
   * from C++.
   *
   * A function topic will be registered under the specified parent topic.
   *
   * @param name The name of the function.
   * @param brief A brief description of the function.
   * @param params A list defining the parameters for the function.
   * @param details A list defining the help details for the function.
   * @param examples A list defining the examples for the function.
   *
   * The entries in the parameter list should come in the next format:
   *
   * "@param <name> [Optional] <description>"
   *
   * The format is NOT verified by this function and it is is used for the
   * proper rendering of the function signature.
   *
   * Each entry help entry in params and details is registered and follows
   * the same rules as the REGISTER_HELP macro.
   */
  void register_function_help(
      const std::string &name, const std::string &brief,
      const std::vector<std::string> &params,
      const std::vector<std::string> &details,
      const Function_definition::Examples &examples = {});

  /**
   * Sets the registered status of this object to true.
   *
   * If this object was unregistered (i.e. created through shell.createObject()
   * when this happens all the help cached for the object is registered on the
   * help system.
   *
   * Otherwise this operation does nothing.
   */
  void set_registered(const std::string &name = "",
                      bool do_register_help = true);

  void set_definition(const std::shared_ptr<Member_definition> &definition) {
    m_definition = definition;
  }

  std::shared_ptr<Extensible_object> get_parent() const {
    return m_parent.expired() ? std::shared_ptr<Extensible_object>()
                              : m_parent.lock();
  }

  bool is_registered() const { return m_registered; }
  void enable_cli() {
    m_cli_enabled = true;
    auto parent = get_parent();
    if (parent) parent->enable_cli();
  }
  bool cli_enabled() const { return m_cli_enabled; }

  std::string get_name() const { return m_name; }
  std::string get_qualified_name() const { return m_qualified_name; }

 protected:
  Extensible_object(const std::string &name, const std::string &qualified_name,
                    bool registered);

  void enable_help();

  Function_definition::Parameters parse_parameters(
      const shcore::Array_t &parameters, shcore::Parameter_context *context,
      const std::set<std::string> &allowed_types, bool as_parameters);

  virtual std::shared_ptr<Parameter_definition> start_parsing_parameter(
      const shcore::Dictionary_t &definition,
      shcore::Option_unpacker *unpacker) const;

 private:
  static void validate_name(const std::string &name, const std::string &label,
                            bool custom_names);

  static void validate_parameter(const shcore::Parameter &parameter,
                                 shcore::Parameter_context *context);

  static void validate_function(
      const std::shared_ptr<Function_definition> &parameters);

  shcore::Value_type map_type(const std::string &type,
                              const std::set<std::string> &allowed_types) const;
  std::shared_ptr<Parameter_definition> parse_parameter(
      const shcore::Dictionary_t &definition,
      shcore::Parameter_context *context,
      const std::set<std::string> &allowed_types, bool as_parameters);

  std::shared_ptr<Function_definition> parse_function_definition(
      const shcore::Dictionary_t &definition);

  std::shared_ptr<Member_definition> parse_member_definition(
      const shcore::Dictionary_t &definition);

  void register_function_help(
      const std::shared_ptr<Function_definition> &definition);

  void get_param_help_brief(const Parameter_definition &param,
                            bool as_parameter,
                            std::vector<std::string> *target);

  void get_param_help_detail(const Parameter_definition &param,
                             bool as_parameter,
                             std::vector<std::string> *details);

  void disable_help();

  void register_object(const std::shared_ptr<Extensible_object> &object);

  std::string m_name;
  std::string m_qualified_name;
  bool m_registered;
  bool m_cli_enabled = false;
  size_t m_detail_sequence;
  std::map<std::string, std::shared_ptr<Extensible_object>> m_children;
  shcore::Value::Map_type m_members;

  std::weak_ptr<Extensible_object> m_parent;
  // Cache for object, member and function definition
  std::shared_ptr<Member_definition> m_definition;
  std::vector<std::shared_ptr<Member_definition>> m_property_definition;
  std::vector<std::shared_ptr<Function_definition>> m_function_definition;
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_EXTENSIBLE_OBJECT_H_
