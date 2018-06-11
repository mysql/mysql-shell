/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest_clean.h>
#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"

using Print_flags = mysqlsh::Print_flags;
using Print_flag = mysqlsh::Print_flag;

#define TEST_DATA_SIZES(t, l, f, edw, ebw) \
  test_get_data_sizes(__FILE__, __LINE__, t, l, f, edw, ebw)

void test_get_data_sizes(const char *file, int line, const char *text,
                         size_t length, Print_flags flags, size_t edw,
                         size_t ebw) {
  auto sizes = get_utf8_sizes(text, length, flags);

  SCOPED_TRACE(text);
  if (edw != std::get<0>(sizes)) {
    std::string error = "Expected display size " + std::to_string(edw) +
                        " got " + std::to_string(std::get<0>(sizes));
    SCOPED_TRACE(error.c_str());
    ADD_FAILURE_AT(file, line);
  }
  if (ebw != std::get<1>(sizes)) {
    std::string error = "Expected buffer size " + std::to_string(ebw) +
                        " got " + std::to_string(std::get<1>(sizes));
    SCOPED_TRACE(error.c_str());
    ADD_FAILURE_AT(file, line);
  }
}

TEST(ResultsetDumper, get_data_sizes) {
  TEST_DATA_SIZES("AB\0CD", 5, Print_flags(), 4, 5);
  TEST_DATA_SIZES("AB\0CD", 5, Print_flags(Print_flag::PRINT_0_AS_SPC), 5, 5);
  TEST_DATA_SIZES("AB\0CD", 5, Print_flags(Print_flag::PRINT_0_AS_ESC), 6, 6);
  TEST_DATA_SIZES("AB\tCD", 5, Print_flags(), 5, 5);
  TEST_DATA_SIZES("AB\tCD", 5, Print_flags(Print_flag::PRINT_CTRL), 6, 6);
  TEST_DATA_SIZES("AB\nCD", 5, Print_flags(), 5, 5);
  TEST_DATA_SIZES("AB\nCD", 5, Print_flags(Print_flag::PRINT_CTRL), 6, 6);
  TEST_DATA_SIZES("AB\\CD", 5, Print_flags(), 5, 5);
  TEST_DATA_SIZES("AB\\CD", 5, Print_flags(Print_flag::PRINT_CTRL), 6, 6);

  // Multibyte character 3 bytes represented in 1 space
  TEST_DATA_SIZES("I ❤ MySQL Shell\0", 17, Print_flags(), 15, 17);

  // Multibyte character 3 bytes represented in 2 spaces
  TEST_DATA_SIZES("I 爱 MySQL Shell\0", 17, Print_flags(), 16, 17);
}
