/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#ifndef UNITTEST_TEST_UTILS_SERVER_MOCK_H_
#define UNITTEST_TEST_UTILS_SERVER_MOCK_H_

#define MY_EXPECT_OUTPUT_CONTAINS(e,o) Shell_base_test::check_string_expectation(e,o,true)
#define MY_EXPECT_OUTPUT_NOT_CONTAINS(e,o) Shell_base_test::check_string_expectation(e,o,false)

#include <string>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <gtest/gtest.h>
#include "unittest/mysqld_mock/mysql_server_mock.h"
#include "common/process_launcher/process_launcher.h"

namespace tests {
// TODO: On master this should be deleted and the Fake_result_data that exists there should be used
enum class Type {
  Null,
  Decimal,
  Date,
  NewDate,
  Time,
  String,
  VarChar,
  VarString,
  NewDecimal,
  TinyBlob,
  MediumBlob,
  LongBlob,
  Blob,
  Geometry,
  Json,
  Year,
  Tiny,
  Short,
  Int24,
  Long,
  LongLong,
  Float,
  Double,
  DateTime,
  Timestamp,
  Bit,
  Enum,
  Set
};

struct Fake_result_data {
  std::string sql;
  std::vector<std::string> names;
  std::vector<Type> types;
  std::vector<std::vector<std::string> > rows;
};

class Server_mock {
public:
  Server_mock();
  std::string map_column_type(Type type);
  std::string create_data_file(const std::vector<Fake_result_data> &data);
  std::string get_path_to_binary();

  void start(int port, const std::vector<Fake_result_data> &data);
  void stop();

private:
  std::shared_ptr<server_mock::MySQLServerMock> _server_mock;
  std::shared_ptr<std::thread> _thread;
  std::shared_ptr<ngcommon::Process_launcher> _process;
  std::mutex _server;
  bool _server_listening;
  std::string _server_output;
};
}

#endif  // UNITTEST_TEST_UTILS_SERVER_MOCK_H_