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

#include "modules/util/upgrade_checker/upgrade_check_options.h"

#include <cinttypes>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check_registry.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_filtering.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {
namespace {

void normalize_value(const Upgrade_check_options::Check_id_uset &value,
                     Check_id_set *output) {
  for (const auto &item : value) {
    if (auto clean_item = shcore::str_strip(item); !clean_item.empty()) {
      output->emplace(std::move(clean_item));
    }
  }
}
}  // namespace

Upgrade_check_options::Upgrade_check_options() {
  filters.schemas().exclude("information_schema");
  filters.schemas().exclude("performance_schema");
  filters.schemas().exclude("mysql");
  filters.schemas().exclude("sys");
}

const shcore::Option_pack_def<Upgrade_check_options>
    &Upgrade_check_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Upgrade_check_options>()
          .optional("outputFormat", &Upgrade_check_options::output_format)
          .optional("targetVersion", &Upgrade_check_options::set_target_version)
          .optional("configPath", &Upgrade_check_options::config_path)
          .optional("password", &Upgrade_check_options::password, "",
                    shcore::Option_extract_mode::CASE_SENSITIVE,
                    shcore::Option_scope::CLI_DISABLED)
          .optional("include", &Upgrade_check_options::include)
          .optional("exclude", &Upgrade_check_options::exclude)
          .optional("list", &Upgrade_check_options::list_checks)
          .optional("checkTimeout", &Upgrade_check_options::check_timeout)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::schemas)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::triggers)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::events)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::users)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::tables)
          .include(&Upgrade_check_options::filters,
                   &mysqlshdk::db::Filtering_options::routines)
          .on_done(&Upgrade_check_options::verify_options);
  return opts;
}

mysqlshdk::utils::Version Upgrade_check_options::get_target_version() const {
  return target_version.value_or(mysqlshdk::utils::k_shell_version);
}

void Upgrade_check_options::set_target_version(const std::string &value) {
  if (!value.empty()) {
    if (k_latest_versions.contains(value)) {
      target_version = k_latest_versions.at(value);
    } else {
      target_version = Version(value);
    }
  }
}

void Upgrade_check_options::include(const Check_id_uset &value) {
  normalize_value(value, &include_list);
}

void Upgrade_check_options::exclude(const Check_id_uset &value) {
  normalize_value(value, &exclude_list);
}

void Upgrade_check_options::verify_options() {
  bool filter_conflicts = false;
  filter_conflicts |=
      mysqlshdk::utils::error_on_conflicts<Check_id_set, std::string>(
          include_list, exclude_list, "check", "", "");

  filter_conflicts |= filters.schemas().error_on_conflicts();
  filter_conflicts |= filters.tables().error_on_conflicts();
  filter_conflicts |= filters.routines().error_on_conflicts();
  filter_conflicts |= filters.triggers().error_on_conflicts();
  filter_conflicts |= filters.events().error_on_conflicts();
  filter_conflicts |= filters.users().error_on_conflicts();

  filter_conflicts |= filters.tables().error_on_cross_filters_conflicts();
  filter_conflicts |= filters.routines().error_on_cross_filters_conflicts();
  filter_conflicts |= filters.triggers().error_on_cross_filters_conflicts();
  filter_conflicts |= filters.events().error_on_cross_filters_conflicts();

  if (filter_conflicts) {
    throw std::invalid_argument("Conflicting filtering options");
  }

  if (!config_path.empty() && !shcore::is_file(config_path)) {
    throw std::invalid_argument("Invalid config path: " + config_path);
  }
  if (check_timeout.has_value() && *check_timeout == 0) {
    throw std::invalid_argument(
        "Check timeout must be non-zero, positive value");
  }
  constexpr auto max_seconds =
      std::chrono::duration_cast<std::chrono::seconds, int64_t>(
          std::chrono::steady_clock::duration::max())
          .count();
  if (check_timeout.has_value() && *check_timeout > max_seconds) {
    throw std::invalid_argument(shcore::str_format(
        "Check timeout value is bigger than supported value %" PRId64,
        max_seconds));
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
