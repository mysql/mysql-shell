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

#include "proj_parser.h"

using namespace mysqlx;

Proj_parser::Proj_parser(const std::string &expr_str, bool document_mode,
                         bool allow_alias)
    : Expr_parser(expr_str, document_mode, allow_alias) {}

/*
 * id ::= IDENT | MUL
 */
const std::string &Proj_parser::id() {
  if (_tokenizer.cur_token_type_is(Token::IDENT))
    return _tokenizer.consume_token(Token::IDENT);
  else
    return _tokenizer.consume_token(Token::MUL);
}

/*
 * column_identifier ::= ( expr [ [AS] IDENT ] ) | ( DOLLAR [ IDENT ]
 * document_path )
 */
void Proj_parser::source_expression(Mysqlx::Crud::Projection &col) {
  if (_document_mode && _tokenizer.cur_token_type_is(Token::DOLLAR)) {
    _tokenizer.consume_token(Token::DOLLAR);
    Mysqlx::Expr::ColumnIdentifier *colid =
        col.mutable_source()->mutable_identifier();
    col.mutable_source()->set_type(Mysqlx::Expr::Expr::IDENT);
    if (_tokenizer.cur_token_type_is(Token::IDENT)) {
      const std::string &ident = _tokenizer.consume_token(Token::IDENT);
      colid->mutable_document_path()->Add()->set_value(ident.c_str(),
                                                       ident.size());
    }
    document_path(*colid);
  } else
    col.set_allocated_source(my_expr().release());

  // Sets the alias token
  if (_allow_alias) {
    if (_tokenizer.cur_token_type_is(Token::AS)) {
      _tokenizer.consume_token(Token::AS);
      const std::string &alias = _tokenizer.consume_token(Token::IDENT);
      col.set_alias(alias.c_str());
    } else if (_tokenizer.cur_token_type_is(Token::IDENT)) {
      const std::string &alias = _tokenizer.consume_token(Token::IDENT);
      col.set_alias(alias.c_str());
    } else if (_document_mode)
      col.set_alias(_tokenizer.get_input());
  }
}
