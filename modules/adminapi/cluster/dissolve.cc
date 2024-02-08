/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/dissolve.h"

#include <utility>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/error.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Dissolve::Dissolve(const bool interactive, std::optional<bool> force,
                   Cluster_impl *cluster)
    : m_interactive(interactive), m_force(force), m_cluster(cluster) {
  assert(cluster);
}

Dissolve::~Dissolve() = default;

void Dissolve::prompt_to_confirm_dissolve() const {
  // Show cluster description if not empty, to help user verify if he is
  // dissolving the right cluster.
  auto console = mysqlsh::current_console();

  // Show cluster description.
  shcore::Value res = m_cluster->describe();
  console->print_info(
      "The cluster still has the following registered instances:");

  // Pretty print description only if wrap_json is not json/raw.
  bool use_pretty_print =
      (current_shell_options()->get().wrap_json.compare("json/raw") != 0);
  console->print_info(res.descr(use_pretty_print));

  console->print_warning(
      "You are about to dissolve the whole cluster and lose the high "
      "availability features provided by it. This operation cannot be "
      "reverted. All members will be removed from the cluster and "
      "replication will be stopped, internal recovery user accounts and "
      "the cluster metadata will be dropped. User data will be maintained "
      "intact in all instances.");
  console->print_info();

  if (console->confirm("Are you sure you want to dissolve the cluster?",
                       Prompt_answer::NO) == Prompt_answer::NO) {
    throw shcore::Exception::runtime_error("Operation canceled by user.");
  }
}

bool Dissolve::prompt_to_force_dissolve() const {
  auto console = mysqlsh::current_console();
  console->print_info();
  bool result = console->confirm(
                    "Do you want to continue anyway (only the instance "
                    "metadata will be removed)?",
                    Prompt_answer::NO) == Prompt_answer::YES;
  console->print_info();
  return result;
}

