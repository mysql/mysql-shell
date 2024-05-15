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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONFIG_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONFIG_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check_formatter.h"
#include "modules/util/upgrade_checker/upgrade_check_options.h"

#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"

namespace mysqlsh {
namespace upgrade_checker {
class Upgrade_check_config final {
 public:
  using Include_issue = std::function<bool(const Upgrade_issue &)>;

  explicit Upgrade_check_config(const Upgrade_check_options &options);

  const Upgrade_info &upgrade_info() const { return m_upgrade_info; }

  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  void set_default_server_data();

  const std::shared_ptr<mysqlshdk::db::ISession> &session() const {
    return m_session;
  }

  std::unique_ptr<Upgrade_check_output_formatter> formatter() const;

  void set_user_privileges(
      const mysqlshdk::mysql::User_privileges *privileges) {
    m_privileges = privileges;
  }

  const mysqlshdk::mysql::User_privileges *user_privileges() const {
    return m_privileges;
  }

  void set_issue_filter(const Include_issue &filter) { m_filter = filter; }

  void set_db_filters(const mysqlshdk::db::Filtering_options *filters) {
    m_db_filters = filters;
  }

  std::vector<Upgrade_issue> filter_issues(
      std::vector<Upgrade_issue> &&issues) const;

  void set_targets(Target_flags flags) { m_target_flags = flags; }

  Target_flags targets() const { return m_target_flags; }

  const Check_id_set &include_list() const noexcept { return m_include; }
  const Check_id_set &exclude_list() const noexcept { return m_exclude; }

  bool list_checks() const noexcept { return m_list_checks; }

  void set_warn_on_excludes(bool value) { m_warn_on_excludes = value; }

  bool warn_on_excludes() const { return m_warn_on_excludes; }

 private:
  Upgrade_check_config();

  Upgrade_info m_upgrade_info;
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  std::string m_output_format;
  const mysqlshdk::mysql::User_privileges *m_privileges = nullptr;
  Include_issue m_filter;
  Target_flags m_target_flags = Target_flags::all().unset(Target::MDS_SPECIFIC);
  Check_id_set m_include;
  Check_id_set m_exclude;
  bool m_list_checks;
  bool m_warn_on_excludes = true;

  friend Upgrade_check_config create_config(
      std::optional<Version> server_version,
      std::optional<Version> target_version, const std::string &server_os,
      size_t server_bits);
  const mysqlshdk::db::Filtering_options *m_db_filters = nullptr;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONFIG_H_