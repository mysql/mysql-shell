/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _ORDERBY_PARSER_H_
#define _ORDERBY_PARSER_H_

#include "expr_parser.h"
#include "utils/utils_string.h"

#include <memory>

namespace mysqlx {
class Orderby_parser : public Expr_parser {
 public:
  Orderby_parser(const std::string &expr_str, bool document_mode = false);

  template <typename Container>
  void parse(Container &result) {
    Mysqlx::Crud::Order *colid = result.Add();
    column_identifier(*colid);

    if (_tokenizer.tokens_available()) {
      const mysqlx::Token &tok = _tokenizer.peek_token();
      throw Parser_error("Expected end of expression", tok,
                         _tokenizer.get_input());
    }
  }

  // const std::string& id();
  void column_identifier(Mysqlx::Crud::Order &orderby_expr);

  std::vector<Token>::const_iterator begin() const {
    return _tokenizer.begin();
  }

  std::vector<Token>::const_iterator end() const { return _tokenizer.end(); }
};

}  // namespace mysqlx
#endif
