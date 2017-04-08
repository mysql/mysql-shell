/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "proj_parser.h"

#include "crud_definition.h"
#include "base_database_object.h"
#include "mod_mysqlx_expression.h"
#include "utils/utils_string.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

Crud_definition::Crud_definition(std::shared_ptr<DatabaseObject> owner) : _owner(owner) {
  try {
    add_method("__shell_hook__", std::bind(&Crud_definition::execute, this, _1), "data");
    add_method("execute", std::bind(&Crud_definition::execute, this, _1), "data");
  } catch (shcore::Exception &e) {
    // Invalid typecast exception is the only option
    // The exception is recreated with a more explicit message
    throw shcore::Exception::argument_error("Invalid connection used on CRUD operation.");
  }
}

shcore::Value Crud_definition::execute(const shcore::Argument_list &UNUSED(args)) {
  // TODO: Callback handling logic
  shcore::Value ret_val;
  /*std::shared_ptr<mysqlsh::X_connection> connection(_conn.lock());

  if (connection)
  ret_val = connection->crud_execute(class_name(), _data);*/

  return ret_val;
}

void Crud_definition::parse_string_list(const shcore::Argument_list &args, std::vector<std::string> &data) {
  // When there is 1 argument, it must be either an array of strings or a string
  if (args.size() == 1 && args[0].type != Array && args[0].type != String)
    throw shcore::Exception::argument_error("Argument #1 is expected to be a string or an array of strings");

  if (args.size() == 1 && args[0].type == Array) {
    Value::Array_type_ref shell_fields = args.array_at(0);
    Value::Array_type::const_iterator index, end = shell_fields->end();

    int count = 0;
    for (index = shell_fields->begin(); index != end; index++) {
      count++;
      if (index->type != shcore::String)
        throw shcore::Exception::argument_error(str_format("Element #%d is expected to be a string", count));
      else
        data.push_back(index->as_string());
    }
  } else {
    for (size_t index = 0; index < args.size(); index++)
      data.push_back(args.string_at(index));
  }
}
