/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include <stdexcept>
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"

namespace shcore {

size_t process_input(const std::string &content) {
  const std::string filename{"test.json"};
  shcore::create_file(filename, content, true);
  auto exit_scope =
      shcore::on_leave_scope([&]() { shcore::delete_file(filename); });

  shcore::Buffered_input input{filename};
  shcore::Document_reader_options options{};
  shcore::Json_reader reader(&input, options);
  reader.parse_bom();

  size_t docs_number = 0;

  while (!reader.eof()) {
    std::string jd = reader.next();

    if (!jd.empty()) {
      ++docs_number;
    }
  }
  return docs_number;
}

TEST(Document_parser, plain) {
  {
    std::string content{""};
    auto docs_number = process_input(content);
    EXPECT_EQ(0, docs_number);
  }
  {
    std::string content{"{}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(1, docs_number);
  }
  {
    std::string content{"{}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }
  {
    std::string content{"  {}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }
  {
    std::string content{" \t {}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }
}

TEST(Document_parser, not_bom) {
  {
    std::string content{"\xfe\xfd\xfc{}{}{} {}\n{} {}"};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "JSON document contains invalid bytes (fe fd fc) at the "
                      "begining of the file.");
  }
  {
    std::string content{"\xfe\xfd\xfc\x12\x20\xdc{}{}{} {}\n{} {}"};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "JSON document contains invalid bytes (fe fd fc 12) at "
                      "the begining of the file.");
  }
}

TEST(Document_parser, bom_utf8) {
  {
    std::string content{"\xef\xbb\xbf{}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }
  {
    std::string content{"\xef\xbb\xbf\t{}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }
  {
    std::string content{"\xef\xbb\xbf  \t{}{}{} {}\n{} {}"};
    auto docs_number = process_input(content);
    EXPECT_EQ(6, docs_number);
  }

  // almost bom_utf8
  {
    std::string content{"\xef\xbb{}{}{} {}\n{} {}"};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "JSON document contains invalid bytes (ef bb) at the "
                      "begining of the file.");
  }
  {
    std::string content{"\xef\xbb {}{}{} {}\n{} {}"};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "JSON document contains invalid bytes (ef bb) at the "
                      "begining of the file.");
  }
}

TEST(Document_parser, bom_utf16le) {
  std::string content{"\xff\xfe{}{}{} {}\n{} {}"};
  EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                    "UTF-16LE encoded document is not supported.");
}

TEST(Document_parser, bom_utf16be) {
  std::string content{"\xfe\xff{}{}{} {}\n{} {}"};
  EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                    "UTF-16BE encoded document is not supported.");
}

TEST(Document_parser, bom_utf32le) {
  {
    std::string content{'\xff', '\xfe', '\x00', '\x00', '{', '}', '{', '}'};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "UTF-32LE encoded document is not supported.");
  }
  {
    std::string content{'\xff', '\xfe', '\x00', ' ', '{', '}', '{', '}'};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "JSON document contains invalid bytes (ff fe 00) at the "
                      "begining of the file.");
  }
}

TEST(Document_parser, bom_utf32be) {
  {
    std::string content{'\x00', '\x00', '\xfe', '\xff', '{', '}', '{', '}'};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "UTF-32BE encoded document is not supported.");
  }
  {
    std::string content{'\x00', '\x00', '\xfe', '\xff', ' ',
                        '{',    '}',    '{',    '}'};
    EXPECT_THROW_LIKE(process_input(content), std::runtime_error,
                      "UTF-32BE encoded document is not supported.");
  }
}
}  // namespace shcore
