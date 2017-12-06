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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_TABLE_DELETE_H_
#define MODULES_DEVAPI_MOD_MYSQLX_TABLE_DELETE_H_

#include <memory>
#include <string>
#include "modules/devapi/table_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Table;

/**
* \ingroup XDevAPI
* Handler for Delete operation on Tables.
*/
class TableDelete : public Table_crud_definition,
                    public std::enable_shared_from_this<TableDelete> {
 public:
  explicit TableDelete(std::shared_ptr<Table> owner);

 public:
  virtual std::string class_name() const { return "TableDelete"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
  shcore::Value remove(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);
#if DOXYGEN_JS
  TableDelete delete ();
  TableDelete where(String searchCondition);
  TableDelete orderBy(List sortExprStr);
  TableDelete limit(Integer numberOfRows);
  TableDelete bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  TableDelete delete ();
  TableDelete where(str searchCondition);
  TableDelete order_by(list sortExprStr);
  TableDelete limit(int numberOfRows);
  TableDelete bind(str name, Value value);
  Result execute();
#endif
private:
  Mysqlx::Crud::Delete message_;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_DELETE_H_
