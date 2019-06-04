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

#include "orderby_parser.h"

using namespace mysqlx;

Orderby_parser::Orderby_parser(const std::string &expr_str, bool document_mode)
    : Expr_parser(expr_str, document_mode) {}

/*
 * document_mode = false:
 *   column_identifier ::= expr ( ASC | DESC )?
 */
void Orderby_parser::column_identifier(Mysqlx::Crud::Order &orderby_expr) {
  orderby_expr.set_allocated_expr(my_expr().release());

  if (_tokenizer.cur_token_type_is(Token::Type::ASC)) {
    orderby_expr.set_direction(Mysqlx::Crud::Order_Direction_ASC);
    _tokenizer.consume_token(Token::Type::ASC);
  } else if (_tokenizer.cur_token_type_is(Token::Type::DESC)) {
    orderby_expr.set_direction(Mysqlx::Crud::Order_Direction_DESC);
    _tokenizer.consume_token(Token::Type::DESC);
  }
}
