/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <vector>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_translate.h"
#include "unittest/gtest_clean.h"

namespace shcore {

class Translate_test : public ::testing::Test {
 public:
  Translate_test()
      : filename(shcore::path::join_path(shcore::get_user_config_path(),
                                         "translate.rc")) {}

  ~Translate_test() { remove(filename.c_str()); }

  void check_file(const std::vector<std::string> &lines) {
    std::ifstream file(filename, std::ifstream::binary);
    ASSERT_TRUE(file.is_open());
    for (const auto &line : lines) {
      std::string fline;
      ASSERT_TRUE(static_cast<bool>(std::getline(file, fline)));
      ASSERT_EQ(line, fline);
    }
  }

  void write_file(const std::vector<std::string> &lines) {
    std::ofstream file(filename, std::ofstream::binary);
    ASSERT_TRUE(file.is_open());
    for (const auto &line : lines) {
      file << line << std::endl;
    }
  }

 protected:
  const std::string filename;
};

TEST_F(Translate_test, create_translation_file) {
  {
    Translation_writer writer(filename.c_str());
    writer.write_entry("reservedKeywordsCheck.title", "Reserved keywords check",
                       nullptr);
    writer.write_entry(
        "reservedKeywordsCheck.advice", nullptr,
        "This is a very long advice\npertaining to reserved keywords");
    writer.write_entry("reservedKeywordsCheck.docLink", nullptr, "");
    check_file({"* reservedKeywordsCheck.title", "# Reserved keywords check",
                "", "* reservedKeywordsCheck.advice",
                "This is a very long advice\\n",
                "pertaining to reserved keywords", "",
                "* reservedKeywordsCheck.docLink", ""});
  }

  {
    Translation_writer writer(filename.c_str());
    writer.write_entry("reservedKeywordsCheck\ntitle",
                       "Reserved keywords check", nullptr);
    writer.write_entry(
        "reservedKeywordsCheck.advice", nullptr,
        "Following table columns use a deprecated and no longer supported "
        "timestamp disk storage format. They must be converted to the new "
        "format "
        "before upgrading. It can by done by rebuilding the table using 'ALTER "
        "TABLE <table_name> FORCE' command");
    writer.write_entry("reservedKeywordsCheck.docLink", nullptr, "");
    check_file({"* reservedKeywordsCheck\\n", "* title",
                "# Reserved keywords check", "",
                "* reservedKeywordsCheck.advice",
                "Following table columns use a deprecated and no "
                "longer supported timestamp disk storage format.",
                "They must be converted to the new format before "
                "upgrading. It can by done by rebuilding the table",
                "using 'ALTER TABLE <table_name> FORCE' command", "",
                "* reservedKeywordsCheck.docLink", ""});
  }
}

TEST_F(Translate_test, read_translation_file) {
  write_file({"# This is preamble", "", "* firstId", "# formatting command",
              "First value", ""});
  Translation translation = read_translation_from_file(filename.c_str());
  ASSERT_EQ(1, translation.size());
  ASSERT_EQ("First value", translation["firstId"]);

  // discontinued id
  write_file({"* firstId", "# formatting command", "* Second value", ""});
  EXPECT_THROW(read_translation_from_file(filename.c_str()),
               std::runtime_error);

  // multiline id
  write_file({"* firstId", "First value", "", "* Second\\n", "* Id", "# xxx",
              "Second value"});
  translation = read_translation_from_file(filename.c_str());
  //  for (const auto &pair : translation)
  //    std::cout << pair.first << ": " << pair.second << std::endl;
  ASSERT_EQ(2, translation.size());
  ASSERT_EQ("First value", translation["firstId"]);
  ASSERT_EQ("Second value", translation["Second\nId"]);

  // multiline value
  write_file({"* firstId", "First value", "", "* SecondId", "# xxx",
              "Second value\\n", "continued", "again", ""});
  translation = read_translation_from_file(filename.c_str());
  ASSERT_EQ(2, translation.size());
  ASSERT_EQ("First value", translation["firstId"]);
  ASSERT_EQ("Second value\ncontinued again", translation["SecondId"]);
}

TEST_F(Translate_test, write_read) {
  {
    Translation_writer writer(filename.c_str(), 20);
    writer.write_entry("reservedKeywordsCheck.title", "Reserved keywords check",
                       nullptr);
    writer.write_entry(
        "reservedKeywordsCheck.advice", nullptr,
        "This is a very long advice\npertaining to reserved keywords");
    writer.write_entry("reservedKeywordsCheck.docLink", nullptr, "");
    writer.write_entry("reservedKeywordsCheck.extended", nullptr,
                       "This\nis\na\nmulti line");
  }

  Translation translation = read_translation_from_file(filename.c_str());
  ASSERT_EQ(2, translation.size());
  ASSERT_EQ("This is a very long advice\npertaining to reserved keywords",
            translation["reservedKeywordsCheck.advice"]);
  ASSERT_EQ("This\nis\na\nmulti line",
            translation["reservedKeywordsCheck.extended"]);
}

}  // namespace shcore
