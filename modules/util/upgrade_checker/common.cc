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

#include "modules/util/upgrade_checker/common.h"

#include <set>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_translate.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace ids {

const std::set<std::string_view> all = {
    k_reserved_keywords_check,
    k_routine_syntax_check,
    k_utf8mb3_check,
    k_innodb_rowformat_check,
    k_nonnative_partitioning_check,
    k_mysql_schema_check,
    k_old_temporal_check,
    k_foreign_key_length_check,
    k_maxdb_sql_mode_flags_check,
    k_obsolete_sql_mode_flags_check,
    k_enum_set_element_length_check,
    k_table_command_check,
    k_partitioned_tables_in_shared_tablespaces_check,
    k_circular_directory_check,
    k_removed_functions_check,
    k_groupby_asc_syntax_check,
    k_removed_sys_log_vars_check,
    k_removed_sys_vars_check,
    k_sys_vars_new_defaults_check,
    k_zero_dates_check,
    k_schema_inconsistency_check,
    k_fts_in_tablename_check,
    k_engine_mixup_check,
    k_old_geometry_types_check,
    k_changed_functions_generated_columns_check,
    k_columns_which_cannot_have_defaults_check,
    k_invalid_57_names_check,
    k_orphaned_objects_check,
    k_dollar_sign_name_check,
    k_index_too_large_check,
    k_empty_dot_table_syntax_check,
    k_invalid_engine_foreign_key_check,
    k_deprecated_router_auth_method_check,
    k_deprecated_default_auth_check,
    k_deprecated_temporal_delimiter_check,
    k_default_authentication_plugin_check,
    k_default_authentication_plugin_mds_check,
    k_auth_method_usage_check,
    k_plugin_usage_check,
    k_sysvar_allowed_values_check,
    k_invalid_privileges_check,
    k_column_definition,
    k_partitions_with_prefix_keys,
    k_foreign_key_references};

}  // namespace ids

namespace {

std::unordered_map<std::string, Version> generate_mapping() {
  std::unordered_map<std::string, Version> result;
  int major;

  for (auto &ver : mysqlshdk::utils::corresponding_versions(
           mysqlshdk::utils::k_shell_version)) {
    major = ver.get_major();

    if (major == 8) {
      // 8 series had two LTS releases
      auto key = std::to_string(major);
      key += '.';
      key += std::to_string(ver.get_minor());

      result.emplace(std::move(key), ver);
    }

    if (major > 8 || ver.get_minor() > 0) {
      // major version corresponds to the latest release in series
      auto key = std::to_string(major);
      result.emplace(std::move(key), std::move(ver));
    }
  }

  return result;
}

}  // namespace

std::unordered_map<std::string, Version> k_latest_versions = generate_mapping();

void Upgrade_info::validate(bool listing) const {
  if (!listing || server_version) {
    if (server_version < Version(5, 7, 0))
      throw std::invalid_argument(shcore::str_format(
          "Detected MySQL server version is %s, but this tool "
          "requires server to be at least at version 5.7",
          server_version.get_base().c_str()));

    if (server_version >= mysqlshdk::utils::k_shell_version)
      throw std::invalid_argument("Detected MySQL Server version is " +
                                  server_version.get_base() +
                                  ". MySQL Shell cannot check MySQL server "
                                  "instances for upgrade if they "
                                  "are at a version the same as or higher than "
                                  "the MySQL Shell version.");
  }

  auto major_minor = [](const Version &version) {
    return Version(version.get_major(), version.get_minor(), 0);
  };

  if (target_version < Version(8, 0, 0) ||
      major_minor(target_version) >
          major_minor(mysqlshdk::utils::k_shell_version))
    throw std::invalid_argument(
        shcore::str_format("This tool supports checking upgrade to MySQL "
                           "servers of the following versions: 8.0 to %d.%d.*",
                           mysqlshdk::utils::k_shell_version.get_major(),
                           mysqlshdk::utils::k_shell_version.get_minor()));

  if (!listing || server_version) {
    if (server_version >= target_version)
      throw std::invalid_argument(
          "Target version must be greater than current version of the server");
  }
}

std::string upgrade_issue_to_string(const Upgrade_issue &problem) {
  std::stringstream ss;
  ss << problem.get_db_object();
  if (!problem.description.empty()) ss << " - " << problem.description;
  return ss.str();
}

const char *Upgrade_issue::level_to_string(const Upgrade_issue::Level level) {
  switch (level) {
    case Upgrade_issue::ERROR:
      return "Error";
    case Upgrade_issue::WARNING:
      return "Warning";
    case Upgrade_issue::NOTICE:
      return "Notice";
  }
  return "Notice";
}

std::string_view Upgrade_issue::type_to_string(
    const Upgrade_issue::Object_type type) {
  switch (type) {
    case Object_type::SCHEMA:
      return "Schema";
    case Object_type::TABLE:
      return "Table";
    case Object_type::VIEW:
      return "View";
    case Object_type::COLUMN:
      return "Column";
    case Object_type::INDEX:
      return "Index";
    case Object_type::FOREIGN_KEY:
      return "ForeignKey";
    case Object_type::ROUTINE:
      return "Routine";
    case Object_type::EVENT:
      return "Event";
    case Object_type::TRIGGER:
      return "Trigger";
    case Object_type::SYSVAR:
      return "SystemVariable";
    case Object_type::USER:
      return "User";
    case Object_type::TABLESPACE:
      return "Tablespace";
    case Object_type::PLUGIN:
      return "Plugin";
  }
  throw std::logic_error("Unexpected Object Type");
}

std::string Upgrade_issue::get_db_object() const {
  std::stringstream ss;
  ss << schema;
  if (!table.empty()) ss << "." << table;
  if (!column.empty()) ss << "." << column;
  return ss.str();
}

