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
#include "mod_mysqlx_table_select.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

TableSelect::TableSelect(boost::shared_ptr<Table> owner)
: Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("select", boost::bind(&TableSelect::select, this, _1), "data");
  add_method("where", boost::bind(&TableSelect::where, this, _1), "data");
  add_method("groupBy", boost::bind(&TableSelect::group_by, this, _1), "data");
  add_method("having", boost::bind(&TableSelect::having, this, _1), "data");
  add_method("orderBy", boost::bind(&TableSelect::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableSelect::limit, this, _1), "data");
  add_method("offset", boost::bind(&TableSelect::offset, this, _1), "data");
  add_method("bind", boost::bind(&TableSelect::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("select", "");
  register_dynamic_function("where", "select");
  register_dynamic_function("groupBy", "select, where");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("orderBy", "select, where, groupBy, having");
  register_dynamic_function("limit", "select, where, groupBy, having, orderBy");
  register_dynamic_function("offset", "limit");
  register_dynamic_function("bind", "select, where, groupBy, having, orderBy, offset, limit");
  register_dynamic_function("execute", "select, where, groupBy, having, orderBy, offset, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value TableSelect::select(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableSelect::select");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      std::vector<std::string> fields;

      if (args.size())
      {
        Value::Array_type_ref shell_fields = args.array_at(0);
        Value::Array_type::const_iterator index, end = shell_fields->end();

        int count = 0;
        for (index = shell_fields->begin(); index != end; index++)
        {
          count++;
          if (index->type != shcore::String)
            throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a string") % count).str());
          else
            fields.push_back(index->as_string());
        }

        if (fields.size() == 0)
          throw shcore::Exception::argument_error("Field selection criteria can not be empty");
      }

      _select_statement.reset(new ::mysqlx::SelectStatement(table->_table_impl->select(fields)));

      // Updates the exposed functions
      update_functions("select");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::select");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::where(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::where");

  try
  {
    _select_statement->where(args.string_at(0));

    update_functions("where");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::where");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::groupBy");

  try
  {
    Value::Array_type_ref shell_fields = args.array_at(0);
    Value::Array_type::const_iterator index, end = shell_fields->end();

    std::vector<std::string> fields;

    int count = 0;
    for (index = shell_fields->begin(); index != end; index++)
    {
      count++;
      if (index->type != shcore::String)
        throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a string") % count).str());
      else
        fields.push_back(index->as_string());
    }

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _select_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::having");

  try
  {
    _select_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::orderBy");

  try
  {
    _select_statement->orderBy(args.string_at(0));

    // Remove and update test suite when orderBy is enabled
    throw shcore::Exception::logic_error("not yet implemented.");

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::limit");

  try
  {
    _select_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::offset(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableSelect::offset");

  try
  {
    _select_statement->offset(args.uint_at(0));

    update_functions("offset");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableSelect::offset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableSelect::bind: not yet implemented.");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableSelect::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableSelect::execute");

  return shcore::Value::wrap(new mysqlx::Resultset(boost::shared_ptr< ::mysqlx::Result>(_select_statement->execute())));
}