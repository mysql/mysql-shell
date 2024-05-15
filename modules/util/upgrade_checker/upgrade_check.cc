/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check.h"

#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include <algorithm>
#include <string>

#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace upgrade_checker {

const std::string &Upgrade_check::get_text(const char *field) const {
  std::string tag{m_name};
  return get_translation(tag.append(".").append(field).c_str());
}

const std::string &Upgrade_check::get_title() const {
  return get_text("title");
}

std::string Upgrade_check::get_description(const std::string &group) const {
  std::string description;

  if (!group.empty()) {
    std::string tag{"description."};
    tag += group;
    return get_text(tag.c_str());
  }

  return get_text("description");
}

const std::string &Upgrade_check::get_doc_link(const std::string &group) const {
  if (!group.empty()) {
    std::string tag{"docLink."};
    tag += group;
    return get_text(tag.c_str());
  }
  return get_text("docLink");
}

Invalid_privileges_check::Invalid_privileges_check(
    const Upgrade_info &server_info)
    : Upgrade_check(ids::k_invalid_privileges_check),
      m_upgrade_info(server_info) {
  if (m_upgrade_info.server_version > Version(8, 0, 0)) {
    add_privileges(Version(8, 4, 0), {"SET_USER_ID"});
  }

  add_privileges(Version(9, 0, 0), {"SUPER"});
}

void Invalid_privileges_check::add_privileges(
    Version version, const std::set<std::string> &privileges) {
  if (Version_condition(version).evaluate(m_upgrade_info)) {
    m_privileges[version] = privileges;
  }
}

bool Invalid_privileges_check::has_privilege(const std::string &privilege) {
  for (const auto &it : m_privileges) {
    if (it.second.find(privilege) != it.second.end()) {
      return true;
    }
  }

  return false;
}

bool Invalid_privileges_check::enabled() const { return !m_privileges.empty(); }

std::vector<Upgrade_issue> Invalid_privileges_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info & /*server_info*/) {
  session->execute("set @@session.sql_mode='TRADITIONAL'");
  shcore::on_leave_scope restore_sql_mode([&session]() {
    session->execute("set @@session.sql_mode=@@global.sql_mode");
  });

  auto result = session->query("select user, host from mysql.user");
  std::vector<std::pair<std::string, std::string>> all_users;
  if (result) {
    while (auto *row = result->fetch_one()) {
      all_users.emplace_back(row->get_string(0), row->get_string(1));
    }
  }

  std::vector<Upgrade_issue> issues;

  mysqlshdk::mysql::Instance instance(session);
  for (const auto &user : all_users) {
    const auto privileges =
        instance.get_user_privileges(user.first, user.second, true);
    for (const auto &invalid : m_privileges) {
      const auto presult = privileges->validate(invalid.second);
      const auto &missing = presult.missing_privileges();

      // If all privileges are missing, we are good, it means no invalid
      // privileges are found
      if (missing.size() != invalid.second.size()) {
        std::vector<std::string> invalid_list;
        std::set_difference(invalid.second.begin(), invalid.second.end(),
                            missing.begin(), missing.end(),
                            std::back_inserter(invalid_list));

        Upgrade_issue issue;
        std::string raw_description = get_text("issue");
        issue.schema = shcore::make_account(user.first, user.second);
        issue.level = Upgrade_issue::NOTICE;
        issue.description = resolve_tokens(
            raw_description,
            {{"account", issue.schema},
             {"privileges", shcore::str_join(invalid_list, ", ")}});
        issue.object_type = Upgrade_issue::Object_type::USER;
        issues.push_back(issue);
      }
    }
  }

  return issues;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh