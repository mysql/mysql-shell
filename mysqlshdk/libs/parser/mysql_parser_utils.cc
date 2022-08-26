/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/parser/mysql_parser_utils.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"

namespace mysqlshdk {
namespace parser {

void prepare_lexer_parser(parsers::MySQLLexer *lexer,
                          parsers::MySQLParser *parser,
                          const mysqlshdk::utils::Version &mysql_version,
                          bool ansi_quotes, bool no_backslash_escapes) {
  lexer->sqlMode = parsers::MySQLRecognizerCommon::NoMode;
  if (ansi_quotes)
    lexer->sqlMode = parsers::MySQLRecognizerCommon::SqlMode(
        lexer->sqlMode | parsers::MySQLRecognizerCommon::AnsiQuotes);
  if (no_backslash_escapes)
    lexer->sqlMode = parsers::MySQLRecognizerCommon::SqlMode(
        lexer->sqlMode | parsers::MySQLRecognizerCommon::NoBackslashEscapes);
  parser->sqlMode = lexer->sqlMode;

  if (mysql_version) {
    lexer->serverVersion = mysql_version.numeric();
  } else {
    lexer->serverVersion = mysqlshdk::utils::Version(MYSH_VERSION).numeric();
  }
  parser->serverVersion = lexer->serverVersion;
}

void check_sql_syntax(const std::string &script,
                      const mysqlshdk::utils::Version &mysql_version,
                      bool ansi_quotes, bool no_backslash_escapes) {
  antlr4::ANTLRInputStream input;
  parsers::MySQLLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parsers::MySQLParser parser(&tokens);

  // TODO(alfredo) stop forcing ansi_quotes when parser fixed
  prepare_lexer_parser(&lexer, &parser, mysql_version, ansi_quotes || true,
                       no_backslash_escapes);

  std::stringstream stream(script);
  mysqlshdk::utils::iterate_sql_stream(
      &stream, 4098,
      [&](const char *stmt, size_t stmt_len, const std::string & /*delim*/,
          size_t /*lnum*/, size_t /* offs */) {
        internal::ParserErrorListener error_listener;
        parser.reset();
        lexer.reset();
        lexer.addErrorListener(&error_listener);
        parser.addErrorListener(&error_listener);
        input.load(std::string(stmt, stmt_len));
        lexer.setInputStream(&input);
        tokens.setTokenSource(&lexer);
        parser.setTokenStream(&tokens);

        parser.query();

        return true;
      },
      [](const std::string &msg) {
        throw std::runtime_error("Error splitting SQL: " + msg);
      },
      ansi_quotes);
}

}  // namespace parser
}  // namespace mysqlshdk
