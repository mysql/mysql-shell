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

#include "modules/util/upgrade_checker/upgrade_check_registry.h"

#include <algorithm>
#include <set>
#include <sstream>

#include "modules/util/upgrade_checker/custom_check.h"
#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_config.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace {
void cross_compare_checks(const std::set<std::string> &check_list,
                          const std::set<std::string_view> &all_checks,
                          const std::string option_name) {
  std::set<std::string_view> unknown_list;
  std::ranges::set_difference(
      check_list, all_checks, std::inserter(unknown_list, unknown_list.begin()),
      [](const auto &l, const auto &r) { return l < r; });

  if (!unknown_list.empty()) {
    throw std::invalid_argument(
        "Option " + option_name + " contains unknown values " +
        shcore::str_join(unknown_list, ", ", [](const std::string_view &value) {
          return shcore::quote_string(std::string(value), '\'');
        }));
  }
}
}  // namespace

using mysqlshdk::utils::Version;

Upgrade_check_registry::Collection Upgrade_check_registry::s_available_checks;

const Target_flags Upgrade_check_registry::k_target_flags_default =
    Target_flags::all().unset(Target::MDS_SPECIFIC);

namespace {

[[maybe_unused]] bool register_old_temporal =
    Upgrade_check_registry::register_check(std::bind(&get_old_temporal_check),
                                           Target::INNODB_INTERNALS, "8.0.11");

[[maybe_unused]] bool register_reserved =
    Upgrade_check_registry::register_check(&get_reserved_keywords_check,
                                           Target::OBJECT_DEFINITIONS, "8.0.11",
                                           "8.0.14", "8.0.17", "8.0.31");

[[maybe_unused]] bool register_utf8mb3 = Upgrade_check_registry::register_check(
    std::bind(&get_utf8mb3_check), Target::OBJECT_DEFINITIONS, "8.0.11");

[[maybe_unused]] bool register_mysql_schema =
    Upgrade_check_registry::register_check(std::bind(&get_mysql_schema_check),
                                           Target::OBJECT_DEFINITIONS,
                                           "8.0.11");

// Zerofill is still available in 8.0.11
// [[maybe_unused]] bool register_zerofill =
// Upgrade_check_registry::register_check(
//    std::bind(&get_zerofill_check), "8.0.11");
//}

[[maybe_unused]] bool register_nonnative_partitioning =
    Upgrade_check_registry::register_check(
        std::bind(&get_nonnative_partitioning_check), Target::ENGINES,
        "8.0.11");

[[maybe_unused]] bool register_foreign_key =
    Upgrade_check_registry::register_check(
        std::bind(&get_foreign_key_length_check), Target::OBJECT_DEFINITIONS,
        "8.0.11");

[[maybe_unused]] bool register_maxdb = Upgrade_check_registry::register_check(
    std::bind(&get_maxdb_sql_mode_flags_check), Target::OBJECT_DEFINITIONS,
    "8.0.11");

[[maybe_unused]] bool register_sqlmode = Upgrade_check_registry::register_check(
    std::bind(&get_obsolete_sql_mode_flags_check), Target::OBJECT_DEFINITIONS,
    "8.0.11");

[[maybe_unused]] bool register_enum_set_element_length_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_enum_set_element_length_check),
        Target::OBJECT_DEFINITIONS, "8.0.11");

[[maybe_unused]] bool register_sharded_tablespaces =
    Upgrade_check_registry::register_check(
        &get_partitioned_tables_in_shared_tablespaces_check,
        Target::TABLESPACES, "8.0.11", "8.0.13");

[[maybe_unused]] bool circular_directory_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_circular_directory_check), Target::TABLESPACES,
        "8.0.17");

[[maybe_unused]] bool register_removed_functions =
    Upgrade_check_registry::register_check(
        std::bind(&get_removed_functions_check), Target::OBJECT_DEFINITIONS,
        "8.0.11");

[[maybe_unused]] bool register_groupby_syntax_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_groupby_asc_syntax_check), Target::OBJECT_DEFINITIONS,
        "8.0.13");

[[maybe_unused]] bool register_sys_vars_check =
    Upgrade_check_registry::register_check(get_sys_vars_check,
                                           Target::SYSTEM_VARIABLES);

[[maybe_unused]] bool register_zero_dates_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_zero_dates_check), Target::OBJECT_DEFINITIONS, "8.0.11");

[[maybe_unused]] bool register_schema_inconsistency_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_schema_inconsistency_check), Target::INNODB_INTERNALS,
        "8.0.11");

