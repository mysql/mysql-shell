/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _WIN32
#include <Windows.h>
#endif

#include "my_config.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

using mysqlshdk::config::Config_file;
using mysqlshdk::utils::nullable;
extern "C" const char *g_test_home;

namespace testing {

class ConfigFileTest : public tests::Shell_base_test {
 protected:
  std::string m_option_files_basedir;
  std::string m_tmpdir;

  void SetUp() {
    tests::Shell_base_test::SetUp();
    m_option_files_basedir =
        shcore::path::join_path(g_test_home, "data", "config");
    m_tmpdir = getenv("TMPDIR");
  }

  void TearDown() { tests::Shell_base_test::TearDown(); }
};

/**
 * Verify all the expected data in the Config_file object.
 *
 * @param expected_data map with the expected groups and option/values.
 * @param cfg Config_file object to check.
 */
void check_config_file_data(
    const std::map<std::string, std::map<std::string, nullable<std::string>>>
        &expected_data,
    const Config_file &cfg) {
  // Make sure the number of groups is the same, i.e. that there are no
  // additional groups in the Config_file object.
  EXPECT_EQ(cfg.groups().size(), expected_data.size());
  for (const auto &grp_pair : expected_data) {
    std::string grp = grp_pair.first;
    // Make sure the number of option for each group is the same, i.e., there
    // are no additional options in the Config_file object.
    EXPECT_EQ(cfg.options(grp).size(), grp_pair.second.size());
    for (const auto &opt_pair : grp_pair.second) {
      std::string opt = opt_pair.first;
      nullable<std::string> expected_val = opt_pair.second;
      nullable<std::string> value = cfg.get(grp, opt);
      std::string str_expected_val =
          expected_val.is_null() ? "NULL" : "'" + (*expected_val) + "'";
      std::string str_value = value.is_null() ? "NULL" : "'" + (*value) + "'";
      // Verify that the value for each option in all groups matches what is
      // expected.
      EXPECT_EQ(expected_val, value)
          << "Value for group '" << grp.c_str() << "' and option '"
          << opt.c_str()
          << "' does not match. Expected: " << str_expected_val.c_str()
          << ", Got: " << str_value.c_str() << ".";
    }
  }
}

TEST_F(ConfigFileTest, test_read) {
  // Test reading a sample MySQL option file (my.cnf).
  Config_file cfg = Config_file();
  std::string cfg_path =
      shcore::path::join_path(m_option_files_basedir, "my.cnf");
  std::map<std::string, std::map<std::string, nullable<std::string>>> data = {
      {"default",
       {{"password", nullable<std::string>("54321")},
        {"repeated_value", nullable<std::string>("what")}}},
      {"client",
       {{"password", nullable<std::string>("12345")},
        {"port", nullable<std::string>("1000")},
        {"socket", nullable<std::string>("/var/run/mysqld/mysqld.sock")},
        {"ssl-ca", nullable<std::string>("dummyCA")},
        {"ssl-cert", nullable<std::string>("dummyCert")},
        {"ssl-key", nullable<std::string>("dummyKey")},
        {"ssl-cipher", nullable<std::string>("AES256-SHA:CAMELLIA256-SHA")},
        {"CaseSensitiveOptions", nullable<std::string>("Yes")},
        {"option_to_delete_with_value", nullable<std::string>("20")},
        {"option_to_delete_without_value", nullable<std::string>()}}},
      {"mysqld_safe",
       {{"socket", nullable<std::string>("/var/run/mysqld/mysqld1.sock")},
        {"nice", nullable<std::string>("0")},
        {"valid_v1", nullable<std::string>("include comment ( #) symbol")},
        {"valid_v2", nullable<std::string>("include comment ( #) symbol")}}},
      {"mysqld",
       {{"option_to_delete_with_value", nullable<std::string>("20")},
        {"option_to_delete_without_value", nullable<std::string>()},
        {"master-info-repository", nullable<std::string>("FILE")},
        {"user", nullable<std::string>("mysql")},
        {"pid-file", nullable<std::string>("/var/run/mysqld/mysqld.pid")},
        {"socket", nullable<std::string>("/var/run/mysqld/mysqld2.sock")},
        {"port", nullable<std::string>("1001")},
        {"basedir", nullable<std::string>("/usr")},
        {"datadir", nullable<std::string>("/var/lib/mysql")},
        {"tmpdir", nullable<std::string>("/tmp")},
        {"to_override", nullable<std::string>()},
        {"to_override_with_value", nullable<std::string>("old_val")},
        {"no_comment_no_value", nullable<std::string>()},
        {"lc-messages-dir", nullable<std::string>("/usr/share/mysql")},
        {"skip-external-locking", nullable<std::string>()},
        {"binlog", nullable<std::string>("True")},
        {"multivalue", nullable<std::string>("Noooooooooooooooo")},
        {"semi-colon", nullable<std::string>(";")},
        {"bind-address", nullable<std::string>("127.0.0.1")},
        {"log_error", nullable<std::string>("/var/log/mysql/error.log")}}},
      {"delete_section",
       {{"option_to_drop_with_no_value", nullable<std::string>()},
        {"option_to_drop_with_value", nullable<std::string>("value")},
        {"option_to_drop_with_value2", nullable<std::string>("value")}}},
      {"escape_sequences",
       {{"backspace", nullable<std::string>("\b")},
        {"tab", nullable<std::string>("\t")},
        {"newline", nullable<std::string>("\n")},
        {"carriage-return", nullable<std::string>("\r")},
        {"backslash", nullable<std::string>("\\")},
        {"space", nullable<std::string>(" ")},
        {"not_esc_seq_char", nullable<std::string>("\\S")}}},
      {"path_options",
       {{"win_path_no_esc_seq_char1",
         nullable<std::string>("C:\\Program Files\\MySQL\\MySQL Server 5.7")},
        {"win_path_no_esc_seq_char2",
         nullable<std::string>("C:\\Program Files\\MySQL\\MySQL Server 5.7")},
        {"win_path_esc_seq_char",
         nullable<std::string>("C:\\Program Files\\MySQL\\MySQL Server 5.7")},
        {"win_path_with_posix_sep",
         nullable<std::string>("C:/Program Files/MySQL/MySQL Server 5.7")}}},
      {"empty section", {}},
      {"delete_section2",
       {{"option_to_drop_with_no_value", nullable<std::string>()},
        {"option_to_drop_with_value", nullable<std::string>("value")},
        {"option_to_drop_with_value2", nullable<std::string>("value")}}}};
  {
    SCOPED_TRACE("Test read of file: my.cnf");
    cfg.read(cfg_path);
    check_config_file_data(data, cfg);
  }
  {
    // Test re-reading the same sample file again (no error)
    // Reset existing configurations with the new read values.
    SCOPED_TRACE("Test re-read of file: my.cnf");
    cfg.read(cfg_path);
    check_config_file_data(data, cfg);
  }
  {
    // Test Error reading a file that does not exists.
    SCOPED_TRACE("Test read of file: not_exist.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir, "not_exist.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Cannot open file: ");
  }
  {
    // Test error reading a file with no group.
    SCOPED_TRACE("Test read of file: my_error_no_grp.cnf");
    cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_error_no_grp.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Group missing before option at line 24 of file '");
  }
  {
    // In case of error, previous state cannot be changed.
    SCOPED_TRACE("Test state not changed in case of error");
    check_config_file_data(data, cfg);
  }
  {
    // No error reading file with duplicated groups.
    SCOPED_TRACE("Test read of file: my_duplicated_grp.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_duplicated_grp.cnf");
    cfg.read(cfg_path);
    data = {{"client",
             {{"password", nullable<std::string>("0000")},
              {"port", nullable<std::string>("1000")}}}};
    check_config_file_data(data, cfg);
  }
  {
    // Read file with !include directives.
    SCOPED_TRACE("Test read of file: my_include.cnf");
    cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include.cnf");
    cfg.read(cfg_path);
    data = {{"group1",
             {{"option1", nullable<std::string>("153")},
              {"option2", nullable<std::string>("20")}}},
            {"group2",
             {{"option11", nullable<std::string>("11")},
              {"option1", nullable<std::string>("203")},
              {"option2", nullable<std::string>("303")},
              {"option22", nullable<std::string>("22")}}},
            {"group3", {{"option3", nullable<std::string>("33")}}},
            {"group4", {{"option3", nullable<std::string>("200")}}},
            {"mysql", {{"user", nullable<std::string>("myuser")}}},
            {"client", {{"user", nullable<std::string>("spam")}}}};
    check_config_file_data(data, cfg);
  }
  {
    // Read file with !include and !includedir directives.
    SCOPED_TRACE("Test read of file: my_include_all.cnf");
    cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include_all.cnf");
    cfg.read(cfg_path);
    data = {{"group1",
             {{"option1", nullable<std::string>("15")},
              {"option2", nullable<std::string>("20")},
              {"option3", nullable<std::string>("0")}}},
            {"group2",
             {{"option1", nullable<std::string>("20")},
              {"option2", nullable<std::string>("30")},
              {"option3", nullable<std::string>("3")},
              {"option22", nullable<std::string>("22")}}},
            {"group3", {{"option3", nullable<std::string>("3")}}},
            {"group4", {{"option3", nullable<std::string>("200")}}},
            {"mysql", {{"user", nullable<std::string>("dam")}}},
            {"client", {{"user", nullable<std::string>("spam")}}}};
    check_config_file_data(data, cfg);
  }
  {
    // Test reading include with loop
    SCOPED_TRACE("Test read of file: my_include_loopA.cnf");
    cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include_loopA.cnf");
    cfg.read(cfg_path);
    data = {{"group1",
             {{"option1", nullable<std::string>("11")},
              {"option2", nullable<std::string>("13")}}},
            {"group2",
             {{"option1", nullable<std::string>("21")},
              {"option2", nullable<std::string>("22")}}},
            {"group4", {{"option3", nullable<std::string>("200")}}},
            {"mysql", {{"user", nullable<std::string>("myuserB")}}}};
    check_config_file_data(data, cfg);
  }
  {
    // Test error including file that does not exist.
    SCOPED_TRACE("Test read of file: my_include_error3.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_include_error3.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Cannot open file: ");
  }
  {
    // Test error including file with an invalid format (parsing error).
    SCOPED_TRACE("Test read of file: my_include_error2.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_include_error2.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Group missing before option at line 24 of file '");
  }
  {
    // Test error reading invalid directive
    SCOPED_TRACE("Test read of file: my_error_invalid_directive.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_invalid_directive.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Invalid directive at line 25 of file '");
  }
  {
    // Test error reading malformed group
    SCOPED_TRACE("Test read of file: my_error_malformed_group.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_malformed_group.cnf");
    EXPECT_THROW_LIKE(
        cfg.read(cfg_path), std::runtime_error,
        "Invalid group, not ending with ']', at line 32 of file '");
  }
  {
    // Test error reading invalid group
    SCOPED_TRACE("Test read of file: my_error_invalid_group.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_invalid_group.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Invalid group at line 32 of file '");
  }
  {
    // Test error missing closing quotes
    SCOPED_TRACE("Test read of file: my_error_quotes_no_match.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_quotes_no_match.cnf");
    EXPECT_THROW_LIKE(
        cfg.read(cfg_path), std::runtime_error,
        "Invalid option, missing closing quote for option value at "
        "line 25 of file '");
  }
  {
    // Test error invalid text after quotes
    SCOPED_TRACE("Test read of file: my_error_quotes_invalid_char.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_quotes_invalid_char.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Invalid option, only comments (started with #) are "
                      "allowed after a quoted value at line 25 of file '");
  }
  {
    // Test error invalid text after quotes
    SCOPED_TRACE(
        "Test read of file: my_error_quotes_single_quote_not_escaped.cnf");
    cfg_path = shcore::path::join_path(
        m_option_files_basedir, "my_error_quotes_single_quote_not_escaped.cnf");
    EXPECT_THROW_LIKE(
        cfg.read(cfg_path), std::runtime_error,
        "Invalid option, missing closing quote for option value at "
        "line 25 of file '");
  }
  {
    // Test error invalid line start
    SCOPED_TRACE("Test read of file: my_error_invalid_line_start.cnf");
    cfg_path = shcore::path::join_path(m_option_files_basedir,
                                       "my_error_invalid_line_start.cnf");
    EXPECT_THROW_LIKE(cfg.read(cfg_path), std::runtime_error,
                      "Line 31 starts with invalid character in file '");
  }
  {
    // Test clear (remove all elements) of the Config_file
    SCOPED_TRACE("Test clear()");
    cfg.clear();
    data = {};
    check_config_file_data(data, cfg);
  }
}

TEST_F(ConfigFileTest, test_write) {
  // Test write to a new file (does not exist) without any changes.
  {
    SCOPED_TRACE("Test write to new file (no changes): my_write_test.cnf");
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_test.cnf");
    std::string cfg_path_tmp = res_cfg_path + ".tmp";

    // Simulate that a backup file already exists to test the random file name
    // creation
    shcore::create_file(cfg_path_tmp, "");
    Config_file cfg = Config_file();
    cfg.read(cfg_path);
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);
    // Delete test files at the end.
    shcore::delete_file(res_cfg_path, true);
    shcore::delete_file(cfg_path_tmp, true);
  }

  // Test write to a new file (does not exist) from the scratch (no read()).
  {
    SCOPED_TRACE("Test write to new file (no read): my_no_read_test.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_no_read_test.cnf");

    Config_file cfg = Config_file();
    cfg.add_group("mysqld");
    cfg.set("mysqld", "test_option", nullable<std::string>("test_value"));
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);

    // Delete test files at the end.
    shcore::delete_file(res_cfg_path, true);
  }

