/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates.
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
  TableInsert insert(String column[, String column, ...]);
  TableInsert insert(JSON columns);
  TableInsert values(Value value[, Value value, ...]);
  Result execute();
#elif DOXYGEN_PY
  TableInsert insert();
  TableInsert insert(list columns);
  TableInsert insert(str column[, str column, ...]);
  TableInsert insert(JSON columns);
  TableInsert values(Value value[, Value value, ...]);
  Result execute();
#endif
  explicit TableInsert(std::shared_ptr<Table> owner);
  std::string class_name() const override { return "TableInsert"; }
  shcore::Value insert(const shcore::Argument_list &args);
  shcore::Value values(const shcore::Argument_list &args);

  shcore::Value execute(const shcore::Argument_list &args) override;

 private:
  Mysqlx::Crud::Insert message_;

  bool allow_prepared_statements() override { return false; }

  struct F {
    static constexpr Allowed_function_mask insert = 1 << 0;
    static constexpr Allowed_function_mask values = 1 << 1;
    static constexpr Allowed_function_mask execute = 1 << 2;
    static constexpr Allowed_function_mask insertFields = 1 << 3;
    static constexpr Allowed_function_mask insertFieldsAndValues = 1 << 4;
  };

  Allowed_function_mask function_name_to_bitmask(
      std::string_view s) const override {
    if ("insert" == s) {
      return F::insert;
    }
    if ("values" == s) {
      return F::values;
    }
    if ("execute" == s) {
      return F::execute;
    }
    if ("insertFields" == s) {
      return F::insertFields;
    }
    if ("insertFieldsAndValues" == s) {
      return F::insertFieldsAndValues;
    }
    if ("help" == s) {
      return enabled_functions_;
    }
    return 0;
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_INSERT_H_
