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

#include "modules/util/upgrade_checker/feature_life_cycle_check.h"

#include <stdexcept>

#include <rapidjson/document.h>
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "mysqlshdk/libs/db/result.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

Feature_life_cycle_check::Feature_life_cycle_check(
    std::string_view name, const Upgrade_info &server_info)
    : Upgrade_check(name), m_server_info(server_info) {
  set_condition(&m_list_condition);
}

bool Feature_life_cycle_check::enabled() const {
  for (const auto &entry : m_features) {
    if (entry.second.enabled) return true;
  }
  return false;
}

std::string Feature_life_cycle_check::get_description(
    const std::string &group, const Token_definitions &tokens) const {
  Token_definitions local_tokens{tokens};
  std::string tag{"description"};

  if (!group.empty()) {
    const auto &feature = m_features.at(group).feature;
    auto level = get_issue_level(feature, m_server_info);

    tag.append(".").append(Upgrade_issue::level_to_string(level));

    if (feature.replacement.has_value()) {
      tag.append(".").append("Replacement");
    }
    local_tokens["feature_id"] = feature.id;

    if (feature.deprecated.has_value()) {
      local_tokens["deprecated"] = feature.deprecated->get_base();
    }

    if (feature.removed.has_value()) {
      local_tokens["removed"] = feature.removed->get_base();
    }

    if (feature.replacement.has_value()) {
      local_tokens["replacement"] = *feature.replacement;
    }

    local_tokens["item_id"] = group;
  }

  std::string description = get_text(tag.c_str());

  if (description.empty()) {
    throw std::logic_error(shcore::str_format(
        "Missing entry for token in upgrade_checker file: %s", tag.c_str()));
  }

  return resolve_tokens(description, local_tokens);
}

void Feature_life_cycle_check::add_feature(const Feature_definition &feature) {
  Life_cycle_condition feature_condition(feature.start, feature.deprecated,
                                         feature.removed);

  m_list_condition.add_condition(feature_condition);

  m_features[feature.id] = {feature, feature_condition.evaluate(m_server_info)};
}

bool Feature_life_cycle_check::has_feature(
    const std::string &feature_id) const {
  const auto &entry = m_features.find(feature_id);

  return entry != m_features.end() && (*entry).second.enabled;
}

const Feature_definition &Feature_life_cycle_check::get_feature(
    const std::string &feature_id) const {
  return m_features.at(feature_id).feature;
}

void Feature_life_cycle_check::add_issue(
    const std::string &feature, const std::string &item,
    Upgrade_issue::Object_type object_type) {
  m_feature_issues[feature].emplace_back(item, object_type);
}

std::vector<const Feature_definition *> Feature_life_cycle_check::get_features(
    bool only_enabled) const {
  std::vector<const Feature_definition *> features;
  for (const auto &entry : m_features) {
    if (!only_enabled || entry.second.enabled) {
      features.push_back(&entry.second.feature);
    }
  }
  return features;
}

std::vector<Upgrade_issue> Feature_life_cycle_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info) {
  auto result = session->query(build_query());
  const mysqlshdk::db::IRow *row = nullptr;

  while ((row = result->fetch_one()) != nullptr) {
    process_row(row);
  }

  // Now creates the issue list
  std::vector<Upgrade_issue> upgrade_issues;

  // Each item found will create a separate issue
  for (const auto &issues : m_feature_issues) {
    const auto &feature = m_features.at(issues.first).feature;
    auto level = get_issue_level(feature, server_info);

    for (const auto &item : issues.second) {
      Upgrade_issue issue;

      // Issues get the feature set as the group
      issue.group = feature.id;

      issue.doclink = get_doc_link(feature.id);

      issue.schema = item.first;
      issue.level = level;
      issue.object_type = item.second;
      upgrade_issues.push_back(issue);
    }
  }

  return upgrade_issues;
}

Auth_method_usage_check::Auth_method_usage_check(
    const Upgrade_info &server_info)
    : Feature_life_cycle_check(ids::k_auth_method_usage_check, server_info) {
  // Since the groups determine the display order, the groups are set in order
  // of removal of the plugins
  set_groups(
      {"authentication_fido", "mysql_native_password", "sha256_password"});

  add_feature({"authentication_fido", Version(8, 0, 27), Version(8, 2, 0),
               Version(8, 4, 0), "authentication_webauthn"});
  add_feature({"sha256_password",
               {},
               Version(8, 0, 0),
               Version(9, 0, 0),
               "caching_sha2_password"});
  add_feature({"mysql_native_password",
               {},
               Version(8, 0, 0),
               Version(9, 0, 0),
               "caching_sha2_password"});
}

