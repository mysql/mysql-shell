/*
 * Copyright (c) 2019, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <string>

#include "modules/adminapi/common/server_features.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/plugin.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk::mysql {

bool install_clone_plugin(const mysqlshdk::mysql::IInstance &instance,
                          mysqlshdk::config::Config *config) {
  return install_plugin(k_mysql_clone_plugin_name, instance, config);
}

bool uninstall_clone_plugin(const mysqlshdk::mysql::IInstance &instance,
                            mysqlshdk::config::Config *config) {
  return uninstall_plugin(k_mysql_clone_plugin_name, instance, config);
}

bool is_clone_available(const mysqlshdk::mysql::IInstance &instance) {
  // Check if clone is supported
  if (!mysqlsh::dba::supports_mysql_clone(instance.get_version())) return false;

  log_debug("Checking if instance '%s' has the clone plugin installed",
            instance.descr().c_str());

  std::optional<std::string> plugin_state =
      instance.get_plugin_status(k_mysql_clone_plugin_name);

  // Check if the plugin_state is "DISABLED"
  if (plugin_state.has_value() && (plugin_state == "DISABLED"))
    log_debug("Instance '%s' has the clone plugin disabled",
              instance.descr().c_str());

  return true;
}

bool verify_compatible_clone_versions(
    const mysqlshdk::utils::Version &donor,
    const mysqlshdk::utils::Version &recipient) {
  // both must support clone (>= 8.0.17)
  if (!mysqlsh::dba::supports_mysql_clone(donor) ||
      !mysqlsh::dba::supports_mysql_clone(recipient))
    return false;

  // easy path: if {major.minor.patch.build/extra} are equal
  if ((donor == recipient) && (donor.get_extra() == recipient.get_extra()))
    return true;

  // can't mix LTS with innovation releases: {major.minor} must always match
  if (donor.numeric_version_series() != recipient.numeric_version_series())
    return false;

  // reaching this point, both donor and recipient have the same {major.minor}
  // version but with different patch numbers and or build/extra.

  const mysqlshdk::utils::Version min_mix_patch_version(8, 4, 0);
  constexpr uint32_t mix_build_version_series(800);

  // from 8.4 or newer, cloning is allowed if {major.minor} is the same (the
  // rest is ignored)
  if ((donor >= min_mix_patch_version) && (recipient >= min_mix_patch_version))
    return true;

  // if neither are in the 8.0 series, there's nothing to do and clone isn't
  // supported
  if ((donor.numeric_version_series() != mix_build_version_series) ||
      (recipient.numeric_version_series() != mix_build_version_series))
    return false;

  // cloning is allowed if {major.minor.patch} is the same (the build/extra is
  // ignored)
  if (donor == recipient) return true;

  // reaching this point, both donor and recipient are in the 8.0 series,
  // {major.minor} is the same but patch (and possibly build/extra) is different

  const mysqlshdk::utils::Version backported_mix_patch_version(8, 0, 37);

  // cloning between different patch numbers is allowed if *both* versions
  // are 8.0.37 or newer
  if ((donor >= backported_mix_patch_version) &&
      (recipient >= backported_mix_patch_version))
    return true;

  // anything else isn't supported
  return false;
}

int64_t force_clone(const mysqlshdk::mysql::IInstance &instance) {
  auto current_threshold =
      instance.get_sysvar_int("group_replication_clone_threshold");

  // Set the threshold to 1 to force a clone
  instance.set_sysvar("group_replication_clone_threshold", int64_t(1));

  // Return the value if we can obtain it, otherwise return -1 to flag that the
  // default should be used
  return current_threshold.value_or(-1);
}

void do_clone(const mysqlshdk::mysql::IInstance &recipient,
              const mysqlshdk::db::Connection_options &clone_donor_opts,
              const mysqlshdk::mysql::Auth_options &clone_recovery_account,
              bool require_ssl) {
  log_debug("Cloning instance '%s' into '%s'%s.",
            clone_donor_opts.uri_endpoint().c_str(), recipient.descr().c_str(),
            require_ssl ? " (encrypted)" : "");

  if (require_ssl) {
    using namespace std::literals;

    std::array<std::string_view, 3> var_names{"ssl_ca"sv, "ssl_cert"sv,
                                              "ssl_key"sv};

    for (const auto key : var_names) {
      auto value = recipient.get_sysvar_string(key);
      if (!value.has_value()) continue;

      auto var_name = shcore::str_format(
          "clone_%.*s", static_cast<int>(key.size()), key.data());

      recipient.set_sysvar(var_name, *value, Var_qualifier::GLOBAL);
    }
  }

  auto stmt = ("CLONE INSTANCE FROM ?@?:?"_sql << clone_recovery_account.user
                                               << clone_donor_opts.get_host()
                                               << clone_donor_opts.get_port())
                  .str();

  if (clone_recovery_account.password.has_value())
    stmt.append((" IDENTIFIED BY /*((*/ ? /*))*/"_sql
                 << *clone_recovery_account.password)
                    .str());
  else
    stmt.append(" IDENTIFIED BY ''");

  if (require_ssl) stmt.append(" REQUIRE SSL");

  recipient.execute(stmt);
}

void cancel_clone(const mysqlshdk::mysql::IInstance &recipient) {
  auto result =
      recipient.query("SELECT PID FROM performance_schema.clone_status");

  if (auto row = result->fetch_one_named()) {
    uint64_t pid = row.get_uint("PID");

    recipient.executef("KILL QUERY ?", pid);
  }
}

Clone_status check_clone_status(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &start_time) {
  Clone_status status;
  std::shared_ptr<mysqlshdk::db::IResult> result;

  std::string query =
      "SELECT *, end_time-begin_time as elapsed"
      " FROM performance_schema.clone_status";

  if (!start_time.empty()) {
    query += " WHERE begin_time >= ?";
  }

  query += " ORDER BY id DESC LIMIT 1";

  if (!start_time.empty()) {
    result = instance.queryf(query, start_time);
  } else {
    result = instance.query(query);
  }

  if (auto row = result->fetch_one_named()) {
    uint64_t id = row.get_uint("ID");
    status.state = row.get_string("STATE");
    status.begin_time = row.get_string("BEGIN_TIME", "");
    status.end_time = row.get_string("END_TIME", "");
    status.seconds_elapsed = row.get_double("elapsed", 0.0);
    status.source = row.get_string("SOURCE");
    status.error_n = row.get_int("ERROR_NO");
    status.error = row.get_string("ERROR_MESSAGE");

    if (!start_time.empty()) {
      result = instance.queryf(
          "SELECT *, end_time-begin_time as elapsed"
          " FROM performance_schema.clone_progress WHERE id = ? AND begin_time "
          ">= ?",
          id, start_time);
    } else {
      result = instance.queryf(
          "SELECT *, end_time-begin_time as elapsed"
          " FROM performance_schema.clone_progress WHERE id = ?",
          id);
    }

    while (auto prow = result->fetch_one_named()) {
      Clone_status::Stage_info stage;

      stage.stage = prow.get_string("STAGE");
      stage.state = prow.get_string("STATE");
      stage.seconds_elapsed = prow.get_double("elapsed", 0.0);
      stage.work_estimated = prow.get_uint("ESTIMATE", 0);
      stage.work_completed = prow.get_uint("DATA", 0);

      status.stages.push_back(std::move(stage));
    }
  }

  return status;
}

}  // namespace mysqlshdk::mysql
