/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CREATORS_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CREATORS_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/sql_upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace ids {
inline constexpr std::string_view k_reserved_keywords_check =
    "reservedKeywords";
inline constexpr std::string_view k_routine_syntax_check = "routineSyntax";
inline constexpr std::string_view k_utf8mb3_check = "utf8mb3";
inline constexpr std::string_view k_innodb_rowformat_check = "innodbRowformat";
inline constexpr std::string_view k_zerofill_check = "zerofill";
inline constexpr std::string_view k_nonnative_partitioning_check =
    "nonNativePartitioning";
inline constexpr std::string_view k_mysql_schema_check = "mysqlSchema";
inline constexpr std::string_view k_old_temporal_check = "oldTemporal";
inline constexpr std::string_view k_foreign_key_length_check =
    "foreignKeyLength";
inline constexpr std::string_view k_maxdb_sql_mode_flags_check =
    "maxdbSqlModeFlags";
inline constexpr std::string_view k_obsolete_sql_mode_flags_check =
    "obsoleteSqlModeFlags";
inline constexpr std::string_view k_enum_set_element_length_check =
    "enumSetElementLength";
inline constexpr std::string_view k_table_command_check = "tableCommand";
inline constexpr std::string_view
    k_partitioned_tables_in_shared_tablespaces_check =
        "partitionedTablesInSharedTablespaces";
inline constexpr std::string_view k_circular_directory_check =
    "circularDirectory";
inline constexpr std::string_view k_removed_functions_check =
    "removedFunctions";
inline constexpr std::string_view k_groupby_asc_syntax_check =
    "groupbyAscSyntax";
inline constexpr std::string_view k_removed_sys_log_vars_check =
    "removedSysLogVars";
inline constexpr std::string_view k_removed_sys_vars_check = "removedSysVars";
inline constexpr std::string_view k_sys_vars_new_defaults_check =
    "sysVarsNewDefaults";
inline constexpr std::string_view k_zero_dates_check = "zeroDates";
inline constexpr std::string_view k_schema_inconsistency_check =
    "schemaInconsistency";
inline constexpr std::string_view k_fts_in_tablename_check = "ftsInTablename";
inline constexpr std::string_view k_engine_mixup_check = "engineMixup";
inline constexpr std::string_view k_old_geometry_types_check =
    "oldGeometryTypes";
inline constexpr std::string_view k_changed_functions_generated_columns_check =
    "changedFunctionsInGeneratedColumns";
inline constexpr std::string_view k_columns_which_cannot_have_defaults_check =
    "columnsWhichCannotHaveDefaults";
inline constexpr std::string_view k_invalid_57_names_check = "invalid57Names";
inline constexpr std::string_view k_orphaned_routines_check =
    "orphanedRoutines";
inline constexpr std::string_view k_dollar_sign_name_check = "dollarSignName";
inline constexpr std::string_view k_index_too_large_check = "indexTooLarge";
inline constexpr std::string_view k_empty_dot_table_syntax_check =
    "emptyDotTableSyntax";
inline constexpr std::string_view k_invalid_engine_foreign_key_check =
    "invalidEngineForeignKey";
inline constexpr std::string_view k_deprecated_auth_method_check =
    "deprecatedAuthMethod";
inline constexpr std::string_view k_deprecated_router_auth_method_check =
    "deprecatedRouterAuthMethod";
inline constexpr std::string_view k_deprecated_default_auth_check =
    "deprecatedDefaultAuth";
inline constexpr std::string_view k_deprecated_temporal_delimiter_check =
    "deprecatedTemporalDelimiter";
inline constexpr std::string_view k_default_authentication_plugin_check =
    "defaultAuthenticationPlugin";
inline constexpr std::string_view k_default_authentication_plugin_mds_check =
    "defaultAuthenticationPluginMds";
}  // namespace ids

std::unique_ptr<Sql_upgrade_check> get_reserved_keywords_check(
    const Upgrade_info &info);
std::unique_ptr<Upgrade_check> get_routine_syntax_check();
std::unique_ptr<Sql_upgrade_check> get_utf8mb3_check();
std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check();
std::unique_ptr<Sql_upgrade_check> get_zerofill_check();
std::unique_ptr<Sql_upgrade_check> get_nonnative_partitioning_check();
std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check();
std::unique_ptr<Sql_upgrade_check> get_old_temporal_check();
std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check();
std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check();
std::unique_ptr<Sql_upgrade_check> get_obsolete_sql_mode_flags_check();
std::unique_ptr<Sql_upgrade_check> get_enum_set_element_length_check();
std::unique_ptr<Upgrade_check> get_table_command_check();
std::unique_ptr<Sql_upgrade_check>
get_partitioned_tables_in_shared_tablespaces_check(const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check> get_circular_directory_check();
std::unique_ptr<Sql_upgrade_check> get_removed_functions_check();
std::unique_ptr<Sql_upgrade_check> get_groupby_asc_syntax_check();
std::unique_ptr<Upgrade_check> get_removed_sys_log_vars_check(
    const Upgrade_info &info);
std::unique_ptr<Upgrade_check> get_removed_sys_vars_check(
    const Upgrade_info &info);

std::unique_ptr<Upgrade_check> get_sys_vars_new_defaults_check();
std::unique_ptr<Sql_upgrade_check> get_zero_dates_check();
std::unique_ptr<Sql_upgrade_check> get_schema_inconsistency_check();
std::unique_ptr<Sql_upgrade_check> get_fts_in_tablename_check();
std::unique_ptr<Sql_upgrade_check> get_engine_mixup_check();
std::unique_ptr<Sql_upgrade_check> get_old_geometry_types_check();
std::unique_ptr<Sql_upgrade_check>
get_changed_functions_generated_columns_check();
std::unique_ptr<Sql_upgrade_check>
get_columns_which_cannot_have_defaults_check();
std::unique_ptr<Sql_upgrade_check> get_invalid_57_names_check();
std::unique_ptr<Sql_upgrade_check> get_orphaned_routines_check();
std::unique_ptr<Sql_upgrade_check> get_dollar_sign_name_check();
std::unique_ptr<Sql_upgrade_check> get_index_too_large_check();
std::unique_ptr<Sql_upgrade_check> get_empty_dot_table_syntax_check();
std::unique_ptr<Sql_upgrade_check> get_deprecated_auth_method_check(
    const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check> get_deprecated_router_auth_method_check(
    const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check> get_deprecated_default_auth_check(
    const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check>
get_deprecated_partition_temporal_delimiter_check();

using Formatter = std::function<std::string(
    const Upgrade_check *check, const std::string &item, const char *extra)>;

std::unique_ptr<Upgrade_check> get_config_check(
    const char *name, const std::map<std::string, const char *> &vars,
    const Formatter &formatter, Config_mode mode = Config_mode::DEFINED,
    Upgrade_issue::Level level = Upgrade_issue::ERROR);

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CREATORS_H_