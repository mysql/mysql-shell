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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_
#define MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_

#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/query_helper.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace ids {
inline constexpr std::string_view k_reserved_keywords_check =
    "reservedKeywords";
inline constexpr std::string_view k_routine_syntax_check = "routineSyntax";
inline constexpr std::string_view k_utf8mb3_check = "utf8mb3";
inline constexpr std::string_view k_innodb_rowformat_check = "innodbRowFormat";
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
inline constexpr std::string_view k_table_command_check = "checkTableCommand";
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
inline constexpr std::string_view k_orphaned_objects_check = "orphanedObjects";
inline constexpr std::string_view k_dollar_sign_name_check = "dollarSignName";
inline constexpr std::string_view k_index_too_large_check = "indexTooLarge";
inline constexpr std::string_view k_empty_dot_table_syntax_check =
    "emptyDotTableSyntax";
inline constexpr std::string_view k_invalid_engine_foreign_key_check =
    "invalidEngineForeignKey";
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
inline constexpr std::string_view k_auth_method_usage_check = "authMethodUsage";
inline constexpr std::string_view k_plugin_usage_check = "pluginUsage";
inline constexpr std::string_view k_sysvar_allowed_values_check =
    "sysvarAllowedValues";
inline constexpr std::string_view k_invalid_privileges_check =
    "invalidPrivileges";
inline constexpr std::string_view k_column_definition = "columnDefinition";
inline constexpr std::string_view k_partitions_with_prefix_keys =
    "partitionsWithPrefixKeys";
inline constexpr std::string_view k_foreign_key_references =
    "foreignKeyReferences";

// NOTE: Every added id should be included here
extern const std::set<std::string_view> all;
}  // namespace ids

// dynamicGroup is meant to be used in checks that require issue grouping, but
// the groups are not hardcoded but generated dynamically
inline constexpr const char *k_dynamic_group = "dynamicGroup";

using mysqlshdk::utils::Version;

// This map should be updated with the latest version of each series to enable
// gatting the latest version available in case a partial version is provided
// as the taget value, so for example, not needed when patch version is 0 in
// the last version of a series
extern std::unordered_map<std::string, Version> k_latest_versions;

enum class Config_mode { DEFINED, UNDEFINED };

enum class Target {
  INNODB_INTERNALS,
  OBJECT_DEFINITIONS,
  ENGINES,
  TABLESPACES,
  SYSTEM_VARIABLES,
  AUTHENTICATION_PLUGINS,
  PLUGINS,
  PRIVILEGES,
  MDS_SPECIFIC,
  LAST,
};

using Target_flags = mysqlshdk::utils::Enum_set<Target, Target::LAST>;
using Check_id_set = std::set<std::string>;

struct Upgrade_info {
  mysqlshdk::utils::Version server_version;
  std::string server_version_long;
  mysqlshdk::utils::Version target_version;
  std::string server_os;
  std::string config_path;
  bool explicit_target_version;

  void validate(bool listing = false) const;
};

struct Upgrade_issue {
  enum Level { ERROR = 0, WARNING, NOTICE };
  enum class Object_type {
    SCHEMA = 0,
    TABLE,
    VIEW,
    COLUMN,
    INDEX,
    FOREIGN_KEY,
    ROUTINE,
    EVENT,
    TRIGGER,
    SYSVAR,
    USER,
    TABLESPACE,
    PLUGIN,
  };

  static std::string_view type_to_string(
      const Upgrade_issue::Object_type level);
  static const char *level_to_string(const Upgrade_issue::Level level);

  std::string schema;
  std::string table;
  std::string column;
  std::string description;
  Level level = ERROR;
  Object_type object_type;

  // To be used for links related to the issue, rather than the check
  std::string doclink;
  std::string group;
  std::string check_name;

  bool empty() const {
    return schema.empty() && table.empty() && column.empty() &&
           description.empty();
  }
  std::string get_db_object() const;
};

std::string upgrade_issue_to_string(const Upgrade_issue &problem);

class Check_configuration_error : public std::runtime_error {
 public:
  explicit Check_configuration_error(const char *what)
      : std::runtime_error(what) {}
};

/**
 * This ENUM defines the possible states of a feature.
 */
enum class Feature_life_cycle_state { OK, DEPRECATED, REMOVED };

class Checker_cache {
 public:
  using Filtering_options = mysqlshdk::db::Filtering_options;
  explicit Checker_cache(Filtering_options *db_filters = nullptr);

  struct Table_info {
    std::string schema_name;
    std::string name;
    std::string engine;
  };

  struct Sysvar_info {
    std::string name;
    std::string value;
    std::string source;
  };

  const Table_info *get_table(const std::string &schema_table,
                              bool case_sensitive = true) const;
  const Sysvar_info *get_sysvar(const std::string &name) const;

  void cache_tables(mysqlshdk::db::ISession *session);
  void cache_sysvars(mysqlshdk::db::ISession *session,
                     const Upgrade_info &server_info);

  const mysqlshdk::db::Query_helper &query_helper() const {
    return m_query_helper;
  }

 private:
  Filtering_options m_filters;
  mysqlshdk::db::Query_helper m_query_helper;
  std::unordered_map<std::string, Table_info> m_tables;
  std::unordered_map<std::string, Sysvar_info> m_sysvars;
};

const std::string &get_translation(const char *item);

using Token_definitions = std::unordered_map<std::string_view, std::string>;

std::string resolve_tokens(const std::string &text,
                           const Token_definitions &replacements);
struct Feature_definition {
  std::string id;
  std::optional<Version> start;
  std::optional<Version> deprecated;
  std::optional<Version> removed;
  std::optional<std::string> replacement;
};

Upgrade_issue::Level get_issue_level(const Feature_definition &feature,
                                     const Upgrade_info &server_info);

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_