Checker_cache::Checker_cache(const Filtering_options *db_filters)
    : m_query_helper(db_filters != nullptr ? *db_filters : m_filters) {
  m_filters.schemas().exclude("information_schema");
  m_filters.schemas().exclude("performance_schema");
  m_filters.schemas().exclude("mysql");
  m_filters.schemas().exclude("sys");

  m_query_helper.set_schema_filter(db_filters != nullptr);
  m_query_helper.set_table_filter();
}

const Checker_cache::Table_info *Checker_cache::get_table(
    const std::string &schema_table, bool case_sensitive) const {
  decltype(m_tables)::const_iterator t;
  if (case_sensitive) {
    t = m_tables.find(schema_table);

  } else {
    t = std::find_if(m_tables.begin(), m_tables.end(),
                     [&schema_table](decltype(m_tables)::value_type it) {
                       return shcore::str_caseeq(it.first, schema_table);
                     });
  }

  return (t != m_tables.end()) ? &t->second : nullptr;
}

void Checker_cache::cache_tables(mysqlshdk::db::ISession *session) {
  if (!m_tables.empty()) return;

  std::string query =
      "SELECT TABLE_SCHEMA, TABLE_NAME, ENGINE FROM information_schema.tables "
      "WHERE ENGINE IS NOT NULL AND " +
      m_query_helper.schema_filter("TABLE_SCHEMA");

  auto res = session->query(query);

  while (auto row = res->fetch_one()) {
    Table_info table{row->get_string(0), row->get_string(1),
                     row->get_string(2)};

    m_tables.emplace(row->get_string(0) + "/" + row->get_string(1),
                     std::move(table));
  }
}

const Checker_cache::Sysvar_info *Checker_cache::get_sysvar(
    const std::string &name) const {
  auto t = m_sysvars.find(name);
  if (t != m_sysvars.end()) return &t->second;
  return nullptr;
}

void Checker_cache::cache_sysvars(mysqlshdk::db::ISession *session,
                                  const Upgrade_info &server_info) {
  // Cache is already loaded...
  if (!m_sysvars.empty()) return;

  if (server_info.server_version < mysqlshdk::utils::Version(8, 0, 0) &&
      server_info.server_version >= mysqlshdk::utils::Version(5, 7, 0)) {
    if (server_info.config_path.empty()) {
      throw Check_configuration_error(
          "To run this check requires full path to MySQL server configuration "
          "file to be specified at 'configPath' key of options dictionary");
    }

    mysqlshdk::config::Config_file cf;
    cf.read(server_info.config_path);

    for (const auto &group : cf.groups()) {
      bool process = false;
      if (shcore::str_beginswith(group, "mysqld")) {
        if (group != "mysqld") {
          const auto tokens = shcore::str_split(group, "-");
          if (tokens.size() == 2) {
            Version config_version(tokens[1]);
            process = server_info.server_version.get_major() ==
                          config_version.get_major() &&
                      server_info.server_version.get_minor() ==
                          config_version.get_minor();
          }
        } else {
          process = true;
        }
      }

      if (process) {
        for (const auto &option : cf.options(group)) {
          auto standardized = shcore::str_replace(option, "-", "_");
          const auto value = cf.get(group, option);

          Sysvar_info sysvar{option, value.value_or("ON"), "SERVER"};

          m_sysvars.emplace(standardized, std::move(sysvar));
        }
      }
    }
  } else {
    auto res = session->query(
        "SELECT sv.variable_name, sv.variable_value, vi.variable_source FROM "
        "performance_schema.session_variables sv, "
        "performance_schema.variables_info vi WHERE sv.variable_name = "
        "vi.variable_name");
    while (auto row = res->fetch_one()) {
      Sysvar_info sysvar{row->get_string(0), row->get_string(1),
                         row->get_string(2)};

      m_sysvars.emplace(row->get_string(0), std::move(sysvar));
    }
  }
}

const std::string &get_translation(const char *item) {
  static shcore::Translation translation = []() {
    std::string path = shcore::get_share_folder();
    path = shcore::path::join_path(path, "upgrade_checker.msg");
    if (!shcore::path::exists(path))
      throw std::runtime_error(
          path + ": not found, shell installation likely invalid");
    return shcore::read_translation_from_file(path.c_str());
  }();

  static const std::string k_empty = "";

  auto it = translation.find(item);
  if (it == translation.end()) return k_empty;
  return it->second;
}

std::string resolve_tokens(const std::string &text,
                           const Token_definitions &replacements) {
  return shcore::str_subvars(
      text,
      [&](std::string_view var) {
        try {
          return replacements.at(var);
        } catch (const std::out_of_range &) {
          throw std::logic_error(
              shcore::str_format("Missing value for token '%s' at "
                                 "Upgrade Checker message '%s'",
                                 std::string(var).c_str(), text.c_str()));
        }
      },
      "%", "%");
}

Upgrade_issue::Level get_issue_level(const Feature_definition &feature,
                                     const Upgrade_info &server_info) {
  // UC Start is either before the feature or after the feature was removed
  if ((feature.start.has_value() &&
       server_info.server_version < *feature.start) ||
      (feature.removed.has_value() &&
       server_info.server_version >= *feature.removed)) {
    throw std::logic_error(
        "Attempt to get issue level on an unavailable feature");
  }

  // UC target is before the feature got deprecated
  if (feature.deprecated.has_value() &&
      server_info.target_version < *feature.deprecated) {
    return Upgrade_issue::Level::NOTICE;
  }

  // UC targe is after deprecation or before removal
  if (!feature.removed.has_value() ||
      server_info.target_version < *feature.removed) {
    return Upgrade_issue::WARNING;
  }

  // UC target is after removal
  return Upgrade_issue::Level::ERROR;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
