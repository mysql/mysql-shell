/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef UNITTEST_MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_T_H_
#define UNITTEST_MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_T_H_

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

#include "mysqlshdk/libs/utils/utils_general.h"

namespace testing {
namespace user_privileges {

struct Setup_options {
  std::string user = "username";
  std::string host = "hostname";
  bool user_exists = true;
  mysqlshdk::utils::Version version{8, 0, 20};
  std::optional<bool> activate_all_roles_on_login = false;
  std::vector<std::string> mandatory_roles;
  std::vector<shcore::Account> active_roles;
  std::vector<std::string> grants;
  bool allow_skip_grants_user = false;
  std::optional<bool> partial_revokes;
};

void setup(const Setup_options &options, Mock_session *session);

std::set<std::string> all_privileges();

}  // namespace user_privileges
}  // namespace testing

#endif  // UNITTEST_MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_T_H_
