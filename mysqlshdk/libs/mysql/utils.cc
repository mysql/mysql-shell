/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <mysqld_error.h>
#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <vector>
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

static constexpr int kMAX_WEAK_PASSWORD_RETRIES = 100;

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

void clone_user(const std::shared_ptr<db::ISession> &session,
                const std::string &orig_user, const std::string &orig_host,
                const std::string &new_user, const std::string &new_host,
                const std::string &password, Account_attribute_set flags) {
  std::string account = shcore::quote_identifier(new_user) + "@" +
                        shcore::quote_identifier(new_host);

  // create user
  session->executef("CREATE USER /*(*/ ?@? /*)*/ IDENTIFIED BY /*(*/ ? /*)*/",
                    new_user, new_host, password);

  if (flags & Account_attribute::Grants) {
    // gather all grants from the original account
    std::vector<std::string> grants;
    auto result = session->queryf("SHOW GRANTS FOR /*(*/ ?@? /*)*/", orig_user,
                                  orig_host);
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

void drop_all_accounts_for_user(const std::shared_ptr<db::ISession> &session,
                                const std::string &user) {
  std::string accounts;

  auto result = session->queryf(
      "SELECT concat(quote(user), '@', quote(host)) from mysql.user where "
      "user=?",
      user);
  while (auto row = result->fetch_one()) {
    if (!accounts.empty()) accounts.push_back(',');
    accounts.append(row->get_string(0));
  }

  if (!accounts.empty()) {
    session->execute("DROP USER IF EXISTS " + accounts);
  }
}

/*
 * Generates a random passwrd
 * with length equal to PASSWORD_LENGTH
 */
std::string generate_password(size_t password_length) {
  std::random_device rd;
  static const char *alpha_lower = "abcdefghijklmnopqrstuvwxyz";
  static const char *alpha_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const char *special_chars = "~@#%$^&*()-_=+]}[{|;:.>,</?";
  static const char *numeric = "1234567890";

  assert(kPASSWORD_LENGTH >= 20);

  if (password_length < kPASSWORD_LENGTH) password_length = kPASSWORD_LENGTH;

  auto get_random = [&rd](size_t size, const char *source) {
    std::string data;

    std::uniform_int_distribution<int> dist_num(0, strlen(source) - 1);

    for (size_t i = 0; i < size; i++) {
      char random = source[dist_num(rd)];

      // Make sure there are no consecutive values
      if (i == 0) {
        data += random;
      } else {
        if (random != data[i - 1])
          data += random;
        else
          i--;
      }
    }

    return data;
  };

  // Generate a passwrd

  // Fill the passwrd string with special chars
  std::string pwd = get_random(password_length, special_chars);

  // Replace 8 random non-consecutive chars by
  // 4 upperCase alphas and 4 lowerCase alphas
  std::string alphas = get_random(4, alpha_upper);
  alphas += get_random(4, alpha_lower);
  std::random_shuffle(alphas.begin(), alphas.end());

  std::uniform_int_distribution<int> rand_pos(0, pwd.length() - 1);
  size_t lower = 0;
  size_t step = password_length / 8;

  for (size_t index = 0; index < 8; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = alphas[index];
  }

  // Replace 3 random non-consecutive chars
  // by 3 numeric chars
  std::string numbers = get_random(3, numeric);
  lower = 0;

  step = password_length / 3;
  for (size_t index = 0; index < 3; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = numbers[index];
  }

  return pwd;
}

void create_user_with_random_password(
    const std::shared_ptr<db::ISession> &session, const std::string &user,
    const std::vector<std::string> &hosts,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants,
    std::string *out_password, bool disable_pwd_expire) {
  assert(!hosts.empty());
  Instance inst(session);

  *out_password = generate_password();

  for (int i = 0;; i++) {
    try {
      inst.create_user(user, hosts.front(), *out_password, grants,
                       disable_pwd_expire);
      break;
    } catch (const mysqlshdk::db::Error &e) {
      // If the error is: ERROR 1819 (HY000): Your password does not satisfy
      // the current policy requirements
      // We regenerate the password to retry
      if (e.code() == ER_NOT_VALID_PASSWORD) {
        if (i == kMAX_WEAK_PASSWORD_RETRIES) {
          throw std::runtime_error(
              "Unable to generate a password that complies with active MySQL "
              "server password policies for account " +
              user + "@" + hosts.front());
        }
        *out_password = mysqlshdk::mysql::generate_password();
      } else {
        throw;
      }
    }
  }

  // subsequent user creations shouldn't fail b/c of password
  for (size_t i = 1; i < hosts.size(); i++) {
    inst.create_user(user, hosts[i], *out_password, grants, disable_pwd_expire);
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
