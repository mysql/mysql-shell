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

#include <mysql/group_replication.h>

#include "modules/adminapi/cluster/dissolve.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Dissolve::Dissolve(const bool interactive,
                   mysqlshdk::utils::nullable<bool> force,
                   Cluster_impl *cluster)
    : m_interactive(interactive), m_force(force), m_cluster(cluster) {
  assert(cluster);
  m_primary_index = SIZE_MAX;
}

Dissolve::~Dissolve() {}

void Dissolve::prompt_to_confirm_dissolve() const {
  // Show cluster description if not empty, to help user verify if he is
  // dissolving the right cluster.
  auto console = mysqlsh::current_console();

  if (!m_cluster->get_metadata_storage()->is_cluster_empty(
          m_cluster->get_id())) {
    // Show cluster description.
    console->println(
        "The cluster still has the following registered ReplicaSets:");
    shcore::Value res = m_cluster->describe();

    // Pretty print description only if wrap_json is not json/raw.
    bool use_pretty_print =
        (current_shell_options()->get().wrap_json.compare("json/raw") != 0);
    console->println(res.descr(use_pretty_print));
  }

  console->print_warning(
      "You are about to dissolve the whole cluster and lose the high "
      "availability features provided by it. This operation cannot be "
      "reverted. All members will be removed from their ReplicaSet and "
      "replication will be stopped, internal recovery user accounts and "
      "the cluster metadata will be dropped. User data will be maintained "
      "intact in all instances.");
  console->println();

  if (console->confirm("Are you sure you want to dissolve the cluster?",
                       Prompt_answer::NO) == Prompt_answer::NO) {
    throw shcore::Exception::runtime_error("Operation canceled by user.");
  }
}

bool Dissolve::prompt_to_force_dissolve() const {
  auto console = mysqlsh::current_console();
  console->println();
  bool result = console->confirm(
                    "Do you want to continue anyway (only the instance "
                    "metadata will be removed)?",
                    Prompt_answer::NO) == Prompt_answer::YES;
  console->println();
  return result;
}

void Dissolve::ensure_instance_reachable(
    const std::string &instance_address,
    const Connection_options &cluster_cnx_opt) {
  // Set login credentials to connect to instance.
  // NOTE: It is assumed that the same login credentials can be used to connect
  //       to all cluster instances.
  Connection_options instance_cnx_opts =
      shcore::get_connection_options(instance_address, false);
  instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

  // Try to connect to instance and add it to list of available instances to
  // remove from the cluster.
  log_debug("Connecting to instance '%s'", instance_address.c_str());
  std::shared_ptr<mysqlshdk::db::ISession> session;
  try {
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(instance_cnx_opts);
    m_available_instances.emplace_back(new mysqlshdk::mysql::Instance(session));
    log_debug("Successfully connected to instance");
  } catch (const std::exception &err) {
    log_debug("Failed to connect to instance: %s", err.what());

    auto console = mysqlsh::current_console();

    // Handle use of 'force' option.
    if (m_force.is_null() || *m_force == false) {
      console->print_error(
          "Unable to connect to instance '" + instance_address +
          "'. Please, verify connection credentials and make sure the "
          "instance is available.");

      // In interactive mode and 'force' option not used, ask user to
      // continue with the operation.
      bool continue_dissolve = false;
      if (m_interactive && m_force.is_null()) {
        continue_dissolve = prompt_to_force_dissolve();
      }

      // If user wants to continue then add instance to list of skipped
      // instances, otherwise issue an error.
      if (continue_dissolve) {
        m_skipped_instances.push_back(instance_address);
      } else {
        throw shcore::Exception::runtime_error(err.what());
      }
    } else {
      m_skipped_instances.push_back(instance_address);
      console->print_note(
          "The instance '" + instance_address +
          "' is not reachable and it will only be removed from the metadata. "
          "Please take any necessary actions to make sure that the instance "
          "will not start/rejoin the cluster if brought back online.");
      console->println();
    }
  }
}

