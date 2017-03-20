/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "mod_mysqlx_expression.h"
#include "scripting/object_factory.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace shcore;
using namespace mysqlsh::mysqlx;

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The the content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li data: returns the String used to create this Expression.
 *
 * See the implementation of DatabaseObject for additional valid members.
 */
#endif
Value Expression::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "data")
    ret_val = Value(_data);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

bool Expression::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

std::shared_ptr<shcore::Object_bridge> Expression::create(const shcore::Argument_list &args) {
  args.ensure_count(1, "mysqlx.expr");

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error("mysqlx.expr: Argument #1 is expected to be a string");

  std::shared_ptr<Expression> expression(new Expression(args[0].as_string()));

  return expression;
}
