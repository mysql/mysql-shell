/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/reset_recovery_accounts_password.h"

#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Reset_recovery_accounts_password::Reset_recovery_accounts_password(
    const bool interactive, std::optional<bool> force,
    const Cluster_impl &cluster)
    : m_interactive(interactive), m_force(force), m_cluster(cluster) {}

bool Reset_recovery_accounts_password::prompt_to_force_reset() const {
  auto console = mysqlsh::current_console();
  console->print_info();
  bool result = console->confirm(
                    "Do you want to continue anyway (the recovery "
                    "password for the instance will not be reset)?",
                    Prompt_answer::NO) == Prompt_answer::YES;
  console->print_info();
  return result;
}

void Reset_recovery_accounts_password::prepare() {
  // check if changing the pwd actually makes sense
  switch (auto auth_type = m_cluster.query_cluster_auth_type(); auth_type) {
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_SUBJECT:
      mysqlsh::current_console()->print_note(shcore::str_format(
          "The Cluster's authentication type is '%s', which doesn't assign a "
          "password for the recovery users.\n",
          to_string(auth_type).c_str()));
      m_is_no_op = true;
      return;
    default:
      break;
  }

  // Get all cluster instances from the Metadata and their respective GR state.
  {
    std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
        instance_defs = m_cluster.get_instances_with_state();

    for (const auto &instance_def : instance_defs) {
      mysqlshdk::gr::Member_state state = instance_def.second.state;
      if (state == mysqlshdk::gr::Member_state::ONLINE) {
        // Verify if ONLINE instances are reachable.
        ensure_instance_reachable(instance_def.first.endpoint, false);
      } else {
        // Handle not ONLINE instances,
        handle_not_online_instances(instance_def.first.endpoint,
                                    mysqlshdk::gr::to_string(state));
      }

      // If user decided to skip all instance with errors then assume
      // force=true.
      if (!m_force.has_value() && (!m_skipped_instances.empty())) {
        m_force = true;
      }
    }
  }

  // retrieve all read replicas
  m_cluster.iterate_read_replicas(
      [this](const Instance_metadata &md,
             const mysqlshdk::mysql::Replication_channel &) {
        // in case of read replicas the state of the channel doesn't really
        // matter, we just need the instances to be reachable
        ensure_instance_reachable(md.endpoint, true);
        return true;
      });
}

void Reset_recovery_accounts_password::ensure_instance_reachable(
    const std::string &instance_address, bool is_read_replica) {
  try {
    m_online_instances.push_back(Instance_online{
        m_cluster.get_session_to_cluster_instance(instance_address),
        is_read_replica});
  } catch (const std::exception &err) {
    // instance not reachable
    auto console = mysqlsh::current_console();

    // Handle use of 'force' option.
    if (!m_force.has_value() || !m_force.value()) {
      console->print_error(shcore::str_format(
          "Unable to connect to instance '%s'. Please, verify connection "
          "credentials and make sure the instance is available.",
          instance_address.c_str()));

      // In interactive mode and 'force' option not used, ask user to
      // continue with the operation.
      bool continueReset = false;
      if (m_interactive && !m_force.has_value()) {
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
          shcore::str_format("The recovery password of instance '%s' will not "
                             "be reset because the instance is not reachable.",
                             instance_address.c_str()));
      console->print_info();
    }
  }
}

