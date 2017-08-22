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

#include <string>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "unittest/gtest_clean.h"
#include "unittest/mysqld_mock/mysql_server_mock.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "mysqlshdk/libs/db/column.h"

namespace tests {
class Server_mock {
public:
  Server_mock();
  std::string map_column_type(mysqlshdk::db::Type type);
  std::string create_data_file(const std::vector<testing::Fake_result_data> &data);
  std::string get_path_to_binary();

  void start(int port, const std::vector<testing::Fake_result_data> &data);
  void stop();

private:
  std::shared_ptr<server_mock::MySQLServerMock> _server_mock;
  std::shared_ptr<std::thread> _thread;
  std::shared_ptr<shcore::Process_launcher> _process;
  std::mutex _mutex;
  std::condition_variable _cond;
  int _server_status;
  bool _started;

  std::string _server_output;
};
}

#endif  // UNITTEST_TEST_UTILS_SERVER_MOCK_H_