  // Test write to a file without write permissions.
  {
    SCOPED_TRACE(
        "Test write to a file without write permissions.: "
        "no_perm_write_test.cnf");
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my.cnf");
    std::string no_write_perm_cfg_path =
        shcore::path::join_path(m_tmpdir, "no_perm_write_test.cnf");

    // Create file to write to and make it read only
    shcore::create_file(no_write_perm_cfg_path, "");
    shcore::make_file_readonly(no_write_perm_cfg_path);
    Config_file cfg = Config_file();
    cfg.read(cfg_path);
    EXPECT_THROW(cfg.write(no_write_perm_cfg_path), std::runtime_error);

#ifdef _WIN32
    // NOTE: On Windows, read-only attribute (previously set) must be removed
    // before deleting the file, otherwise an Access is denied error is
    // issued.
    const auto wide_no_write_perm_cfg_path =
        shcore::utf8_to_wide(no_write_perm_cfg_path);
    auto attributes = GetFileAttributesW(wide_no_write_perm_cfg_path.c_str());
    // Remove (reset) read-only attribute from the file
    SetFileAttributesW(wide_no_write_perm_cfg_path.c_str(),
                       attributes & ~FILE_ATTRIBUTE_READONLY);
#endif
    // Delete test file at the end.
    shcore::delete_file(no_write_perm_cfg_path, true);
  }