std::string Auth_method_usage_check::build_query() {
  const auto features = get_features(true);

  std::string raw_query;

  if (m_server_info.server_version >= Version(8, 0, 27)) {
    raw_query =
        "SELECT CONCAT(user, '@', host), "
        "COALESCE(JSON_ARRAY_INSERT(JSON_EXTRACT(user_attributes, "
        "'$.multi_factor_authentication[0].plugin', "
        "'$.multi_factor_authentication[1].plugin'), '$[0]', plugin), "
        "JSON_ARRAY(plugin)) FROM "
        "mysql.user WHERE PLUGIN IN (";
  } else {
    raw_query =
        "SELECT CONCAT(user, '@', host), "
        "JSON_ARRAY(plugin) FROM "
        "mysql.user WHERE PLUGIN IN (";
  }

  std::vector<std::string> wildcards(features.size(), "?");
  raw_query.append(shcore::str_join(wildcards, ", "));
  raw_query.append(") AND user NOT LIKE 'mysql_router%'");

  // In 8.0.27 with the introduction of multi-factor authentication,
  // information about 2nd and 3rd factor authentication is stored in
  // user_attributes
  if (m_server_info.server_version >= Version(8, 0, 27)) {
    std::vector<std::string> mfa_searches(
        features.size(), "JSON_SEARCH(user_attributes, 'one', ?) IS NOT NULL");

    raw_query.append(" OR " + shcore::str_join(mfa_searches, " OR "));
  }

  shcore::sqlstring query(std::move(raw_query), 0);

  // Fill the array for plugin column search
  for (const auto &feature : features) {
    query << feature->id;
  }

  // Fill the list of nth factor auth searches
  if (m_server_info.server_version >= Version(8, 0, 27)) {
    for (const auto &feature : features) {
      query << feature->id;
    }
  }
  query.done();

  return query.str();
}

void Auth_method_usage_check::process_row(const mysqlshdk::db::IRow *row) {
  const auto account = row->get_string(0);
  rapidjson::Document doc;

  doc.Parse(row->get_string(1).c_str());

  for (const rapidjson::Value &plugin : doc.GetArray()) {
    std::string name = plugin.GetString();
    if (m_features.find(name) != m_features.end()) {
      add_issue(name, account, Upgrade_issue::Object_type::USER);
    }
  }
}

Plugin_usage_check::Plugin_usage_check(const Upgrade_info &server_info)
    : Feature_life_cycle_check(ids::k_plugin_usage_check, server_info) {
  add_feature({"authentication_fido", Version(8, 0, 27), Version(8, 2, 0),
               Version(8, 4, 0), "'authentication_webauthn' plugin"});
  add_feature({"keyring_file",
               {},
               Version(8, 0, 34),
               Version(8, 4, 0),
               "the 'component_keyring_file' component"});
  add_feature({"keyring_encrypted_file",
               {},
               Version(8, 0, 34),
               Version(8, 4, 0),
               "the 'component_encrypted_keyring_file' component"});
  add_feature({"keyring_oci",
               {},
               Version(8, 0, 31),
               Version(8, 4, 0),
               "the 'component_keyring_oci' component"});
}

std::string Plugin_usage_check::build_query() {
  const auto features = get_features(true);
  std::vector<std::string> wildcards(features.size(), "?");
  std::string raw_query =
      "SELECT plugin_name FROM information_schema.plugins WHERE "
      "plugin_status "
      "= 'ACTIVE' and plugin_name IN (" +
      shcore::str_join(wildcards, ", ") + ")";

  shcore::sqlstring query(std::move(raw_query), 0);

  // Fill the array for plugin column search
  for (const auto &feature : features) {
    query << feature->id;
  }

  query.done();

  return query.str();
}

void Plugin_usage_check::process_row(const mysqlshdk::db::IRow *row) {
  const auto plugin = row->get_string(0);
  if (m_features.find(plugin) != m_features.end()) {
    add_issue(plugin, plugin, Upgrade_issue::Object_type::PLUGIN);
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
