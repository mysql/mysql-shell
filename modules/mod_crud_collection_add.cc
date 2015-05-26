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
#include "mod_crud_collection_add.h"

using namespace mysh;
using namespace shcore;

CollectionAdd::CollectionAdd(const shcore::Argument_list &args) :
Crud_definition(args)
{
  // _conn, schema, collection
  args.ensure_count(3, "CollectionAdd");

  std::string path;
  _data.reset(new shcore::Value::Map_type());
  (*_data)["data_model"] = Value(Mysqlx::Crud::DOCUMENT);
  (*_data)["schema"] = args[1];
  (*_data)["collection"] = args[2];

  // Exposes the methods available for chaining
  add_method("add", boost::bind(&CollectionAdd::add, this, _1), "data");
  add_method("bind", boost::bind(&CollectionAdd::bind, this, _1), "data");
  add_method("execute", boost::bind(&Crud_definition::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("add", "");
  register_dynamic_function("bind", "add, addDocuments");
  register_dynamic_function("execute", "addDocuments, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionAdd::add(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionAdd::add");

  // default path is with no arguments
  std::string path;
  if (args.size())
  {
    path = "Documents";
    (*_data)[path] = args[0];
  }

  // Updates the exposed functions
  update_functions("add" + path);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionAdd::bind(const shcore::Argument_list &args)
{
  // TODO: Logic to determine the kind of parameter passed
  //       Should end up adding one of the next to the data dictionary:
  //       - ValuesAndnestedFinds
  //       - ParamsValuesAndNestedFinds
  //       - IteratorObject

  // Updates the exposed functions
  update_functions("bind");

  return Value(Object_bridge_ref(this));
}

boost::shared_ptr<shcore::Object_bridge> CollectionAdd::create(const shcore::Argument_list &args)
{
  args.ensure_count(3, 4, "CollectionAdd()");
  return boost::shared_ptr<shcore::Object_bridge>(new CollectionAdd(args));
}