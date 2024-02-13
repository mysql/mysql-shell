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

Feature_life_cycle_check::Feature_life_cycle_check(std::string_view name,
                                                   Grouping grouping)
    : Upgrade_check(name), m_grouping(grouping) {}

bool Feature_life_cycle_check::enabled() const {
  for (const auto &entry : m_features) {
    if (entry.second.enabled) return true;
  }
  return false;
}

const std::string &Feature_life_cycle_check::get_description() const {
  const auto &description = get_text("description");

  if (description.empty()) {
    throw std::logic_error(shcore::str_format(
        "Missing entry for token in upgrade_checker file: %s.description",
        get_name().c_str()));
  }

  return description;
}

std::string Feature_life_cycle_check::resolve_check_description() const {
  return get_description();
}

std::string Feature_life_cycle_check::resolve_feature_description(
    const Feature_definition &feature, Upgrade_issue::Level level,
    const std::string &item) const {
  std::string tag{"description"};

  tag.append(".").append(Upgrade_issue::level_to_string(level));

  if (feature.replacement.has_value()) {
    tag.append(".").append("Replacement");
  }

  std::string description = get_text(tag.c_str());

  if (description.empty()) {
    throw std::logic_error(shcore::str_format(
        "Missing entry for token in upgrade_checker file: %s", tag.c_str()));
  }

  Token_definitions tokens{{"feature_id", feature.id}};
  if (feature.deprecated.has_value()) {
    tokens["deprecated"] = feature.deprecated->get_base();
  }

  if (feature.removed.has_value()) {
    tokens["removed"] = feature.removed->get_base();
  }

  if (feature.replacement.has_value()) {
    tokens["replacement"] = *feature.replacement;
  }

  if (!item.empty()) {
    tokens["item_id"] = item;
  }

  return resolve_tokens(description, tokens);
}

void Feature_life_cycle_check::add_feature(const Feature_definition &feature,
                                           const Upgrade_info &server_info) {
  m_features[feature.id] = {
      feature,
      Life_cycle_condition(feature.start, feature.deprecated, feature.removed)
          .evaluate(server_info)};
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

void Feature_life_cycle_check::add_issue(const std::string &feature,
                                         const std::string &item) {
  m_feature_issues[feature].emplace_back(std::move(item));
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
  auto result = session->query(build_query(server_info));
  const mysqlshdk::db::IRow *row = nullptr;

  while ((row = result->fetch_one()) != nullptr) {
    process_row(row);
  }

  // Now creates the issue list
  std::vector<Upgrade_issue> upgrade_issues;

  switch (m_grouping) {
    case Grouping::NONE:
      // Each item found will create a separate issue
      for (const auto &issues : m_feature_issues) {
        const auto &feature = m_features.at(issues.first).feature;
        auto level = get_issue_level(feature, server_info);

        for (const auto &item : issues.second) {
          Upgrade_issue issue;
          std::string desc = resolve_feature_description(feature, level, item);
          issue.description = shcore::str_format(
              "%s: %s", Upgrade_issue::level_to_string(level), desc.c_str());

          auto feature_doclink_tag = feature.id + ".docLink";
          issue.doclink = get_text(feature_doclink_tag.c_str());

          issue.level = level;
          upgrade_issues.push_back(issue);
        }
      }
      break;
    case Grouping::FEATURE:
      for (const auto &issues : m_feature_issues) {
        const auto &feature = m_features.at(issues.first).feature;
        auto level = get_issue_level(feature, server_info);
        std::vector<std::string> desc = {
            resolve_feature_description(feature, level)};

        for (const auto &item : issues.second) {
          desc.push_back(item);
        }
        desc.push_back("");  // Forces double \n at the end

        Upgrade_issue issue;
        issue.description =
            shcore::str_format("%s: %s", Upgrade_issue::level_to_string(level),
                               shcore::str_join(desc, "\n").c_str());

        auto feature_doclink_tag = feature.id + ".docLink";
        issue.doclink = get_text(feature_doclink_tag.c_str());

        issue.level = level;
        upgrade_issues.push_back(issue);
      }
      break;
    case Grouping::ALL:
      std::vector<std::string> desc = {resolve_check_description()};
      Upgrade_issue::Level level = Upgrade_issue::NOTICE;
      for (const auto &issues : m_feature_issues) {
        const auto &feature = m_features.at(issues.first).feature;
        auto this_level = get_issue_level(feature, server_info);
        if (this_level < level) {
          level = this_level;
        }
        for (const auto &item : issues.second) {
          desc.push_back(item);
        }
      }
      Upgrade_issue issue;
      issue.description =
          shcore::str_format("%s: %s", Upgrade_issue::level_to_string(level),
                             shcore::str_join(desc, "\n").c_str());
      issue.level = level;
      upgrade_issues.push_back(issue);
      break;
  }

  return upgrade_issues;
}

Auth_method_usage_check::Auth_method_usage_check(
    const Upgrade_info &server_info)
    : Feature_life_cycle_check(ids::k_auth_method_usage_check,
                               Grouping::FEATURE) {
  add_feature({"authentication_fido", Version(8, 0, 27), Version(8, 2, 0),
               Version(8, 4, 0), "authentication_webauthn"},
              server_info);
  add_feature({"sha256_password",
               {},
               Version(8, 0, 0),
               Version(8, 4, 0),
               "caching_sha2_password"},
              server_info);
  add_feature({"mysql_native_password",
               {},
               Version(8, 0, 0),
               Version(8, 4, 0),
               "caching_sha2_password"},
              server_info);
}

std::string Auth_method_usage_check::build_query(
    const Upgrade_info &server_info) {
  const auto features = get_features(true);

  std::string raw_query;

  if (server_info.server_version >= Version(8, 0, 27)) {
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
  if (server_info.server_version >= Version(8, 0, 27)) {
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
  if (server_info.server_version >= Version(8, 0, 27)) {
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
      add_issue(name, account);
    }
  }
}

Plugin_usage_check::Plugin_usage_check(const Upgrade_info &server_info)
    : Feature_life_cycle_check(ids::k_plugin_usage_check, Grouping::NONE) {
  add_feature({"authentication_fido", Version(8, 0, 27), Version(8, 2, 0),
               Version(8, 4, 0), "'authentication_webauthn' plugin"},
              server_info);
  add_feature({"keyring_file",
               {},
               Version(8, 0, 34),
               Version(8, 4, 0),
               "the 'component_keyring_file' component"},
              server_info);
  add_feature({"keyring_encrypted_file",
               {},
               Version(8, 0, 34),
               Version(8, 4, 0),
               "the 'component_encrypted_keyring_file' component"},
              server_info);
  add_feature({"keyring_oci",
               {},
               Version(8, 0, 31),
               Version(8, 4, 0),
               "the 'component_keyring_oci' component"},
              server_info);
}

std::string Plugin_usage_check::build_query(
    const Upgrade_info & /*server_info*/) {
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
    add_issue(plugin, plugin);
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
