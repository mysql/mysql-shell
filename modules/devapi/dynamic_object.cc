/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/dynamic_object.h"

#include "db/mysqlx/mysqlx_parser.h"

#include "modules/devapi/base_database_object.h"
#include "modules/devapi/crud_definition.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "utils/utils_general.h"

#ifdef _WIN32
#include <intrin.h>
#endif

namespace mysqlsh {
namespace mysqlx {
using shcore::Value;

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

void Dynamic_object::register_dynamic_function(
    Allowed_function_mask name, Allowed_function_mask enable_after) {
  // name must be a power of 2 and not 0
  assert((name & (name - 1)) == 0 && name != 0);

  // Adds the function to the enabled/disabled state registry
  enabled_functions_ |= name;

#ifdef _WIN32
  DWORD x = 0;
  (void)_BitScanForward(&x, name);
#else
  size_t x = __builtin_ctz(name);
#endif
  // We can't register more functions than enabled_paths_ can store.
  assert(static_cast<size_t>(x) < shcore::array_size(enabled_paths_));

  enabled_paths_[x] = enable_after;
}

void Dynamic_object::update_functions(Allowed_function_mask f) {
  // f must be a power of 2 and not 0
  assert((f & (f - 1)) == 0 && f != 0);

  for (size_t i = 0; i < shcore::array_size(enabled_paths_); i++) {
    bool enable = enabled_paths_[i] & f;
    uint32_t name = (1U << i);

    // enable function
    if (enable) {
      enabled_functions_ |= name;
    } else {
      enabled_functions_ &= ~name;
    }
  }
}

bool Dynamic_object::is_enabled(const std::string &name) const {
  auto func = lookup_function(name);
  if (func) {
    // filter out disabled functions
    auto f = function_name_to_bitmask(func->name(shcore::LowerCamelCase));
    return f & enabled_functions_;
  }
  return false;
}
}  // namespace mysqlx
}  // namespace mysqlsh
