/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_SCANNER_H_
#define MODULES_UTIL_IMPORT_TABLE_SCANNER_H_

#include <cstdint>
#include <string>

#include "modules/util/import_table/dialect.h"

namespace mysqlsh {
namespace import_table {

class Scanner {
 public:
  Scanner() = delete;

  Scanner(const Dialect &dialect, uint64_t skip_rows);

  Scanner(const Scanner &) = delete;
  Scanner(Scanner &&) = default;

  Scanner &operator=(const Scanner &) = delete;
  Scanner &operator=(Scanner &&) = default;

  ~Scanner() = default;

  /**
   * Scans the block of data for rows.
   *
   * @param data Data to be scanned.
   * @param length Length of data.
   *
   * @returns index of the first row in the block, or -1 if not found
   */
  int64_t scan(const char *data, std::size_t length) noexcept;

 private:
  struct Sequence {
    explicit Sequence(const std::string &s);

    bool used() const noexcept;

    const char *ptr;
    std::size_t length;
    int first;
  };

  /**
   * Parsing status of a single row.
   */
  enum class Row_status {
    BEGIN,
    PREFIX,
    BEGIN_OF_FIELD,
    FIELD,
  };

  /**
   * Parsing status of a single field.
   */
  enum class Field_status {
    CONTINUED,
    END_OF_FIELD,
    END_OF_LINE,
  };

  /**
   * Reads next character from the block.
   */
  int get() noexcept;

  /**
   * Pushes back character which was already read.
   *
   * @param c Character
   */
  void unget(char c) noexcept;

  /**
   * Pushes back multipe characters, first argument is the first character
   * pushed.
   */
  template <typename... chars>
  void unget(char c, chars... cs) noexcept {
    unget(c);
    unget(cs...);
  }

  /**
   * Checks if block data contains the given sequence at current position.
   *
   * NOTE: first character is NOT checked
   *
   * @param s Sequence to be checked
   *
   * @returns true if sequence is found
   */
  bool contains(const Sequence &s) noexcept;

  /**
   * Skips characters, looks only for LINES TERMINATED BY sequence, first
   * character of this sequence cannot be escaped.
   *
   * @returns true if LINES TERMINATED BY sequence was found and skipped
   */
  bool skip_row() noexcept;

  /**
   * Skips characters, looks only for LINES STARTING BY sequence, doesn't do any
   * other processing.
   *
   * @returns true if LINES STARTING BY sequence was found and skipped
   */
  bool skip_line_start() noexcept;

  /**
   * Scans a single field.
   *
   * @returns status
   */
  Field_status scan_field() noexcept;

  uint64_t m_skip_rows;

  Sequence m_fields_terminated_by;
  Sequence m_lines_starting_by;
  Sequence m_lines_terminated_by;

  int m_enclosed_char;
  int m_escaped_char;

  std::string m_stack;
  char *m_stack_bottom;
  char *m_stack_position;

  const char *m_data = nullptr;
  std::size_t m_length = 0;
  bool m_end_of_block;
  Row_status m_status = Row_status::BEGIN;
  int m_found_enclosed_char;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_SCANNER_H_
