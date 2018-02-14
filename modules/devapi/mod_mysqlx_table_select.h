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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_TABLE_SELECT_H_
#define MODULES_DEVAPI_MOD_MYSQLX_TABLE_SELECT_H_

#include <memory>
#include <string>
#include "modules/devapi/table_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Table;

/**
* \ingroup XDevAPI
* Handler for record selection on a Table.
*
* This object provides the necessary functions to allow selecting record data
* from a table.
*
* This object should only be created by calling the select function on the table
* object from which the record data will be retrieved.
*
* \sa Table
*/
class TableSelect : public Table_crud_definition,
                    public std::enable_shared_from_this<TableSelect> {
 public:
#if DOXYGEN_JS
  TableSelect select(List searchExprStr);
  TableSelect where(String searchCondition);
  TableSelect groupBy(List searchExprStr);
  TableSelect having(String searchCondition);
  TableSelect orderBy(List sortExprStr);
  TableSelect limit(Integer numberOfRows);
  TableSelect offset(Integer limitOffset);
  TableSelect lockShared(String lockContention);
  TableSelect lockExclusive(String lockContention);
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
  TableSelect lock_shared(str lockContention);
  TableSelect lock_exclusive(str lockContention);
  TableSelect bind(str name, Value value);
  RowResult execute();
#endif
  explicit TableSelect(std::shared_ptr<Table> owner);
  std::string class_name() const override { return "TableSelect"; }
  shcore::Value select(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value group_by(const shcore::Argument_list &args);
  shcore::Value having(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value offset(const shcore::Argument_list &args);
  shcore::Value lock_shared(const shcore::Argument_list &args);
  shcore::Value lock_exclusive(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);

  shcore::Value execute(const shcore::Argument_list &args) override;
private:
   Mysqlx::Crud::Find message_;
   void set_lock_contention(const shcore::Argument_list &args);

  struct F {
    static constexpr Allowed_function_mask _empty = 1 << 0;
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 1;
    static constexpr Allowed_function_mask select = 1 << 2;
    static constexpr Allowed_function_mask where = 1 << 3;
    static constexpr Allowed_function_mask groupBy = 1 << 4;
    static constexpr Allowed_function_mask having = 1 << 5;
    static constexpr Allowed_function_mask orderBy = 1 << 6;
    static constexpr Allowed_function_mask limit = 1 << 7;
    static constexpr Allowed_function_mask offset = 1 << 8;
    static constexpr Allowed_function_mask lockShared = 1 << 9;
    static constexpr Allowed_function_mask lockExclusive = 1 << 10;
    static constexpr Allowed_function_mask bind = 1 << 11;
    static constexpr Allowed_function_mask execute = 1 << 12;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("" == s) { return F::_empty; }
    if ("__shell_hook__" == s) { return F::__shell_hook__; }
    if ("select" == s) { return F::select; }
    if ("where" == s) { return F::where; }
    if ("groupBy" == s) { return F::groupBy; }
    if ("having" == s) { return F::having; }
    if ("orderBy" == s) { return F::orderBy; }
    if ("limit" == s) { return F::limit; }
    if ("offset" == s) { return F::offset; }
    if ("lockShared" == s) { return F::lockShared; }
    if ("lockExclusive" == s) { return F::lockExclusive; }
    if ("bind" == s) { return F::bind; }
    if ("execute" == s) { return F::execute; }
    return 0;
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_SELECT_H_
