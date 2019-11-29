/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
 * $(TABLEDELETE_BRIEF)
 *
 * Handler for Delete operation on Tables.
 */
class TableDelete : public Table_crud_definition,
                    public std::enable_shared_from_this<TableDelete> {
 public:
  explicit TableDelete(std::shared_ptr<Table> owner);

 public:
  std::string class_name() const override { return "TableDelete"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
  shcore::Value remove(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value execute(const shcore::Argument_list &args) override;
#if DOXYGEN_JS
  TableDelete delete ();
  TableDelete where(String expression);
  TableDelete orderBy(List sortCriteria);
  TableDelete orderBy(String sortCriterion[, String sortCriterion, ...]);
  TableDelete limit(Integer numberOfRows);
  TableDelete bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  TableDelete delete ();
  TableDelete where(str expression);
  TableDelete order_by(list sortCriteria);
  TableDelete order_by(str sortCriterion[, str sortCriterion, ...]);
  TableDelete limit(int numberOfRows);
  TableDelete bind(str name, Value value);
  Result execute();
#endif
 private:
  Mysqlx::Crud::Delete message_;

  void set_prepared_stmt() override;
  void update_limits() override { set_limits_on_message(&message_); }
  shcore::Value this_object() override;

  struct F {
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 0;
    static constexpr Allowed_function_mask delete_ = 1 << 1;
    static constexpr Allowed_function_mask where = 1 << 2;
    static constexpr Allowed_function_mask orderBy = 1 << 3;
    static constexpr Allowed_function_mask limit = 1 << 4;
    static constexpr Allowed_function_mask bind = 1 << 5;
    static constexpr Allowed_function_mask execute = 1 << 6;
  };

  uint32_t function_name_to_bitmask(const std::string &s) const override {
    if ("__shell_hook__" == s) {
      return F::__shell_hook__;
    }
    if ("delete" == s) {
      return F::delete_;
    }
    if ("where" == s) {
      return F::where;
    }
    if ("orderBy" == s) {
      return F::orderBy;
    }
    if ("limit" == s) {
      return F::limit;
    }
    if ("bind" == s) {
      return F::bind;
    }
    if ("execute" == s) {
      return F::execute;
    }
    if ("help" == s) {
      return enabled_functions_;
    }
    return 0;
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_DELETE_H_
