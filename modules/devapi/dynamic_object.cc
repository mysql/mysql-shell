/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "dynamic_object.h"

#include "db/mysqlx/mysqlx_parser.h"

#include "modules/devapi/base_database_object.h"
#include "modules/devapi/crud_definition.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "utils/utils_general.h"

using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

std::vector<std::string> Dynamic_object::get_members() const {
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  size_t last = 0;
  for (size_t i = 0, c = members.size(); i < c; i++) {
    // filter out disabled functions
    if (is_enabled(members[i]) && members[i] != "__shell_hook__") {
      members[last++] = members[i];
    }
  }
  members.resize(last);
  return members;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the
 * scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this
 * function. The content of the returned value depends on the property being
 * requested. The next list shows the valid properties as well as the returned
 * value for each of them:
 *
 * This object represents objects that can dynamically enable or disable
 * functions. The functions will only be returned if they are found and enabled,
 * otherwise a shcore::Exception will be thrown:
 *
 * \li Invalid object member: when the function is not found.
 * \li Forbidden usage of: when the function found but is disabled.
 *
 */
#endif
Value Dynamic_object::get_member(const std::string &prop) const {
  Value tmp(Cpp_object_bridge::get_member(prop));
  if (tmp && prop != "help") {
    if (is_enabled(prop)) {
      return tmp;
    } else {
      throw shcore::Exception::logic_error("Invalid chaining of method '" +
                                           prop + "'");
    }
  }
  throw shcore::Exception::attrib_error("Invalid object member " + prop);
}

bool Dynamic_object::has_member(const std::string &prop) const {
  return Cpp_object_bridge::has_member(prop) && is_enabled(prop);
}

Value Dynamic_object::call(const std::string &name,
                           const shcore::Argument_list &args) {
  if (!is_enabled(name)) {
    throw shcore::Exception::logic_error("Invalid chaining of method '" + name +
                                         "'");
  }
  return Cpp_object_bridge::call(name, args);
}

/*
* This method registers the "dynamic" behavior of the functions exposed by the
* object.
* Parameters:
*   - name: indicates the exposed function to be enabled/disabled.
*   - enable_after: indicate the "states" under which the function should be
* enabled.
*/
void Dynamic_object::register_dynamic_function(
    const std::string &name, const std::string &enable_after) {
  // Adds the function to the enabled/disabled state registry
  _enabled_functions[name] = true;

  // Splits the 'enable' states and associates them to the function
  std::vector<std::string> tokens =
      shcore::split_string_chars(enable_after, ", ", true);
  std::set<std::string> after(tokens.begin(), tokens.end());
  _enable_paths[name] = after;
}

void Dynamic_object::update_functions(const std::string &source) {
  std::map<std::string, bool>::iterator it, end = _enabled_functions.end();

  for (it = _enabled_functions.begin(); it != end; it++) {
    size_t count = _enable_paths[it->first].count(source);
    enable_function(it->first.c_str(), count > 0);
  }
}

void Dynamic_object::enable_function(const char *name, bool enable) {
  if (_enabled_functions.find(name) != _enabled_functions.end())
    _enabled_functions[name] = enable;
}

bool Dynamic_object::is_enabled(const std::string &name) const {
  auto func = lookup_function(name);
  if (func) {
    // filter out disabled functions
    auto it = _enabled_functions.find(func->name(LowerCamelCase));
    if (it != _enabled_functions.end() && it->second)
      return true;
  }
  return false;
}
