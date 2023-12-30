/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check_registry.h"

#include <sstream>

#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_translate.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

Upgrade_check_registry::Collection Upgrade_check_registry::s_available_checks;

namespace {

bool UNUSED_VARIABLE(register_old_temporal) =
    Upgrade_check_registry::register_check(std::bind(&get_old_temporal_check),
                                           Target::INNODB_INTERNALS, "8.0.11");

bool UNUSED_VARIABLE(register_routine_syntax) =
    Upgrade_check_registry::register_check(std::bind(&get_routine_syntax_check),
                                           Target::OBJECT_DEFINITIONS,
                                           "8.0.11");

bool UNUSED_VARIABLE(register_reserved) =
    Upgrade_check_registry::register_check(&get_reserved_keywords_check,
                                           Target::OBJECT_DEFINITIONS, "8.0.11",
                                           "8.0.14", "8.0.17", "8.0.31");

bool UNUSED_VARIABLE(register_utf8mb3) = Upgrade_check_registry::register_check(
    std::bind(&get_utf8mb3_check), Target::OBJECT_DEFINITIONS, "8.0.11");

bool UNUSED_VARIABLE(register_mysql_schema) =
    Upgrade_check_registry::register_check(std::bind(&get_mysql_schema_check),
                                           Target::OBJECT_DEFINITIONS,
                                           "8.0.11");

// Zerofill is still available in 8.0.11
// bool UNUSED_VARIABLE(register_zerofill) =
// Upgrade_check_registry::register_check(
//    std::bind(&get_zerofill_check), "8.0.11");
//}

bool UNUSED_VARIABLE(register_nonnative_partitioning) =
    Upgrade_check_registry::register_check(
        std::bind(&get_nonnative_partitioning_check), Target::ENGINES,
        "8.0.11");

bool UNUSED_VARIABLE(register_foreing_key) =
    Upgrade_check_registry::register_check(
        std::bind(&get_foreign_key_length_check), Target::OBJECT_DEFINITIONS,
        "8.0.11");

bool UNUSED_VARIABLE(register_maxdb) = Upgrade_check_registry::register_check(
    std::bind(&get_maxdb_sql_mode_flags_check), Target::OBJECT_DEFINITIONS,
    "8.0.11");

bool UNUSED_VARIABLE(register_sqlmode) = Upgrade_check_registry::register_check(
    std::bind(&get_obsolete_sql_mode_flags_check), Target::OBJECT_DEFINITIONS,
    "8.0.11");

bool UNUSED_VARIABLE(register_enum_set_element_length_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_enum_set_element_length_check),
        Target::OBJECT_DEFINITIONS, "8.0.11");

bool UNUSED_VARIABLE(register_sharded_tablespaces) =
    Upgrade_check_registry::register_check(
        &get_partitioned_tables_in_shared_tablespaces_check,
        Target::TABLESPACES, "8.0.11", "8.0.13");

bool UNUSED_VARIABLE(circular_directory_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_circular_directory_check), Target::TABLESPACES,
        "8.0.17");

bool UNUSED_VARIABLE(register_removed_functions) =
    Upgrade_check_registry::register_check(
        std::bind(&get_removed_functions_check), Target::OBJECT_DEFINITIONS,
        "8.0.11");

bool UNUSED_VARIABLE(register_groupby_syntax_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_groupby_asc_syntax_check), Target::OBJECT_DEFINITIONS,
        "8.0.13");

bool UNUSED_VARIABLE(register_removed_sys_log_vars_check) =
    Upgrade_check_registry::register_check(&get_removed_sys_log_vars_check,
                                           Target::SYSTEM_VARIABLES, "8.0.13");

bool UNUSED_VARIABLE(register_removed_sys_vars_check) =
    Upgrade_check_registry::register_check(&get_removed_sys_vars_check,
                                           Target::SYSTEM_VARIABLES, "8.0.11",
                                           "8.0.13", "8.0.16");

bool UNUSED_VARIABLE(register_sys_vars_new_defaults_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_sys_vars_new_defaults_check), Target::SYSTEM_VARIABLES,
        "8.0.11");

