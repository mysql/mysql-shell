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

void Aggregated_life_cycle_condition::add_condition(
    const Life_cycle_condition &condition) {
  if (m_handle_start_version) {
    // The oldest start version will be used: NOTE that a coming condition with
    // no start version is basically the lowest start version possible, meaning,
    // no start version version
    if (condition.start_version().has_value()) {
      if (!m_start_version.has_value() ||
          condition.start_version() < m_start_version) {
        m_start_version = condition.start_version();
      }
    } else {
      // The lowest start version is present in the coming condition, so we
      // clear the start version and stop processing start version in new
      // conditions.
      m_handle_start_version = false;
      m_start_version.reset();
    }
  }

  // In the case of deprecation versions, the lower version defined is the one
  // that should be used
  if (condition.deprecation_version().has_value()) {
    if (!m_deprecation_version.has_value() ||
        condition.deprecation_version() < m_deprecation_version) {
      m_deprecation_version = condition.deprecation_version();
    }
  }

  // The highest removal version will be used: NOTE that a coming condition with
  // no removal version is basically the highest start version possible,
  // meaning, no removal version
  if (m_handle_removal_version) {
    if (condition.removal_version().has_value()) {
      if (!m_removal_version.has_value() ||
          condition.removal_version() > m_removal_version) {
        m_removal_version = condition.removal_version();
      }
    } else {
      m_handle_removal_version = false;
      m_removal_version.reset();
    }
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
