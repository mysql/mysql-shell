/*
   Copyright (c) 2014, 2024, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* A lexical scanner for optimizer hints pseudo-commentary syntax */

#include "mysqlshdk/libs/yparser/mysql/yacc/sql_lex_hints.h"

namespace internal {

// this constant is defined in "sql/system_variables.h", but including it gives
// some warnings
constexpr THD::sql_mode_t MODE_ANSI_QUOTES = 4;

/**
  Constructor.

  @param thd_arg          The thread handler.
  @param lineno_arg       The starting line number of a hint string in a query.
  @param buf              The rest of a query buffer with hints at the start.
  @param len              The length of the buf.
  @param digest_state_arg The digest buffer to output scanned token data.
*/
Hint_scanner::Hint_scanner(THD *thd_arg, size_t lineno_arg, const char *buf,
                           size_t len)
    : thd(thd_arg),
      cs(thd->charset()),
      is_ansi_quotes(thd->variables.sql_mode & MODE_ANSI_QUOTES),
      lineno(lineno_arg),
      char_classes(cs->state_maps->hint_map),
      input_buf(buf),
      input_buf_end(input_buf + len),
      ptr(input_buf + 3),  // skip "/*+"
      prev_token(0),
      raw_yytext(ptr),
      yytext(ptr),
      yyleng(0),
      has_hints(false) {}

int Hint_scanner::scan() {
  int whitespaces = 0;
  for (;;) {
    start_token();
    switch (peek_class()) {
      case HINT_CHR_NL:
        skip_newline();
        whitespaces++;
        continue;
      case HINT_CHR_SPACE:
        skip_byte();
        whitespaces++;
        continue;
      case HINT_CHR_DIGIT:
        return scan_number_or_multiplier_or_ident();
      case HINT_CHR_IDENT:
        return scan_ident_or_keyword();
      case HINT_CHR_MB:
        return scan_ident();
      case HINT_CHR_QUOTE:
        return scan_quoted<HINT_CHR_QUOTE>();
      case HINT_CHR_BACKQUOTE:
        return scan_quoted<HINT_CHR_BACKQUOTE>();
      case HINT_CHR_DOUBLEQUOTE:
        return scan_quoted<HINT_CHR_DOUBLEQUOTE>();
      case HINT_CHR_ASTERISK:
        if (peek_class2() == HINT_CHR_SLASH) {
          ptr += 2;  // skip "*/"
          input_buf_end = ptr;
          return HINT_CLOSE;
        } else
          return get_byte();
      case HINT_CHR_AT:
        if (prev_token == '(' ||
            (prev_token == HINT_ARG_IDENT && whitespaces == 0))
          return scan_query_block_name();
        else
          return get_byte();
      case HINT_CHR_DOT:
        return scan_fraction_digits();
      case HINT_CHR_EOF:
        return 0;
      default:
        return get_byte();
    }
  }
}

}  // namespace internal
