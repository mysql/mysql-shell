/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/yparser/mysql/parser.h"

#include <cassert>
#include <cctype>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// server's headers
#include "include/my_thread.h"
#include "router/src/routing/src/sql_parser_state.h"
// generated header
#include "sql/sql_yacc.h"

#include "mysqlshdk/libs/yparser/mysql/yacc/parser_context.h"
#include "mysqlshdk/libs/yparser/mysql/yacc/sql_lex_hints.h"

namespace {

using sql_mode_t = THD::sql_mode_t;

// Default sql_mode is (8.0):
//  - ONLY_FULL_GROUP_BY
//  - STRICT_TRANS_TABLES
//  - NO_ZERO_IN_DATE
//  - NO_ZERO_DATE
//  - ERROR_FOR_DIVISION_BY_ZERO
//  - NO_ENGINE_SUBSTITUTION
// In 5.7 it also included NO_AUTO_CREATE_USER.
// Parser only cares about:
//  - PIPES_AS_CONCAT
//  - ANSI_QUOTES
//  - IGNORE_SPACE
//  - NO_BACKSLASH_ESCAPES
//  - HIGH_NOT_PRECEDENCE
// None of these is the default, so we simply set the default mode to zero.
constexpr sql_mode_t k_default_sql_mode = 0;

}  // namespace

struct Parser_error {
  explicit Parser_error(const char *msg) : error{msg} {}

  explicit Parser_error(std::string msg) : error{std::move(msg)} {}

  explicit Parser_error(internal::Parser_error &&e) : error(std::move(e)) {}

  Parser_error &operator=(internal::Parser_error &&e) {
    error = std::move(e);
    return *this;
  }

  internal::Parser_error error;
};

struct Parser {
  void clear_errors() noexcept {
    errors.clear();
    current_error = 0;
  }

  void add_error(const char *msg) { errors.emplace_back(Parser_error{msg}); }

  std::vector<Parser_error> errors;
  std::size_t current_error;
  sql_mode_t sql_mode = k_default_sql_mode;
};

const char *parser_version() { return PARSER_VERSION; }

Parser_result parser_create(Parser_h *parser) {
  if (NULL == parser) {
    return Parser_result::PARSER_RESULT_INVALID_HANDLE;
  }

  *parser = PARSER_INVALID_HANDLE;

  try {
    *parser = new Parser();
    my_thread_init();
    return Parser_result::PARSER_RESULT_OK;
  } catch (...) {
    return Parser_result::PARSER_RESULT_FAILURE;
  }
}

Parser_result parser_destroy(Parser_h parser) {
  delete parser;
  my_thread_end();
  return Parser_result::PARSER_RESULT_OK;
}

namespace {

constexpr const char *k_sql_mode_names[] = {
    "REAL_AS_FLOAT",               // 0x000000001
    "PIPES_AS_CONCAT",             // 0x000000002
    "ANSI_QUOTES",                 // 0x000000004
    "IGNORE_SPACE",                // 0x000000008
    "NOT_USED",                    // 0x000000010
    "ONLY_FULL_GROUP_BY",          // 0x000000020
    "NO_UNSIGNED_SUBTRACTION",     // 0x000000040
    "NO_DIR_IN_CREATE",            // 0x000000080
    "POSTGRESQL",                  // 0x000000100, removed in 8.0
    "ORACLE",                      // 0x000000200, removed in 8.0
    "MSSQL",                       // 0x000000400, removed in 8.0
    "DB2",                         // 0x000000800, removed in 8.0
    "MAXDB",                       // 0x000001000, removed in 8.0
    "NO_KEY_OPTIONS",              // 0x000002000, removed in 8.0
    "NO_TABLE_OPTIONS",            // 0x000004000, removed in 8.0
    "NO_FIELD_OPTIONS",            // 0x000008000, removed in 8.0
    "MYSQL323",                    // 0x000010000, removed in 8.0
    "MYSQL40",                     // 0x000020000, removed in 8.0
    "ANSI",                        // 0x000040000
    "NO_AUTO_VALUE_ON_ZERO",       // 0x000080000
    "NO_BACKSLASH_ESCAPES",        // 0x000100000
    "STRICT_TRANS_TABLES",         // 0x000200000
    "STRICT_ALL_TABLES",           // 0x000400000
    "NO_ZERO_IN_DATE",             // 0x000800000
    "NO_ZERO_DATE",                // 0x001000000
    "ALLOW_INVALID_DATES",         // 0x002000000
    "ERROR_FOR_DIVISION_BY_ZERO",  // 0x004000000
    "TRADITIONAL",                 // 0x008000000
    "NO_AUTO_CREATE_USER",         // 0x010000000, removed in 8.0
    "HIGH_NOT_PRECEDENCE",         // 0x020000000
    "NO_ENGINE_SUBSTITUTION",      // 0x040000000
    "PAD_CHAR_TO_FULL_LENGTH",     // 0x080000000
    "TIME_TRUNCATE_FRACTIONAL",    // 0x100000000, added in 8.0
    nullptr,
};

constexpr bool k_is_5_7 = PARSER_IS_5_7;

constexpr bool k_sql_mode_enabled[] = {
    true,       // 0x000000001
    true,       // 0x000000002
    true,       // 0x000000004
    true,       // 0x000000008
    true,       // 0x000000010
    true,       // 0x000000020
    true,       // 0x000000040
    true,       // 0x000000080
    k_is_5_7,   // 0x000000100
    k_is_5_7,   // 0x000000200
    k_is_5_7,   // 0x000000400
    k_is_5_7,   // 0x000000800
    k_is_5_7,   // 0x000001000
    k_is_5_7,   // 0x000002000
    k_is_5_7,   // 0x000004000
    k_is_5_7,   // 0x000008000
    k_is_5_7,   // 0x000010000
    k_is_5_7,   // 0x000020000
    true,       // 0x000040000
    true,       // 0x000080000
    true,       // 0x000100000
    true,       // 0x000200000
    true,       // 0x000400000
    true,       // 0x000800000
    true,       // 0x001000000
    true,       // 0x002000000
    true,       // 0x004000000
    true,       // 0x008000000
    k_is_5_7,   // 0x010000000
    true,       // 0x020000000
    true,       // 0x040000000
    true,       // 0x080000000
    !k_is_5_7,  // 0x100000000
};

[[maybe_unused]] constexpr sql_mode_t MODE_REAL_AS_FLOAT = 1ULL << 0;
[[maybe_unused]] constexpr sql_mode_t MODE_PIPES_AS_CONCAT = 1ULL << 1;
[[maybe_unused]] constexpr sql_mode_t MODE_ANSI_QUOTES = 1ULL << 2;
[[maybe_unused]] constexpr sql_mode_t MODE_IGNORE_SPACE = 1ULL << 3;
[[maybe_unused]] constexpr sql_mode_t MODE_NOT_USED = 1ULL << 4;
[[maybe_unused]] constexpr sql_mode_t MODE_ONLY_FULL_GROUP_BY = 1ULL << 5;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_UNSIGNED_SUBTRACTION = 1ULL << 6;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_DIR_IN_CREATE = 1ULL << 7;
[[maybe_unused]] constexpr sql_mode_t MODE_POSTGRESQL = 1ULL << 8;
[[maybe_unused]] constexpr sql_mode_t MODE_ORACLE = 1ULL << 9;
[[maybe_unused]] constexpr sql_mode_t MODE_MSSQL = 1ULL << 10;
[[maybe_unused]] constexpr sql_mode_t MODE_DB2 = 1ULL << 11;
[[maybe_unused]] constexpr sql_mode_t MODE_MAXDB = 1ULL << 12;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_KEY_OPTIONS = 1ULL << 13;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_TABLE_OPTIONS = 1ULL << 14;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_FIELD_OPTIONS = 1ULL << 15;
[[maybe_unused]] constexpr sql_mode_t MODE_MYSQL323 = 1ULL << 16;
[[maybe_unused]] constexpr sql_mode_t MODE_MYSQL40 = 1ULL << 17;
[[maybe_unused]] constexpr sql_mode_t MODE_ANSI = 1ULL << 18;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_AUTO_VALUE_ON_ZERO = 1ULL << 19;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_BACKSLASH_ESCAPES = 1ULL << 20;
[[maybe_unused]] constexpr sql_mode_t MODE_STRICT_TRANS_TABLES = 1ULL << 21;
[[maybe_unused]] constexpr sql_mode_t MODE_STRICT_ALL_TABLES = 1ULL << 22;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_ZERO_IN_DATE = 1ULL << 23;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_ZERO_DATE = 1ULL << 24;
[[maybe_unused]] constexpr sql_mode_t MODE_ALLOW_INVALID_DATES = 1ULL << 25;
[[maybe_unused]] constexpr sql_mode_t MODE_ERROR_FOR_DIVISION_BY_ZERO = 1ULL
                                                                        << 26;
[[maybe_unused]] constexpr sql_mode_t MODE_TRADITIONAL = 1ULL << 27;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_AUTO_CREATE_USER = 1ULL << 28;
[[maybe_unused]] constexpr sql_mode_t MODE_HIGH_NOT_PRECEDENCE = 1ULL << 29;
[[maybe_unused]] constexpr sql_mode_t MODE_NO_ENGINE_SUBSTITUTION = 1ULL << 30;
[[maybe_unused]] constexpr sql_mode_t MODE_PAD_CHAR_TO_FULL_LENGTH = 1ULL << 31;
[[maybe_unused]] constexpr sql_mode_t MODE_TIME_TRUNCATE_FRACTIONAL = 1ULL
                                                                      << 32;

sql_mode_t expand_sql_mode(sql_mode_t sql_mode) {
  if (sql_mode & MODE_ANSI) {
    sql_mode |= (MODE_REAL_AS_FLOAT | MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES |
                 MODE_IGNORE_SPACE | MODE_ONLY_FULL_GROUP_BY);
  }

  if (sql_mode & MODE_TRADITIONAL) {
    // NOTE: NO_AUTO_CREATE_USER is a 5.7 mode, we set it for completeness, but
    // it's simply not going to be used anywhere
    sql_mode |= (MODE_STRICT_TRANS_TABLES | MODE_STRICT_ALL_TABLES |
                 MODE_NO_ZERO_IN_DATE | MODE_NO_ZERO_DATE |
                 MODE_ERROR_FOR_DIVISION_BY_ZERO | MODE_NO_AUTO_CREATE_USER |
                 MODE_NO_ENGINE_SUBSTITUTION);
  }

  // 5.7 combination modes:

  if (sql_mode & MODE_DB2) {
    sql_mode |=
        (MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES | MODE_IGNORE_SPACE |
         MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS | MODE_NO_FIELD_OPTIONS);
  }

  if (sql_mode & MODE_MAXDB) {
    sql_mode |= (MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES | MODE_IGNORE_SPACE |
                 MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS |
                 MODE_NO_FIELD_OPTIONS | MODE_NO_AUTO_CREATE_USER);
  }

  if (sql_mode & MODE_MSSQL) {
    sql_mode |=
        (MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES | MODE_IGNORE_SPACE |
         MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS | MODE_NO_FIELD_OPTIONS);
  }

  if (sql_mode & MODE_MYSQL323) {
    sql_mode |= MODE_HIGH_NOT_PRECEDENCE;
  }

  if (sql_mode & MODE_MYSQL40) {
    sql_mode |= MODE_HIGH_NOT_PRECEDENCE;
  }

  if (sql_mode & MODE_ORACLE) {
    sql_mode |= (MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES | MODE_IGNORE_SPACE |
                 MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS |
                 MODE_NO_FIELD_OPTIONS | MODE_NO_AUTO_CREATE_USER);
  }

  if (sql_mode & MODE_POSTGRESQL) {
    sql_mode |=
        (MODE_PIPES_AS_CONCAT | MODE_ANSI_QUOTES | MODE_IGNORE_SPACE |
         MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS | MODE_NO_FIELD_OPTIONS);
  }

  return sql_mode;
}

char toupper(char ch) {
  return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
}

/**
 * Returns 0 on error, otherwise position in array + 1;
 */
std::size_t find_sql_mode(const char *sql_mode, size_t length) {
  const auto end = sql_mode + length;
  const char *i;
  const char *j;

  for (std::size_t pos = 0; (j = k_sql_mode_names[pos++]);) {
    for (i = sql_mode; i != end && toupper(*i) == *j; ++i, ++j) {
    }

    if (i == end && !*j) {
      return pos;
    }
  }

  return 0;
}

sql_mode_t parse_sql_mode(const char *sql_mode, size_t length,
                          std::vector<Parser_error> *errors) {
  static constexpr char separator = ',';
  sql_mode_t mode = 0;

  if (0 == length) {
    return mode;
  }

  auto start = sql_mode;
  const auto end = start + length;

  while (true) {
    auto pos = start;

    while (pos != end && *pos != separator) {
      ++pos;
    }

    if (const auto mode_length = pos - start) {
      auto idx = find_sql_mode(start, mode_length);

      if (!idx) {
        if (errors) {
          std::string error = "Unknown SQL mode: ";
          error.append(start, mode_length);
          errors->emplace_back(std::move(error));
        }
      } else {
        --idx;

        if (k_sql_mode_enabled[idx]) {
          mode |= 1ULL << idx;
        } else if (errors) {
          std::string error = "Unsupported SQL mode: ";
          error.append(start, mode_length);
          errors->emplace_back(std::move(error));
        }
      }
    }

    if (pos >= end) {
      break;
    }

    start = pos + 1;
  }

  return expand_sql_mode(mode);
}

}  // namespace