void Dissolve::ensure_transactions_sync() {
  // Sync transactions with the cluster on all instance to ensure there are no
  // replication errors (until now).
  for (const auto &instance : m_available_instances) {
    try {
      m_cluster->sync_transactions(*instance);
    } catch (const std::exception &err) {
      std::string instance_address =
          (*instance).get_connection_options().as_uri(
              mysqlshdk::db::uri::formats::only_transport());

      // Handle use of 'force' option.
      if (m_force.is_null() || *m_force == false) {
        mysqlsh::current_console()->print_error(
            "The instance '" + instance_address +
            "' was unable to catch up with cluster transactions. There might "
            "be too many transactions to apply or some replication error. In "
            "the former case, you can retry the operation (using a higher "
            "timeout value by setting the global shell option "
            "'dba.gtidWaitTimeout'). In the later case, analyze and fix any "
            "replication error. You can also choose to skip this error using "
            "the 'force: true' option, but it might leave the instance in an "
            "inconsistent state and lead to errors if you want to reuse it.");

        // In interactive mode and 'force' option not used, ask user to
        // continue with the operation.
        bool continue_dissolve = false;
        if (m_interactive && m_force.is_null()) {
          continue_dissolve = prompt_to_force_dissolve();
        }

        // If 'force' is false (even after user prompt) then issue an error.
        if (continue_dissolve) {
          m_sync_error_instances.push_back(instance_address);
        } else {
          throw;
        }
      } else {
        m_sync_error_instances.push_back(instance_address);
        // If 'force' is true then just log the error and continue (catching up
        // with cluster transaction might work when actually removing the
        // instance). Warning will be issued later if it does not work.
        log_error(
            "Instance '%s' was unable to catch up with cluster transactions "
            "during dissolve preparation: %s",
            instance_address.c_str(), err.what());
      }
    }
  }
}

void Dissolve::handle_unavailable_instances(const std::string &instance_address,
                                            const std::string &instance_state) {
  auto console = mysqlsh::current_console();

  if (m_force.is_null() || *m_force == false) {
    // Issue an error if 'force' option is not used or false.
    std::string message =
        "The instance '" + instance_address +
        "' cannot be removed because it is on a '" + instance_state +
        "' state. Please bring the instance back ONLINE and try to "
        "dissolve the cluster again. If the instance is permanently not "
        "reachable, ";
    if (m_interactive && m_force.is_null()) {
      message +=
          "then you can choose to proceed with the operation and only "
          "remove the instance from the Cluster Metadata.";

    } else {
      message +=
          "then please use <Cluster>.dissolve() with the force option set to "
          "true to proceed with the operation and only remove the instance "
          "from the Cluster Metadata.";
    }
    console->print_error(message);

    // In interactive mode and 'force' option not used, ask user to
    // continue with the operation.
    bool continue_dissolve = false;
    if (m_interactive && m_force.is_null()) {
      continue_dissolve = prompt_to_force_dissolve();
    }

    // If user wants to continue then add instance to list of skipped
    // instances, otherwise issue an error.
    if (continue_dissolve) {
      m_skipped_instances.push_back(instance_address);
    } else {
      std::string err_msg =
          "The instance '" + instance_address + "' is '" + instance_state + "'";
      throw shcore::Exception::runtime_error(err_msg);
    }
  } else {
    m_skipped_instances.push_back(instance_address);
    console->print_note(
        "The instance '" + instance_address + "' is '" + instance_state +
        "' and it will only be removed from the metadata. "
        "Please take any necessary actions to make sure that the instance "
        "will not start/rejoin the cluster if brought back online.");
    console->println();
  }
}

