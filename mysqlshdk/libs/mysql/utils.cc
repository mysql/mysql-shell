/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/mysql/utils.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

namespace {
std::string replace_grantee(const std::string &grant,
                            const std::string &replacement) {
  // GRANT .... TO `user`@`host` KEYWORD*
  std::string s = grant;
  std::reverse(s.begin(), s.end());
  std::string::size_type p = 0;
  while (s[p] != '`' && p < s.length()) {
    ++p;
  }

  std::string::size_type e =
      mysqlshdk::utils::span_quoted_sql_identifier_bt(s, p);
  if (e == std::string::npos || s[e] != '@')
    throw std::logic_error("Could not parse GRANT string: " + grant);
  e = mysqlshdk::utils::span_quoted_sql_identifier_bt(s, e + 1);
  if (e == std::string::npos)
    throw std::logic_error("Could not parse GRANT string: " + grant);
  e++;

  e = s.length() - e;
  p = s.length() - p;

  return grant.substr(0, e) + replacement + grant.substr(p);
}
}  // namespace

void clone_current_user(std::shared_ptr<db::ISession> session,
                        const std::string &account_user,
                        const std::string &account_host,
                        const std::string &password) {
  std::string account = shcore::quote_identifier(account_user, '`') + "@" +
                        shcore::quote_identifier(account_host, '`');
  // gather all grants from the original account
  std::vector<std::string> grants;
  auto result = session->queryf("SHOW GRANTS");
  while (auto row = result->fetch_one()) {
    std::string grant = row->get_string(0);
    assert(shcore::str_beginswith(grant, "GRANT "));

    // Ignore GRANT PROXY
    if (shcore::str_beginswith(grant, "GRANT PROXY"))
      continue;

    // replace original user with new one
    grants.push_back(replace_grantee(grant, account));
  }

  // create user
  session->executef("CREATE USER ?@? IDENTIFIED BY ?", account_user,
                    account_host, password);

  // grant everything
  for (const auto &grant : grants) {
    session->execute(grant);
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
