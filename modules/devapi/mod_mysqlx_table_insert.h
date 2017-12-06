/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_TABLE_INSERT_H_
#define MODULES_DEVAPI_MOD_MYSQLX_TABLE_INSERT_H_

#include <memory>
#include <string>
#include "modules/devapi/table_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Table;

/**
* \ingroup XDevAPI
* Handler for Insert operations on Tables.
*/
class TableInsert : public Table_crud_definition,
                    public std::enable_shared_from_this<TableInsert> {
 public:
#if DOXYGEN_JS
  TableInsert insert();
  TableInsert insert(List columns);
  TableInsert insert(String col1, String col2, ...);
  TableInsert values(Value value, Value value, ...);
  Result execute();
#elif DOXYGEN_PY
  TableInsert insert();
  TableInsert insert(list columns);
  TableInsert insert(str col1, str col2, ...);
  TableInsert values(Value value, Value value, ...);
  Result execute();
#endif
  explicit TableInsert(std::shared_ptr<Table> owner);
  virtual std::string class_name() const { return "TableInsert"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
  shcore::Value insert(const shcore::Argument_list &args);
  shcore::Value values(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);
private:
  Mysqlx::Crud::Insert message_;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_INSERT_H_