  // Test write to an existing file without any sections
  {
    SCOPED_TRACE(
        "Test write to an existing file without any sections: "
        "my_write_test.cnf");
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_test.cnf");

    // simulate that a backup file already exists to test the random file name
    // creation
    shcore::create_file(res_cfg_path, "key=value\n");
    Config_file cfg = Config_file();
    cfg.read(cfg_path);
    EXPECT_THROW_LIKE(cfg.write(res_cfg_path), std::runtime_error,
                      "Group missing before option at line 1 of file ")
    // Delete test file at the end.
    shcore::delete_file(res_cfg_path, true);
  }

  // Test write to the same file without any changes.
  {
    SCOPED_TRACE("Test write to same file (no changes): my_write_test.cnf");
    // Create a copy of the original test file to always keep it unchanged.
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_test.cnf");
    shcore::copy_file(cfg_path, res_cfg_path);

    Config_file cfg = Config_file();
    cfg.read(res_cfg_path);
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);

    // Delete test file at the end.
    shcore::delete_file(res_cfg_path, true);
  }

  // Test write to the same file with changes to the configuration.
  {
    SCOPED_TRACE("Test write to same file (with changes): my_write_test.cnf");
    // Create a copy of the original test file to always keep it unchanged.
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_test.cnf");
    shcore::copy_file(cfg_path, res_cfg_path);

    Config_file cfg = Config_file();
    cfg.read(res_cfg_path);
    cfg.set("default", "new_option", nullable<std::string>("with value"));
    cfg.set("default", "new_option_no_value", nullable<std::string>());
    cfg.set("default", "new_option_empty_value", nullable<std::string>(""));
    cfg.remove_option("client", "option_to_delete-with_value");
    cfg.remove_option("client", "option_to_delete-without_value");
    cfg.set("client", "MyNameIs", nullable<std::string>("007"));
    cfg.remove_option("mysqld", "loose_option_to_delete_with_value");
    cfg.remove_option("MySQLd", "loose-option_to_delete_without_value");
    cfg.set("mysqld", "to_override", nullable<std::string>("True"));
    cfg.set("MySQLd", "to_override_with_value",
            nullable<std::string>("new_val"));
    cfg.set("mysqld", "binlog", nullable<std::string>());
    cfg.remove_option("mysqld", "master-info-repository");
    cfg.remove_group("delete_section");
    cfg.remove_group("delete_section2");
    cfg.add_group("IAmNew");
    cfg.set("iamnew", "loose_option-with-prefix",
            nullable<std::string>("need_quote_with_#_symbol"));
    cfg.set("iamnew", "new_option1",
            nullable<std::string>("need_quote_different_from_'_with_#_symbol"));
    cfg.set("iamnew", "new_option2",
            nullable<std::string>("still need \" ' quote with space"));
    cfg.set("iamnew", "new_option_no-value", nullable<std::string>());
    cfg.set("empty section", "new_option_no-value", nullable<std::string>());
    cfg.set("empty section", "new_option_value",
            nullable<std::string>("value"));
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);

    // Verify that updated options keep their inline comment.
    std::ifstream cfg_file(res_cfg_path);
    std::string line;
    while (std::getline(cfg_file, line)) {
      if (line.empty()) continue;
      if (shcore::str_beginswith(line, "to_override=")) {
        EXPECT_STREQ(
            "to_override = True # this option is going to be overridden",
            line.c_str());
      } else if (shcore::str_beginswith(line, "to_override_with_value")) {
        EXPECT_STREQ(
            "to_override_with_value = new_val # this is also to be "
            "overridden",
            line.c_str());
      } else if (shcore::str_beginswith(line, "binlog")) {
        EXPECT_STREQ("binlog # ignore this comment", line.c_str());
      }
    }
    cfg_file.close();

    // Delete test file at the end.
    shcore::delete_file(res_cfg_path, true);
  }

  // Test write to the same file with !include directives.
  {
    SCOPED_TRACE(
        "Test write to file with !include directives: "
        "my_write_include_test.cnf");
    // Create a copy of the original test files to keep them unchanged.
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_include_test.cnf");
    shcore::copy_file(cfg_path, res_cfg_path);

    std::string include_dir =
        shcore::path::join_path(m_option_files_basedir, "include_files");
    std::string tmp_include_dir =
        shcore::path::join_path(m_tmpdir, "include_files");
    shcore::create_directory(tmp_include_dir, false);

    std::string include_2_cnf = shcore::path::join_path(include_dir, "2.cnf");
    std::string include_3_txt = shcore::path::join_path(include_dir, "3.txt");
    std::string tmp_include_2_cnf =
        shcore::path::join_path(tmp_include_dir, "2.cnf");
    std::string tmp_include_3_txt =
        shcore::path::join_path(tmp_include_dir, "3.txt");
    shcore::copy_file(include_2_cnf, tmp_include_2_cnf);
    shcore::copy_file(include_3_txt, tmp_include_3_txt);

    Config_file cfg = Config_file();
    cfg.read(res_cfg_path);
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);

    // Delete all test files at the end.
    shcore::delete_file(res_cfg_path, true);
    shcore::remove_directory(tmp_include_dir, true);
  }

  // Test write to the same file with !include and !includedir directives.
  {
    SCOPED_TRACE(
        "Test write to file with !include and !includedir directives: "
        "my_write_include_all_test.cnf");
    // Create a copy of the original test files to keep them unchanged.
    std::string cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include_all.cnf");
    std::string res_cfg_path =
        shcore::path::join_path(m_tmpdir, "my_write_include_all_test.cnf");
    shcore::copy_file(cfg_path, res_cfg_path);

    std::string include_dir =
        shcore::path::join_path(m_option_files_basedir, "include_files");
    std::string tmp_include_dir =
        shcore::path::join_path(m_tmpdir, "include_files");
    shcore::create_directory(tmp_include_dir, false);

    std::string include_1_cnf = shcore::path::join_path(include_dir, "1.cnf");
    std::string include_1_txt = shcore::path::join_path(include_dir, "1.txt");
    std::string include_2_cnf = shcore::path::join_path(include_dir, "2.cnf");
    std::string include_3_txt = shcore::path::join_path(include_dir, "3.txt");
    std::string tmp_include_1_cnf =
        shcore::path::join_path(tmp_include_dir, "1.cnf");
    std::string tmp_include_1_txt =
        shcore::path::join_path(tmp_include_dir, "1.txt");
    std::string tmp_include_2_cnf =
        shcore::path::join_path(tmp_include_dir, "2.cnf");
    std::string tmp_include_3_txt =
        shcore::path::join_path(tmp_include_dir, "3.txt");
    shcore::copy_file(include_1_cnf, tmp_include_1_cnf);
    shcore::copy_file(include_1_txt, tmp_include_1_txt);
    shcore::copy_file(include_2_cnf, tmp_include_2_cnf);
    shcore::copy_file(include_3_txt, tmp_include_3_txt);

    Config_file cfg = Config_file();
    cfg.read(res_cfg_path);
    EXPECT_NO_THROW(cfg.write(res_cfg_path));
    Config_file cfg_res = Config_file();
    cfg_res.read(res_cfg_path);
    EXPECT_EQ(cfg, cfg_res);

    // Delete all test files at the end.
    shcore::delete_file(res_cfg_path, true);
    shcore::remove_directory(tmp_include_dir, true);
  }
}

