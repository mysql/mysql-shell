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

#ifndef MODULES_MOD_EXTENSIBLE_OBJECT_H_
#define MODULES_MOD_EXTENSIBLE_OBJECT_H_

#include <memory>
#include <string>
#include <vector>

#include "scripting/types_cpp.h"

namespace mysqlsh {

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

struct Function_definition {
  using Parameters = std::vector<std::shared_ptr<Parameter_definition>>;
  Parameters parameters;
  std::string brief;
  std::vector<std::string> details;
};

/**
 * Base class for extensible objects to be exposed on the API.
 *
 * This adds dynamic behavior where:
 * - Additional objects exposed as properties can be added to this object.
 * - Additional functions can be added to this object.
 *
 * Any object that can be extended dynamically should inherit from this class.
 */
class Extensible_object
    : public std::enable_shared_from_this<Extensible_object>,
      public shcore::Cpp_object_bridge {
 public:
  Extensible_object(const std::string &name, const std::string &qualified_name);
  virtual ~Extensible_object();
  virtual std::string class_name() const { return m_name; }
  virtual bool operator==(const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  /**
   * Registers a new child object on this object.
   *
   * A new topic will be created for the object on the shell help system.
   *
   * @param name The name of the object to be created.
   * @param brief A brief description of the new object.
   * @param details Detailed description of the new object.
   * @param type_label: String identifying the type of object being registered.
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
   */
  void register_object(const std::string &name, const std::string &brief,
                       const std::vector<std::string> &details,
                       const std::string &type_label);

  /**
   * Registers a new function on this object.
   *
   * This function is to be used with function data coming from JavaScript
   * or Python.
   *
   * To do the same from C++:
   * - Use the expose() to register the function and get the metadata
   * - Use the metadata to define custom validators
   * - Use the public register_function_help to register the help data
   *
   * @param name The function name in camelCase format.
   * @param function The callback that will be registered under the specified
   * name.
   * @param definition Container for help and metadata for the function
   *
   * With the name given in camelCase format, the function will be exposed
   * using camelCase in JavaScript and snake_case in Python.
   *
   * The function definition may contain the following options:
   *
   * - brief: brief description of the function being registered.
   * - details: array of strings with details about the function being
   *   registered. Each entry on this list is turned into a paragraph
   *   in the help system.
   * - parameters: list of parameters that the function accepts.
   *
   * Each parameter is defined as another dictionary where the following options
   * are allowed:
   * - name: required, the name of the parameter.
   * - type: required, the parameter data type.
   * - required: boolean indicating if the parameter is required or not.
   * - brief: brief description of the parameter.
   * - details: list of strings with additional details about the parameter.
   *   Each entry in the list becomes a paragraph on the help system.
   *
   * Supported data types include:
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
   * for a parameter.
   */
  void register_function(const std::string &name,
                         const shcore::Function_base_ref &function,
                         const shcore::Dictionary_t &definition);

  void register_function(const std::string &name,
                         const shcore::Function_base_ref &function,
                         const Function_definition &definition);

  /**
   * Searches for an object given a fully qualified name.
   *
   * @param name_chain The name of the target object.
   * @returns The target object if found, otherwise nullptr
   *
   * The name_chain comes in the format of
   *
   * parent[.child]*
   *
   * If this object is named 'parent' then the 'child' is going to be searched
   * on m_children, repeating the operation for all the names included on the
   * name_chain.
   *
   * If the final object is found, it will be returned
   */
  std::shared_ptr<Extensible_object> search_object(
      const std::string &name_chain);

  /**
   * Utility function to ease the registration of help data for this object.
   *
   * @param brief A brief description of the object.
   * @param details A list defining the details for the object.
   *
   * Each help entry in details is registered and follows the same rules as
   * the REGISTER_HELP macro.
   */
  void register_object_help(const std::string &brief,
                            const std::vector<std::string> &details);

  /**
   * Utility function to ease the registration of help data for a function
   * from C++.
   *
   * A function topic will be registered under the specified parent topic.
   *
   * @param parent The parent topic for the function topic.
   * @param name The name of the function.
   * @param brief A brief description of the function.
   * @param params A list defining the parameters for the function.
   * @param details A list defining the help details for the function.
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
  void register_function_help(const std::string &name, const std::string &brief,
                              const std::vector<std::string> &params,
                              const std::vector<std::string> &details);

 protected:
  void enable_help();

  Function_definition::Parameters parse_parameters(
      const shcore::Array_t &parameters,
      const shcore::Parameter_context &context, bool default_require);

  virtual std::shared_ptr<Parameter_definition> start_parsing_parameter(
      const shcore::Dictionary_t &definition,
      shcore::Option_unpacker *unpacker) const;

 private:
  std::string m_name;
  std::string m_qualified_name;
  std::map<std::string, std::shared_ptr<Extensible_object>> m_children;

  shcore::Value_type map_type(const std::string &value);
  std::shared_ptr<Parameter_definition> parse_parameter(
      const shcore::Dictionary_t &definition,
      const shcore::Parameter_context &context, bool default_require);
  std::shared_ptr<Extensible_object> search_object(
      std::vector<std::string> *name_chain);

  void register_function_help(const std::string &name,
                              const Function_definition &definition);

  void get_param_help_brief(const Parameter_definition &param,
                            bool as_parameter,
                            std::vector<std::string> *target);

  void get_param_help_detail(const Parameter_definition &param,
                             bool as_parameter,
                             std::vector<std::string> *details);

  static void validate_function(
      const std::string &name,
      const Function_definition::Parameters &parameters);

  static void validate_parameter(const shcore::Parameter &parameter,
                                 const shcore::Parameter_context &context);

  void disable_help();
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_EXTENSIBLE_OBJECT_H_
