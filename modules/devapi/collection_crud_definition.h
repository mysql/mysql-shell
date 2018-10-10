/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_COLLECTION_CRUD_DEFINITION_H_
#define MODULES_DEVAPI_COLLECTION_CRUD_DEFINITION_H_

#include "db/mysqlx/expr_parser.h"
#include "modules/devapi/crud_definition.h"
#include "mysqlxtest_utils.h"
#include "scripting/common.h"
#include "scripting/types_cpp.h"

#include <memory>
#include <set>
#include <string>

namespace mysqlsh {
namespace mysqlx {
#if DOXYGEN_CPP
//! Base class for collection CRUD operations.
#endif
class Collection_crud_definition : public Crud_definition {
 public:
  explicit Collection_crud_definition(std::shared_ptr<DatabaseObject> owner)
      : Crud_definition(owner) {}

 protected:
  virtual void parse_string_expression(::Mysqlx::Expr::Expr *expr,
                                       const std::string &expr_str) {
    ::mysqlx::Expr_parser parser(expr_str, true, false, &_placeholders);
    // FIXME the parser should be changed to encode into the provided object
    expr->CopyFrom(*parser.expr());
  }

  std::unique_ptr<::Mysqlx::Expr::Expr> encode_document_expr(
      shcore::Value docexpr);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_COLLECTION_CRUD_DEFINITION_H_
