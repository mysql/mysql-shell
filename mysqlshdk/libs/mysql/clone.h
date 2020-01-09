/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_CLONE_H_
#define MYSQLSHDK_LIBS_MYSQL_CLONE_H_

#include <string>
#include <vector>

#include "mysql/instance.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlshdk {
namespace mysql {

constexpr const char k_mysql_clone_plugin_name[] = "clone";

// # of seconds to wait until server finished restarting during recovery
constexpr const int k_server_recovery_restart_timeout = 60;

// The 1st version where the remote clone plugin became available
const mysqlshdk::utils::Version k_mysql_clone_plugin_initial_version("8.0.17");

/**
 * Check if the Clone  plugin is installed, and if not try to
 * install it. The option file with the instance configuration is updated
 * accordingly if provided.
 *
 * @param instance session object to connect to the target instance.
 * @param config instance configuration file to be updated
 *
 * @throw std::runtime_error if the Clone plugin is DISABLED (cannot be
 * installed online) or if an error occurs installing the plugin.
 *
 * @return A boolean value indicating if the Clone plugin was installed (true)
 * or if it is already installed and ACTIVE (false).
 */
bool install_clone_plugin(const mysqlshdk::mysql::IInstance &instance,
                          mysqlshdk::config::Config *config);

/**
 * Check if the Clone plugin is installed and uninstall it if
 * needed. The option file with the instance configuration is updated
 * accordingly if provided (plugin disabled).
 *
 * @param instance session object to connect to the target instance.
 * @param config instance configuration file to be updated
 *
 * @throw std::runtime_error if an error occurs uninstalling the Clone plugin.
 *
 * @return A boolean value indicating if the Clone plugin was uninstalled (true)
 *         or if it is already not available (false).
 */
bool uninstall_clone_plugin(const mysqlshdk::mysql::IInstance &instance,
                            mysqlshdk::config::Config *config);

/**
 * Check if the Clone plugin is available.
 * The plugin is available if MySQL >= 8.0.17 and is not DISABLED
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A boolean value indicating if the Clone plugin is available (true) or
 * not.
 */
bool is_clone_available(const mysqlshdk::mysql::IInstance &instance);

/**
 * Forces the usage of clone by setting the value of
 * group_replication_clone_threshold to 1
 *
 * @param instance session object to connect to the target instance.
 *
 * @return The original value of group_replication_clone_threshold
 */
int64_t force_clone(const mysqlshdk::mysql::IInstance &instance);

/**
 * Performs a clone of an instance using the MySQL Clone Plugin
 *
 * @param recipient Instance object of the target instance (recipient).
 * @param clone_donor_ops Connections_options object with the connection
 * information of the donor instance
 * @param clone_recovery_account Auth_options object with the clone recovery
 * account
 *
 * @return nothing
 */
void do_clone(const mysqlshdk::mysql::IInstance &recipient,
              const mysqlshdk::db::Connection_options &clone_donor_opts,
              const mysqlshdk::mysql::Auth_options &clone_recovery_account);

/**
 * Cancels the on-going execution of a clone of an instance using the MySQL
 * Clone Plugin
 *
 * @param recipient Instance object of the target instance (recipient)
 *
 * @return nothing
 */
void cancel_clone(const mysqlshdk::mysql::IInstance &recipient);

constexpr const char *k_CLONE_STATE_NONE = "Not Started";
constexpr const char *k_CLONE_STATE_STARTED = "In Progress";
constexpr const char *k_CLONE_STATE_SUCCESS = "Completed";
constexpr const char *k_CLONE_STATE_FAILED = "Failed";

constexpr const char *k_CLONE_STAGE_NONE = "None";
constexpr const char *k_CLONE_STAGE_CLEANUP = "DROP DATA";
constexpr const char *k_CLONE_STAGE_FILE_COPY = "FILE COPY";
constexpr const char *k_CLONE_STAGE_PAGE_COPY = "PAGE COPY";
constexpr const char *k_CLONE_STAGE_REDO_COPY = "REDO COPY";
constexpr const char *k_CLONE_STAGE_FILE_SYNC = "FILE SYNC";
constexpr const char *k_CLONE_STAGE_RESTART = "RESTART";
constexpr const char *k_CLONE_STAGE_RECOVERY = "RECOVERY";

struct Clone_status {
  struct Stage_info {
    std::string state;
    std::string stage;
    uint64_t seconds_elapsed = 0;

    uint64_t work_completed = 0;
    uint64_t work_estimated = 0;
  };

  std::string state;
  std::string begin_time;
  std::string end_time;
  uint64_t seconds_elapsed = 0;
  std::string source;
  int error_n = 0;
  std::string error;

  std::vector<Stage_info> stages;

  int current_stage() const {
    for (size_t i = 0; i < stages.size(); i++) {
      if (stages[i].state == k_CLONE_STATE_STARTED) return i;
    }

    if (stages.size() > 0 && stages[0].state == k_CLONE_STATE_NONE) return -1;

    return stages.size() - 1;
  }
};

/**
 * Check clone progress at the given instance.
 *
 * @param instance being cloned (the receiver)
 * @param start_time the timestamp right before clone has started
 *
 * This function assumes the CLONE was started by GR, which means we don't
 * have details about clone errors.
 */
Clone_status check_clone_status(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &start_time);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_CLONE_H_
