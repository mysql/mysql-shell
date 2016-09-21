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

#ifndef _MOD_CRUD_TABLE_SELECT_H_
#define _MOD_CRUD_TABLE_SELECT_H_

#include "table_crud_definition.h"

namespace mysh {
namespace mysqlx {
class Table;

/**
* Handler for record selection on a Table.
*
* This object provides the necessary functions to allow selecting record data from a table.
*
* This object should only be created by calling the select function on the table object from which the record data will be retrieved.
*
* \sa Table
*/
class TableSelect : public Table_crud_definition, public std::enable_shared_from_this<TableSelect> {
public:
#if DOXYGEN_JS
  TableSelect select(List searchExprStr);
  TableSelect where(String searchCondition);
  TableSelect groupBy(List searchExprStr);
  TableSelect having(String searchCondition);
  TableSelect orderBy(List sortExprStr);
  TableSelect limit(Integer numberOfRows);
  TableSelect offset(Integer limitOffset);
  TableSelect bind(String name, Value value);
  RowResult execute();
#elif DOXYGEN_PY
  TableSelect select(list searchExprStr);
  TableSelect where(str searchCondition);
  TableSelect group_by(list searchExprStr);
  TableSelect having(str searchCondition);
  TableSelect order_by(list sortExprStr);
  TableSelect limit(int numberOfRows);
  TableSelect offset(int limitOffset);
  TableSelect bind(str name, Value value);
  RowResult execute();
#endif
  TableSelect(std::shared_ptr<Table> owner);
  virtual std::string class_name() const { return "TableSelect"; }
  shcore::Value select(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value group_by(const shcore::Argument_list &args);
  shcore::Value having(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value offset(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);
private:
  std::unique_ptr< ::mysqlx::SelectStatement> _select_statement;
};
};
};

#endif