bool UNUSED_VARIABLE(register_zero_dates_check) =
    Upgrade_check_registry::register_check(std::bind(&get_zero_dates_check),
                                           Target::OBJECT_DEFINITIONS,
                                           "8.0.11");

bool UNUSED_VARIABLE(register_schema_inconsistency_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_schema_inconsistency_check), Target::INNODB_INTERNALS,
        "8.0.11");

bool UNUSED_VARIABLE(register_fts_tablename_check) =
    Upgrade_check_registry::register_check(&get_fts_in_tablename_check,
                                           Target::OBJECT_DEFINITIONS,
                                           "8.0.11");

// clang-format on

bool UNUSED_VARIABLE(register_engine_mixup_check) =
    Upgrade_check_registry::register_check(std::bind(&get_engine_mixup_check),
                                           Target::ENGINES, "8.0.11");

bool UNUSED_VARIABLE(register_old_geometry_check) =
    Upgrade_check_registry::register_check(&get_old_geometry_types_check,
                                           Target::INNODB_INTERNALS, "8.0.11");

bool UNUSED_VARIABLE(register_check_table) =
    Upgrade_check_registry::register_check(std::bind(&get_table_command_check),
                                           Target::INNODB_INTERNALS,
                                           ALL_VERSIONS);

bool register_manual_checks() {
  Upgrade_check_registry::register_manual_check(
      "8.0.11", "defaultAuthenticationPlugin", Upgrade_issue::WARNING,
      Target::AUTHENTICATION_PLUGINS);
  Upgrade_check_registry::register_manual_check(
      "8.0.11", "defaultAuthenticationPluginMds", Upgrade_issue::WARNING,
      Target::MDS_SPECIFIC);
  return true;
}

bool UNUSED_VARIABLE(reg_manual_checks) = register_manual_checks();

bool UNUSED_VARIABLE(register_changed_functions_generated_columns) =
    Upgrade_check_registry::register_check(
        &get_changed_functions_generated_columns_check,
        Target::OBJECT_DEFINITIONS, "5.7.0");

bool UNUSED_VARIABLE(register_columns_which_cannot_have_defaults) =
    Upgrade_check_registry::register_check(
        std::bind(&get_columns_which_cannot_have_defaults_check),
        Target::OBJECT_DEFINITIONS, "8.0.12");

bool UNUSED_VARIABLE(register_get_invalid_57_names_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_invalid_57_names_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

bool UNUSED_VARIABLE(register_get_orphaned_routines_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_orphaned_routines_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

bool UNUSED_VARIABLE(register_get_dollar_sign_name_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_dollar_sign_name_check), Target::OBJECT_DEFINITIONS,
        "8.0.31");

bool UNUSED_VARIABLE(register_get_index_too_large_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_index_too_large_check), Target::OBJECT_DEFINITIONS,
        "8.0.0");

bool UNUSED_VARIABLE(register_get_empty_dot_table_syntax_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_empty_dot_table_syntax_check),
        Target::OBJECT_DEFINITIONS, "8.0.0");

bool UNUSED_VARIABLE(register_get_invalid_engine_foreign_key_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_invalid_engine_foreign_key_check),
        Target::OBJECT_DEFINITIONS, "8.0.0");

bool UNUSED_VARIABLE(register_get_deprecated_auth_method_check) =
    Upgrade_check_registry::register_check(&get_deprecated_auth_method_check,
                                           Target::AUTHENTICATION_PLUGINS,
                                           "8.0.0", "8.1.0", "8.2.0");

bool UNUSED_VARIABLE(register_get_deprecated_default_auth_check) =
    Upgrade_check_registry::register_check(&get_deprecated_default_auth_check,
                                           Target::SYSTEM_VARIABLES, "8.0.0",
                                           "8.1.0", "8.2.0");

bool UNUSED_VARIABLE(register_get_deprecated_router_auth_method_check) =
    Upgrade_check_registry::register_check(
        &get_deprecated_router_auth_method_check,
        Target::AUTHENTICATION_PLUGINS, "8.0.0", "8.1.0", "8.2.0");

