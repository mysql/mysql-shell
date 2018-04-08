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

namespace detail {

std::string::size_type skip_quoted_thing(const std::string &grant,
                                         std::string::size_type p) {
  if (grant[p] == '\'')
    return utils::span_quoted_string_sq(grant, p);
  else if (grant[p] == '"')
    return utils::span_quoted_string_dq(grant, p);
  else if (grant[p] == '`')
    return utils::span_quoted_sql_identifier_bt(grant, p);
  else
    return std::string::npos;
}

std::string replace_grantee(const std::string &grant,
                            const std::string &replacement) {
  // GRANT .... TO `user`@`host` KEYWORD*
  std::string::size_type p = 0;

  while (p < grant.size()) {
    if (grant.compare(p, 3, "TO ", 0, 3) == 0) {
      p += 2;                                     // skip the TO
      p = grant.find_first_not_of(" \t\n\r", p);  // skip spaces
      auto start = p;
      p = skip_quoted_thing(grant, p);
      if (p == std::string::npos)
        throw std::logic_error("Could not parse GRANT string: " + grant);
      if (grant[p] != '@')
        throw std::logic_error("Could not parse GRANT string: " + grant);
      ++p;
      auto end = p = skip_quoted_thing(grant, p);
      if (p == std::string::npos)
        throw std::logic_error("Could not parse GRANT string: " + grant);

      return grant.substr(0, start) + replacement + grant.substr(end);
    }
    switch (grant[p]) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        p = grant.find_first_not_of(" \t\n\r", p);
        break;
      case '`':
        p = utils::span_quoted_sql_identifier_bt(grant, p);
        break;
      case '\'':
        p = utils::span_quoted_string_sq(grant, p);
        break;
      case '"':
        p = utils::span_quoted_string_dq(grant, p);
        break;
      default:
        if (std::isalpha(grant[p]))
          p = utils::span_keyword(grant, p);
        else
          ++p;
        break;
    }
  }
  throw std::logic_error("Could not parse GRANT string: " + grant);
}
}  // namespace detail

void clone_user(std::shared_ptr<db::ISession> session,
                const std::string &orig_user, const std::string &orig_host,
                const std::string &new_user, const std::string &new_host,
                const std::string &password, Account_attribute_set flags) {
  std::string account = shcore::quote_identifier(new_user, '`') + "@" +
                        shcore::quote_identifier(new_host, '`');

  // create user
  session->executef("CREATE USER ?@? IDENTIFIED BY ?", new_user, new_host,
                    password);

  if (flags & Account_attribute::Grants) {
    // gather all grants from the original account
    std::vector<std::string> grants;
    auto result = session->queryf("SHOW GRANTS FOR ?@?", orig_user, orig_host);
    while (auto row = result->fetch_one()) {
      std::string grant = row->get_string(0);
      assert(shcore::str_beginswith(grant, "GRANT "));

      // Ignore GRANT PROXY
      if (shcore::str_beginswith(grant, "GRANT PROXY")) continue;

      // replace original user with new one
      grants.push_back(detail::replace_grantee(grant, account));
    }

    // grant everything
    for (const auto &grant : grants) {
      session->execute(grant);
    }
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
