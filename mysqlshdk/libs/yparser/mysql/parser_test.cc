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

#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>

#include "mysqlshdk/libs/yparser/mysql/parser.h"

namespace {

void usage(const char *name) {
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << name << " \"sql-statement\" [sql-mode]" << std::endl;
}

void fail(const char *msg) { std::cout << "ERROR: " << msg << std::endl; }

void fail(const char *msg, Parser_h parser, std::string_view stmt = {}) {
  fail(msg);

  if (!parser_pending_errors(parser)) {
    std::cout << "  unknown error" << std::endl;
    return;
  }

  Parser_error_h error = PARSER_INVALID_HANDLE;

  while (PARSER_INVALID_HANDLE != (error = parser_next_error(parser))) {
    const auto what = parser_error_message(error);
    const auto line = parser_error_line(error);
    const auto line_offset = parser_error_line_offset(error);

    std::cout << "  ";

    if (line) {
      std::cout << line << ':' << (line_offset ? line_offset : -1) << ": ";
    }

    std::cout << what << std::endl;

    const auto token_offset = parser_error_token_offset(error);
    const auto token_length = parser_error_token_length(error);

    if (token_length) {
      std::cout << "  Erroneous token: ";

      if (token_offset == stmt.length()) {
        std::cout << "<EOF>";
      } else if (token_offset > stmt.length()) {
        std::cout << "parser provided invalid offset";
      } else {
        std::cout << stmt.substr(token_offset, token_length);
      }

      std::cout << std::endl;
    }
  }
}

}  // namespace

int main(int argc, const char *argv[]) {
  if (argc < 2 || argc > 3) {
    usage(argv[0]);
    return 1;
  }

  std::cout << "Using parser: " << parser_version() << std::endl;

  Parser_h parser = PARSER_INVALID_HANDLE;

  if (Parser_result::PARSER_RESULT_OK != parser_create(&parser)) {
    fail("failed to create the parser");
    return 1;
  }

  std::unique_ptr<std::remove_pointer_t<Parser_h>, decltype(&parser_destroy)>
      deleter{parser, &parser_destroy};

  if (argc > 2) {
    if (Parser_result::PARSER_RESULT_OK !=
        parser_set_sql_mode(parser, argv[2], ::strlen(argv[2]))) {
      fail("failed to set the sql_mode:", parser);
      return 1;
    }
  }

  std::string_view stmt{argv[1]};

  if (Parser_result::PARSER_RESULT_OK !=
      parser_check_syntax(parser, stmt.data(), stmt.length())) {
    fail("parsing failed:", parser, stmt);
    return 1;
  } else {
    std::cout << "OK" << std::endl;
    return 0;
  }
}
