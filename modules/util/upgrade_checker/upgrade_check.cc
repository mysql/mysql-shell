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

#include "modules/util/upgrade_checker/upgrade_check.h"

#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include <algorithm>
#include <string>

#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace upgrade_checker {

const std::string &Upgrade_check::get_text(const char *field) const {
  std::string tag{m_name};
  return get_translation(tag.append(".").append(field).c_str());
}

const std::string &Upgrade_check::get_title() const {
  return get_text("title");
}

std::string Upgrade_check::get_description(
    const std::string &group, const Token_definitions &tokens) const {
  std::string description;

  if (!group.empty()) {
    std::string tag{"description."};
    tag += group;
    return resolve_tokens(get_text(tag.c_str()), tokens);
  }

  return resolve_tokens(get_text("description"), tokens);
}

std::vector<std::string> Upgrade_check::get_solutions(
    const std::string &group) const {
  std::vector<std::string> solutions;

  std::string tag{"solution"};
  if (!group.empty()) {
    tag += "." + group;
  }

  auto solution = get_text(tag.c_str());
  int index = 0;
  while (!solution.empty()) {
    solutions.push_back(std::move(solution));
    auto indexed_tag{tag};
    indexed_tag += std::to_string(++index);
    solution = get_text(indexed_tag.c_str());
  }

  return solutions;
}

const std::string &Upgrade_check::get_doc_link(const std::string &group) const {
  if (!group.empty()) {
    std::string tag{"docLink."};
    tag += group;
    return get_text(tag.c_str());
  }
  return get_text("docLink");
}

Upgrade_issue Upgrade_check::create_issue() const {
  Upgrade_issue issue;
  issue.check_name = get_name();
  return issue;
}

Removed_sys_var_check::Removed_sys_var_check(const std::string_view name,
                                             const Upgrade_info &server_info)
    : Upgrade_check(name), m_server_info{server_info} {}

void Removed_sys_var_check::add_sys_var(
    const Version &version, std::map<std::string, const char *> removed_vars) {
  if (Version_condition(version).evaluate(m_server_info)) {
    m_vars.merge(removed_vars);
  }
}

bool Removed_sys_var_check::has_sys_var(const std::string &name) const {
  return m_vars.find(name) != m_vars.end();
}

bool Removed_sys_var_check::enabled() const { return !m_vars.empty(); }

std::vector<Upgrade_issue> Removed_sys_var_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info, Checker_cache *cache) {
  cache->cache_sysvars(session.get(), server_info);

  std::vector<Upgrade_issue> issues;
  const auto &raw_description = get_text("issue");
  const auto &raw_replacement = get_text("replacement");

  for (const auto &var : m_vars) {
    auto sysvar = cache->get_sysvar(var.first);
    if (sysvar && sysvar->source != "COMPILED") {
      Token_definitions tokens = {
          {"item", sysvar->name},
          {"value", sysvar->value},
          {"source", sysvar->source},
      };

      std::string description{raw_description};
      if (var.second) {
        description += ", " + raw_replacement;
        tokens["replacement"] = std::string(var.second);
      }

      description += ".";

      description = resolve_tokens(description, tokens);

      auto issue = create_issue();
      issue.schema = sysvar->name;
      issue.level = Upgrade_issue::ERROR;
      issue.description = shcore::str_format(
          "%s: %s", Upgrade_issue::level_to_string(Upgrade_issue::ERROR),
          description.c_str());

      issue.object_type = Upgrade_issue::Object_type::SYSVAR;

      issues.push_back(std::move(issue));
    }
  }

  return issues;
}