void Dissolve::prepare() {
  // Confirm execution of operation in interactive mode.
  if (m_interactive) {
    prompt_to_confirm_dissolve();
    mysqlsh::current_console()->println();
  }

  // Get cluster session to use the same authentication credentials for all
  // cluster instances.
  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      m_cluster->get_group_session();
  Connection_options cluster_cnx_opt =
      cluster_session->get_connection_options();

  // Determine the primary (if it exists), in order to be the last to be
  // removed from the cluster. Only for single primary mode.
  // NOTE: This is need to avoid a new primary election in the group and GR
  //       BUG#24818604.

  std::string primary =
      mysqlshdk::gr::get_group_primary_uuid(cluster_session, nullptr);

  if (!primary.empty()) {
    mysqlshdk::utils::nullable<mysqlshdk::innodbcluster::Instance_info> info =
        m_cluster->get_metadata_storage()
            ->get_new_metadata()
            ->get_instance_info_by_uuid(primary);
    if (info) m_primary_address = info->classic_endpoint;
  }

  // Get all cluster instances, including state information.
  std::vector<Instance_definition> instance_defs =
      m_cluster->get_metadata_storage()->get_replicaset_instances(
          m_cluster->get_default_replicaset()->get_id(), true);

  // Check if instances can be dissolved. More specifically, verify if their
  // state is valid, if they are reachable (i.e., able to connect to them).
  // Also determine if instance removal can be skipped, according to 'force'
  // option and user prompts.
  for (const auto &instance_def : instance_defs) {
    // Use the GR state hold by instance_def.state (but convert it to a proper
    // mysqlshdk::gr::Member_state to be handled properly).
    mysqlshdk::gr::Member_state state =
        mysqlshdk::gr::to_member_state(instance_def.state);
    if (state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Verify if active instances are reachable.
      ensure_instance_reachable(instance_def.endpoint, cluster_cnx_opt);

      // If the instance is the primary then set the index in the list of
      // available instance (to remove it last from the cluster).
      if (!m_primary_address.empty() &&
          m_primary_address == instance_def.endpoint) {
        m_primary_index = m_available_instances.size() - 1;
      }
    } else {
      // Handle not active instances, determining if they can be skipped.
      handle_unavailable_instances(instance_def.endpoint,
                                   mysqlshdk::gr::to_string(state));
    }
  }

  // Verify if there is any replication error that might make the operation
  // fails, i.e., all (available/reachable) instances are able to catchup with
  // cluster transactions.
  ensure_transactions_sync();

  // If user decided to skip all instance with errors then assume force=true.
  if (m_force.is_null() &&
      (!m_skipped_instances.empty() || !m_sync_error_instances.empty())) {
    m_force = true;
  }
}

void Dissolve::remove_instance(const std::string &instance_address,
                               const std::size_t instance_index) {
  try {
    // Stop Group Replication and reset (persist) GR variables.
    mysqlsh::dba::leave_replicaset(*m_available_instances[instance_index]);

  } catch (const std::exception &err) {
    auto console = mysqlsh::current_console();

    // Skip error if force=true otherwise issue an error.
    if (m_force.is_null() || *m_force == false) {
      console->print_error("Unable to remove instance '" + instance_address +
                           "' from the cluster.");
      throw shcore::Exception::runtime_error(err.what());
    } else {
      m_skipped_instances.push_back(instance_address);
      log_error("Instance '%s' failed to leave the ReplicaSet: %s",
                instance_address.c_str(), err.what());
      console->print_warning(
          "An error occurred when trying to remove instance '" +
          instance_address +
          "' from the cluster. The instance might have been left active "
          "and in an inconsistent state, requiring manual action to "
          "fully dissolve the cluster.");
      console->println();
    }
  }
}

