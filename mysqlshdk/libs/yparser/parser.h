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

#ifndef MYSQLSHDK_LIBS_YPARSER_PARSER_H_
#define MYSQLSHDK_LIBS_YPARSER_PARSER_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {
namespace yacc {

/**
 * An error reported by the parser.
 */
class Parser_error : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

/**
 * A syntax error reported by the parser.
 */
class Syntax_error : public Parser_error {
 public:
  /**
   * Creates a syntax error.
   *
   * @param what Error message.
   * @param token_offset Offset of the erroneous token.
   * @param token_length Length of the erroneous token.
   * @param line Line number.
   * @param line_offset Offset in line.
   */
  Syntax_error(const char *what, std::size_t token_offset,
               std::size_t token_length, std::size_t line,
               std::size_t line_offset);

  /**
   * Provides the 0-based offset of the erroneous token.
   */
  [[nodiscard]] std::size_t token_offset() const noexcept {
    return m_token_offset;
  }

  /**
   * Provides the length of the erroneous token.
   */
  [[nodiscard]] std::size_t token_length() const noexcept {
    return m_token_length;
  }

  /**
   * Provides the 0-based line number where syntax error has occurred.
   */
  [[nodiscard]] std::size_t line() const noexcept { return m_line; }

  /**
   * Provides the 0-based offset within the line where syntax error has
   * occurred.
   */
  [[nodiscard]] std::size_t line_offset() const noexcept {
    return m_line_offset;
  }

  /**
   * Formats the error message.
   */
  [[nodiscard]] std::string format() const;

 private:
  std::size_t m_token_offset;
  std::size_t m_token_length;
  std::size_t m_line;
  std::size_t m_line_offset;
};

/**
 * Validates syntax of MySQL statements.
 *
 * NOTE: Only syntax errors are reported, semantic ones are not. This means that
 *       i.e.
 *         GRANT SELECT(col) ON *.* TO user@host;
 *       is not reported as an error (grant on a column is used with *.*).
 */
class Parser final {
 public:
  Parser() = delete;

  /**
   * Creates parser, requesting syntax support for the given MySQL version. The
   * final version used by the parser is adjusted to one of the supported
   * versions.
   *
   * NOTE: It's unsafe to move this object between threads.
   *
   * @param version Requested MySQL version.
   */
  explicit Parser(const utils::Version &version);

  Parser(const Parser &) = delete;
  Parser(Parser &&) = default;

  Parser &operator=(const Parser &) = delete;
  Parser &operator=(Parser &&) = default;

  ~Parser();

  /**
   * Provides the MySQL version used by the parser (may be different from the
   * requested version).
   */
  [[nodiscard]] const utils::Version &version() const noexcept;

  /**
   * Sets the SQL mode.
   *
   * Note: by default, no SQL mode is set.
   *
   * @param sql_mode New SQL mode.
   *
   * @throws Parser_error If operation fails.
   */
  void set_sql_mode(std::string_view sql_mode);

  /**
   * Checks syntax of the given SQL statement.
   *
   * @param statement SQL statement.
   *
   * @returns Reported errors, if any.
   *
   * @throws Parser_error If operation fails.
   */
  [[nodiscard]] std::vector<Syntax_error> check_syntax(
      std::string_view statement) const;

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace yacc
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_YPARSER_PARSER_H_