void Reset_recovery_accounts_password::handle_not_online_instances(
    const std::string &instance_address, const std::string &instance_state) {
  auto console = mysqlsh::current_console();

  if (!m_force.has_value() || !m_force.value()) {
    // Issue an error if 'force' option is not used or false.
    auto message = shcore::str_format(
        "The recovery password of instance '%s' cannot be reset because it is "
        "on a '%s' state. Please bring the instance back ONLINE and try the "
        "<Cluster>.<<<resetRecoveryAccountsPassword>>>() operation again.",
        instance_address.c_str(), instance_state.c_str());
    if (m_interactive && !m_force.has_value()) {
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
    if (m_interactive && !m_force.has_value()) {
      continue_reset = prompt_to_force_reset();
    }

    // If user wants to continue then add instance to list of skipped
    // instances, otherwise issue an error.
    if (continue_reset) {
      m_skipped_instances.push_back(instance_address);
    } else {
      auto err_msg =
          shcore::str_format("The instance '%s' is '%s' (it must be ONLINE).",
                             instance_address.c_str(), instance_state.c_str());
      throw shcore::Exception::runtime_error(err_msg);
    }
  } else {
    m_skipped_instances.push_back(instance_address);
    console->print_note(shcore::str_format(
        "Skipping reset of the recovery account password for instance '%s' "
        "because it is '%s'. To reset the recovery password, bring the "
        "instance back ONLINE and run the "
        "<Cluster>.<<<resetRecoveryAccountsPassword>>>() again.",
        instance_address.c_str(), instance_state.c_str()));
    console->print_info();
  }
}

shcore::Value Reset_recovery_accounts_password::execute() {
  if (m_is_no_op) return {};

  // Determine the recovery user each of the online instances and reset its
  // password

  auto console = mysqlsh::current_console();
  std::string primary_rpr = m_cluster.get_cluster_server()->descr();

  for (const auto &instance_online : m_online_instances) {
    std::string user;
    std::vector<std::string> hosts;

    std::string instance_repr = instance_online.instance->descr();
    // get recovery user for the instance
    log_debug("Getting recovery user for instance '%s'", instance_repr.c_str());
    try {
      std::tie(user, hosts, std::ignore) = m_cluster.get_replication_user(
          *instance_online.instance, instance_online.is_read_replica);
    } catch (const shcore::Exception &err) {
      if (!err.is_metadata()) {
        console->print_error(shcore::str_format(
            "The recovery user name for instance '%s' does not match the "
            "expected format for users created automatically by InnoDB "
            "Cluster. Please remove and add the instance back to the Cluster "
            "to ensure a supported recovery account is used. Aborting password "
            "reset operation.",
            instance_online.instance->descr().c_str()));
      }
      throw err;
    }
    // generate a password for the user
    std::string password = mysqlshdk::mysql::generate_password();
    // Change the password on the primary for the user to the newly generated
    // password
    for (const auto &host : hosts) {
      log_info("Changing the password for recovery user '%s'@'%s' on '%s'",
               user.c_str(), host.c_str(), primary_rpr.c_str());
      m_cluster.get_metadata_storage()->get_md_server()->set_user_password(
          user, host, password);
    }

    // do a change master on the instance to user the new replication account
    log_info("Changing '%s'\'s recovery credentials", instance_repr.c_str());

    mysqlshdk::mysql::Replication_credentials_options options;
    options.password = std::move(password);

    if (!instance_online.is_read_replica) {
      mysqlshdk::mysql::change_replication_credentials(
          *instance_online.instance, mysqlshdk::gr::k_gr_recovery_channel, user,
          options);
    } else {
      mysqlshdk::mysql::stop_replication_receiver(
          *instance_online.instance,
          mysqlsh::dba::k_read_replica_async_channel_name);

      // we need to re-start the channel regardless of what happens (i.e.: an
      // exception)
      shcore::Scoped_callback start_channel([&instance_online]() {
        mysqlshdk::mysql::start_replication_receiver(
            *instance_online.instance,
            mysqlsh::dba::k_read_replica_async_channel_name);
      });

      mysqlshdk::mysql::change_replication_credentials(
          *instance_online.instance,
          mysqlsh::dba::k_read_replica_async_channel_name, user, options);
    }
  }

  // Print appropriate output message depending if some operation was skipped.
  if (!m_skipped_instances.empty()) {
    // Some instance were skipped and their recovery account passwords were not
    // reset.
    std::string warning_msg =
        "Not all recovery or replication account passwords were successfully "
        "reset, the following instance";

    if (m_skipped_instances.size() > 1)
      warning_msg.append("s were ");
    else
      warning_msg.append(" was ");

    warning_msg.append("skipped: '");
    warning_msg.append(shcore::str_join(m_skipped_instances, "', '"));
    warning_msg.append("'. Bring ");

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
    console->print_info(
        "The recovery and replication account passwords of all the cluster "
        "instances' were successfully reset.");
    console->print_info();
  }
  return shcore::Value();
}

void Reset_recovery_accounts_password::finish() {
  // Close all sessions to online instances.
  for (const auto &instance_online : m_online_instances) {
    instance_online.instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_online_instances.clear();
  m_skipped_instances.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