[[maybe_unused]] bool register_fts_tablename_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_fts_in_tablename_check), Target::OBJECT_DEFINITIONS,
        [](const Upgrade_info &info) {
          return Version_condition(Version(8, 0, 11)).evaluate(info) &&
                 info.target_version < Version(8, 0, 18) &&
                 !shcore::str_beginswith(info.server_os, "WIN");
        },
        "When upgrading to a version between 8.0.11 and 8.0.17 on non Windows "
        "platforms.");

// clang-format on

[[maybe_unused]] bool register_engine_mixup_check =
    Upgrade_check_registry::register_check(std::bind(&get_engine_mixup_check),
                                           Target::ENGINES, "8.0.11");

[[maybe_unused]] bool register_old_geometry_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_old_geometry_types_check), Target::INNODB_INTERNALS,
        [](const Upgrade_info &info) {
          return Version_condition(Version(8, 0, 11)).evaluate(info) &&
                 info.target_version < Version(8, 0, 24);
        },
        "When upgrading to a version between 8.0.11 and 8.0.23");

[[maybe_unused]] bool register_check_table =
    Upgrade_check_registry::register_check(std::bind(&get_table_command_check),
                                           Target::INNODB_INTERNALS);

bool register_manual_checks() {
  Upgrade_check_registry::register_manual_check(
      "8.0.11", ids::k_default_authentication_plugin_check, Category::ACCOUNTS,
      Upgrade_issue::WARNING, Target::AUTHENTICATION_PLUGINS);
  Upgrade_check_registry::register_manual_check(
      "8.0.11", ids::k_default_authentication_plugin_mds_check,
      Category::ACCOUNTS, Upgrade_issue::WARNING, Target::MDS_SPECIFIC);
  return true;
}

[[maybe_unused]] bool reg_manual_checks = register_manual_checks();

[[maybe_unused]] bool register_changed_functions_generated_columns =
    Upgrade_check_registry::register_check(
        std::bind(&get_changed_functions_generated_columns_check),
        Target::OBJECT_DEFINITIONS, "5.7.0", "8.0.28");

[[maybe_unused]] bool register_columns_which_cannot_have_defaults =
    Upgrade_check_registry::register_check(
        std::bind(&get_columns_which_cannot_have_defaults_check),
        Target::OBJECT_DEFINITIONS, "8.0.12");

[[maybe_unused]] bool register_get_invalid_57_names_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_invalid_57_names_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

[[maybe_unused]] bool register_get_orphaned_objects_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_orphaned_objects_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

[[maybe_unused]] bool register_get_dollar_sign_name_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_dollar_sign_name_check), Target::OBJECT_DEFINITIONS,
        "8.0.31");

[[maybe_unused]] bool register_get_index_too_large_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_index_too_large_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

[[maybe_unused]] bool register_get_empty_dot_table_syntax_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_empty_dot_table_syntax_check),
        Target::OBJECT_DEFINITIONS, "8.0.0");

/*
 * The "syntax" check should be registered after:
 *  - reservedKeywords
 *  - removedFunctions
 *  - groupByAscSyntax
 *  - orphanedObjects
 *  - dollarSignName
 *  - emptyDotTableSyntax
 *  - maxdbSqlModeFlags
 *  - obsoleteSqlModeFlags
 */
[[maybe_unused]] bool register_syntax = Upgrade_check_registry::register_check(
    &get_syntax_check, Target::OBJECT_DEFINITIONS,
    [](const Upgrade_info &info) {
      if (info.server_version.get_major() == info.target_version.get_major() &&
          info.server_version.get_minor() == info.target_version.get_minor()) {
        return false;
      }
      return true;
    },
    "When the major.minor version of the source and target servers is "
    "different.");

[[maybe_unused]] bool register_get_invalid_engine_foreign_key_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_invalid_engine_foreign_key_check),
        Target::OBJECT_DEFINITIONS, "8.0.0");

[[maybe_unused]] bool register_get_foreign_key_references_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_foreign_key_references_check),
        Target::OBJECT_DEFINITIONS,
        [](const Upgrade_info &info) {
          return info.target_version >= Version(8, 4, 0);
        },
        "When the target server is equal or above 8.4.0.");

[[maybe_unused]] bool register_auth_method_usage_check =
    Upgrade_check_registry::register_check(&get_auth_method_usage_check,
                                           Target::AUTHENTICATION_PLUGINS);

[[maybe_unused]] bool register_plugin_usage_check =
    Upgrade_check_registry::register_check(&get_plugin_usage_check,
                                           Target::PLUGINS);

