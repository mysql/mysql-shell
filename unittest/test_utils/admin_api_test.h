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

#ifndef UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
#define UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_

#include <vector>
#include <string>
#include "unittest/test_utils.h"
#include "modules/adminapi/mod_dba_common.h"

namespace tests {
class Admin_api_test: public Shell_core_test_wrapper {
 public:
  virtual void SetUp();

  void add_instance_type_queries(std::vector<tests::Fake_result_data> *data,
                                 mysqlsh::dba::GRInstanceType type);
  void add_get_server_variable_query(std::vector<tests::Fake_result_data> *data,
                                     const std::string& variable,
                                     tests::Type type,
                                     const std::string& value);
  void add_replication_filters_query(std::vector<tests::Fake_result_data> *data,
                                     const std::string& binlog_do_db,
                                    const std::string& binlog_ignore_db);
};
}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_ADMIN_API_TEST_H_
