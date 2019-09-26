/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/validations.h"

#include <string>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/dba/check_instance.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

void ensure_gr_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance, bool full) {
  auto console = mysqlsh::current_console();

  shcore::Value result;

  if (full) {
    console->print_info(
        "Validating instance configuration at " +
        target_instance->get_connection_options().uri_endpoint() + "...");

    Check_instance check(target_instance->get_connection_options(), "", true);
    check.prepare();
    result = check.execute();
    check.finish();

    if (!result || result.as_map()->at("status").get_string() == "ok") {
      console->print_info("Instance configuration is suitable.");
    } else {
      console->print_error(
          "Instance must be configured and validated with "
          "dba.<<<checkInstanceConfiguration>>>() and "
          "dba.<<<configureInstance>>>() "
          "before it can be used in an InnoDB cluster.");
      throw shcore::Exception::runtime_error("Instance check failed");
    }
  } else {
    checks::validate_host_address(*target_instance, 0);
  }

  validate_replication_filters(*target_instance,
                               Cluster_type::GROUP_REPLICATION);
}

namespace {

void validate_ar_configuration(mysqlshdk::mysql::IInstance *target_instance) {
  auto console = mysqlsh::current_console();

  // Perform check with no update
  bool restart = false;
  bool config_file_change = false;
  bool sysvar_change = false;
  shcore::Value ret_val;

  auto cfg = mysqlsh::dba::create_server_config(
      target_instance, mysqlshdk::config::k_dft_cfg_server_handler, true);

  if (!checks::validate_configuration(
           target_instance, "", cfg.get(), Cluster_type::ASYNC_REPLICATION,
           true, &restart, &config_file_change, &sysvar_change, &ret_val)
           .empty()) {
    if (restart && !config_file_change && !sysvar_change) {
      console->print_note(target_instance->descr() +
                          ": Please restart the MySQL server and try again.");
    } else {
      console->print_error(target_instance->descr() +
                           ": Instance must be configured and validated with "
                           "dba.<<<configureReplicaSetInstance>>>() "
                           "before it can be used in a replicaset.");
    }
    throw shcore::Exception("Instance check failed",
                            SHERR_DBA_INVALID_SERVER_CONFIGURATION);
  } else {
    console->print_info(target_instance->descr() +
                        ": Instance configuration is suitable.");
  }
}

}  // namespace

void ensure_ar_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance) {
  auto console = mysqlsh::current_console();

  checks::validate_host_address(*target_instance, 1);

  validate_ar_configuration(target_instance);

  validate_replication_filters(*target_instance,
                               Cluster_type::ASYNC_REPLICATION);
}

void ensure_user_privileges(const mysqlshdk::mysql::IInstance &instance) {
  std::string current_user, current_host;
  log_debug("Checking user privileges");
  // Get the current user/host
  instance.get_current_user(&current_user, &current_host);

  std::string error_info;
  if (!validate_cluster_admin_user_privileges(instance, current_user,
                                              current_host, &error_info)) {
    auto console = mysqlsh::current_console();
    console->print_error(error_info);
    console->print_info("For more information, see the online documentation.");
    throw shcore::Exception::runtime_error(
        "The account " + shcore::make_account(current_user, current_host) +
        " is missing privileges required to manage an InnoDB cluster.");
  }
}

bool ensure_gtid_sync_possible(const mysqlshdk::mysql::IInstance &master,
                               const mysqlshdk::mysql::IInstance &instance,
                               bool fatal) {
  std::string missing_gtids = master.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(@@global.gtid_purged, ?)",
      mysqlshdk::mysql::get_executed_gtid_set(instance));

  if (!missing_gtids.empty()) {
    log_info("Transactions missing at %s that were purged from primary %s: %s",
             instance.descr().c_str(), master.descr().c_str(),
             missing_gtids.c_str());

    std::string msg = instance.descr() + " is missing " +
                      std::to_string(mysqlshdk::mysql::estimate_gtid_set_size(
                          missing_gtids)) +
                      " transactions that have been purged from the "
                      "current PRIMARY (" +
                      master.descr() + ")";

    if (fatal) {
      current_console()->print_error(msg);
      throw shcore::Exception(
          "Missing purged transactions at " + instance.descr(),
          SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
    } else {
      current_console()->print_warning(msg);
      return false;
    }
  }
  return true;
}

bool ensure_gtid_no_errants(const mysqlshdk::mysql::IInstance &master,
                            const mysqlshdk::mysql::IInstance &instance,
                            bool fatal) {
  std::string slave_gtid_executed =
      mysqlshdk::mysql::get_executed_gtid_set(instance);

  std::string errant_gtids = master.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(?, @@global.gtid_executed)",
      slave_gtid_executed);

  if (!errant_gtids.empty()) {
    log_info("Errant transactions at %s compared to %s: %s",
             instance.descr().c_str(), master.descr().c_str(),
             errant_gtids.c_str());

    std::string msg =
        instance.descr() + " has " +
        std::to_string(mysqlshdk::mysql::estimate_gtid_set_size(errant_gtids)) +
        " errant transactions that have not originated from the current "
        "PRIMARY (" +
        master.descr() + ")";

    if (fatal) {
      current_console()->print_error(msg);
      throw shcore::Exception("Errant transactions at " + instance.descr(),
                              SHERR_DBA_DATA_ERRANT_TRANSACTIONS);
    } else {
      current_console()->print_warning(msg);
      return false;
    }
  }
  return true;
}

}  // namespace dba
}  // namespace mysqlsh