[[maybe_unused]] bool register_get_deprecated_default_auth_check =
    Upgrade_check_registry::register_check(&get_deprecated_default_auth_check,
                                           Target::SYSTEM_VARIABLES, "8.0.0",
                                           "8.1.0", "8.2.0");

[[maybe_unused]] bool register_get_deprecated_router_auth_method_check =
    Upgrade_check_registry::register_check(
        &get_deprecated_router_auth_method_check,
        Target::AUTHENTICATION_PLUGINS, "8.0.0", "8.1.0", "8.2.0");

[[maybe_unused]] bool
    register_get_deprecated_partition_temporal_delimiter_check =
        Upgrade_check_registry::register_check(
            std::bind(&get_deprecated_partition_temporal_delimiter_check),
            Target::OBJECT_DEFINITIONS, "8.0.29");

[[maybe_unused]] bool register_get_column_definition_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_column_definition_check), Target::OBJECT_DEFINITIONS,
        "8.4.0");

[[maybe_unused]] bool register_invalid_privileges_check =
    Upgrade_check_registry::register_check(&get_invalid_privileges_check,
                                           Target::PRIVILEGES, "8.4.0");

[[maybe_unused]] bool register_gget_partitions_with_prefix_keys_check =
    Upgrade_check_registry::register_check(
        &get_partitions_with_prefix_keys_check, Target::OBJECT_DEFINITIONS,
        "8.4.0");

[[maybe_unused]] bool register_get_spatial_index_check =
    Upgrade_check_registry::register_check(
        std::bind(&get_spatial_index_check), Target::OBJECT_DEFINITIONS,
        [](const Upgrade_info &info) {
          static const std::vector<std::pair<Version, bool>> k_available_list =
              {
                  {Version(9, 2, 0), true},  {Version(9, 0, 0), false},
                  {Version(8, 4, 4), true},  {Version(8, 1, 0), false},
                  {Version(8, 0, 41), true},
          };
          auto check_target = [&]() {
            for (auto &item : k_available_list) {
              if (info.target_version >= item.first) {
                return item.second;
              }
            }
            return false;
          };

          if (Version(8, 0, 3) > info.server_version) {
            return false;
          }
          if (info.server_version < Version(8, 0, 41)) {
            return check_target();
          }
          if (Version(8, 1, 0) <= info.server_version &&
              info.server_version < Version(8, 4, 4)) {
            return check_target();
          }
          if (Version(9, 0, 0) <= info.server_version &&
              info.server_version < Version(9, 2, 0)) {
            return check_target();
          }
          return false;
        },
        "When the source server is between (inclusive) 8.0.3-8.0.40, "
        "8.1.0-8.4.3, 9.0.0-9.1.0 and the target server version is above the "
        "listed versions.");

}  // namespace

Upgrade_check_registry::Upgrade_check_vec
Upgrade_check_registry::create_checklist(const Upgrade_check_config &config,
                                         bool include_all,
                                         Upgrade_check_vec *rejected) {
  cross_compare_checks(config.include_list(), ids::all, "include");
  cross_compare_checks(config.exclude_list(), ids::all, "exclude");

  auto cond_accepted = [&](const Creator_info &c) {
    return !c.condition || c.condition->evaluate(config.upgrade_info());
  };

  auto list_accepted = [&](const std::string &name) {
    if (config.exclude_list().contains(name)) return false;

    if (!config.include_list().empty() && !config.include_list().contains(name))
      return false;

    return true;
  };

  Upgrade_check_vec result;
  for (const auto &c : s_available_checks) {
    if (include_all || config.targets().is_set(c.target)) {
      auto check = c.creator(config.upgrade_info());

      // If a condition is defined in the creator, updates the check with it for
      // listing purposes, some cases (Feature like checks) have an inner
      // condition
      if (c.condition) {
        check->set_condition(c.condition.get());
      }

      if ((include_all || (cond_accepted(c) && check->enabled())) &&
          list_accepted(check->get_name())) {
        result.emplace_back(std::move(check));
      } else if (rejected) {
        rejected->emplace_back(std::move(check));
      }
    }
  }

  return result;
}

void Upgrade_check_registry::register_manual_check(const char *ver,
                                                   std::string_view name,
                                                   Category category,
                                                   Upgrade_issue::Level level,
                                                   Target target) {
  register_check(
      [name, category, level](const Upgrade_info &) {
        return std::make_unique<Manual_check>(name, category, level);
      },
      target, Version(ver));
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
