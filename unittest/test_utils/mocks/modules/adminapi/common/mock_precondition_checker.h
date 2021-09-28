/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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
#ifndef UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_COMMON_MOCK_PRECONDITION_CHECKER_H_
#define UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_COMMON_MOCK_PRECONDITION_CHECKER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "unittest/test_utils/mocks/gmock_clean.h"

#include "modules/adminapi/common/preconditions.h"
#include "mysqlshdk/libs/utils/version.h"

namespace testing {
class Mock_precondition_checker : public mysqlsh::dba::Precondition_checker {
 public:
  explicit Mock_precondition_checker(
      const std::shared_ptr<mysqlsh::dba::MetadataStorage> &metadata,
      const std::shared_ptr<mysqlsh::dba::Instance> &group_server);
  virtual ~Mock_precondition_checker() {}

  MOCK_METHOD0(get_cluster_global_state, mysqlsh::dba::Cluster_global_status());
  MOCK_METHOD0(get_cluster_status, mysqlsh::dba::Cluster_status());

 private:
  mysqlsh::dba::Cluster_global_status def_get_cluster_global_state();
  mysqlsh::dba::Cluster_status def_get_cluster_status();
};
}  // namespace testing

#endif  // UNITTEST_TEST_UTILS_MOCKS_MODULES_ADMINAPI_COMMON_MOCK_PRECONDITION_CHECKER_H_
