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
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionRemove::CollectionRemove(boost::shared_ptr<Collection> owner)
:Crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("remove", boost::bind(&CollectionRemove::remove, this, _1), "data");
  add_method("orderBy", boost::bind(&CollectionRemove::order_by, this, _1), "data");
  add_method("limit", boost::bind(&CollectionRemove::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionRemove::bind, this, _1), "data");
  add_method("execute", boost::bind(&Crud_definition::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("remove", "");
  register_dynamic_function("orderBy", "remove");
  register_dynamic_function("limit", "remove, orderBy");
  register_dynamic_function("bind", "remove, orderBy, limit");
  register_dynamic_function("execute", "remove, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionRemove::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionRemove::remove");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args[0].as_string();

      _remove_statement.reset(new ::mysqlx::RemoveStatement(collection->_collection_impl->remove(search_condition)));

      // Updates the exposed functions
      update_functions("remove");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::remove", "string");

    return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
  }
}

shcore::Value CollectionRemove::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::orderBy");

  try
  {
    _remove_statement->orderBy(args[0].as_string());

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::orderBy", "string");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionRemove::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::limit");

  try
  {
    _remove_statement->limit(args[0].as_uint());

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::limit", "integer");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionRemove::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionRemove::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionRemove::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionRemove::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_remove_statement->execute())));
}