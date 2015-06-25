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
#include "mod_mysqlx_collection_remove.h"
#include "mod_mysqlx_collection.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionRemove::CollectionRemove(boost::shared_ptr<Collection> owner)
:Crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // _conn, schema, collection
  //args.ensure_count(3, "CollectionRemove");

  /*std::string path;
  _data.reset(new shcore::Value::Map_type());
  (*_data)["data_model"] = Value(Mysqlx::Crud::DOCUMENT);
  (*_data)["schema"] = args[1];
  (*_data)["collection"] = args[2];*/

  // Exposes the methods available for chaining
  add_method("remove", boost::bind(&CollectionRemove::remove, this, _1), "data");
  add_method("orderBy", boost::bind(&CollectionRemove::order_by, this, _1), "data");
  add_method("limit", boost::bind(&CollectionRemove::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionRemove::bind, this, _1), "data");
  add_method("execute", boost::bind(&Crud_definition::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("remove", "");
  register_dynamic_function("orderBy", "remove, removeSearchCondition");
  register_dynamic_function("limit", "remove, removeSearchCondition, orderBy");
  register_dynamic_function("bind", "remove, removeSearchCondition, orderBy, limit");
  register_dynamic_function("execute", "remove, removeSearchCondition, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionRemove::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionRemove::remove");

  // default path is with no arguments
  std::string path;
  if (args.size())
  {
    set_filter("CollectionRemove::remove", "SearchCondition", args[0], true);

    path = "SearchCondition";
  }

  // Updates the exposed functions
  update_functions("remove" + path);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionRemove::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::orderBy");

  set_order("CollectionRemove::orderBy", "SortFields", args[0]);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionRemove::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::limit");

  if (args[0].type != shcore::Integer)
    throw shcore::Exception::argument_error("CollectionRemove::limit: integer parameter required.");

  //(*_data)["NumberOfRows"] = args[0];

  update_functions("limit");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionRemove::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionRemove::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}
/*
boost::shared_ptr<shcore::Object_bridge> CollectionRemove::create(const shcore::Argument_list &args)
{
args.ensure_count(3, 4, "CollectionRemove()");
return boost::shared_ptr<shcore::Object_bridge>(new CollectionRemove(args));
}*/