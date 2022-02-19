/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#include <cstring>
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

extern "C" const char *g_test_home;

using Version = mysqlshdk::utils::Version;

namespace tests {

class Mysqlsh_fieldtypes_all : public Command_line_test {
 public:
  static int prepare_data(const std::string &shell_binary,
                          const std::string &uri) {
    // Set up test database
    std::stringstream cmd;
    cmd << shell_binary << " " << uri << " --sql -f "
        << shcore::path::join_path(g_test_home, "data", "sql",
                                   "fieldtypes_all.sql")
               .c_str();
    return system(cmd.str().c_str());
  }

  Mysqlsh_fieldtypes_all() {}

  static void TearDownTestCase() {
    if (_cleanup_cmd) {
      if (system(_cleanup_cmd) != 0)
        std::cerr << "Error: unable to clean up after the test\n";
      free(_cleanup_cmd);
      _cleanup_cmd = nullptr;
    }
  }

 protected:
  void SetUp() {
    Command_line_test::SetUp();
    if (!_cleanup_cmd) {
      if (prepare_data(_mysqlsh_path, _uri) != 0) {
        std::cerr << "Error while preparing test environment\n";
        FAIL();
      } else {
        std::stringstream cmd;
        cmd << _mysqlsh_path << " " << _uri << " --sql -e \""
            << "drop schema if exists xtest;\"";
        _cleanup_cmd = strdup(cmd.str().c_str());
      }
    }
  }

  static char *_cleanup_cmd;
};

char *Mysqlsh_fieldtypes_all::_cleanup_cmd = nullptr;

TEST_F(Mysqlsh_fieldtypes_all, Integer_types_X) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_tinyint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_tinyint;",
      multiline({"c1\tc2", "-128\t0", "-1\t1", "0\t127", "1\t200", "127\t255"}),
      _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_smallint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_smallint;",
      multiline({"c1\tc2", "-32768\t0", "-1\t1", "0\t32767", "1\t65534",
                 "32767\t65535"}),
      _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_mediumint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_mediumint;",
      multiline({"c1\tc2", "-8388608\t0", "-1\t1", "0\t8388607", "1\t16777214",
                 "8388607\t16777215"}),
      _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_int;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_int;",
      multiline({"c1\tc2", "-2147483648\t0", "-1\t1", "0\t2147483647",
                 "1\t4294967294", "2147483647\t4294967295"}),
      _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_bigint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_bigint;",
      multiline({"c1\tc2", "-9223372036854775808\t0", "-1\t1",
                 "0\t9223372036854775807", "1\t18446744073709551614",
                 "9223372036854775807\t18446744073709551615"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Integer_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_tinyint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_tinyint;",
      multiline({"c1\tc2", "-128\t0", "-1\t1", "0\t127", "1\t200", "127\t255"}),
      _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_smallint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_smallint;",
      multiline({"c1\tc2", "-32768\t0", "-1\t1", "0\t32767", "1\t65534",
                 "32767\t65535"}),
      _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_mediumint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_mediumint;",
      multiline({"c1\tc2", "-8388608\t0", "-1\t1", "0\t8388607", "1\t16777214",
                 "8388607\t16777215"}),
      _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_int;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_int;",
      multiline({"c1\tc2", "-2147483648\t0", "-1\t1", "0\t2147483647",
                 "1\t4294967294", "2147483647\t4294967295"}),
      _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_bigint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_bigint;",
      multiline({"c1\tc2", "-9223372036854775808\t0", "-1\t1",
                 "0\t9223372036854775807", "1\t18446744073709551614",
                 "9223372036854775807\t18446744073709551615"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Fixed_point_types_X) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_decimal1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_decimal1;",
      multiline({"c1\tc2", "-1.1\t0.0", "-9.9\t9.8", "9.9\t9.9"}), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_decimal2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_decimal2;",
                             multiline({"c1\tc2",
                                        "-1234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "1234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "0.000000000000000000000000000000\t"
                                        "0.000000000000000000000000000000"}),
                             _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_numeric1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_numeric1;",
      multiline({"c1\tc2", "-1.1\t0.0", "-9.9\t9.8", "9.9\t9.9"}), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_numeric2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_numeric2;",
                             multiline({"c1\tc2",
                                        "-1234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "1234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "0.000000000000000000000000000000\t"
                                        "0.000000000000000000000000000000"}),
                             _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Fixed_point_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_decimal1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_decimal1;",
      multiline({"c1\tc2", "-1.1\t0.0", "-9.9\t9.8", "9.9\t9.9"}), _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_decimal2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_decimal2;",
                             multiline({"c1\tc2",
                                        "-1234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "1234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "0.000000000000000000000000000000\t"
                                        "0.000000000000000000000000000000"}),
                             _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_numeric1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_numeric1;",
      multiline({"c1\tc2", "-1.1\t0.0", "-9.9\t9.8", "9.9\t9.9"}), _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_numeric2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_numeric2;",
                             multiline({"c1\tc2",
                                        "-1234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "1234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234\t"
                                        "9234567890123456789012345678901234."
                                        "567890123456789012345678901234",
                                        "0.000000000000000000000000000000\t"
                                        "0.000000000000000000000000000000"}),
                             _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Floating_point_types_X) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_real;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_real;",
                             multiline({"c1\tc2", "-1220.001\t0",
                                        "-1.01\t1.201", "1235.67\t11235.67"}),
                             _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_float;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_float;",
                             multiline({"c1\tc2", "-1220220\t0.0001",
                                        "-1.02323\t1.2333", "123523\t112353"}),
                             _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_double;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_double;",
      multiline({"c1\tc2", "-122022323.0230221\t2320.0012301",
                 "-1.232023231\t1231231231.2333124",
                 "1235212322.6123123\t11235212312322.672"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Floating_point_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_real;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_real;",
                             multiline({"c1\tc2", "-1220.001\t0",
                                        "-1.01\t1.201", "1235.67\t11235.67"}),
                             _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_float;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_float;",
                             multiline({"c1\tc2", "-1220220\t0.0001",
                                        "-1.02323\t1.2333", "123523\t112353"}),
                             _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_double;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_double;",
      multiline({"c1\tc2", "-122022323.0230221\t2320.0012301",
                 "-1.232023231\t1231231231.2333124",
                 "1235212322.6123123\t11235212312322.672"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Date_types_X) {
  if (_target_server_version < Version("8.0")) {
    PENDING_BUG_TEST("BUG#27169735 DATETIME libmysqlxclient regression");
  } else {
    execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
             "SELECT * FROM t_date;", NULL});
    MY_EXPECT_MULTILINE_OUTPUT(
        "SELECT * FROM t_date;",
        multiline({"c1\tc2\tc3\tc4\tc5",
                   "2015-07-23\t16:34:12\t2015-07-23 16:34:12\t"
                   "2015-07-23 16:34:12\t2015",
                   "0000-01-01\t-01:00:00\t2000-01-01 00:00:02\t"
                   "0000-01-01 00:00:00\t1999",
                   "NULL\tNULL\tNULL\tNULL\tNULL",
                   "0000-00-00\t00:00:00\t0000-00-00 00:00:00\t"
                   "0000-00-00 00:00:00\t0"}),
        _output);
  }
}

TEST_F(Mysqlsh_fieldtypes_all, Date_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_date;", NULL});
  //  In 5.7 NULL timestamp is changed to Now on insert
  if (_target_server_version < Version("8.0"))
    MY_EXPECT_MULTILINE_OUTPUT(
        "SELECT * FROM t_date;",
        multiline({"c1\tc2\tc3\tc4\tc5",
                   "2015-07-23\t16:34:12\t2015-07-23 16:34:12\t"
                   "2015-07-23 16:34:12\t2015",
                   "0000-01-01\t-01:00:00\t2000-01-01 00:00:02\t"
                   "0000-01-01 00:00:00\t1999"}),
        _output);
  else
    MY_EXPECT_MULTILINE_OUTPUT(
        "SELECT * FROM t_date;",
        multiline({"c1\tc2\tc3\tc4\tc5",
                   "2015-07-23\t16:34:12\t2015-07-23 16:34:12\t"
                   "2015-07-23 16:34:12\t2015",
                   "0000-01-01\t-01:00:00\t2000-01-01 00:00:02\t"
                   "0000-01-01 00:00:00\t1999",
                   "NULL\tNULL\tNULL\tNULL\tNULL",
                   "0000-00-00\t00:00:00\t0000-00-00 00:00:00\t"
                   "0000-00-00 00:00:00\t0000"}),
        _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Binary_types_X) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_lob;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_lob;",
      multiline({"c1\tc2\tc3\tc4\tc5\tc6\tc7\tc8\t"
                 "c9\tc10\tc11\tc12",
                 "0x\t0x\t0x\t0x\t\t\t\t\t\t\t\t",
                 "0x74696E79626C6F622D74657874207265616461626C65\t0x626C6F622D7"
                 "4657874207265616461626C65\t"
                 "0x6D656469756D626C6F622D74657874207265616461626C65\t0x6C6F6E6"
                 "7626C6F622D74657874207265616461626C65\t"
                 "tinytext\ttext\tmediumtext\tlongtext\t"
                 "tinytext-binary\\nnext line\t"
                 "text-binary\\nnext line\t"
                 "mediumtext-binary\\nnext line\t"
                 "longtext-binary \\nnext line"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Binary_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_lob;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_lob;",
      multiline({"c1\tc2\tc3\tc4\tc5\tc6\tc7\tc8\t"
                 "c9\tc10\tc11\tc12",
                 "0x\t0x\t0x\t0x\t\t\t\t\t\t\t\t",
                 "0x74696E79626C6F622D74657874207265616461626C65\t0x626C6F622D7"
                 "4657874207265616461626C65\t"
                 "0x6D656469756D626C6F622D74657874207265616461626C65\t0x6C6F6E6"
                 "7626C6F622D74657874207265616461626C65\t"
                 "tinytext\ttext\tmediumtext\tlongtext\t"
                 "tinytext-binary\\nnext line\t"
                 "text-binary\\nnext line\t"
                 "mediumtext-binary\\nnext line\t"
                 "longtext-binary \\nnext line"}),
      _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Other_types_X) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_bit;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_bit;",
      multiline({"c1\tc2", "0\t0", "1\t1", "1\t18446744073709551615"}),
      _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_enum;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_enum;",
                             multiline({"c1\tc2", "v1\t", "v2\t"}), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_set;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_set;",
                             multiline({"c1\tc2", "v1,v2\t", "\t"}), _output);
}

TEST_F(Mysqlsh_fieldtypes_all, Other_types_classic) {
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_bit;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT(
      "SELECT * FROM t_bit;",
      multiline({"c1\tc2", "0\t0", "1\t1", "1\t18446744073709551615"}),
      _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_enum;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_enum;",
                             multiline({"c1\tc2", "v1\t", "v2\t"}), _output);

  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--database=xtest", "-e",
           "SELECT * FROM t_set;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_set;",
                             multiline({"c1\tc2", "v1,v2\t", "\t"}), _output);
}
}  // namespace tests
