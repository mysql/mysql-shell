/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_TABLE_UPDATE_H_
#define MODULES_DEVAPI_MOD_MYSQLX_TABLE_UPDATE_H_

#include <memory>
#include <string>
#include "modules/devapi/table_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Table;

/**
 * \ingroup XDevAPI
 * Handler for record update operations on a Table.
 *
 * This object provides the necessary functions to allow updating records on a
 * table.
 *
 * This object should only be created by calling the update function on the
 * table object on which the records will be updated.
 *
 * \sa Table
 */
class TableUpdate : public Table_crud_definition,
                    public std::enable_shared_from_this<TableUpdate> {
 public:
#if DOXYGEN_JS
  TableUpdate update();
  TableUpdate set(String attribute, Value value);
  TableUpdate where(String expression);
  TableUpdate orderBy(List sortCriteria);
  TableUpdate orderBy(String sortCriterion[, String sortCriterion, ...]);
  TableUpdate limit(Integer numberOfRows);
  TableUpdate bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  TableUpdate update();
  TableUpdate set(str attribute, Value value);
  TableUpdate where(str expression);
  TableUpdate order_by(list sortCriteria);
  TableUpdate order_by(str sortCriterion[, str sortCriterion, ...]);
  TableUpdate limit(int numberOfRows);
  TableUpdate bind(str name, Value value);
  Result execute();
#endif
  explicit TableUpdate(std::shared_ptr<Table> owner);
  std::string class_name() const override { return "TableUpdate"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
  shcore::Value update(const shcore::Argument_list &args);
  shcore::Value set(const shcore::Argument_list &args);
  shcore::Value where(const shcore::Argument_list &args);
  shcore::Value order_by(const shcore::Argument_list &args);
  shcore::Value execute(const shcore::Argument_list &args) override;

 private:
  Mysqlx::Crud::Update message_;

  void set_prepared_stmt() override;
  void update_limits() override { set_limits_on_message(&message_); }
  shcore::Value this_object() override;

  struct F {
    static constexpr Allowed_function_mask update = 1 << 0;
    static constexpr Allowed_function_mask set = 1 << 1;
    static constexpr Allowed_function_mask where = 1 << 2;
    static constexpr Allowed_function_mask orderBy = 1 << 3;
    static constexpr Allowed_function_mask limit = 1 << 4;
    static constexpr Allowed_function_mask bind = 1 << 5;
    static constexpr Allowed_function_mask execute = 1 << 6;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("update" == s) {
      return F::update;
    }
    if ("set" == s) {
      return F::set;
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

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_UPDATE_H_
