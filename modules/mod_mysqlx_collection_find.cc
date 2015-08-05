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
#include <boost/format.hpp>
#include "mod_mysqlx_collection_find.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionFind::CollectionFind(boost::shared_ptr<Collection> owner)
: Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("find", boost::bind(&CollectionFind::find, this, _1), "data");
  add_method("fields", boost::bind(&CollectionFind::fields, this, _1), "data");
  add_method("groupBy", boost::bind(&CollectionFind::group_by, this, _1), "data");
  add_method("having", boost::bind(&CollectionFind::having, this, _1), "data");
  add_method("sort", boost::bind(&CollectionFind::sort, this, _1), "data");
  add_method("skip", boost::bind(&CollectionFind::skip, this, _1), "data");
  add_method("limit", boost::bind(&CollectionFind::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionFind::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("find", "");
  register_dynamic_function("fields", "find");
  register_dynamic_function("groupBy", "find, fields");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("sort", "find, fields, groupBy, having");
  register_dynamic_function("limit", "find, fields, groupBy, having, sort");
  register_dynamic_function("skip", "limit");
  register_dynamic_function("bind", "find, fields, groupBy, having, sort, skip, limit");
  register_dynamic_function("execute", "find, fields, groupBy, having, sort, skip, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionFind::find(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionFind::find");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _find_statement.reset(new ::mysqlx::FindStatement(collection->_collection_impl->find(search_condition)));

      // Updates the exposed functions
      update_functions("find");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::find");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::fields(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::fields");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Field selection criteria can not be empty");

    _find_statement->fields(fields);

    update_functions("fields");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::fields");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::groupBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _find_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::having");

  try
  {
    _find_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::sort");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _find_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::limit");

  try
  {
    _find_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::skip(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::skip");

  try
  {
    _find_statement->skip(args.uint_at(0));

    update_functions("skip");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::skip");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionFind::bind: not yet implemented.");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionFind::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_find_statement->execute())));
}