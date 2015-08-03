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
#include "mod_mysqlx_table_delete.h"
#include "mod_mysqlx_table.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

TableDelete::TableDelete(boost::shared_ptr<Table> owner)
:Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("delete", boost::bind(&TableDelete::remove, this, _1), "data");
  add_method("where", boost::bind(&TableDelete::where, this, _1), "data");
  add_method("orderBy", boost::bind(&TableDelete::order_by, this, _1), "data");
  add_method("limit", boost::bind(&TableDelete::limit, this, _1), "data");
  add_method("bind", boost::bind(&TableDelete::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("delete", "");
  register_dynamic_function("where", "delete");
  register_dynamic_function("orderBy", "delete, where");
  register_dynamic_function("limit", "delete, where, orderBy");
  register_dynamic_function("bind", "delete, where, orderBy, limit");
  register_dynamic_function("execute", "delete, where, orderBy, limit, bind");

  // Initial function update
  update_functions("");
}

shcore::Value TableDelete::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, "TableDelete::delete");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement.reset(new ::mysqlx::DeleteStatement(table->_table_impl->remove()));

      // Updates the exposed functions
      update_functions("delete");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::delete");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableDelete::where(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableDelete::where");

  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  if (table)
  {
    try
    {
      _delete_statement->where(args.string_at(0));

      // Updates the exposed functions
      update_functions("where");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::where");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableDelete::order_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete::orderBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Order criteria can not be empty");

    _delete_statement->orderBy(fields);

    update_functions("orderBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::orderBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableDelete::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "TableDelete::limit");

  try
  {
    _delete_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableDelete::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value TableDelete::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("TableDelete::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

shcore::Value TableDelete::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableDelete::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_delete_statement->execute())));
}