bool UNUSED_VARIABLE(
    register_get_deprecated_partition_temporal_delimiter_check) =
    Upgrade_check_registry::register_check(
        std::bind(&get_deprecated_partition_temporal_delimiter_check),
        Target::OBJECT_DEFINITIONS, "8.0.29");
}  // namespace

std::vector<std::unique_ptr<Upgrade_check>>
Upgrade_check_registry::create_checklist(const Upgrade_info &info,
                                         Target_flags flags) {
  const Version &src_version(info.server_version);
  const Version &dst_version(info.target_version);

  if (src_version < Version("5.7.0"))
    throw std::invalid_argument(
        shcore::str_format("Detected MySQL server version is %s, but this tool "
                           "requires server to be at least at version 5.7",
                           src_version.get_base().c_str()));

  if (src_version >= Version(MYSH_VERSION))
    throw std::invalid_argument(
        "Detected MySQL Server version is " + src_version.get_base() +
        ". MySQL Shell cannot check MySQL server instances for upgrade if they "
        "are at a version the same as or higher than the MySQL Shell version.");

  if (dst_version < Version("8.0") || dst_version > Version(MYSH_VERSION))
    throw std::invalid_argument(
        "This tool supports checking upgrade to MySQL server versions 8.0.11 "
        "to " MYSH_VERSION);

  if (src_version >= dst_version)
    throw std::invalid_argument(
        "Target version must be greater than current version of the server");

  std::vector<std::unique_ptr<Upgrade_check>> result;
  for (const auto &c : s_available_checks) {
    if (flags.is_set(c.target)) {
      for (const auto &ver : c.versions) {
        if (ver == ALL_VERSIONS || (ver > src_version && ver <= dst_version)) {
          try {
            result.emplace_back(c.creator(info));
          } catch (const Check_not_needed &) {
          }
          break;
        }
      }
    }
  }

  return result;
}

void Upgrade_check_registry::prepare_translation_file(const char *filename) {
  shcore::Translation_writer writer(filename);

  const char *oracle_copyright =
      "Copyright (c) 2018, " PACKAGE_YEAR
      ", Oracle and/or its affiliates.\n\n"
      "This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License, version 2.0,\n"
      "as published by the Free Software Foundation.\n\n"
      "This program is also distributed with certain software (including\n"
      "but not limited to OpenSSL) that is licensed under separate terms, as\n"
      "designated in a particular file or component or in included license\n"
      "documentation.  The authors of MySQL hereby grant you an additional\n"
      "permission to link the program and your derivative works with the\n"
      "separately licensed software that they have included with MySQL.\n"
      "This program is distributed in the hope that it will be useful,  but\n"
      "WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See\n"
      "the GNU General Public License, version 2.0, for more details.\n\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program; if not, write to the Free Software Foundation, "
      "Inc.,\n"
      "51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA";

  writer.write_header(oracle_copyright);
  writer.write_header();
  Upgrade_info info;

  for (const auto &it : s_available_checks) {
    auto check = it.creator(info);
    std::string prefix(check->get_name());
    prefix += ".";

    writer.write_entry((prefix + "title").c_str(), check->get_title_internal(),
                       check->get_text("title"));

    writer.write_entry((prefix + "description").c_str(),
                       check->get_description_internal(),
                       check->get_text("description"));

    writer.write_entry((prefix + "docLink").c_str(),
                       check->get_doc_link_internal(),
                       check->get_text("docLink"));
  }
}

void Upgrade_check_registry::register_manual_check(const char *ver,
                                                   const char *name,
                                                   Upgrade_issue::Level level,
                                                   Target target) {
  s_available_checks.emplace_back(
      Creator_info{std::forward_list<mysqlshdk::utils::Version>{
                       mysqlshdk::utils::Version(ver)},
                   [name, level](const Upgrade_info &) {
                     return std::make_unique<Manual_check>(name, level);
                   },
                   target});
}

}  // namespace upgrade_checker
}  // namespace mysqlsh