Sys_var_allowed_values_check::Sys_var_allowed_values_check(
    const Upgrade_info &server_info)
    : Upgrade_check(ids::k_sysvar_allowed_values_check),
      m_server_info{server_info} {
  auto version = Version(8, 4, 0);
  std::vector<std::string> valid_ssl_ciphers{"ECDHE-ECDSA-AES128-GCM-SHA256",
                                             "ECDHE-ECDSA-AES256-GCM-SHA384",
                                             "ECDHE-RSA-AES128-GCM-SHA256",
                                             "ECDHE-RSA-AES256-GCM-SHA384",
                                             "ECDHE-ECDSA-CHACHA20-POLY1305",
                                             "ECDHE-RSA-CHACHA20-POLY1305",
                                             "ECDHE-ECDSA-AES256-CCM",
                                             "ECDHE-ECDSA-AES128-CCM",
                                             "DHE-RSA-AES128-GCM-SHA256",
                                             "DHE-RSA-AES256-GCM-SHA384",
                                             "DHE-RSA-AES256-CCM",
                                             "DHE-RSA-AES128-CCM",
                                             "DHE-RSA-CHACHA20-POLY1305",
                                             ""};

  add_sys_var(version, "ssl_cipher", valid_ssl_ciphers);
  add_sys_var(version, "admin_ssl_cipher", valid_ssl_ciphers);

  std::vector<std::string> valid_tls_cipherssuites{
      "TLS_AES_128_GCM_SHA256", "TLS_AES_256_GCM_SHA384",
      "TLS_CHACHA20_POLY1305_SHA256", "TLS_AES_128_CCM_SHA256", ""};
  add_sys_var(version, "tls_ciphersuites", valid_tls_cipherssuites);
  add_sys_var(version, "admin_tls_ciphersuites", valid_tls_cipherssuites);
}

void Sys_var_allowed_values_check::add_sys_var(
    const Version &version, const std::string &name,
    const std::vector<std::string> &defaults) {
  if (Version_condition(version).evaluate(m_server_info)) {
    m_sys_vars[name] = defaults;
  }
}

bool Sys_var_allowed_values_check::has_sys_var(const std::string &name) const {
  return m_sys_vars.find(name) != m_sys_vars.end();
}

bool Sys_var_allowed_values_check::enabled() const {
  return !m_sys_vars.empty();
}

std::vector<Upgrade_issue> Sys_var_allowed_values_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info, Checker_cache *cache) {
  cache->cache_sysvars(session.get(), server_info);

  const auto &raw_description = get_text("issue");

  std::vector<Upgrade_issue> issues;

  for (const auto &variable : m_sys_vars) {
    // Tests for the definition to be enabled

    const auto *cached_var = cache->get_sysvar(variable.first);

    if (cached_var && cached_var->source != "COMPILED") {
      if (std::find(variable.second.begin(), variable.second.end(),
                    cached_var->value) == std::end(variable.second)) {
        auto allowed = shcore::str_join(variable.second, ", ");

        auto description =
            resolve_tokens(raw_description, {{"variable", variable.first},
                                             {"value", cached_var->value},
                                             {"source", cached_var->source},
                                             {"allowed", allowed}});

        auto issue = create_issue();
        issue.schema = variable.first;
        issue.level = Upgrade_issue::ERROR;
        issue.description = shcore::str_format(
            "%s: %s", Upgrade_issue::level_to_string(Upgrade_issue::ERROR),
            description.c_str());
        issue.object_type = Upgrade_issue::Object_type::SYSVAR;

        issues.push_back(std::move(issue));
      }
    }
  }

  return issues;
}