Parser_result parser_set_sql_mode(Parser_h parser, const char *sql_mode,
                                  size_t length) {
  if (PARSER_INVALID_HANDLE == parser) {
    return Parser_result::PARSER_RESULT_INVALID_HANDLE;
  }

  parser->clear_errors();
  parser->sql_mode = parse_sql_mode(sql_mode, length, &parser->errors);

  if (!parser->errors.empty()) {
    parser->sql_mode = k_default_sql_mode;
    return Parser_result::PARSER_RESULT_FAILURE;
  }

  return Parser_result::PARSER_RESULT_OK;
}

namespace {

// context is needed by the hint scanner
thread_local internal::Parser_context *g_current_context = nullptr;

}  // namespace

Parser_result parser_check_syntax(Parser_h parser, const char *statement,
                                  size_t length) {
  if (PARSER_INVALID_HANDLE == parser) {
    return Parser_result::PARSER_RESULT_INVALID_HANDLE;
  }

  parser->clear_errors();

  SqlParserState parser_state;
  parser_state.statement(std::string_view{statement, length});
  parser_state.thd()->variables.sql_mode = parser->sql_mode;
  parser_state.thd()->variables.character_set_client =
      &my_charset_utf8mb4_0900_ai_ci;
  parser_state.thd()->variables.default_collation_for_utf8mb4 =
      &my_charset_utf8mb4_0900_ai_ci;

  internal::Parser_context context(parser_state.thd());

  g_current_context = &context;
  const auto rc = mysqlsh_sql_parser_parse(&context);
  g_current_context = nullptr;

  // hints grammar uses the `error` token to allow for syntax errors (i.e.
  // unrecognized hints), in such case a syntax error is reported, the rest of
  // the optimizer comment is discarded, and query is executed; we want to
  // report unknown hints, hence we treat these as errors
  if (0 == rc && context.errors.empty()) {
    return Parser_result::PARSER_RESULT_OK;
  } else {
    parser->errors.insert(parser->errors.end(),
                          std::make_move_iterator(context.errors.begin()),
                          std::make_move_iterator(context.errors.end()));
    return Parser_result::PARSER_RESULT_FAILURE;
  }
}

