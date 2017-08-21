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
