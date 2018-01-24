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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_DYNAMIC_OBJECT_H_
#define MODULES_DEVAPI_DYNAMIC_OBJECT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "scripting/common.h"
#include "scripting/types_cpp.h"
#include "utils/utils_general.h"

namespace mysqlsh {
namespace mysqlx {
#if DOXYGEN_CPP
//! Layer to provide the behavior of objects that can dynamically enable/disable
//! its functions.
#endif
class Dynamic_object : public shcore::Cpp_object_bridge {
 public:
  using Allowed_function_mask = uint32_t;

 private:
  // Overrides to consider enabled/disabled status
  virtual std::vector<std::string> get_members() const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual bool has_member(const std::string &prop) const;
  virtual shcore::Value call(const std::string &name,
                             const shcore::Argument_list &args);

  // T the moment will put these since we don't really care about them
  virtual bool operator==(const Object_bridge &) const { return false; }

 protected:
  //! Holds the current dynamic functions enable/disable status
  Allowed_function_mask enabled_functions_ = 0;

  //! Holds relation of 'enabled' states for every dynamic function
  //! IMPORTANT: 16 is number of max items in `struct F` in child.
  Allowed_function_mask enabled_paths_[16] = {0};

  /**
   * Registers a dynamic function and it's associated 'enabled' states exposed
   * by the object.
   *
   * @param name Bitmask with one bit set to true, indicates function name.
   * @param enable_after Bitmask built with bit encoded function names, which
   * indicates after what functions, `name`-function should be enabled.
   */
  void register_dynamic_function(Allowed_function_mask name,
                                 Allowed_function_mask enable_after);

  //! Enable/disable functions based on the received and registered states
  void update_functions(Allowed_function_mask source);

  bool is_enabled(const std::string &name) const;

 private:
  virtual Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const = 0;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_DYNAMIC_OBJECT_H_
