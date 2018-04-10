/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_mysql_constants.h"
#include <memory>
#include <string>
#include "modules/devapi/base_constants.h"
#include "scripting/object_factory.h"
#include "shellcore/utils_help.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

REGISTER_HELP(MYSQL_TYPE_BRIEF, "Data type constants");

namespace mysqlsh {
namespace mysql {

Type::Type() {
  add_constant("NULL");
  add_constant("BIT");
  add_constant("TINYINT");
  add_constant("SMALLINT");
  add_constant("MEDIUMINT");
  add_constant("INT");
  add_constant("BIGINT");
  add_constant("FLOAT");
  add_constant("DECIMAL");
  add_constant("DOUBLE");
  add_constant("JSON");
  add_constant("STRING");
  add_constant("BYTES");
  add_constant("TIME");
  add_constant("DATE");
  add_constant("DATETIME");
  add_constant("SET");
  add_constant("ENUM");
  add_constant("GEOMETRY");
}

shcore::Value Type::get_member(const std::string &prop) const {
  shcore::Value ret_val = mysqlsh::Constant::get_constant(
      "mysql", "Type", prop, shcore::Argument_list());

  if (!ret_val) ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

std::shared_ptr<shcore::Object_bridge> Type::create(
    const shcore::Argument_list &args) {
  args.ensure_count(0, "mysql.Type");

  std::shared_ptr<Type> ret_val(new Type());

  return ret_val;
}

}  // namespace mysql
}  // namespace mysqlsh
