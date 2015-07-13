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

#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_table.h"
#include <boost/bind.hpp>

#include "mod_mysqlx_table_insert.h"
#include "mod_mysqlx_table_delete.h"
//#include "mod_mysqlx_table_select.h"
//#include "mod_mysqlx_table_update.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Table::Table(boost::shared_ptr<Schema> owner, const std::string &name)
: DatabaseObject(owner->_session.lock(), boost::static_pointer_cast<DatabaseObject>(owner), name), _table_impl(owner->_schema_impl->getTable(name))
{
  init();
}

Table::Table(boost::shared_ptr<const Schema> owner, const std::string &name) :
DatabaseObject(owner->_session.lock(), boost::const_pointer_cast<Schema>(owner), name)
{
  init();
}

void Table::init()
{
  add_method("insert", boost::bind(&Table::insert_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("modify", boost::bind(&Table::update_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("select", boost::bind(&Table::select_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("delete", boost::bind(&Table::delete_, this, _1), "searchCriteria", shcore::String, NULL);
}

Table::~Table()
{
}

shcore::Value Table::insert_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableInsert> tableInsert(new TableInsert(shared_from_this()));

  return tableInsert->insert(args);
}

shcore::Value Table::update_(const shcore::Argument_list &args)
{
  /*std::string searchCriteria;
  args.ensure_count(1, "Collection::modify");
  searchCriteria = args.string_at(0);
  // return shcore::Value::wrap(new CollectionFind(_coll->find(searchCriteria)));*/
  return shcore::Value();
}

shcore::Value Table::delete_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableDelete> tableDelete(new TableDelete(shared_from_this()));

  return tableDelete->remove(args);
}

shcore::Value Table::select_(const shcore::Argument_list &args)
{
  boost::shared_ptr<TableSelect> tableSelect(new TableSelect(shared_from_this()));

  return tableSelect->select(args);
}