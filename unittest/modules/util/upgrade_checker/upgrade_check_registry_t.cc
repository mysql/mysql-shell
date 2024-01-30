/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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
#include "unittest/gprod_clean.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "modules/util/upgrade_checker/upgrade_check_registry.h"
#include "unittest/test_utils.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_check_registry, messages) {
  // checkTableOutput is available all the time as it has no conditions
  Upgrade_info ui;
  ui.server_version = Version(5, 7, 44);
  ui.target_version = Version(8, 0, 0);
  auto checklist =
      Upgrade_check_registry::create_checklist(ui, Target_flags::all(), true);

  using Tag_list = std::vector<std::string>;
  using Additional_checks = std::pair<bool, Tag_list>;
  std::unordered_map<std::string_view, Additional_checks> additional_checks{
      {ids::k_reserved_keywords_check, {true, {}}},
      {ids::k_routine_syntax_check, {true, {}}},
      {ids::k_utf8mb3_check, {true, {}}},
      {ids::k_innodb_rowformat_check, {false, {}}},
      {ids::k_zerofill_check, {false, {}}},
      {ids::k_nonnative_partitioning_check, {true, {}}},
      {ids::k_mysql_schema_check, {true, {}}},
      {ids::k_old_temporal_check, {true, {}}},
      {ids::k_foreign_key_length_check, {true, {}}},
      {ids::k_maxdb_sql_mode_flags_check, {true, {}}},
      {ids::k_obsolete_sql_mode_flags_check, {true, {}}},
      {ids::k_enum_set_element_length_check, {true, {}}},
      {ids::k_table_command_check, {false, {}}},
      {ids::k_partitioned_tables_in_shared_tablespaces_check, {true, {}}},
      {ids::k_circular_directory_check, {true, {}}},
      {ids::k_removed_functions_check, {true, {}}},
      {ids::k_groupby_asc_syntax_check, {true, {}}},
      {ids::k_removed_sys_log_vars_check, {true, {}}},
      {ids::k_removed_sys_vars_check, {true, {"issue", "replacement"}}},
      {ids::k_sys_vars_new_defaults_check, {true, {"issue"}}},
      {ids::k_zero_dates_check, {true, {}}},
      {ids::k_schema_inconsistency_check, {false, {}}},
      {ids::k_fts_in_tablename_check, {false, {}}},
      {ids::k_engine_mixup_check, {false, {}}},
      {ids::k_old_geometry_types_check, {false, {}}},
      {ids::k_changed_functions_generated_columns_check, {true, {}}},
      {ids::k_columns_which_cannot_have_defaults_check, {true, {}}},
      {ids::k_invalid_57_names_check, {true, {}}},
      {ids::k_orphaned_routines_check, {false, {}}},
      {ids::k_dollar_sign_name_check, {false, {}}},
      {ids::k_index_too_large_check, {false, {}}},
      {ids::k_empty_dot_table_syntax_check, {false, {}}},
      {ids::k_invalid_engine_foreign_key_check, {false, {}}},
      {ids::k_deprecated_auth_method_check, {false, {}}},
      {ids::k_deprecated_router_auth_method_check, {true, {}}},
      {ids::k_deprecated_default_auth_check, {false, {}}},
      {ids::k_default_authentication_plugin_check, {true, {}}},
      {ids::k_default_authentication_plugin_mds_check, {true, {}}},
      {ids::k_deprecated_temporal_delimiter_check, {true, {}}}};

  for (const auto &check : checklist) {
    std::string name = check->get_name();

    std::vector<std::string> tags{"title", "description", "docLink"};

    for (const std::string &tag : tags) {
      std::string full_tag = name;
      full_tag.append(".");
      full_tag.append(tag);

      SCOPED_TRACE(
          shcore::str_format("Missing translation for %s", full_tag.c_str()));

      // This guaranteeds the test is properly updated with new checks.
      EXPECT_NO_THROW(additional_checks.at(name));

      const std::string &value = get_translation(full_tag.c_str()).c_str();

      if (tag.compare("docLink") == 0) {
        EXPECT_NE(additional_checks.at(name).first, value.empty());
      } else {
        EXPECT_FALSE(value.empty());
      }
      if (tag.compare("title") == 0) {
        EXPECT_STREQ(check->get_title().c_str(), value.c_str());
      } else if (tag.compare("description") == 0) {
        EXPECT_STREQ(check->get_description().c_str(), value.c_str());
      }
    }

    for (const auto &tag : additional_checks.at(name).second) {
      const auto full_tag = name + "." + tag;
      shcore::str_format("Missing translation for %s", full_tag.c_str());
      const std::string &value = get_translation(full_tag.c_str());
      EXPECT_FALSE(value.empty());
      EXPECT_STREQ(check->get_text(tag.c_str()).c_str(), value.c_str());
    }
  }
}
}  // namespace upgrade_checker
}  // namespace mysqlsh