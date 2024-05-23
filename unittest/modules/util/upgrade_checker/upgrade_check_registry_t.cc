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
#include "unittest/gprod_clean.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/feature_life_cycle_check.h"
#include "modules/util/upgrade_checker/sql_upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check_config.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "modules/util/upgrade_checker/upgrade_check_registry.h"
#include "unittest/modules/util/upgrade_checker/test_utils.h"
#include "unittest/test_utils.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_check_registry, messages) {
  // checkTableOutput is available all the time as it has no conditions
  Upgrade_check_config config =
      create_config(Version(5, 7, 44), Version(8, 0, 0), "");
  config.set_targets(Target_flags::all());

  auto checklist = Upgrade_check_registry::create_checklist(config, true);

  using Tag_list = std::vector<std::string>;
  using Additional_checks = std::pair<bool, Tag_list>;
  std::unordered_map<std::string_view, Additional_checks> additional_checks;

  // By default, all checks must have title, description, link and no additional
  // tags
  for (const auto &id : ids::all) {
    additional_checks[id] = {true, {}};
  }

  // If not the case, the correct info should be set
  additional_checks[ids::k_innodb_rowformat_check] = {false, {}};
  additional_checks[ids::k_table_command_check] = {false, {}};
  additional_checks[ids::k_schema_inconsistency_check] = {false, {}};
  additional_checks[ids::k_fts_in_tablename_check] = {false, {}};
  additional_checks[ids::k_engine_mixup_check] = {false, {}};
  additional_checks[ids::k_old_geometry_types_check] = {false, {}};
  additional_checks[ids::k_orphaned_objects_check] = {false,
                                                      {"routine", "event"}};
  additional_checks[ids::k_dollar_sign_name_check] = {false, {}};
  additional_checks[ids::k_index_too_large_check] = {false, {}};
  additional_checks[ids::k_empty_dot_table_syntax_check] = {false, {}};
  additional_checks[ids::k_invalid_engine_foreign_key_check] = {false, {}};
  additional_checks[ids::k_deprecated_default_auth_check] = {false, {}};
  additional_checks[ids::k_auth_method_usage_check] = {
      false,
      {"description.Notice", "description.Warning", "description.Error",
       "description.Notice.Replacement", "description.Warning.Replacement",
       "description.Error.Replacement", "docLink.authentication_fido",
       "docLink.sha256_password", "docLink.mysql_native_password"}};

  additional_checks[ids::k_plugin_usage_check] = {
      false,
      {"description.Notice", "description.Warning", "description.Error",
       "description.Notice.Replacement", "description.Warning.Replacement",
       "description.Error.Replacement", "docLink.authentication_fido",
       "docLink.keyring_file", "docLink.keyring_encrypted_file",
       "docLink.keyring_oci"}};

  additional_checks[ids::k_column_definition] = {
      false, {"floatAutoIncrement", "doubleAutoIncrement"}};
  additional_checks[ids::k_invalid_privileges_check] = {false, {}};
  additional_checks[ids::k_partitions_with_prefix_keys] = {true, {"issue"}};
  additional_checks[ids::k_sys_vars_check] = {
      false,
      {"description.removed", "description.removedLogging",
       "description.allowedValues", "description.forbiddenValues",
       "description.newDefaults", "description.deprecated", "issue.deprecated",
       "issue.removed", "issue.removedLogging", "issue.newDefaults",
       "issue.allowedValues", "issue.forbiddenValues", "replacement",
       "docLink.removed", "docLink.removedLogging"}};
  additional_checks[ids::k_foreign_key_references] = {
      false,
      {"fkToNonUniqueKey", "fkToPartialKey", "solution", "solution1",
       "solution2"}};

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
      SCOPED_TRACE(
          shcore::str_format("Error in translation for %s", full_tag.c_str())
              .c_str());
      const std::string &value = get_translation(full_tag.c_str());
      EXPECT_FALSE(value.empty());
      EXPECT_STREQ(check->get_text(tag.c_str()).c_str(), value.c_str());
    }
  }
}  // namespace upgrade_checker
namespace {

std::set<std::string_view> positive;
std::set<std::string_view> negative;

bool has_sys_var(Sysvar_check *ch, const std::string &name,
                 const std::string &group) {
  auto it = std::ranges::find_if(
      ch->get_checks(), [&](const auto &item) { return item.name == name; });
  if (it == ch->get_checks().end()) return false;
  if (group == "allowedValues") return !it->allowed_values.empty();
  return false;
}

void test_check_availability(
    std::string_view name, bool availability,
    const std::vector<std::pair<Version, Version>> &versions,
    const std::string &feature = "", const std::string &server_os = "",
    const std::string &group = "") {
  // Track the checks that have been verified
  if (availability) {
    positive.insert(name);
  } else {
    negative.insert(name);
  }

  for (const auto &item : versions) {
    auto ui = mysqlsh::upgrade_checker::create_config(item.first, item.second,
                                                      server_os);
    ui.set_targets(Target_flags::all());

    auto checklist = Upgrade_check_registry::create_checklist(ui);

    bool found = false;
    for (const auto &check : checklist) {
      if (check->get_name() == name) {
        if (feature.empty()) {
          found = true;
        } else {
          const auto feature_check =
              dynamic_cast<Feature_life_cycle_check *>(check.get());
          if (feature_check) {
            found = feature_check->has_feature(feature);
            break;
          }
          const auto sys_var_values_check =
              dynamic_cast<Sysvar_check *>(check.get());
          if (sys_var_values_check) {
            found = has_sys_var(sys_var_values_check, feature, group);
            break;
          }
        }
        break;
      }
    }

    std::string feature_description{feature};
    if (!feature.empty()) {
      feature_description.append(" in ");
    }

    SCOPED_TRACE(shcore::str_format(
        "Validating %s of %supgrade check named "
        "'%.*s' using start version as "
        "%s and target version as %s",
        availability ? "existence" : "inexistence", feature_description.c_str(),
        static_cast<int>(name.length()), name.data(),
        item.first.get_base().c_str(), item.second.get_base().c_str()));
    EXPECT_EQ(found, availability);
  }
}

}  // namespace

