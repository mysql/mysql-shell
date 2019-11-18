/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/plugin.h"

namespace mysqlshdk {
namespace mysql {

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
  if (instance.get_version() >= k_mysql_clone_plugin_initial_version) {
    log_debug("Checking if instance '%s' has the clone plugin installed",
              instance.descr().c_str());

    mysqlshdk::utils::nullable<std::string> plugin_state =
        instance.get_plugin_status(k_mysql_clone_plugin_name);

    // Check if the plugin_state is "DISABLED"
    if (!plugin_state.is_null() && (*plugin_state).compare("DISABLED") == 0) {
      log_debug("Instance '%s' has the clone plugin disabled",
                instance.descr().c_str());
    }

    return true;
  }

  return false;
}

int64_t force_clone(const mysqlshdk::mysql::IInstance &instance) {
  mysqlshdk::utils::nullable<int64_t> current_threshold =
      instance.get_sysvar_int("group_replication_clone_threshold");

  // Set the threshold to 1 to force a clone
  instance.set_sysvar("group_replication_clone_threshold", int64_t(1));

  // Return the value if we can obtain it, otherwise return -1 to flag that the
  // default should be used
  if (!current_threshold.is_null()) {
    return *current_threshold;
  } else {
    return -1;
  }
}

void do_clone(const mysqlshdk::mysql::IInstance &recipient,
              const mysqlshdk::db::Connection_options &clone_donor_opts,
              const mysqlshdk::mysql::Auth_options &clone_recovery_account) {
  log_debug("Cloning instance '%s' into '%s'.",
            clone_donor_opts.uri_endpoint().c_str(), recipient.descr().c_str());

  std::string clone_donor_host = clone_donor_opts.get_host();
  std::string clone_donor_port = std::to_string(clone_donor_opts.get_port());

  std::string clone_stmt_ftm = "CLONE INSTANCE FROM ?@?:" + clone_donor_port +
                               " IDENTIFIED BY /*((*/ ? /*))*/";

  try {
    recipient.executef(clone_stmt_ftm, clone_recovery_account.user,
                       clone_donor_host, *clone_recovery_account.password);
  } catch (const std::exception &err) {
    throw;
  }
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

    result = instance.queryf(
        "SELECT *, end_time-begin_time as elapsed"
        " FROM performance_schema.clone_progress WHERE id = ?",
        id);

    while (auto prow = result->fetch_one_named()) {
      Clone_status::Stage_info stage;

      stage.stage = prow.get_string("STAGE");
      stage.state = prow.get_string("STATE");
      stage.seconds_elapsed = prow.get_double("elapsed", 0.0);
      stage.work_estimated = prow.get_uint("ESTIMATE", 0);
      stage.work_completed = prow.get_uint("DATA", 0);

      status.stages.push_back(stage);
    }
  }

  return status;
}

}  // namespace mysql
}  // namespace mysqlshdk