std::shared_ptr<Instance> Dissolve::ensure_instance_reachable(
    const std::string &instance_address, const Instance_type instance_type) {
  std::shared_ptr<Instance> ret_instance;
  try {
    ret_instance = m_cluster->get_session_to_cluster_instance(instance_address);

    m_available_instances.emplace_back(
        std::make_pair(ret_instance, instance_type));
  } catch (const std::exception &err) {
    // instance not reachable
    auto console = mysqlsh::current_console();
    // Handle use of 'force' option.
    if (!m_force.has_value() || !m_force.value()) {
      console->print_error(
          "Unable to connect to instance '" + instance_address +
          "'. Please verify connection credentials and make sure the "
          "instance is available.");

      // In interactive mode and 'force' option not used, ask user to
      // continue with the operation.
      bool continue_dissolve = false;
      if (m_interactive && !m_force.has_value()) {
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
      console->print_info();
    }
  }

  return ret_instance;
}

void Dissolve::ensure_transactions_sync() {
  // Sync transactions with the cluster on all instance to ensure there are no
  // replication errors (until now).
  for (const auto &instance : m_available_instances) {
    try {
      auto console = mysqlsh::current_console();
      console->print_info("* Waiting for instance '" + instance.first->descr() +
                          "' to apply received transactions...");

      m_cluster->sync_transactions(
          *instance.first, instance.second,
          current_shell_options()->get().dba_gtid_wait_timeout, true);
    } catch (const std::exception &err) {
      std::string instance_address =
          (*instance.first)
              .get_connection_options()
              .as_uri(mysqlshdk::db::uri::formats::only_transport());

      // Handle use of 'force' option.
      if (!m_force.has_value() || !m_force.value()) {
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
        if (m_interactive && !m_force.has_value()) {
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

  if (!m_force.has_value() || !m_force.value()) {
    // Issue an error if 'force' option is not used or false.
    std::string message =
        "The instance '" + instance_address +
        "' cannot be removed because it is on a '" + instance_state +
        "' state. Please bring the instance back ONLINE and try to "
        "dissolve the cluster again. If the instance is permanently not "
        "reachable, ";
    if (m_interactive && !m_force.has_value()) {
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
    if (m_interactive && !m_force.has_value()) {
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
    console->print_info();
  }
}

void Dissolve::prepare() {
  // Can't dissolve if in a clusterset
  if (m_cluster->is_cluster_set_member() && !m_cluster->is_invalidated()) {
    std::vector<Cluster_set_member_metadata> cs_members;

    if (m_cluster->get_metadata_storage()->get_cluster_set(
            m_cluster->get_clusterset_metadata().cluster_set_id, false, nullptr,
            &cs_members) &&
        cs_members.size() > 1) {
      throw shcore::Exception::runtime_error(
          "Cluster '" + m_cluster->get_name() +
          "' cannot be dissolved because it is part of a ClusterSet with other "
          "Clusters.");
    }
  }

  // Confirm execution of operation in interactive mode.
  if (m_interactive) {
    prompt_to_confirm_dissolve();
    mysqlsh::current_console()->print_info();
  }

  // Get cluster session to use the same authentication credentials for all
  // cluster instances.
  std::shared_ptr<Instance> cluster_instance = m_cluster->get_cluster_server();

  // Determine the primary (if it exists), in order to be the last to be
  // removed from the cluster. Only for single primary mode.
  // NOTE: This is need to avoid a new primary election in the group and GR
  //       BUG#24818604.

  bool single_primary = false;
  std::vector<mysqlshdk::gr::Member> members = mysqlshdk::gr::get_members(
      *cluster_instance, &single_primary, nullptr, nullptr);

  auto get_member_state = [&members](const std::string &uuid) {
    auto it = std::find_if(
        members.begin(), members.end(),
        [&](const mysqlshdk::gr::Member &m) { return m.uuid == uuid; });
    if (it != members.end())
      return std::make_pair(it->state,
                            it->role == mysqlshdk::gr::Member_role::PRIMARY);
    return std::make_pair(mysqlshdk::gr::Member_state::MISSING, false);
  };

  // Get all cluster instances, including state information.
  std::vector<Instance_metadata> instance_defs = m_cluster->get_all_instances();
  // Check if instances can be dissolved. More specifically, verify if their
  // state is valid, if they are reachable (i.e., able to connect to them).
  // Also determine if instance removal can be skipped, according to 'force'
  // option and user prompts.
  for (const auto &instance_def : instance_defs) {
    if (instance_def.instance_type != Instance_type::READ_REPLICA) {
      mysqlshdk::gr::Member_state state;
      bool is_primary;
      std::tie(state, is_primary) = get_member_state(instance_def.uuid);

      if (state == mysqlshdk::gr::Member_state::ONLINE ||
          state == mysqlshdk::gr::Member_state::RECOVERING) {
        // Verify if active instances are reachable.
        ensure_instance_reachable(instance_def.endpoint,
                                  instance_def.instance_type);
      } else {
        // Handle not active instances, determining if they can be skipped.
        handle_unavailable_instances(instance_def.endpoint,
                                     mysqlshdk::gr::to_string(state));
      }
    } else {
      // Check Read-Replicas to check if reachable
      try {
        auto rr_instance = ensure_instance_reachable(
            instance_def.endpoint, instance_def.instance_type);

        // Get the status
        if (rr_instance) {
          auto status = mysqlshdk::mysql::get_read_replica_status(*rr_instance);

          if (status != mysqlshdk::mysql::Read_replica_status::ONLINE) {
            // Handle unreachable read-replica to determine if can be skipped
            handle_unavailable_instances(instance_def.endpoint,
                                         to_string(status));
          }
        }
      } catch (const mysqlshdk::db::Error &) {
        log_info("Unable to connect to '%s'", instance_def.endpoint.c_str());

        // Handle unreachable read-replica to determine if can be skipped
        handle_unavailable_instances(
            instance_def.endpoint,
            to_string(mysqlshdk::mysql::Read_replica_status::UNREACHABLE));
      } catch (const shcore::Error &) {
        throw;
      }
    }
  }

  // Verify if there is any replication error that might make the operation
  // fails, i.e., all (available/reachable) instances are able to catchup with
  // cluster transactions.
  ensure_transactions_sync();

  // If user decided to skip all instance with errors then assume force=true.
  if (!m_force.has_value() &&
      (!m_skipped_instances.empty() || !m_sync_error_instances.empty())) {
    m_force = true;
  }

  // Check if Cluster supports GR member actions to reset them if so
  m_supports_member_actions = m_cluster->get_lowest_instance_version() >=
                              Precondition_checker::k_min_cs_version;
}

shcore::Value Dissolve::execute() {
  std::shared_ptr<MetadataStorage> metadata = m_cluster->get_metadata_storage();
  auto console = mysqlsh::current_console();

  console->print_info("* Dissolving the Cluster...");

  // Disable super_read_only mode if it is enabled.

  if (auto sro = m_cluster->get_cluster_server()->get_sysvar_bool(
          "super_read_only", false);
      sro) {
    log_info(
        "Disabling super_read_only mode on instance '%s' to run dissolve().",
        m_cluster->get_cluster_server()->descr().c_str());
    m_cluster->get_cluster_server()->set_sysvar("super_read_only", false);
  }

  // Remove replication accounts used for recovery of GR and ClusterSet
  // replication accounts
  // Note: This operation MUST be performed before leave-cluster to ensure
  // that all changed are propagated to the online instances.
  m_cluster->wipe_all_replication_users();

  // Drop the metadata schema
  metadata::uninstall(m_cluster->get_cluster_server());

  // We must stop GR on the online instances only, otherwise we'll
  // get connection failures to the (MISSING) instances
  // BUG#26001653.
  for (const auto &instance : m_available_instances) {
    auto instance_type = instance.second;
    auto instance_ptr = instance.first;

    std::string instance_address = instance_ptr->descr();

    // JOB: Sync transactions with the cluster.
    try {
      // Catch-up with all cluster transaction to ensure cluster metadata is
      // removed on the instance.
      console->print_info("* Waiting for instance '" + instance_address +
                          "' to apply received transactions...");

      // We only need to apply received transactions, since any transactions
      // certified by the group will have been received
      m_cluster->sync_transactions(
          *instance_ptr, instance_type,
          current_shell_options()->get().dba_gtid_wait_timeout, true);

      // Remove instance from list of instance with sync error in case it
      // was previously added during initial verification, since it
      // succeeded now.
      m_sync_error_instances.erase(
          std::remove_if(
              m_sync_error_instances.begin(), m_sync_error_instances.end(),
              mysqlshdk::utils::Endpoint_predicate{instance_address}),
          m_sync_error_instances.end());
    } catch (const std::exception &err) {
      // Skip error if force=true otherwise issue an error.
      if (!m_force.has_value() || !m_force.value()) {
        console->print_error(
            "The instance '" + instance_address +
            "' was unable to catch up with cluster transactions. There might "
            "be too many transactions to apply or some replication error.");
        throw;
      }

      // Only add to list of instance with sync error if not already there.
      if (std::find(m_sync_error_instances.begin(),
                    m_sync_error_instances.end(),
                    instance_address) == m_sync_error_instances.end()) {
        m_sync_error_instances.push_back(instance_address);
      }

      log_error(
          "Instance '%s' was unable to catch up with cluster transactions: %s",
          instance_address.c_str(), err.what());
      console->print_warning(
          "An error occurred when trying to catch up with cluster transactions "
          "and instance '" +
          instance_address +
          "' might have been left in an inconsistent state that will lead to "
          "errors if it is reused.");
      console->print_info();
    }

    // JOB: Remove the instance from the cluster
    switch (instance_type) {
      case Instance_type::NONE:
      case Instance_type::GROUP_MEMBER: {
        try {
          // Stop Group Replication and reset (persist) GR variables.
          mysqlsh::dba::leave_cluster(*instance_ptr, m_supports_member_actions);
        } catch (const std::exception &err) {
          // Skip error if force=true otherwise issue an error.
          if (!m_force.has_value() || !m_force.value()) {
            console->print_error("Unable to remove instance '" +
                                 instance_address + "' from the cluster.");
            throw shcore::Exception::runtime_error(err.what());
          } else {
            m_skipped_instances.push_back(instance_address);
            log_error("Instance '%s' failed to leave the cluster: %s",
                      instance_address.c_str(), err.what());
            console->print_warning(
                "An error occurred when trying to remove instance '" +
                instance_address +
                "' from the cluster. The instance might have been left active "
                "and in an inconsistent state, requiring manual action to "
                "fully dissolve the cluster.");
            console->print_info();
          }
        }

        break;
      }
      case Instance_type::READ_REPLICA: {
        console->print_info("* Removing Read-Replica '" + instance_address +
                            "' from the cluster...");

        // Remove the async channel
        remove_channel(*instance_ptr, k_read_replica_async_channel_name, false);

        try {
          log_info("Disabling automatic failover management at %s",
                   instance_ptr->descr().c_str());

          reset_managed_connection_failover(*instance_ptr, false);
        } catch (...) {
          log_error("Error disabling automatic failover at %s: %s",
                    instance_address.c_str(),
                    format_active_exception().c_str());
          console->print_warning(
              "An error occurred when trying to disable automatic failover on "
              "Read-Replica at '" +
              instance_address + "'");
          console->print_info();
        }

        break;
      }
      case Instance_type::ASYNC_MEMBER:
        break;
    }
  }

  // Disconnect all internal sessions
  m_cluster->disconnect();

  // Print appropriate output message depending if some operation was skipped.
  console->print_info();
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
    console->print_info("The cluster was successfully dissolved.");
    console->print_info(
        "Replication was disabled but user data was left intact.");
    console->print_info();
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
    instance.first->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_available_instances.clear();
  m_skipped_instances.clear();
  m_sync_error_instances.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
