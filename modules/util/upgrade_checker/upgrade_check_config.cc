/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check_config.h"

#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"

namespace mysqlsh {
namespace upgrade_checker {

Upgrade_check_config::Upgrade_check_config(const Upgrade_check_options &options)
    : m_output_format(options.output_format),
      m_include(options.include_list),
      m_exclude(options.exclude_list),
      m_list_checks(options.list_checks) {
  m_upgrade_info.target_version = options.get_target_version();
  m_upgrade_info.explicit_target_version = options.target_version.has_value();
  m_upgrade_info.config_path = options.config_path;
  m_upgrade_info.skip_target_version_check = options.skip_target_version_check;

  if (m_output_format.empty()) {
    m_output_format =
        shcore::str_beginswith(
            mysqlsh::current_shell_options()->get().wrap_json, "json")
            ? "JSON"
            : "TEXT";
  }
}

Upgrade_check_config::Upgrade_check_config()
    : m_output_format("TEXT"), m_list_checks(false) {
  m_upgrade_info.target_version = mysqlshdk::utils::k_shell_version;
  m_upgrade_info.explicit_target_version = true;
}

void Upgrade_check_config::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_session = session;
  if (!m_session) return;

  const auto result = session->query(
      "select @@version, @@version_comment, UPPER(@@version_compile_os);");

  if (const auto row = result->fetch_one()) {
    m_upgrade_info.server_version = Version(row->get_string(0));
    m_upgrade_info.server_version_long =
        row->get_string(0) + " - " + row->get_string(1);
    m_upgrade_info.server_os = row->get_string(2);
  } else {
    throw std::runtime_error("Unable to get server version");
  }
}

std::unique_ptr<Upgrade_check_output_formatter>
Upgrade_check_config::formatter() const {
  return Upgrade_check_output_formatter::get_formatter(m_output_format);
}

std::vector<Upgrade_issue> Upgrade_check_config::filter_issues(
    std::vector<Upgrade_issue> &&issues) const {
  if (m_filter) {
    std::vector<Upgrade_issue> result;
    result.reserve(issues.size());

    for (auto &issue : issues) {
      if (m_filter(issue)) {
        result.emplace_back(std::move(issue));
      }
    }

    return result;
  } else {
    return std::move(issues);
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
