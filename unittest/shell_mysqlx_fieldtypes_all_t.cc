/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "unittest/test_utils/command_line_test.h"

namespace tests {

class Shell_mysqlx_fieldtypes_all : public Command_line_test {
 public:
  static int prepare_data(const std::string& shell_binary,
                          const std::string& uri) {
    // Set up test database
    std::stringstream cmd;
    cmd << shell_binary << " " << uri << " --sql -f " << MYSQLX_SOURCE_HOME
        << "/unittest/data/sql/fieldtypes_all.sql";
    return system(cmd.str().c_str());
  }

  Shell_mysqlx_fieldtypes_all() : _data_prepared(false) {}

  ~Shell_mysqlx_fieldtypes_all() {
    if (_data_prepared) {
      std::stringstream cmd;
      cmd << _mysqlsh_path << " " << _uri << " --sql -e \""
          << "drop schema if exists xtest;\"";
      if (system(cmd.str().c_str()) != 0)
        std::cerr << "Error: unable to clean up after the test\n";
    }
  }

 protected:
  virtual void SetUp() {
    Command_line_test::SetUp();
    if (!_data_prepared) {
      if (prepare_data(_mysqlsh_path, _uri) != 0) {
        std::cerr << "Error while preparing test environment\n";
        FAIL();
      } else
        _data_prepared = true;
    }
  }

  bool _data_prepared;
};

TEST_F(Shell_mysqlx_fieldtypes_all, Integer_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_tinyint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_tinyint;", multiline({
    "c1	c2",
    "-128	0",
    "-1	1",
    "0	127",
    "1	200",
    "127	255"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_smallint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_smallint;", multiline({
    "c1	c2",
    "-32768	0",
    "-1	1",
    "0	32767",
    "1	65534",
    "32767	65535"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_mediumint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_mediumint;", multiline({
    "c1	c2",
    "-8388608	0",
    "-1	1",
    "0	8388607",
    "1	16777214",
    "8388607	16777215"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_int;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_int;", multiline({
    "c1	c2",
    "-2147483648	0",
    "-1	1",
    "0	2147483647",
    "1	4294967294",
    "2147483647	4294967295"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_bigint;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_bigint;", multiline({
    "c1	c2",
    "-9223372036854775808	0",
    "-1	1",
    "0	9223372036854775807",
    "1	18446744073709551614",
    "9223372036854775807	18446744073709551615"
      }), _output);
}

TEST_F(Shell_mysqlx_fieldtypes_all, Fixed_point_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_decimal1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_decimal1;", multiline({
    "c1	c2",
    "-1.1	0.0",
    "-9.9	9.8",
    "9.9	9.9"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_decimal2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_decimal2;", multiline({
    "c1	c2",
    "-1234567890123456789012345678901234.567890123456789012345678901234	1234567890123456789012345678901234.567890123456789012345678901234",
    "9234567890123456789012345678901234.567890123456789012345678901234	9234567890123456789012345678901234.567890123456789012345678901234",
    "0.000000000000000000000000000000	0.000000000000000000000000000000"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_numeric1;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_numeric1;", multiline({
    "c1	c2",
    "-1.1	0.0",
    "-9.9	9.8",
    "9.9	9.9"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_numeric2;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_numeric2;", multiline({
    "c1	c2",
    "-1234567890123456789012345678901234.567890123456789012345678901234	1234567890123456789012345678901234.567890123456789012345678901234",
    "9234567890123456789012345678901234.567890123456789012345678901234	9234567890123456789012345678901234.567890123456789012345678901234",
    "0.000000000000000000000000000000	0.000000000000000000000000000000"
      }), _output);
}

TEST_F(Shell_mysqlx_fieldtypes_all, Floating_point_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_real;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_real;", multiline({
    "c1	c2",
    "-1220.001	0",
    "-1.01	1.201",
    "1235.67	11235.67"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_float;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_float;", multiline({
    "c1	c2",
    "-1220220	0.0001",
    "-1.02323	1.2333",
    "123523	112353"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_double;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_double;", multiline({
    "c1	c2",
    "-122022323.0230221	2320.0012301",
    "-1.232023231	1231231231.2333124",
    "1235212322.6123123	11235212312322.672"
      }), _output);
}

TEST_F(Shell_mysqlx_fieldtypes_all, Date_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_date;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_date;", multiline({
    "c1	c2	c3	c4	c5",
    "2015-07-23 00:00:00	16:34:12	2015-07-23 16:34:12	2015-07-23 16:34:12	2015",
    "0000-01-01 00:00:00	-01:00:00	2000-01-01 00:00:02	0000-01-01 00:00:00	1999"
      }), _output);
}

TEST_F(Shell_mysqlx_fieldtypes_all, Binary_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_lob;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_lob;", multiline({
    "c1	c2	c3	c4	c5	c6	c7	c8	c9	c10	c11	c12",
    "											",
    "tinyblob-text readable	blob-text readable	mediumblob-text readable	longblob-text readable	tinytext	text	mediumtext	longtext	tinytext-binary",
    "next line	text-binary",
    "next line	mediumtext-binary",
    "next line	longtext-binary ",
    "next line"
      }), _output);
}


TEST_F(Shell_mysqlx_fieldtypes_all, Other_types) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_bit;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_bit;", multiline({
    "c1	c2",
    "0	0",
    "1	1",
    "1	18446744073709551615"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_enum;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_enum;", multiline({
    "c1	c2",
    "v1	",
    "v2	"
      }), _output);

  execute({_mysqlsh, _uri.c_str(), "--sql", "--database=xtest", "-e", "SELECT * FROM t_set;", NULL});
  MY_EXPECT_MULTILINE_OUTPUT("SELECT * FROM t_set;", multiline({
    "c1	c2",
    "v1,v2	",
    "	"
      }), _output);
}
}