Sysvar_new_defaults::Sysvar_new_defaults(const Upgrade_info &server_info)
    : Upgrade_check(ids::k_sys_vars_new_defaults_check),
      m_server_info{server_info} {
  add_sys_var(
      Version(8, 0, 11),
      {
          {"character_set_server", "from latin1 to utf8mb4"},
          {"collation_server", "from latin1_swedish_ci to utf8mb4_0900_ai_ci"},
          {"explicit_defaults_for_timestamp", "from OFF to ON"},
          {"optimizer_trace_max_mem_size", "from 16KB to 1MB"},
          {"back_log", nullptr},
          {"max_allowed_packet", "from 4194304 (4MB) to 67108864 (64MB)"},
          {"max_error_count", "from 64 to 1024"},
          {"event_scheduler", "from OFF to ON"},
          {"table_open_cache", "from 2000 to 4000"},
          {"log_error_verbosity", "from 3 (Notes) to 2 (Warning)"},
          {"innodb_undo_tablespaces", "from 0 to 2"},
          {"innodb_undo_log_truncate", "from OFF to ON"},
          {"innodb_flush_method",
           "from NULL to fsync (Unix), unbuffered (Windows)"},
          {"innodb_autoinc_lock_mode",
           "from 1 (consecutive) to 2 (interleaved)"},
          {"innodb_flush_neighbors", "from 1 (enable) to 0 (disable)"},
          {"innodb_max_dirty_pages_pct_lwm", "from_0 (%) to 10 (%)"},
          {"innodb_max_dirty_pages_pct", "from 75 (%)  90 (%)"},
          {"performance_schema_consumer_events_transactions_current",
           "from OFF to ON"},
          {"performance_schema_consumer_events_transactions_history",
           "from OFF to ON"},
          {"log_bin", "from OFF to ON"},
          {"server_id", "from 0 to 1"},
          {"log_slave_updates", "from OFF to ON"},
          {"master_info_repository", "from FILE to TABLE"},
          {"relay_log_info_repository", "from FILE to TABLE"},
          {"transaction_write_set_extraction", "from OFF to XXHASH64"},
          {"slave_rows_search_algorithms",
           "from 'INDEX_SCAN, TABLE_SCAN' to 'INDEX_SCAN, HASH_SCAN'"},
      });

  add_sys_var(
      Version(8, 4, 0),
      {{"innodb_buffer_pool_instances",
        "from 8 (or 1 if innodb_buffer_pool_size < 1GB) to MAX(1, #vcpu/4)"},
       {"innodb_page_cleaners", "from 4 to innodb_buffer_pool_instances"},
       {"innodb_numa_interleave", "from OFF to ON"},
       {"innodb_purge_threads", "from 4 to 1 ( if #vcpu <= 16 )"},
       {"innodb_read_io_threads", "from 4 to MAX(#vcpu/2, 4)"},
       {"innodb_parallel_read_threads", "from 4 to MAX(#vcpu/8, 4)"},
       {"innodb_io_capacity", "from 200 to 10000"},
       {"innodb_io_capacity_max", "from 200 to 2 x innodb_io_capacity"},
       {"innodb_flush_method",
        "from fsynch (unix) or unbuffered (windows) to O_DIRECT"},
       {"innodb_log_buffer_size", "from 16777216 (16MB) to 67108864 (64MB)"},
       {"innodb_log_writer_threads",
        "from ON to OFF ( if #vcpu <= 32 )"},  // WTF!
       {"innodb_doublewrite_files",
        "from innodb_buffer_pool_instances * 2 to 2"},
       {"innodb_doublewrite_pages", "from innodb_write_io_threads to 128"},
       {"innodb_change_buffering", "from all to none"},
       {"innodb_adaptive_hash_index", "from ON to OFF"},
       {"innodb_buffer_pool_in_core_file", "from ON to OFF"},
       {"innodb_redo_log_capacity",
        "from 104857600 (100MB) to MIN ( #vcpu/2, 16 )GB"},
       {"group_replication_consistency",
        "from EVENTUAL to BEFORE_ON_PRIMARY_FAILOVER"},
       {"group_replication_exit_state_action",
        "from READ_ONLY to OFFLINE_MODE"},
       {"binlog_transaction_dependency_tracking",
        "from COMMIT_ORDER to WRITESET"}});
}

void Sysvar_new_defaults::add_sys_var(Version version,
                                      const Sysvar_defaults &defaults) {
  if (Version_condition(version).evaluate(m_server_info)) {
    m_version_defaults[std::move(version)] = defaults;
  }
}

bool Sysvar_new_defaults::has_sys_var(const std::string &name) const {
  for (const auto &version_defaults : m_version_defaults) {
    if (version_defaults.second.find(name) != version_defaults.second.end()) {
      return true;
    }
  }

  return false;
}

bool Sysvar_new_defaults::enabled() const {
  return !m_version_defaults.empty();
}

