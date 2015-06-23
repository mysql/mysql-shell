/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
#include <boost/bind.hpp>
#include "mod_crud_table_insert.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

/*
* Class constructor represents the call to the first method on the
* call chain, on this case insert.
* It will reveive not only the parameter documented on the insert function
* but also other initialization data for the object:
* - The connection represents the intermediate class in charge of actually
*   creating the message.
* - Message information that is not provided through the different functions
*/
TableInsert::TableInsert(const shcore::Argument_list &args) :
Crud_definition()
{
  args.ensure_count(3, "TableInsert");

  /*std::string path;
  _data.reset(new shcore::Value::Map_type());
  (*_data)["data_model"] = shcore::Value(Mysqlx::Crud::TABLE);
  (*_data)["schema"] = args[1];
  (*_data)["collection"] = args[2];*/

  // The values function should not be enabled if values were already given
  add_method("insert", boost::bind(&TableInsert::insert, this, _1), "data");
  add_method("values", boost::bind(&TableInsert::values, this, _1), "data");
  add_method("bind", boost::bind(&TableInsert::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("insert", "");
  register_dynamic_function("values", "insert, insertFields, values");
  register_dynamic_function("bind", "insert, insertFields, insertFieldsAndValues, values");
  register_dynamic_function("execute", "insertFields, insertFieldsAndValues, values, bind");

  // Initial function update
  update_functions("");
}

shcore::Value TableInsert::insert(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableInsert::insert");

  std::string path;
  if (args.size())
  {
    switch (args[0].type)
    {
      case Array:
        path = "Fields";
        break;
      case Map:
        path = "FieldsAndValues";
        break;
      default:
        throw shcore::Exception::argument_error("Invalid data received on TableInsert::insert");
    }

    // Stores the data
    //(*_data)[path] = args[0];
  }

  // Updates the exposed functions
  update_functions("insert" + path);

  return Value(Object_bridge_ref(this));
}

shcore::Value TableInsert::values(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableInsert::values");

  // Adds the parameters to the data map
  //(*_data)["Values"] = args[0];

  // Updates the exposed functions
  update_functions("values");

  // Returns the same object
  return Value(Object_bridge_ref(this));
}

shcore::Value TableInsert::bind(const shcore::Argument_list &UNUSED(args))
{
  // TODO: Logic to determine the kind of parameter passed
  //       Should end up adding one of the next to the data dictionary:
  //       - ValuesAndSubQueries
  //       - ParamsValuesAndSubQueries
  //       - IteratorObject

  // Updates the exposed functions
  update_functions("bind");

  return Value(Object_bridge_ref(this));
}

/*boost::shared_ptr<shcore::Object_bridge> TableInsert::create(const shcore::Argument_list &args)
{
args.ensure_count(3, 4, "TableInsert()");
return boost::shared_ptr<shcore::Object_bridge>(new TableInsert(args));
}*/