shcore::Value Dissolve::execute() {
  std::shared_ptr<MetadataStorage> metadata = m_cluster->get_metadata_storage();
  auto console = mysqlsh::current_console();

  // JOB: Remove replication accounts used for recovery of GR.
  // Note: This operation MUST be performed before leave-replicaset to ensure
  // that all changed are propagated to the online instances.
  for (auto &instance : m_available_instances) {
    m_cluster->drop_replication_user(instance.get());
  }

  // Drop the cluster's metadata.
  std::string cluster_name = m_cluster->get_name();
  metadata->drop_cluster(cluster_name);

  // We must stop GR on the online instances only, otherwise we'll
  // get connection failures to the (MISSING) instances
  // BUG#26001653.
  for (std::size_t i = 0; i < m_available_instances.size(); i++) {
    // Remove all instances except the primary.
    if (i != m_primary_index) {
      std::string instance_address =
          m_available_instances[i]->get_connection_options().as_uri(
              mysqlshdk::db::uri::formats::only_transport());

      // JOB: Sync transactions with the cluster.
      try {
        // Catch-up with all cluster transaction to ensure cluster metadata is
        // removed on the instance.
        m_cluster->sync_transactions(*m_available_instances[i]);

        // Remove instance from list of instance with sync error in case it was
        // previously added during initial verification, since it succeeded now.
        m_sync_error_instances.erase(
            std::remove(m_sync_error_instances.begin(),
                        m_sync_error_instances.end(), instance_address),
            m_sync_error_instances.end());
      } catch (const std::exception &err) {
        // Skip error if force=true otherwise issue an error.
        if (m_force.is_null() || *m_force == false) {
          console->print_error(
              "The instance '" + instance_address +
              "' was unable to catch up with cluster transactions. There might "
              "be too many transactions to apply or some replication error.");
          throw;
        } else {
          // Only add to list of instance with sync error if not already there.
          if (std::find(m_sync_error_instances.begin(),
                        m_sync_error_instances.end(),
                        instance_address) == m_sync_error_instances.end()) {
            m_sync_error_instances.push_back(instance_address);
          }

          log_error(
              "Instance '%s' was unable to catch up with cluster transactions: "
              "%s",
              instance_address.c_str(), err.what());
          console->print_warning(
              "An error occurred when trying to catch up with cluster "
              "transactions and instance '" +
              instance_address +
              "' might have been left in an inconsistent state that will lead "
              "to errors if it is reused.");
          console->println();
        }
      }

      // JOB: Remove the instance from the replicaset (Stop GR)
      remove_instance(instance_address, i);
    }
  }

  // Remove primary instance at the end (if there is one).
  if (m_primary_index != SIZE_MAX) {
    // No need to catch with transactions, since it is the primary.
    // JOB: Remove the instance from the replicaset (Stop GR)
    std::string instance_address =
        m_available_instances[m_primary_index]->get_connection_options().as_uri(
            mysqlshdk::db::uri::formats::only_transport());
    remove_instance(instance_address, m_primary_index);
  }

  // Disconnect all internal sessions
  m_cluster->disconnect();

  // Print appropriate output message depending if some operation was skipped.
  console->println();
  if (!m_skipped_instances.empty()) {
    // Some instance were skipped and not properly removed from the cluster
    // despite being dissolved.
    std::string warning_msg =
        "The cluster was successfully dissolved, but the following instance";

    if (m_skipped_instances.size() > 1)
      warning_msg.append("s were ");
    else
      warning_msg.append(" was ");

    warning_msg.append("skipped: '" +
                       shcore::str_join(m_skipped_instances, "', '") +
                       "'. Please make sure ");
    if (m_skipped_instances.size() > 1)
      warning_msg.append("these instances are ");
    else
      warning_msg.append("this instance is ");

    warning_msg.append(
        "permanently unavailable or take any necessary manual action to ensure "
        "the cluster is fully dissolved.");
    console->print_warning(warning_msg);
  } else if (!m_sync_error_instances.empty()) {
    // Some instance failed to catch up with cluster transaction and cluster's
    // metadata might not have been removed.
    std::string warning_msg =
        "The cluster was successfully dissolved, but the following instance";

    if (m_sync_error_instances.size() > 1)
      warning_msg.append("s were ");
    else
      warning_msg.append(" was ");
    warning_msg.append(
        "unable to catch up with the cluster transactions: '" +
        shcore::str_join(m_sync_error_instances, "', '") +
        "'. Please make sure the cluster metadata was removed on ");
    if (m_sync_error_instances.size() > 1)
      warning_msg.append("these instances ");
    else
      warning_msg.append("this instance ");

    warning_msg.append("in order to be able to be reused.");
    console->print_warning(warning_msg);
  } else {
    // Full cluster successfully removed.
    console->println("The cluster was successfully dissolved.");
    console->println("Replication was disabled but user data was left intact.");
    console->println();
  }

  return shcore::Value();
}

void Dissolve::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Dissolve::finish() {
  // Close all sessions to available instances.
  for (const auto &instance : m_available_instances) {
    instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_available_instances.clear();
  m_skipped_instances.clear();
  m_sync_error_instances.clear();
  m_primary_address.clear();
  m_primary_index = SIZE_MAX;
}

}  // namespace dba
}  // namespace mysqlsh