TEST(Upgrade_check_registry, create_checklist) {
  std::map<Version, std::set<std::string_view>> single_version_checks = {
      {Version(8, 0, 0),
       {ids::k_invalid_57_names_check, ids::k_orphaned_objects_check,
        ids::k_index_too_large_check, ids::k_empty_dot_table_syntax_check,
        ids::k_invalid_engine_foreign_key_check}},
      {Version(8, 0, 11),
       {ids::k_old_temporal_check, ids::k_routine_syntax_check,
        ids::k_utf8mb3_check, ids::k_mysql_schema_check,
        ids::k_nonnative_partitioning_check, ids::k_foreign_key_length_check,
        ids::k_maxdb_sql_mode_flags_check, ids::k_obsolete_sql_mode_flags_check,
        ids::k_enum_set_element_length_check, ids::k_removed_functions_check,
        ids::k_zero_dates_check, ids::k_schema_inconsistency_check,
        ids::k_engine_mixup_check,
        ids::k_old_geometry_types_check,  // REVIEW OTHER NEGATIVE TEST
        ids::k_default_authentication_plugin_check,
        ids::k_default_authentication_plugin_mds_check}},
      {Version(8, 0, 12), {ids::k_columns_which_cannot_have_defaults_check}},
      {Version(8, 0, 13), {ids::k_groupby_asc_syntax_check}},
      {Version(8, 0, 17), {ids::k_circular_directory_check}},
      {Version(8, 0, 29), {ids::k_deprecated_temporal_delimiter_check}},
      {Version(8, 0, 31), {ids::k_dollar_sign_name_check}},
      {Version(8, 4, 0),
       {ids::k_column_definition, ids::k_partitions_with_prefix_keys}}};

  auto v5_7_0 = Version(5, 7, 0);
  for (const auto &version_check : single_version_checks) {
    for (const auto check : version_check.second) {
      test_check_availability(
          check, true,
          {{v5_7_0, version_check.first},
           {before_version(version_check.first), version_check.first},
           {v5_7_0, after_version(version_check.first)}});

      test_check_availability(
          check, false,
          {{v5_7_0, Version(5, 7, 99)},
           {v5_7_0, before_version(version_check.first)},
           {version_check.first, after_version(version_check.first)}});
    }
  }

  // Check is only available if > 8.0
  test_check_availability(ids::k_invalid_privileges_check, false,
                          {{v5_7_0, Version(5, 7, 99)},
                           {v5_7_0, Version(8, 4, 0)},
                           {v5_7_0, Version(8, 4, 1)}});

  test_check_availability(ids::k_invalid_privileges_check, true,
                          {{Version(8, 0, 11), Version(8, 4, 0)},
                           {Version(8, 0, 11), Version(8, 4, 1)}});

  auto vShell = Version(MYSH_VERSION);
  test_check_availability(ids::k_changed_functions_generated_columns_check,
                          true,
                          {{Version(5, 6, 0), v5_7_0},
                           {Version(8, 0, 27), Version(8, 0, 28)},
                           {Version(5, 6, 0), vShell}});

  test_check_availability(ids::k_changed_functions_generated_columns_check,
                          false,
                          {{Version(5, 6, 8), Version(5, 6, 9)},
                           {v5_7_0, Version(8, 0, 27)},
                           {Version(8, 0, 28), vShell}});

  test_check_availability(ids::k_deprecated_default_auth_check, true,
                          {{v5_7_0, Version(8, 0, 0)},
                           {Version(8, 0, 33), Version(8, 1, 0)},
                           {Version(8, 1, 0), Version(8, 2, 0)},
                           {v5_7_0, vShell}});

  test_check_availability(ids::k_deprecated_default_auth_check, false,
                          {{v5_7_0, Version(5, 7, 44)},
                           {Version(8, 0, 0), Version(8, 0, 99)},
                           {Version(8, 1, 0), Version(8, 1, 99)},
                           {Version(8, 2, 0), vShell}});

  test_check_availability(ids::k_deprecated_router_auth_method_check, true,
                          {{v5_7_0, Version(8, 0, 0)},
                           {Version(8, 0, 33), Version(8, 1, 0)},
                           {Version(8, 1, 0), Version(8, 2, 0)},
                           {v5_7_0, vShell}});

  test_check_availability(ids::k_deprecated_router_auth_method_check, false,
                          {{v5_7_0, Version(5, 7, 44)},
                           {Version(8, 0, 0), Version(8, 0, 99)},
                           {Version(8, 1, 0), Version(8, 1, 99)},
                           {Version(8, 2, 0), vShell}});

  // checkTableCommand is available all the time as it has no conditions
  test_check_availability(ids::k_table_command_check, true,
                          {{v5_7_0, after_version(v5_7_0)},
                           {before_version(vShell), vShell},
                           {vShell, after_version(vShell)}});

  // Avaliable on these conditions for non WIN
  test_check_availability(ids::k_fts_in_tablename_check, true,
                          {{v5_7_0, Version(8, 0, 11)},
                           {Version(8, 0, 10), Version(8, 0, 11)},
                           {v5_7_0, Version(8, 0, 17)}});

  // Unavailable on the same conditions if WIN
  test_check_availability(ids::k_fts_in_tablename_check, false,
                          {{v5_7_0, Version(8, 0, 11)},
                           {Version(8, 0, 10), Version(8, 0, 11)},
                           {v5_7_0, Version(8, 0, 17)}},
                          "", "WIN");

  // Unavailable if reaches 8.0.18
  test_check_availability(
      ids::k_fts_in_tablename_check, false,
      {{v5_7_0, Version(8, 0, 18)}, {v5_7_0, Version(8, 0, 19)}});

  test_check_availability(ids::k_partitioned_tables_in_shared_tablespaces_check,
                          true,
                          {{v5_7_0, Version(8, 0, 11)},
                           {Version(8, 0, 12), Version(8, 0, 13)},
                           {Version(5, 7, 0), vShell}});

  test_check_availability(ids::k_partitioned_tables_in_shared_tablespaces_check,
                          false,
                          {{v5_7_0, Version(8, 0, 10)},
                           {Version(8, 0, 11), Version(8, 0, 12)},
                           {Version(8, 0, 13), vShell}});

  test_check_availability(ids::k_reserved_keywords_check, true,
                          {{v5_7_0, Version(8, 0, 11)},
                           {Version(8, 0, 11), Version(8, 0, 14)},
                           {Version(8, 0, 14), Version(8, 0, 17)},
                           {Version(8, 0, 17), Version(8, 0, 31)}});

  test_check_availability(ids::k_reserved_keywords_check, false,
                          {{v5_7_0, Version(8, 0, 10)},
                           {Version(8, 0, 11), Version(8, 0, 13)},
                           {Version(8, 0, 14), Version(8, 0, 16)},
                           {Version(8, 0, 17), Version(8, 0, 30)},
                           {Version(8, 0, 31), vShell}});

  // Check unavaliability impossible due to present deprecated sysvars that ware
  // never removed
  test_check_availability(ids::k_sys_vars_check, true, {{v5_7_0, vShell}});

  // authentication_fido:
  // Introduced: 8.0.27
  // Deprecated: 8.2.0
  // Removed: 8.4.0
  // Any version starting after the feature was introduced but before the
  // feature was removed, independently of the end version
  test_check_availability(
      ids::k_auth_method_usage_check, true,
      {{Version(8, 0, 27), Version(8, 0, 28)},  // Start on feature introduction
       {Version(8, 0, 28),
        Version(8, 0, 29)},  // Start after feature introduction
       {Version(8, 2, 0), Version(8, 2, 1)},  // Start on feature deprecation
       {Version(8, 2, 1),
        Version(8, 2, 2)}},  // Start after feature deprecation
      "authentication_fido");

  test_check_availability(
      ids::k_auth_method_usage_check, false,
      {{Version(8, 0, 25),
        Version(8, 0, 26)},  // Start and end before the feature
       {Version(8, 0, 26), Version(8, 0, 27)},  // Start before the feature, end
                                                // when feature was introduced
       {Version(8, 0, 26), Version(8, 0, 28)},  // Start before the feature, end
                                                // after feature was introduced
       {Version(8, 4, 0), Version(8, 4, 1)},    // Start on feature removal
       {Version(8, 4, 0), Version(8, 4, 1)}},   // Start after feature removal
      "authentication_fido");

  // sha256_password and "mysql_native_password"
  // Since no start date is defined for these, they are included all the time
  // before the feature is removed
  for (const auto &method : {"sha256_password", "mysql_native_password"}) {
    test_check_availability(
        ids::k_auth_method_usage_check, true,
        {{Version(5, 7, 44),
          Version(8, 0, 0)},  // Start before feature deprecation
         {Version(8, 0, 0), Version(8, 0, 33)},  // Start on feature deprecation
         {Version(8, 0, 33),
          Version(8, 0, 34)},  // Start after feature deprecation
         {Version(8, 3, 0), Version(8, 4, 0)}},  // Start before feature removal
        method);
  }

  // Not included after feature removal
  test_check_availability(
      ids::k_auth_method_usage_check, false,
      {{Version(9, 0, 0), Version(9, 1, 0)},   // Start on feature removal
       {Version(9, 1, 0), Version(9, 2, 0)}},  // Start on feature deprecation
      "mysql_native_password");

  // authentication_fido:
  // Introduced: 8.0.27
  // Deprecated: 8.2.0
  // Removed: 8.4.0
  // Any version starting after the feature was introduced but before the
  // feature was removed, independently of the end version
  test_check_availability(
      ids::k_plugin_usage_check, true,
      {{Version(8, 0, 27), Version(8, 0, 28)},  // Start on feature introduction
       {Version(8, 0, 28),
        Version(8, 0, 29)},  // Start after feature introduction
       {Version(8, 2, 0), Version(8, 2, 1)},  // Start on feature deprecation
       {Version(8, 2, 1),
        Version(8, 2, 2)}},  // Start after feature deprecation
      "authentication_fido");

  test_check_availability(
      ids::k_plugin_usage_check, false,
      {{Version(8, 0, 25),
        Version(8, 0, 26)},  // Start and end before the feature
       {Version(8, 0, 26), Version(8, 0, 27)},  // Start before the feature, end
                                                // when feature was introduced
       {Version(8, 0, 26), Version(8, 0, 28)},  // Start before the feature, end
                                                // after feature was introduced
       {Version(8, 4, 0), Version(8, 4, 1)},    // Start on feature removal
       {Version(8, 4, 0), Version(8, 4, 1)}},   // Start after feature removal
      "authentication_fido");

  for (const auto &method : {"keyring_file", "keyring_encrypted_file"}) {
    test_check_availability(
        ids::k_plugin_usage_check, true,
        {{Version(5, 7, 44),
          Version(8, 0, 34)},  // Start before feature deprecation
         {Version(8, 0, 34),
          Version(8, 0, 35)},  // Start on feature deprecation
         {Version(8, 0, 35),
          Version(8, 0, 36)},  // Start after feature deprecation
         {Version(8, 3, 0), Version(8, 4, 0)}},  // Start before feature removal
        method);

    // Not included after feature removal
    test_check_availability(
        ids::k_plugin_usage_check, false,
        {{Version(8, 4, 0), Version(8, 4, 1)},   // Start on feature removal
         {Version(8, 4, 1), Version(8, 4, 2)}},  // Start on feature deprecation
        method);
  }

  test_check_availability(
      ids::k_plugin_usage_check, true,
      {{Version(5, 7, 44),
        Version(8, 0, 31)},  // Start before feature deprecation
       {Version(8, 0, 31), Version(8, 0, 35)},  // Start on feature deprecation
       {Version(8, 0, 35),
        Version(8, 0, 36)},  // Start after feature deprecation
       {Version(8, 3, 0), Version(8, 4, 0)}},  // Start before feature removal
      "keyring_oci");

  // Not included after feature removal
  test_check_availability(
      ids::k_plugin_usage_check, false,
      {{Version(8, 4, 0), Version(8, 4, 1)},   // Start on feature removal
       {Version(8, 4, 1), Version(8, 4, 2)}},  // Start on feature deprecation
      "keyring_oci");

  // Not included in upgrades that don't cross 8.4.0
  for (const auto &item : {"admin_ssl_cipher", "admin_tls_ciphersuites"}) {
    test_check_availability(ids::k_sys_vars_check, false,
                            {{Version(8, 0, 0), Version(8, 1, 0)},
                             {Version(8, 2, 0), Version(8, 3, 0)}},
                            item, "", "allowedValues");
    // Included in upgrades that get/cross 8.4.0
    test_check_availability(ids::k_sys_vars_check, true,
                            {{Version(8, 3, 0), Version(8, 4, 0)},
                             {Version(8, 4, 0), Version(8, 4, 1)}},
                            item, "", "allowedValues");
  }

  test_check_availability(ids::k_foreign_key_references, true,
                          {{v5_7_0, Version(8, 4, 0)},
                           {v5_7_0, Version(8, 4, 1)},
                           {v5_7_0, Version(9, 0, 0)},
                           {Version(8, 0, 37), Version(8, 4, 0)},
                           {Version(8, 0, 37), Version(8, 4, 1)},
                           {Version(8, 0, 37), Version(9, 0, 0)},
                           {Version(8, 4, 0), Version(9, 0, 0)},
                           {Version(8, 4, 1), Version(9, 0, 0)},
                           {Version(9, 0, 0), Version(9, 1, 0)}});

  test_check_availability(
      ids::k_foreign_key_references, false,
      {{v5_7_0, Version(8, 3, 0)}, {Version(8, 0, 37), Version(8, 3, 0)}});
  {
    // Tests without positive verification are marked as done
    positive.insert(ids::k_innodb_rowformat_check);  // Not used
    positive.insert(ids::k_sys_vars_check);

    std::set<std::string_view> missing;
    std::set_difference(ids::all.begin(), ids::all.end(), positive.begin(),
                        positive.end(), std::inserter(missing, missing.end()));

    if (!missing.empty()) {
      SCOPED_TRACE(shcore::str_format(
          "Missing positive check availability tests for: %s",
          shcore::str_join(missing, ", ").c_str()));
      EXPECT_TRUE(false);
    }
  }

  {
    // Tests without negative verification are marked as done
    negative.insert(ids::k_table_command_check);     // Always enabled
    negative.insert(ids::k_innodb_rowformat_check);  // Not used
    negative.insert(ids::k_sys_vars_check);

    std::set<std::string_view> missing;
    std::set_difference(ids::all.begin(), ids::all.end(), negative.begin(),
                        negative.end(), std::inserter(missing, missing.end()));

    if (!missing.empty()) {
      SCOPED_TRACE(shcore::str_format(
          "Missing negative check availability tests for: %s",
          shcore::str_join(missing, ", ").c_str()));
      EXPECT_TRUE(false);
    }
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
