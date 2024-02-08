/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_CONTEXT_H_
#define MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_CONTEXT_H_

#include <cstddef>
#include <string>
#include <vector>

// server's header
#include "router/src/routing/src/sql_lexer.h"

namespace internal {

class Hint_scanner;

struct Parser_error {
  std::string message;

  // 1-based, 0 means line number is unknown
  std::size_t line = 0;
  // 1-based, 0 means offset in line is unknown
  std::size_t line_offset = 0;

  // 0-based offset of the erroneous token
  std::size_t token_offset = 0;
  // 0 means token is unknown
  std::size_t token_length = 0;
};

struct Parser_context {
  explicit Parser_context(THD *thd_)
      // NOTE: This is a hack to initialize `iter` to point before the first
      // token. This allows to always use ++operator() and postpones lexing
      // till the first call to that operator.
      : lexer(thd_), thd(thd_), iter(thd_, {}), end(lexer.end()) {}

  SqlLexer lexer;
  THD *thd;
  SqlLexer::iterator iter;
  SqlLexer::iterator end;
  std::vector<Parser_error> errors;
  Hint_scanner *hint_scanner = nullptr;
};

}  // namespace internal

#endif  // MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_CONTEXT_H_
