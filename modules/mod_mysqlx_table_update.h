/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_CRUD_TABLE_UPDATE_H_
#define _MOD_CRUD_TABLE_UPDATE_H_

#include "table_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Table;

/**
* \ingroup XDevAPI
* Handler for record update operations on a Table.
*
* This object provides the necessary functions to allow updating records on a table.
*
* This object should only be created by calling the update function on the table object on which the records will be updated.
*
* \sa Table
*/
class TableUpdate : public Table_crud_definition, public std::enable_shared_from_this<TableUpdate> {
public:
#if DOXYGEN_JS
  TableUpdate update();
  TableUpdate set(String attribute, Value value);
  TableUpdate where(String searchCondition);
  TableUpdate orderBy(List sortExprStr);
  TableUpdate limit(Integer numberOfRows);
  TableUpdate bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  TableUpdate update();
  TableUpdate set(str attribute, Value value);
  TableUpdate where(str searchCondition);
  TableUpdate order_by(list sortExprStr);
  TableUpdate limit(int numberOfRows);
  TableUpdate bind(str name, Value value);
  Result execute();
#endif
  TableUpdate(std::shared_ptr<Table> owner);
  virtual std::string class_name() const { return "TableUpdate"; }
  static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
  shcore::Value update(const shcore::Argument_list &args);
  shcore::Value set(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);
private:
  std::unique_ptr< ::mysqlx::UpdateStatement> _update_statement;
};
};
};

#endif
