/*
 * Copyright (c) 2015, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Interactive Expression access module
// Exposed as "Expression" in the shell

#ifndef MODULES_DEVAPI_MOD_MYSQLX_EXPRESSION_H_
#define MODULES_DEVAPI_MOD_MYSQLX_EXPRESSION_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types/cpp.h"

namespace mysqlsh {
namespace mysqlx {

class SHCORE_PUBLIC Expression : public shcore::Cpp_object_bridge {
 public:
  explicit Expression(std::string expression) {
    add_property("data");
    _data = std::move(expression);
  }
  virtual ~Expression() noexcept = default;

  // Virtual methods from object bridge
  virtual std::string class_name() const { return "Expression"; }
  virtual bool operator==(const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);

  std::string get_data() { return _data; }

 private:
  std::string _data;
};

}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_EXPRESSION_H_
