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

#ifndef _EXPR_PARSER_PROJ_H_
#define _EXPR_PARSER_PROJ_H_

#define _EXPR_PARSER_HAS_PROJECTION_KEYWORDS_ 1

#include <memory>

#include "expr_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlx {
class Proj_parser : public Expr_parser {
 public:
  Proj_parser(const std::string &expr_str, bool document_mode = false,
              bool allow_alias = true);

  template <typename Container>
  void parse(Container &result) {
    Mysqlx::Crud::Projection *colid = result.Add();
    source_expression(*colid);

    if (_tokenizer.tokens_available()) {
      const mysqlx::Token &tok = _tokenizer.peek_token();
      throw Parser_error("Expected end of expression", tok,
                         _tokenizer.get_input());
    }
  }

  const std::string &id();
  void source_expression(Mysqlx::Crud::Projection &column);
};
}  // namespace mysqlx
#endif
