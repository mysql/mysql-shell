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
#include "mod_mysqlx_collection_modify.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include <sstream>
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionModify::CollectionModify(boost::shared_ptr<Collection> owner)
:Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("modify", boost::bind(&CollectionModify::modify, this, _1), "data");
  add_method("set", boost::bind(&CollectionModify::set, this, _1), "data");
  add_method("change", boost::bind(&CollectionModify::change, this, _1), "data");
  add_method("unset", boost::bind(&CollectionModify::unset, this, _1), "data");
  add_method("arrayInsert", boost::bind(&CollectionModify::array_insert, this, _1), "data");
  add_method("arrayAppend", boost::bind(&CollectionModify::array_append, this, _1), "data");
  add_method("arrayDelete", boost::bind(&CollectionModify::array_delete, this, _1), "data");
  add_method("sort", boost::bind(&CollectionModify::sort, this, _1), "data");
  add_method("limit", boost::bind(&CollectionModify::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionModify::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("modify", "");
  register_dynamic_function("set", "modify, operation");
  register_dynamic_function("change", "modify, operation");
  register_dynamic_function("unset", "modify, operation");
  register_dynamic_function("arrayInsert", "modify, operation");
  register_dynamic_function("arrayAppend", "modify, operation");
  register_dynamic_function("arrayDelete", "modify, operation");
  register_dynamic_function("sort", "operation");
  register_dynamic_function("limit", "operation, sort");
  register_dynamic_function("bind", "operation, sort, limit");
  register_dynamic_function("execute", "operation, sort, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionModify::modify(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionModify::modify");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _modify_statement.reset(new ::mysqlx::ModifyStatement(collection->_collection_impl->modify(search_condition)));

      // Updates the exposed functions
      update_functions("modify");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::modify");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::set(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionModify::set");

  try
  {
    shcore::Value::Map_type_ref values = args.map_at(0);
    shcore::Value::Map_type::iterator index, end = values->end();

    // Iterates over the values to be updated
    for (index = values->begin(); index != end; index++)
      _modify_statement->set(index->first, map_document_value(index->second));

    if (values->size())
      update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::set");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::change(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionModify::change");

  try
  {
    shcore::Value::Map_type_ref values = args.map_at(0);
    shcore::Value::Map_type::iterator index, end = values->end();

    // Iterates over the values to be updated
    for (index = values->begin(); index != end; index++)
      _modify_statement->change(index->first, map_document_value(index->second));

    if (values->size())
      update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::change");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::unset(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_at_least(1, "CollectionModify::unset");

  try
  {
    size_t unset_count = 0;

    // Could receive either a List or many strings
    if (args.size() == 1 && args[0].type == Array)
    {
      shcore::Value::Array_type_ref items = args.array_at(0);
      shcore::Value::Array_type::iterator index, end = items->end();

      int int_index = 0;
      for (index = items->begin(); index != end; index++)
      {
        int_index++;
        if (index->type == shcore::String)
          _modify_statement->remove(index->as_string());
        else
          throw shcore::Exception::type_error((boost::format("Element #%1% is expected to be a string") % (int_index)).str());
      }

      unset_count = items->size();
    }
    else
    {
      for (size_t index = 0; index < args.size(); index++)
      {
        if (args[index].type == shcore::String)
          _modify_statement->remove(args.string_at(index));
        else
        {
          std::string error;

          if (args.size() == 1)
            error = (boost::format("Argument #%1% is expected to be either string or list of strings") % (index + 1)).str();
          else
            error = (boost::format("Argument #%1% is expected to be a string") % (index + 1)).str();

          throw shcore::Exception::type_error(error);
        }
      }

      unset_count = args.size();
    }

    // Updates the exposed functions
    if (unset_count)
      update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::unset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::array_insert(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(3, "CollectionModify::arrayInsert");

  try
  {
    //_modify_statement.update something

    // Updates the exposed functions
    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::arrayInsert");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::array_append(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "CollectionModify::arrayAppend");

  try
  {
    std::string field = args.string_at(0);

    _modify_statement->arrayAppend(field, map_document_value(args[1]));

    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::arrayAppend");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::array_delete(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "CollectionModify::arrayDelete");

  try
  {
    //_modify_statement->

    // Updates the exposed functions
    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::arrayDelete");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionModify::sort");

  try
  {
    _modify_statement->sort(args.string_at(0));

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionModify::limit");

  try
  {
    _modify_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionModify::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionModify::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionModify::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionModify::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_modify_statement->execute())));
}