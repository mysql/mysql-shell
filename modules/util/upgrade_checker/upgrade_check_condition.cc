/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check_condition.h"

namespace mysqlsh {
namespace upgrade_checker {
Version_condition::Version_condition(std::forward_list<Version> versions)
    : m_versions{std::move(versions)} {}

Version_condition::Version_condition(Version version) {
  m_versions.emplace_front(std::move(version));
}

void Version_condition::add_version(const Version &version) {
  if (std::find(m_versions.begin(), m_versions.end(), version) ==
      std::end(m_versions)) {
    m_versions.emplace_front(version);
  }
}

std::string Version_condition::description() const {
  return "When the upgrade reaches any of the following versions: " +
         shcore::str_join(m_versions, ", ",
                          [](const auto &ver) { return ver.get_base(); });
}

bool Version_condition::evaluate(const Upgrade_info &info) {
  // The condition is met if the source -> target versions cross one of the
  // registered versions
  for (const auto &ver : m_versions) {
    if (ver > info.server_version && ver <= info.target_version) {
      return true;
    }
  }
  return false;
}

Life_cycle_condition::Life_cycle_condition(std::optional<Version> start,
                                           std::optional<Version> deprecation,
                                           std::optional<Version> removal)
    : m_start_version{std::move(start)},
      m_deprecation_version{std::move(deprecation)},
      m_removal_version{std::move(removal)} {
  if (!m_deprecation_version.has_value() && !m_removal_version.has_value()) {
    throw std::logic_error(
        "Only deprecated or removed features should use life cycle "
        "conditions.");
  }
}

bool Life_cycle_condition::evaluate(const Upgrade_info &info) {
  // Source server did not have the feature
  if (m_start_version.has_value() && info.server_version < *m_start_version)
    return false;

  // Source server has the feature already removed
  if (m_removal_version.has_value() &&
      info.server_version >= *m_removal_version)
    return false;

  return true;
}

std::string Life_cycle_condition::description() const {
  std::vector<std::string> source_conditions;
  if (m_start_version.has_value()) {
    source_conditions.push_back("at least " + m_start_version->get_base());
  }
  if (m_removal_version.has_value()) {
    source_conditions.push_back("lower than " + m_removal_version->get_base());
  }

  std::string result;
  if (!source_conditions.empty()) {
    result =
        "Server version is " + shcore::str_join(source_conditions, " and ");
    result += " and the target version is at least ";
  } else {
    result = "Target version is at least ";
  }

  if (m_deprecation_version.has_value()) {
    result += m_deprecation_version->get_base();
  } else {
    result += m_removal_version->get_base();
  }
  return result;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
