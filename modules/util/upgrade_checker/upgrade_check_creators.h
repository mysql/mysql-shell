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

#include <map>
#include <memory>
#include <string>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/sql_upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check.h"

namespace mysqlsh {
namespace upgrade_checker {

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
std::unique_ptr<Sql_upgrade_check> get_fts_in_tablename_check(
    const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check> get_engine_mixup_check();
std::unique_ptr<Sql_upgrade_check> get_old_geometry_types_check(
    const Upgrade_info &info);
std::unique_ptr<Sql_upgrade_check>
get_changed_functions_generated_columns_check(const Upgrade_info &info);
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

std::unique_ptr<Upgrade_check> get_config_check(
    const char *name, std::map<std::string, const char *> &&vars,
    Config_mode mode = Config_mode::DEFINED,
    Upgrade_issue::Level level = Upgrade_issue::ERROR,
    const char *problem_description = "is set and will be removed",
    const char *title = "", const char *advice = "");

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CREATORS_H_