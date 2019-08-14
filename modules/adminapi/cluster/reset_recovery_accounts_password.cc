/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/reset_recovery_accounts_password.h"

#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/utils.h"

namespace mysqlsh {
namespace dba {

Reset_recovery_accounts_password::Reset_recovery_accounts_password(
    const bool interactive, mysqlshdk::utils::nullable<bool> force,
    const Cluster_impl &cluster)
    : m_interactive(interactive), m_force(force), m_cluster(cluster) {}

bool Reset_recovery_accounts_password::prompt_to_force_reset() const {
  auto console = mysqlsh::current_console();
  console->println();
  bool result = console->confirm(
                    "Do you want to continue anyway (the recovery password for "
                    "the instance will not be reset)?",
                    Prompt_answer::NO) == Prompt_answer::YES;
  console->println();
  return result;
}

void Reset_recovery_accounts_password::prepare() {
  // Get all cluster instances from the Metadata and their respective GR state.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instance_defs =
          m_cluster.get_default_replicaset()->get_instances_with_state();

  for (const auto &instance_def : instance_defs) {
    mysqlshdk::gr::Member_state state = instance_def.second.state;
    if (state == mysqlshdk::gr::Member_state::ONLINE) {
      // Verify if ONLINE instances are reachable.
      ensure_instance_reachable(instance_def.first.endpoint);
    } else {
      // Handle not ONLINE instances,
      handle_not_online_instances(instance_def.first.endpoint,
                                  mysqlshdk::gr::to_string(state));
    }

    // If user decided to skip all instance with errors then assume force=true.
    if (m_force.is_null() && (!m_skipped_instances.empty())) {
      m_force = true;
    }
  }
}

void Reset_recovery_accounts_password::ensure_instance_reachable(
    const std::string &instance_address) {
  try {
    m_online_instances.emplace_back(
        m_cluster.get_session_to_cluster_instance(instance_address));
  } catch (const std::exception &err) {
    // instance not reachable
    auto console = mysqlsh::current_console();

    // Handle use of 'force' option.
    if (m_force.is_null() || *m_force == false) {
      console->print_error(
          "Unable to connect to instance '" + instance_address +
          "'. Please, verify connection credentials and make sure the "
          "instance is available.");

      // In interactive mode and 'force' option not used, ask user to
      // continue with the operation.
      bool continueReset = false;
      if (m_interactive && m_force.is_null()) {
        continueReset = prompt_to_force_reset();
      }

      // If user wants to continue then add instance to list of skipped
      // instances, otherwise issue an error.
      if (continueReset) {
        m_skipped_instances.push_back(instance_address);
      } else {
        throw shcore::Exception::runtime_error(err.what());
      }
    } else {
      m_skipped_instances.push_back(instance_address);
      console->print_note(
          "The recovery password of instance '" + instance_address +
          "' will not be reset because the instance is not reachable.");
      console->println();
    }
  }
}

void Reset_recovery_accounts_password::handle_not_online_instances(
    const std::string &instance_address, const std::string &instance_state) {
  auto console = mysqlsh::current_console();

  if (m_force.is_null() || *m_force == false) {
    // Issue an error if 'force' option is not used or false.
    std::string message =
        "The recovery password of instance '" + instance_address +
        "' cannot be reset because it is on a '" + instance_state +
        "' state. Please bring the instance back ONLINE and try the "
        "<Cluster>.<<<resetRecoveryAccountsPassword>>>() operation again.";
    if (m_interactive && m_force.is_null()) {
      message +=
          " You can choose to proceed with the operation and skip resetting "
          "the instances' recovery account password.";

    } else {
      message +=
          " You can choose to run the "
          "<Cluster>.<<<resetRecoveryAccountsPassword>>>() "
          "with the force option enabled to ignore resetting the password "
          "of instances that are not ONLINE.";
    }
    console->print_error(message);

    // In interactive mode and 'force' option not used, ask user to
    // continue with the operation.
    bool continue_reset = false;
    if (m_interactive && m_force.is_null()) {
      continue_reset = prompt_to_force_reset();
    }

    // If user wants to continue then add instance to list of skipped
    // instances, otherwise issue an error.
    if (continue_reset) {
      m_skipped_instances.push_back(instance_address);
    } else {
      std::string err_msg = "The instance '" + instance_address + "' is '" +
                            instance_state + "' (it must be ONLINE).";
      throw shcore::Exception::runtime_error(err_msg);
    }
  } else {
    m_skipped_instances.push_back(instance_address);
    console->print_note(
        "Skipping reset of the recovery account password for instance '" +
        instance_address + "' because it is '" + instance_state +
        "'. To reset the recovery password, bring the instance back ONLINE and "
        "run the <Cluster>.<<<resetRecoveryAccountsPassword>>>() again.");
    console->println();
  }
}
shcore::Value Reset_recovery_accounts_password::execute() {
  // Determine the recovery user each of the online instances and reset its
  // password

  auto console = mysqlsh::current_console();
  std::string user;
  std::vector<std::string> hosts;
  std::string primary_rpr = m_cluster.get_target_instance()->descr();

  for (const auto &instance : m_online_instances) {
    std::string instance_repr = instance->descr();
    // get recovery user for the instance
    log_debug("Getting recovery user for instance '%s'", instance_repr.c_str());
    try {
      std::tie(user, hosts, std::ignore) =
          m_cluster.get_replication_user(*instance);
    } catch (const shcore::Exception &err) {
      console->print_error(
          "The recovery user name for instance '" + instance->descr() +
          "' does not match the expected format for users "
          "created automatically by InnoDB Cluster. Aborting password reset "
          "operation.");
      throw err;
    }
    // generate a password for the user
    std::string password = mysqlshdk::mysql::generate_password();
    // Change the password on the primary for the user to the newly generated
    // password
    for (const auto &host : hosts) {
      log_debug("Changing the password for recovery user '%s'@'%s' on '%s'",
                user.c_str(), host.c_str(), primary_rpr.c_str());
      m_cluster.get_target_instance()->set_user_password(user, host, password);
    }
    // wait for the password change to be replicated to the instance
    log_debug("Waiting for instance %s to catch up with instance '%s'",
              instance_repr.c_str(), primary_rpr.c_str());
    m_cluster.sync_transactions(*instance);
    // do a change master on the instance to user the new replication account
    log_debug("Changing '%s'\'s recovery credentials", instance_repr.c_str());
    mysqlshdk::gr::change_recovery_credentials(*instance, user, password);
  }

  // Print appropriate output message depending if some operation was skipped.
  if (!m_skipped_instances.empty()) {
    // Some instance were skipped and their recovery account passwords were not
    // reset.
    std::string warning_msg =
        "Not all recovery account passwords were successfully reset, the "
        "following instance";

    if (m_skipped_instances.size() > 1)
      warning_msg.append("s were ");
    else
      warning_msg.append(" was ");

    warning_msg.append("skipped: '" +
                       shcore::str_join(m_skipped_instances, "', '") +
                       "'. Bring ");
    if (m_skipped_instances.size() > 1)
      warning_msg.append("these instances ");
    else
      warning_msg.append("this instance ");

    warning_msg.append(
        "back online and run the "
        "<Cluster>.<<<resetRecoveryAccountsPassword>>>() "
        "operation again if you want to reset ");
    if (m_skipped_instances.size() > 1)
      warning_msg.append("their recovery account passwords.");
    else
      warning_msg.append("its recovery account password.");
    console->print_warning(warning_msg);
  } else {
    // All of cluster's recovery accounts were reset
    console->println(
        "The recovery account passwords of all the cluster instances' were "
        "successfully reset.");
    console->println();
  }
  return shcore::Value();
}

void Reset_recovery_accounts_password::rollback() {
  // Do nothing.
}

void Reset_recovery_accounts_password::finish() {
  // Close all sessions to online instances.
  for (const auto &instance : m_online_instances) {
    instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_online_instances.clear();
  m_skipped_instances.clear();
}

}  // namespace dba
}  // namespace mysqlsh
