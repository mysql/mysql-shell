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
#include "mod_mysqlx_table_update.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include "mod_mysqlx_expression.h"
#include <sstream>

using namespace mysh::mysqlx;
using namespace shcore;

TableUpdate::TableUpdate(boost::shared_ptr<Table> owner)
:Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("update", boost::bind(&TableUpdate::update, this, _1), "data");
  add_method("set", boost::bind(&TableUpdate::set, this, _1), "data");
  add_method("where", boost::bind(&TableUpdate::where, this, _1), "data");
  add_method("orderBy", boost::bind(&TableUpdate::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableUpdate::limit, this, _1), "data");
  add_method("bind", boost::bind(&TableUpdate::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("update", "");
  register_dynamic_function("set", "update");
  register_dynamic_function("where", "set");
  register_dynamic_function("orderBy", "set, where");
  register_dynamic_function("limit", "set, where, orderBy");
  register_dynamic_function("bind", "set, where, orderBy, limit");
  register_dynamic_function("execute", "set, where, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value TableUpdate::update(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, "TableUpdate::update");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _update_statement.reset(new ::mysqlx::UpdateStatement(table->_table_impl->update()));

      // Updates the exposed functions
      update_functions("update");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate::update");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableUpdate::set(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableUpdate::set");

  try
  {
    shcore::Value::Map_type_ref values = args.map_at(0);
    shcore::Value::Map_type::iterator index, end = values->end();

    // Iterates over the values to be updated
    for (index = values->begin(); index != end; index++)
    {
      std::string field = index->first;
      shcore::Value value = index->second;

      // Only expression objects are allowed as values
      std::string expr_data;
      if (value.type == shcore::Object)
      {
        shcore::Object_bridge_ref object = value.as_object();

        boost::shared_ptr<Expression> expression = boost::dynamic_pointer_cast<Expression>(object);

        if (expression)
          expr_data = expression->get_data();
        else
        {
          std::stringstream str;
          str << "TableUpdate::set: Unsupported value received for table update operation on field \"" << field << "\", received: " << value.descr();
          throw shcore::Exception::argument_error(str.str());
        }
      }

      // Calls set for each of the values
      if (!expr_data.empty())
        _update_statement->set(field, expr_data);
      else
        _update_statement->set(field, map_table_value(value));

      update_functions("set");
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate::set");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableUpdate::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableUpdate::where");

  try
  {
    _update_statement->where(args.string_at(0));

    // Updates the exposed functions
    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate::where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableUpdate::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableUpdate::orderBy");

  try
  {
    _update_statement->orderBy(args.string_at(0));

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate::orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableUpdate::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableUpdate::limit");

  try
  {
    _update_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableUpdate::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableUpdate::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableUpdate::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

shcore::Value TableUpdate::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableUpdate::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_update_statement->execute())));
}