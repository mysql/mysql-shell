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
#include "mod_crud_collection_find.h"

using namespace mysh;
using namespace shcore;

CollectionFind::CollectionFind(const shcore::Argument_list &args) :
Crud_definition(args)
{
  // _conn, schema, collection
  args.ensure_count(3, "CollectionFind");

  std::string path;
  _data.reset(new shcore::Value::Map_type());
  (*_data)["data_model"] = Value(Mysqlx::Crud::DOCUMENT);
  (*_data)["schema"] = args[1];
  (*_data)["collection"] = args[2];

  // Exposes the methods available for chaining
  add_method("find", boost::bind(&CollectionFind::find, this, _1), "data");
  add_method("fields", boost::bind(&CollectionFind::fields, this, _1), "data");
  add_method("groupBy", boost::bind(&CollectionFind::group_by, this, _1), "data");
  add_method("having", boost::bind(&CollectionFind::having, this, _1), "data");
  add_method("sort", boost::bind(&CollectionFind::sort, this, _1), "data");
  add_method("skip", boost::bind(&CollectionFind::skip, this, _1), "data");
  add_method("limit", boost::bind(&CollectionFind::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionFind::bind, this, _1), "data");
  add_method("execute", boost::bind(&Crud_definition::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("find", "");
  register_dynamic_function("fields", "find");
  register_dynamic_function("groupBy", "find, fields");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("sort", "find, fields, groupBy, having");
  register_dynamic_function("limit", "find, fields, groupBy, having, sort");
  register_dynamic_function("skip", "limit");
  register_dynamic_function("bind", "find, fields, groupBy, having, sort, skip, limit");
  register_dynamic_function("execute", "find, findSearchCondition, fields, groupBy, having, sort, skip, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionFind::find(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionFind::find");

  // default path is with no arguments
  std::string path;
  if (args.size())
  {
    set_expression("CollectionFind::find", "find.SearchCondition", args[0]);

    path = "SearchCondition";
  }

  // Updates the exposed functions
  update_functions("find" + path);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::fields(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::fields");

  set_projection("CollectionFind::fields", "fields.SearchFields", args[0]);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::groupBy");

  set_projection("CollectionFind::groupBy", "groupby.SearchFields", args[0]);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::having");

  set_projection("CollectionFind::having", "having.SearchCondition", args[0]);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::sort");

  set_projection("CollectionFind::sort", "SortFields", args[0]);

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::skip(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::skip");

  if (args[0].type != shcore::Integer)
    throw shcore::Exception::argument_error("CollectionFind::skip: integer parameter required.");

  (*_data)["LimitOffset"] = args[0];

  update_functions("skip");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::limit");

  if (args[0].type != shcore::Integer)
    throw shcore::Exception::argument_error("CollectionFind::limit: integer parameter required.");

  (*_data)["NumberOfRows"] = args[0];

  update_functions("limit");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionFind::bind(const shcore::Argument_list &args)
{
  throw shcore::Exception::logic_error("CollectionFind::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

boost::shared_ptr<shcore::Object_bridge> CollectionFind::create(const shcore::Argument_list &args)
{
  args.ensure_count(3, 4, "CollectionFind()");
  return boost::shared_ptr<shcore::Object_bridge>(new CollectionFind(args));
}