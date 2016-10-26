/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mod_mysqlx.h"
#include "base_session.h"
#include "mod_mysqlx_expression.h"
#include "mod_mysqlx_constants.h"
#include "shellcore/obj_date.h"
#include "utils/utils_help.h"

using namespace std::placeholders;
using namespace mysh::mysqlx;

REGISTER_HELP(MYSQLX_INTERACTIVE_BRIEF, "Used to work with X Protocol sessions using the MySQL X DevAPI.");

REGISTER_MODULE(Mysqlx, mysqlx) {
  add_property("Type|Type");
  add_property("IndexType|IndexType");
  //REGISTER_VARARGS_FUNCTION(Mysqlx, get_session, getSession);
  REGISTER_VARARGS_FUNCTION(Mysqlx, get_node_session, getNodeSession);
  REGISTER_VARARGS_FUNCTION(Mysqlx, date_value, dateValue);
  REGISTER_FUNCTION(Mysqlx, expr, expr, "expression", shcore::String, NULL);

  _type.reset(new Type());
  _index_type.reset(new IndexType());
}

shcore::Value Mysqlx::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "Type")
    ret_val = shcore::Value(_type);
  else if (prop == "IndexType")
    ret_val = shcore::Value(_index_type);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

// DEFINE_FUNCTION(Mysqlx, get_session) {
//   auto session = connect_session(args, mysh::SessionType::X);
//   return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(session));
// }

DEFINE_FUNCTION(Mysqlx, get_node_session) {
  auto session = connect_session(args, mysh::SessionType::Node);
  return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(session));
}

DEFINE_FUNCTION(Mysqlx, expr) {
  return shcore::Value(Expression::create(args));
}

DEFINE_FUNCTION(Mysqlx, date_value) {
  return shcore::Value(shcore::Date::create(args));
}
