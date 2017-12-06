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

#include "modules/devapi/collection_crud_definition.h"
#include <memory>
#include <sstream>
#include <string>
#include "db/mysqlx/expr_parser.h"
#include "modules/devapi/mod_mysqlx_expression.h"

namespace mysqlsh {
namespace mysqlx {

std::unique_ptr<::Mysqlx::Expr::Expr>
Collection_crud_definition::encode_document_expr(shcore::Value docexpr) {
  assert(docexpr.type == shcore::Map);

  std::unique_ptr<::Mysqlx::Expr::Expr> expr(new ::Mysqlx::Expr::Expr());
  encode_expression_value(expr.get(), docexpr);
  return expr;
}

}  // namespace mysqlx
}  // namespace mysqlsh