std::vector<Upgrade_issue> Sysvar_new_defaults::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info, Checker_cache *cache) {
  cache->cache_sysvars(session.get(), server_info);

  std::vector<Upgrade_issue> issues;

  // Gets the issue description to be used
  auto raw_description = get_text("issue");

  for (const auto &version_defaults : m_version_defaults) {
    for (const auto &sys_default : version_defaults.second) {
      auto sysvar = cache->get_sysvar(sys_default.first);
      // If the variable doesn't exist (i.e. in the config file) or exist
      // with the COMPILED value (the default)
      if (!sysvar || (sysvar && sysvar->source == "COMPILED")) {
        auto issue = create_issue();
        issue.schema = sys_default.first;
        Token_definitions tokens = {
            {"level", Upgrade_issue::level_to_string(Upgrade_issue::WARNING)},
            {"item", sys_default.first}};

        std::string description{raw_description};
        description = resolve_tokens(description, tokens);

        if (sys_default.second != nullptr) {
          description.append(" ");
          description.append(sys_default.second);
        }

        description += ".";

        issue.description = description;

        issue.level = Upgrade_issue::WARNING;

        issue.object_type = Upgrade_issue::Object_type::SYSVAR;

        issues.push_back(std::move(issue));
      }
    }
  }

  return issues;
}

Invalid_privileges_check::Invalid_privileges_check(
    const Upgrade_info &server_info)
    : Upgrade_check(ids::k_invalid_privileges_check),
      m_upgrade_info(server_info) {
  set_groups({k_dynamic_group});
  if (m_upgrade_info.server_version > Version(8, 0, 0)) {
    add_privileges(Version(8, 4, 0), {"SET_USER_ID"});
  }
}

void Invalid_privileges_check::add_privileges(
    Version version, const std::set<std::string> &privileges) {
  if (Version_condition(version).evaluate(m_upgrade_info)) {
    m_privileges[version] = privileges;
  }
}

bool Invalid_privileges_check::has_privilege(const std::string &privilege) {
  for (const auto &it : m_privileges) {
    if (it.second.find(privilege) != it.second.end()) {
      return true;
    }
  }

  return false;
}

bool Invalid_privileges_check::enabled() const { return !m_privileges.empty(); }

std::vector<Upgrade_issue> Invalid_privileges_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info & /*server_info*/) {
  session->execute("set @@session.sql_mode='TRADITIONAL'");
  shcore::on_leave_scope restore_sql_mode([&session]() {
    session->execute("set @@session.sql_mode=@@global.sql_mode");
  });

  auto result = session->query("select user, host from mysql.user");
  std::vector<std::pair<std::string, std::string>> all_users;
  if (result) {
    while (auto *row = result->fetch_one()) {
      all_users.emplace_back(row->get_string(0), row->get_string(1));
    }
  }

  std::vector<Upgrade_issue> issues;

  mysqlshdk::mysql::Instance instance(session);
  for (const auto &user : all_users) {
    const auto privileges =
        instance.get_user_privileges(user.first, user.second, true);
    for (const auto &invalid : m_privileges) {
      const auto presult = privileges->validate(invalid.second);
      const auto &missing = presult.missing_privileges();

      // If all privileges are missing, we are good, it means no invalid
      // privileges are found
      if (missing.size() != invalid.second.size()) {
        std::vector<std::string> invalid_list;
        std::set_difference(invalid.second.begin(), invalid.second.end(),
                            missing.begin(), missing.end(),
                            std::back_inserter(invalid_list));

        auto issue = create_issue();
        std::string raw_description = get_text("issue");
        issue.schema = shcore::make_account(user.first, user.second);
        issue.level = Upgrade_issue::NOTICE;
        issue.group = shcore::str_join(invalid_list, ", ");

        issue.description = resolve_tokens(
            raw_description,
            {{"account", issue.schema},
             {"privileges", shcore::str_join(invalid_list, ", ")}});
        issue.object_type = Upgrade_issue::Object_type::USER;
        issues.push_back(issue);
      }
    }
  }

  return issues;
}

}  // namespace upgrade_checker
}  // namespace mysqlsh