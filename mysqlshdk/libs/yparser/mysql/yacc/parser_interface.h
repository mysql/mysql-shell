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

#ifndef MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_INTERFACE_H_
#define MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_INTERFACE_H_

// server's headers
#include "router/src/routing/src/sql_lexer_input_stream.h"
#include "sql/lexer_yystype.h"

#include "mysqlshdk/libs/yparser/mysql/yacc/parser_context.h"

using MYSQLSH_SQL_PARSER_STYPE = Lexer_yystype;
using MYSQLSH_HINT_PARSER_STYPE = Lexer_yystype;

void mysqlsh_sql_parser_error(internal::Parser_context *, const char *);
int mysqlsh_sql_parser_lex(MYSQLSH_SQL_PARSER_STYPE *,
                           internal::Parser_context *);

bool consume_optimizer_hints(Lex_input_stream *);

void mysqlsh_hint_parser_error(internal::Parser_context *, const char *);
int mysqlsh_hint_parser_lex(MYSQLSH_HINT_PARSER_STYPE *,
                            internal::Parser_context *);

#endif  // MYSQLSHDK_LIBS_YPARSER_MYSQL_YACC_PARSER_INTERFACE_H_
