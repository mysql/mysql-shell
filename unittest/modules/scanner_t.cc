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

#include "modules/util/import_table/scanner.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlsh {
namespace import_table {
namespace {

void test_scanner(const std::string &data,
                  const std::vector<std::size_t> &row_lengths,
                  const Dialect &dialect,
                  std::vector<std::size_t> skip_rows = {}) {
  SCOPED_TRACE("data: " + ::testing::PrintToString(data) +
               ", dialect: " + ::testing::PrintToString(dialect.build_sql()));

  std::vector<std::size_t> row_offsets;
  row_offsets.emplace_back(0);

  for (const auto row : row_lengths) {
    row_offsets.emplace_back(row_offsets.back() + row);
  }

  if (skip_rows.empty()) {
    for (std::size_t skip = 0; skip <= row_lengths.size(); ++skip) {
      skip_rows.emplace_back(skip);
    }
  }

  for (auto skip : skip_rows) {
    SCOPED_TRACE("skip: " + std::to_string(skip));

    for (std::size_t length = 1; length < 5; ++length) {
      SCOPED_TRACE("length: " + std::to_string(length));

      Scanner s{dialect, skip};
      std::size_t offset = 0;
      auto next_offset = skip < row_offsets.size() ? row_offsets.begin() + skip
                                                   : row_offsets.end();

      while (offset < data.length()) {
        SCOPED_TRACE("offset: " + std::to_string(offset));
        int64_t expected = -1;
        auto size = std::min(length, data.size() - offset);

        while (row_offsets.end() != next_offset && offset > *next_offset) {
          ++next_offset;
        }

        if (row_offsets.end() != next_offset && offset <= *next_offset &&
            *next_offset < offset + size) {
          expected = *next_offset - offset;
          ++next_offset;
        }

        EXPECT_EQ(expected, s.scan(data.c_str() + offset,
                                   std::min(length, data.size() - offset)));
        offset += length;
      }
    }
  }
}

TEST(Scanner, constructor) {
  {
    // empty line terminator
    Dialect dialect;
    dialect.lines_terminated_by = "";
    EXPECT_THROW(Scanner(dialect, 0), std::invalid_argument);
  }

  {
    // lines and fields are terminated with the same sequence
    Dialect dialect;
    dialect.lines_terminated_by = dialect.fields_terminated_by;
    EXPECT_THROW(Scanner(dialect, 0), std::invalid_argument);
  }
}

TEST(Scanner, skip_rows) {
  const std::string file =
      "a\r\n"   // 3
      "bb\r\n"  // 4
      "c\r\n"   // 3
      "d";      // 1

  for (const auto terminator : {"\r\n", "\n"}) {
    Dialect dialect;
    dialect.lines_terminated_by = terminator;

    test_scanner(file, {3, 4, 3, 1}, dialect);
  }
}

TEST(Scanner, lines_starting_by) {
  const std::string file_template =
      "<lsb>a\r\n"      // 3 + lines_starting_by
      "yyy<lsb>bb\r\n"  // 7 + lines_starting_by
      "cccccc\r\n"      // 8
      "<lsb>d\r\n";     // 3 + lines_starting_by

  for (std::size_t length = 1; length < 6; ++length) {
    const auto lines_starting_by = std::string(length, 'x');
    const auto file = shcore::str_subvars(
        file_template,
        [&lines_starting_by](std::string_view) { return lines_starting_by; },
        "<", ">");

    for (const auto terminator : {"\r\n", "\n"}) {
      Dialect dialect;
      dialect.lines_terminated_by = terminator;
      dialect.lines_starting_by = lines_starting_by;

      test_scanner(file, {3 + length, 7 + length, 8 + 3 + length}, dialect,
                   {0, 1, 2, 4});
      test_scanner(file, {3 + length, 7 + length, 8, 3 + length}, dialect, {3});
    }
  }
}

TEST(Scanner, rows) {
  const std::string file_template =
      "'a'<ft>\\b<ft>'c'<lt>"                 // 8 + 2 * FT + LT
      "'\\d'<ft>'ee'<lt>"                     // 8 + FT + LT
      "'ff''ff'<ft>\\'g\\<lt>\\'<lt>"         // 14 + FT + 2 * LT
      "h<ft>ii<ft>jjj<ft>kkkk<ft>'ll\r\nl'";  // 17 + 4 * FT

  for (const auto field_terminator : {"+,", ","}) {
    for (const auto line_terminator : {"\r\n", "\n"}) {
      const auto file = shcore::str_subvars(
          file_template,
          [&field_terminator, &line_terminator](std::string_view name) {
            if (name == "ft") {
              return field_terminator;
            } else {
              return line_terminator;
            }
          },
          "<", ">");

      Dialect dialect;
      dialect.fields_enclosed_by = "'";
      dialect.fields_escaped_by = "\\";
      dialect.fields_terminated_by = field_terminator;
      dialect.lines_terminated_by = line_terminator;

      const auto ft = dialect.fields_terminated_by.length();
      const auto lt = dialect.lines_terminated_by.length();

      test_scanner(
          file, {8 + 2 * ft + lt, 8 + ft + lt, 14 + ft + 2 * lt, 17 + 4 * ft},
          dialect, {0, 1, 2, 3});

      // last row contains unescaped LINES TERMINATED BY sequence, when skipping
      // that row field is cut in half
      {
        Scanner s{dialect, 4};
        EXPECT_EQ(45 + 8 * ft + 4 * lt, s.scan(file.c_str(), file.length()));
      }

      {
        Scanner s{dialect, 5};
        EXPECT_EQ(-1, s.scan(file.c_str(), file.length()));
      }
    }
  }
}

}  // namespace
}  // namespace import_table
}  // namespace mysqlsh
