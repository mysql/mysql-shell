/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_TABLE_CRUD_DEFINITION_H_
#define MODULES_DEVAPI_TABLE_CRUD_DEFINITION_H_

#include "db/mysqlx/expr_parser.h"
#include "modules/devapi/crud_definition.h"
#include "mysqlxtest_utils.h"
#include "scripting/common.h"
#include "scripting/types_cpp.h"

#include <memory>
#include <set>

namespace mysqlsh {
namespace mysqlx {
#if DOXYGEN_CPP
//! Base class for table CRUD operations.
#endif
class Table_crud_definition : public Crud_definition {
 public:
  explicit Table_crud_definition(std::shared_ptr<DatabaseObject> owner)
      : Crud_definition(owner) {}

 protected:
  void parse_string_expression(::Mysqlx::Expr::Expr *expr,
                               const std::string &expr_str) override {
    ::mysqlx::Expr_parser parser(expr_str, false);
    // FIXME the parser should be changed to encode into the provided object
    expr->CopyFrom(*parser.expr());
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_TABLE_CRUD_DEFINITION_H_
