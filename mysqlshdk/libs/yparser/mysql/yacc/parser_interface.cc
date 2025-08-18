/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/yparser/mysql/yacc/parser_interface.h"

#include <string_view>
#include <utility>

// server's header
#include "router/src/routing/src/sql_parser_state.h"
// generated header
#include "sql/sql_yacc.h"

#include "mysqlshdk/libs/yparser/mysql/yacc/sql_lex_hints.h"

namespace {

using Token = SqlLexer::iterator::Token;

// 0 == YYEOF
constexpr int k_yyeof = 0;

std::size_t compute_line_offset(const char *start, const char *current,
                                std::size_t token_length) {
  // line is 1-based
  std::size_t offset = 1;

  // compute the offset
  while (--current >= start && *current != '\n') {
    ++offset;
  }

  offset -= token_length;

  return offset;
}

void report_parser_error(internal::Parser_context *context, std::string msg,
                         const Token &token, std::size_t line,
                         const char *start, const char *current) {
  constexpr std::string_view k_eof = "<EOF>";

  const auto is_eof = k_yyeof == token.id || END_OF_INPUT == token.id;
  const auto token_length = token.text.length();
  const auto token_offset = current - start - token_length;

  // both line and offset are 1-based
  internal::Parser_error error{
      std::move(msg), line, compute_line_offset(start, current, token_length),
      token_offset, is_eof ? 1 : token_length};

  // construct the error message
  error.message.append(": near '", 8);
  error.message.append(is_eof ? k_eof : token.text);
  error.message.append(1, '\'');

  context->errors.emplace_back(std::move(error));
}

}  // namespace

void mysqlsh_sql_parser_error(internal::Parser_context *context,
                              const char *s) {
  const auto &lip = context->thd->m_parser_state->m_lip;

  report_parser_error(context, s, *context->iter, lip.yylineno, lip.get_buf(),
                      lip.get_ptr());
}

int mysqlsh_sql_parser_lex(MYSQLSH_SQL_PARSER_STYPE *,
                           internal::Parser_context *context) {
  if (context->end) {
    return k_yyeof;
  }

  ++context->iter;

  if (END_OF_INPUT == context->iter->id || ABORT_SYM == context->iter->id) {
    // no more tokens
    context->end = true;
  }

  return context->iter->id;
}

void mysqlsh_hint_parser_error(internal::Parser_context *context,
                               const char *s) {
  const auto &lip = context->thd->m_parser_state->m_lip;
  const auto hint_scanner = context->hint_scanner;
  Token token{{hint_scanner->yytext, hint_scanner->yyleng},
              hint_scanner->token()};

  report_parser_error(context, std::string(s) + " (optimizer hints)", token,
                      hint_scanner->get_lineno(), lip.get_buf(),
                      hint_scanner->get_ptr());
}

int mysqlsh_hint_parser_lex(MYSQLSH_HINT_PARSER_STYPE *,
                            internal::Parser_context *context) {
  assert(context->hint_scanner);
  return context->hint_scanner->get_next_token();
}