size_t parser_pending_errors(Parser_h parser) {
  if (PARSER_INVALID_HANDLE == parser) {
    return 0;
  }

  return parser->errors.size() - parser->current_error;
}

Parser_error_h parser_next_error(Parser_h parser) {
  if (PARSER_INVALID_HANDLE == parser) {
    return PARSER_INVALID_HANDLE;
  }

  if (parser->current_error >= parser->errors.size()) {
    return PARSER_INVALID_HANDLE;
  }

  return &parser->errors[parser->current_error++];
}

const char *parser_error_message(const Parser_error_h error) {
  if (PARSER_INVALID_HANDLE == error) {
    return NULL;
  }

  return error->error.message.c_str();
}

size_t parser_error_token_offset(const Parser_error_h error) {
  if (PARSER_INVALID_HANDLE == error) {
    return 0;
  }

  return error->error.token_offset;
}

size_t parser_error_token_length(const Parser_error_h error) {
  if (PARSER_INVALID_HANDLE == error) {
    return 0;
  }

  return error->error.token_length;
}

size_t parser_error_line(const Parser_error_h error) {
  if (PARSER_INVALID_HANDLE == error) {
    return 0;
  }

  return error->error.line;
}

size_t parser_error_line_offset(const Parser_error_h error) {
  if (PARSER_INVALID_HANDLE == error) {
    return 0;
  }

  return error->error.line_offset;
}

bool consume_optimizer_hints(Lex_input_stream *lip) {
  const auto state_map = lip->query_charset->state_maps->main_map;
  int whitespace = 0;
  auto c = lip->yyPeek();
  size_t newlines = 0;

  for (; state_map[c] == MY_LEX_SKIP;
       whitespace++, c = lip->yyPeekn(whitespace)) {
    if (c == '\n') newlines++;
  }

  if (lip->yyPeekn(whitespace) == '/' && lip->yyPeekn(whitespace + 1) == '*' &&
      lip->yyPeekn(whitespace + 2) == '+') {
    lip->yylineno += newlines;
    lip->yySkipn(whitespace);  // skip whitespace

    internal::Hint_scanner hint_scanner(
        lip->m_thd, lip->yylineno, lip->get_ptr(),
        lip->get_end_of_query() - lip->get_ptr());

    assert(g_current_context);
    g_current_context->hint_scanner = &hint_scanner;
    const int rc = mysqlsh_hint_parser_parse(g_current_context);
    g_current_context->hint_scanner = nullptr;

    if (rc == 2) return true;  // Bison's internal OOM error

    if (rc == 1) {
      /*
        This branch is for 2 cases:
        1. YYABORT in the hint parser grammar (we use it to process OOM errors),
        2. open commentary error.
      */
      lip->start_token();  // adjust error message text pointer to "/*+"
    }

    lip->yylineno = hint_scanner.get_lineno();
    lip->yySkipn(hint_scanner.get_ptr() - lip->get_ptr());

    return rc != 0;
  } else {
    return false;
  }
}