TEST_F(ConfigFileTest, test_groups_case_insensitive) {
  // Test config file with no groups
  Config_file cfg = Config_file();
  EXPECT_FALSE(cfg.has_group("test_group"));

  // Add new group and verify it was added.
  EXPECT_TRUE(cfg.add_group("Test_Group"));
  EXPECT_TRUE(cfg.has_group("tesT_grouP"));
  std::vector<std::string> groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);
  EXPECT_THAT(groups, UnorderedElementsAre("Test_Group"));

  // Adding a group with the same name does nothing (false returned).
  EXPECT_FALSE(cfg.add_group("TEST_group"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);

  // Removing a non existing group (false returned).
  EXPECT_FALSE(cfg.remove_group("not_exist"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);

  // Removing an existing group (true returned).
  EXPECT_TRUE(cfg.remove_group("test_GROUP"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 0);
}

TEST_F(ConfigFileTest, test_groups_case_sensitive) {
  // Test config file with no groups
  Config_file cfg = Config_file(mysqlshdk::config::Case::SENSITIVE);
  EXPECT_FALSE(cfg.has_group("test_group"));

  // Add new group and verify it was added.
  EXPECT_TRUE(cfg.add_group("Test_Group"));
  EXPECT_TRUE(cfg.has_group("Test_Group"));

  EXPECT_FALSE(cfg.has_group("tesT_grouP"));
  std::vector<std::string> groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);
  EXPECT_THAT(groups, UnorderedElementsAre("Test_Group"));

  // Adding a group with the same name and different case.
  EXPECT_TRUE(cfg.add_group("TEST_group"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 2);
  EXPECT_THAT(groups, UnorderedElementsAre("Test_Group", "TEST_group"));

  // Removing a non existing group (false returned).
  EXPECT_FALSE(cfg.remove_group("not_exist"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 2);

  // Removing an existing group (true returned).
  EXPECT_TRUE(cfg.remove_group("TEST_group"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);

  // Removing a non existing group (false returned).
  EXPECT_FALSE(cfg.remove_group("TEST_group"));
  groups = cfg.groups();
  EXPECT_EQ(groups.size(), 1);
  EXPECT_THAT(groups, UnorderedElementsAre("Test_Group"));
}

TEST_F(ConfigFileTest, test_options) {
  // Test config file with a group and no options
  Config_file cfg = Config_file();
  cfg.add_group("test_group");
  EXPECT_FALSE(cfg.has_option("test_group", "test_option"));

  // Setting a new option to a non-existing group throws an exception.
  mysqlshdk::utils::nullable<std::string> value("test value");
  EXPECT_THROW(cfg.set("no_group", "test_option", value), std::out_of_range);

  // Adding (set) new options (with and without value) to an existing group.
  // NOTE: Options can use '-' and '_' interchangeably, and have the 'loose_'
  // prefix, but they are case sensitive.
  cfg.set("Test_Group", "test-option", value);
  EXPECT_TRUE(cfg.has_option("tesT_grouP", "test_option"));
  EXPECT_TRUE(cfg.has_option("tesT_grouP", "test-option"));
  EXPECT_TRUE(cfg.has_option("tesT_grouP", "loose-test-option"));
  EXPECT_TRUE(cfg.has_option("tesT_grouP", "loose_test_option"));
  cfg.set("test_group", "loose_no_value_option",
          mysqlshdk::utils::nullable<std::string>());
  EXPECT_TRUE(cfg.has_option("test_group", "no_value_option"));
  EXPECT_TRUE(cfg.has_option("test_group", "no-value_option"));
  EXPECT_TRUE(cfg.has_option("test_group", "loose-no_value_option"));
  EXPECT_TRUE(cfg.has_option("test_group", "loose_no-value-option"));
  // The option name that was first set for the option is returned by
  // options()
  std::vector<std::string> options = cfg.options("test_group");
  EXPECT_EQ(options.size(), 2);
  EXPECT_THAT(options,
              UnorderedElementsAre("test-option", "loose_no_value_option"));

  // Getting the value for a non existing option or group throws an exception.
  EXPECT_THROW(cfg.get("test_group", "no_option"), std::out_of_range);
  EXPECT_THROW(cfg.get("no_group", "test-option"), std::out_of_range);

  // Get value of added options.
  // NOTE: Options can use '-' and '_' interchangeably, and have the 'loose_'
  // prefix, but they are case sensitive.
  mysqlshdk::utils::nullable<std::string> ret_value =
      cfg.get("test_group", "test-option");
  EXPECT_FALSE(ret_value.is_null());
  EXPECT_EQ(*value, *ret_value);
  ret_value = cfg.get("test_group", "loose-test_option");
  EXPECT_FALSE(ret_value.is_null());
  EXPECT_EQ(*value, *ret_value);
  ret_value = cfg.get("test_group", "loose_no_value_option");
  EXPECT_TRUE(ret_value.is_null());
  ret_value = cfg.get("test_group", "no-value-option");
  EXPECT_TRUE(ret_value.is_null());

  // Changing (set) existing options (with and without value),
  // Switch value of previously set options.
  // NOTE: Options can use '-' and '_' interchangeably, and have the 'loose_'
  // prefix, but they are case sensitive.
  cfg.set("test_group", "loose-test_option",
          mysqlshdk::utils::nullable<std::string>());
  EXPECT_TRUE(cfg.has_option("test_group", "test-option"));
  cfg.set("test_group", "no-value-option", value);
  EXPECT_TRUE(cfg.has_option("test_group", "loose_no_value_option"));
  options = cfg.options("test_group");
  EXPECT_EQ(options.size(), 2);
  EXPECT_THAT(options,
              UnorderedElementsAre("test-option", "loose_no_value_option"));

  // Get value of changed options.
  // NOTE: Options can use '-' and '_' interchangeably, and have the 'loose_'
  // prefix, but they are case sensitive.
  ret_value = cfg.get("test_group", "test-option");
  EXPECT_TRUE(ret_value.is_null());
  ret_value = cfg.get("test_group", "loose-test_option");
  EXPECT_TRUE(ret_value.is_null());
  ret_value = cfg.get("test_group", "loose_no_value_option");
  EXPECT_FALSE(ret_value.is_null());
  EXPECT_EQ(*value, *ret_value);
  ret_value = cfg.get("test_group", "no-value-option");
  EXPECT_FALSE(ret_value.is_null());
  EXPECT_EQ(*value, *ret_value);

  // Removing an option from a non existing group throws an exception.
  EXPECT_THROW(cfg.remove_option("no_group", "test-option"), std::out_of_range);

  // Removing a non existing option (false returned).
  EXPECT_FALSE(cfg.remove_option("test_group", "not_exist"));
  options = cfg.options("test_group");
  EXPECT_EQ(options.size(), 2);

  // Removing an existing option (true returned).
  // NOTE: Options can use '-' and '_' interchangeably, and have the 'loose_'
  // prefix, but they are case sensitive.
  EXPECT_TRUE(cfg.remove_option("test_group", "loose_test-option"));
  options = cfg.options("test_group");
  EXPECT_EQ(options.size(), 1);
  EXPECT_TRUE(cfg.remove_option("test_group", "no-value-option"));
  options = cfg.options("test_group");
  EXPECT_EQ(options.size(), 0);

  // Getting the options from a non existing group should throw exception
  EXPECT_THROW(cfg.options("does_not_exist"), std::out_of_range);
}

TEST_F(ConfigFileTest, test_constructor) {
  // Test default (no parameters) constructor, no groups (thus no options)
  Config_file cfg1 = Config_file();
  EXPECT_TRUE(cfg1.groups().empty());

  // Test constructor with same object as parameter (copy)
  Config_file cfg = Config_file();
  std::string cfg_path =
      shcore::path::join_path(m_option_files_basedir, "my.cnf");
  cfg.read(cfg_path);
  Config_file cfg_copy = Config_file(cfg);
  EXPECT_TRUE(cfg_copy.groups() == cfg.groups());
}

TEST_F(ConfigFileTest, get_default_config_paths) {
  using mysqlshdk::config::get_default_config_paths;
  using shcore::OperatingSystem;

  // BUG#30171324 : mysql shell does not know about my.cnf default location
  // Test default path include /etc/my.cnf and /etc/mysql/my.cnf for all
  // unix and unix-like systems.
  auto test_unix_defaults = [](shcore::OperatingSystem os) {
    SCOPED_TRACE("Test mandatory default paths for '" + to_string(os) + "'");
    std::vector<std::string> res = get_default_config_paths(os);
    std::string sysconfdir;
#ifdef DEFAULT_SYSCONFDIR
    sysconfdir = std::string(DEFAULT_SYSCONFDIR);
#endif
    EXPECT_THAT(res, Contains("/etc/my.cnf"));
    EXPECT_THAT(res, Contains("/etc/mysql/my.cnf"));
    if (!sysconfdir.empty()) {
      EXPECT_THAT(res, Contains(sysconfdir + "/my.cnf"));
    }
  };

  std::vector<shcore::OperatingSystem> unix_os_list{
      shcore::OperatingSystem::DEBIAN, shcore::OperatingSystem::REDHAT,
      shcore::OperatingSystem::SOLARIS, shcore::OperatingSystem::LINUX,
      shcore::OperatingSystem::MACOS};
  for (const auto &unix_os : unix_os_list) {
    test_unix_defaults(unix_os);
  }
}

}  // namespace testing
