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

#include "modules/adminapi/replicaset/remove_instance.h"

#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Remove_instance::Remove_instance(
    mysqlshdk::db::Connection_options instance_cnx_opts, const bool interactive,
    mysqlshdk::utils::nullable<bool> force, const ReplicaSet &replicaset,
    const shcore::NamingStyle &naming_style)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_interactive(interactive),
      m_force(force),
      m_replicaset(replicaset),
      m_naming_style(naming_style) {
  assert(&instance_cnx_opts);
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_metadata = m_instance_address;
}

Remove_instance::~Remove_instance() { delete m_target_instance; }

void Remove_instance::ensure_instance_belong_to_replicaset(
    const std::string &address) {
  // Check if the instance exists on the ReplicaSet
  log_debug("Checking if the instance %s belongs to the replicaset",
            address.c_str());
  bool is_instance_on_md =
      m_replicaset.get_cluster()
          ->get_metadata_storage()
          ->is_instance_on_replicaset(m_replicaset.get_id(), address);

  if (!is_instance_on_md) {
    mysqlsh::current_console()->print_error(
        "The instance '" + address +
        "' cannot be removed because it does not belong "
        "to the cluster (not found in the metadata). "
        "If you really want to remove this instance because "
        "it is still using Group Replication then it must "
        "be stopped manually.");

    std::string err_msg = "The instance '" + address +
                          "' does not belong to the ReplicaSet: '" +
                          m_replicaset.get_member("name").get_string() + "'.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Remove_instance::ensure_not_last_instance_in_replicaset() {
  // Check if it is the last instance in the ReplicaSet and issue an error.
  // NOTE: When multiple replicasets are supported this check needs to be moved
  //       to a higher level (to check if the instance is the last one of the
  //       last replicaset, which should be the default replicaset).
  log_debug("Checking if the instance is the last in the replicaset");
  if (m_replicaset.get_cluster()->get_metadata_storage()->get_replicaset_count(
          m_replicaset.get_id()) == 1) {
    mysqlsh::current_console()->print_error(
        "The instance '" + m_instance_address +
        "' cannot be removed because it is the only member of the Cluster. "
        "Please use <Cluster>." +
        shcore::get_member_name("dissolve", m_naming_style) +
        "() instead to remove the last instance and dissolve the Cluster.");

    std::string err_msg = "The instance '" + m_instance_address +
                          "' is the last member of the ReplicaSet";
    throw shcore::Exception::logic_error(err_msg);
  }
}

Instance_definition Remove_instance::remove_instance_metadata() {
  // Save the instance row details (definition). This is required to be able
  // to revert the removal of the instance from the metadata if needed.
  log_debug("Saving instance definition");
  Instance_definition instance_def =
      m_replicaset.get_cluster()->get_metadata_storage()->get_instance(
          m_address_in_metadata);

  log_debug("Removing instance from metadata");
  m_replicaset.get_cluster()->get_metadata_storage()->remove_instance(
      m_address_in_metadata);

  return instance_def;
}

void Remove_instance::undo_remove_instance_metadata(
    const Instance_definition &instance_def) {
  log_debug("Reverting instance removal from metadata");
  m_replicaset.get_cluster()->get_metadata_storage()->insert_instance(
      instance_def);
}

void Remove_instance::find_failure_cause(const std::exception &err) {
  // Check the state of the instance (from the cluster perspective) to assess
  // the possible cause of the failure.
  ManagedInstance::State state = get_instance_state(
      m_replicaset.get_cluster()->get_metadata_storage()->get_session(),
      m_instance_address);

  auto console = mysqlsh::current_console();

  // Print and throw a different error depending if the instance state is known
  // and expected to be unreachable.
  if (state == ManagedInstance::Unreachable ||
      state == ManagedInstance::Missing) {
    // Only print an error if the force option was not used (or is false).
    if (m_force.is_null() || *m_force == false) {
      std::string message =
          "The instance '" + m_instance_address +
          "' cannot be removed because it is on a '" +
          ManagedInstance::describe(state) +
          "' state. Please bring the instance back ONLINE and try to remove "
          "it again. If the instance is permanently not reachable, ";
      if (m_interactive && m_force.is_null()) {
        message +=
            "then you can choose to proceed with the operation and only remove "
            "the instance from the Cluster Metadata.";
      } else {
        message +=
            "then please use <Cluster>." +
            shcore::get_member_name("removeInstance", m_naming_style) +
            "() with the force option set to true to proceed with the "
            "operation and only remove the instance from the Cluster Metadata.";
      }

      console->print_error(message);
    }
    std::string err_msg = "The instance '" + m_instance_address + "' is '" +
                          ManagedInstance::describe(state) + "'";
    throw shcore::Exception::runtime_error(err_msg);
  } else {
    console->print_error(
        "Unable to connect to instance '" + m_instance_address +
        "'. Please, verify connection credentials and make sure the "
        "instance is available.");
    throw shcore::Exception::runtime_error(err.what());
  }
}

void Remove_instance::prompt_to_force_remove() {
  auto console = mysqlsh::current_console();

  console->println();
  if (console->confirm("Do you want to continue anyway (only the instance "
                       "metadata will be removed)?",
                       Prompt_answer::NO) == Prompt_answer::YES) {
    m_force = true;
    console->println();
  }
}

bool Remove_instance::is_protocol_upgrade_required() {
  bool ret = false;

  // Get the instance server_uuid
  mysqlshdk::utils::nullable<std::string> server_uuid;
  mysqlshdk::mysql::Instance group_session_instance(
      m_replicaset.get_cluster()->get_group_session());

  // Determine which session shall be used to determine if an upgrade is
  // required and afterwards to perform the upgrade.
  // If the target_instance is leaving the group and is the same as the group
  // session, we must use another instance
  if (m_target_instance) {
    server_uuid = m_target_instance->get_sysvar_string(
        "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }

  std::string group_session_uuid = *group_session_instance.get_sysvar_string(
      "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  if (!server_uuid.is_null() && (*server_uuid == group_session_uuid)) {
    auto metadata(mysqlshdk::innodbcluster::Metadata_mysql::create(
        group_session_instance.get_session()));

    // Find any online instance that is not the target instance
    std::vector<ReplicaSet::Instance_info> replicaset_instances =
        m_replicaset.get_instances();

    const ReplicaSet::Instance_info *instance = nullptr;

    for (const auto &i : replicaset_instances) {
      if (i.uuid != *server_uuid) {
        instance = &i;
        break;
      }
    }

    try {
      mysqlshdk::db::Connection_options coptions(instance->classic_endpoint);

      coptions.set_login_options_from(
          group_session_instance.get_session()->get_connection_options());

      log_info("Opening session to the member of InnoDB cluster at %s...",
               coptions.as_uri().c_str());

      std::shared_ptr<mysqlshdk::db::mysql::Session> session;
      session = mysqlshdk::db::mysql::Session::create();
      session->connect(coptions);

      // Set the instance used for protocol upgrade to point to the newly
      // established session
      m_target_instance_protocol_upgrade =
          shcore::make_unique<mysqlshdk::mysql::Instance>(session);
    } catch (std::exception &e) {
      throw shcore::Exception::runtime_error(
          std::string("Unable to establish a session to the cluster member: ") +
          e.what());
    }
  } else {
    m_target_instance_protocol_upgrade =
        shcore::make_unique<mysqlshdk::mysql::Instance>(
            group_session_instance.get_session());
  }

  try {
    ret = mysqlshdk::gr::is_protocol_upgrade_required(
        *m_target_instance_protocol_upgrade, server_uuid,
        &m_gr_protocol_version_to_upgrade);
  } catch (const shcore::Exception &error) {
    // The UDF may fail with MySQL Error 1123 if any of the members is
    // RECOVERING In such scenario, we must abort the upgrade protocol version
    // process and warn the user
    if (error.code() == ER_CANT_INITIALIZE_UDF) {
      auto console = mysqlsh::current_console();
      console->print_note(
          "Unable to determine the Group Replication protocol version, while "
          "verifying if a protocol upgrade would be possible: " +
          std::string(error.what()) + ".");
    } else {
      throw;
    }
  }

  return ret;
}

void Remove_instance::prepare() {
  // Validate connection options.
  log_debug("Verifying connection options");
  validate_connection_options(m_instance_cnx_opts);

  // Use default port if not provided in the connection options.
  if (!m_instance_cnx_opts.has_port()) {
    m_instance_cnx_opts.set_port(mysqlshdk::db::k_default_mysql_port);
    m_instance_address = m_instance_cnx_opts.as_uri(
        mysqlshdk::db::uri::formats::only_transport());
  }

  // Get instance login information from the cluster session if missing.
  if (!m_instance_cnx_opts.has_user() || !m_instance_cnx_opts.has_password()) {
    std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
        m_replicaset.get_cluster()->get_group_session();
    Connection_options cluster_cnx_opt =
        cluster_session->get_connection_options();

    if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user())
      m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());

    if (!m_instance_cnx_opts.has_password() && cluster_cnx_opt.has_password())
      m_instance_cnx_opts.set_password(cluster_cnx_opt.get_password());
  }

  // Try to establish a connection to the target instance, although it might
  // fail if the instance is OFFLINE.
  // NOTE: Connection required to perform some last validations if the target
  //       instance is available.
  log_debug("Connecting to instance '%s'", m_instance_address.c_str());
  std::shared_ptr<mysqlshdk::db::ISession> session;
  try {
    session = mysqlshdk::db::mysql::Session::create();
    session->connect(m_instance_cnx_opts);
    m_target_instance = new mysqlshdk::mysql::Instance(session);
    log_debug("Successfully connected to instance");
  } catch (std::exception &err) {
    log_warning("Failed to connect to %s: %s",
                m_instance_cnx_opts.uri_endpoint().c_str(), err.what());
    // Make sure the target instance is not set if an connection error occurs.
    m_target_instance = nullptr;
    // Find error cause to print more information about before prompt in
    // interactive mode.
    try {
      find_failure_cause(err);
    } catch (std::exception &err) {
      log_warning("%s", err.what());

      // Before anything, check if the instance actually belongs to the
      // cluster, since it wouldn't make sense to ask about removing a
      // bogus instance from the metadata.
      ensure_instance_belong_to_replicaset(m_address_in_metadata);

      // Ask the user if in interactive is used and 'force' option was not used.
      if (m_interactive && m_force.is_null()) {
        prompt_to_force_remove();
      }
      if (m_force.is_null() || *m_force == false) {
        // If force was not used throw the exception from find_failure_cause().
        throw;
      } else {
        auto console = mysqlsh::current_console();
        console->print_note(
            "The instance '" + m_instance_address +
            "' is not reachable and it will only be removed from the metadata. "
            "Please take any necessary actions to make sure that the instance "
            "will not rejoin the cluster if brought back online.");
        console->println();
      }
    }
  }

  // Set the metadata address to use if instance is reachable, otherwise use
  // the provided instance connection address.
  if (m_target_instance) {
    m_address_in_metadata = m_target_instance->get_canonical_address();
  }

  // Ensure instance belong to replicaset.
  ensure_instance_belong_to_replicaset(m_address_in_metadata);

  // Ensure instance is not the last in the replicaset.
  // Should be called after we know there's any chance the instance actually
  // belongs to the replicaset.
  ensure_not_last_instance_in_replicaset();

  // Validate user privileges to use the command (only if the instance is
  // available).
  if (m_target_instance) {
    ensure_user_privileges(*m_target_instance);
  }

  // Handling of GR protocol version:
  // Verify if an upgrade of the protocol will be required after removing the
  // target instance
  m_upgrade_gr_protocol_version = is_protocol_upgrade_required();
}

shcore::Value Remove_instance::execute() {
  std::string msg =
      "The instance will be removed from the InnoDB cluster. Depending on the "
      "instance being the Seed or not, the Metadata session might become "
      "invalid. If so, please start a new session to the Metadata Storage R/W "
      "instance.";
  std::string message =
      mysqlshdk::textui::format_markup_text({msg}, 80, 0, false);
  auto console = mysqlsh::current_console();
  console->print_info(message);
  console->println();

  // JOB: Remove instance from the MD (metadata).
  // NOTE: This operation MUST be performed before leave-replicaset to ensure
  // that the MD change is also propagated to the target instance to
  // remove (if ONLINE). This avoid issues removing and adding an instance
  // again (error adding the instance because it is already in the MD).
  Instance_definition instance_def = remove_instance_metadata();

  // Only try to sync transactions with cluster and execute leave-replicaset
  // if connection to the instance was previously established (however instance
  // might still fail during the execution of those steps).
  if (m_target_instance) {
    // JOB: Sync transactions with the cluster.
    try {
      m_replicaset.get_cluster()->sync_transactions(*m_target_instance);
    } catch (std::exception &err) {
      // Skip error if force=true, otherwise revert instance remove from MD and
      // issue error.
      if (m_force.is_null() || *m_force == false) {
        // REVERT JOB: Remove instance from the MD (metadata).
        undo_remove_instance_metadata(instance_def);
        bool is_rejoining =
            mysqlshdk::gr::is_running_gr_auto_rejoin(*m_target_instance);
        if (is_rejoining)
          console->print_error(
              "The instance '" + m_instance_address +
              "' was unable to catch up with cluster "
              "transactions because auto-rejoin is in progress. It's possible "
              "to use 'force' option to force the removal of it, but that can "
              "leave the instance in an inconsistent state and lead to errors "
              "if you want to reuse it later. It's recommended to wait for the "
              "auto-rejoin process to terminate and retry the remove "
              "operation.");
        else
          console->print_error(
              "The instance '" + m_instance_address +
              "' was unable to catch up with cluster transactions. There might "
              "be too many transactions to apply or some replication error. In "
              "the former case, you can retry the operation (using a higher "
              "timeout value by setting the global shell option "
              "'dba.gtidWaitTimeout'). In the later case, analyze and fix any "
              "replication error. You can also choose to skip this error using "
              "the 'force: true' option, but it might leave the instance in an "
              "inconsistent state and lead to errors if you want to reuse it.");
        throw;
      } else {
        log_error(
            "Instance '%s' was unable to catch up with cluster transactions: "
            "%s",
            m_instance_address.c_str(), err.what());
        console->print_warning(
            "An error occured when trying to catch up with cluster "
            "transactions and the instance might have been left in an "
            "inconsistent state that will lead to errors if it is reused.");
        console->println();
      }
    }

    // JOB: Remove the instance from the replicaset (Stop GR)
    // NOTE: We always try (best effort) to execute leave_replicaset(), but
    // ignore any error if force is used (even if the instance is not
    // reachable, since it is already expected to fail).
    try {
      // Get the group_replication_local address value of the instance we will
      // remove from the replicaset (to update remaining members after).
      std::string local_gr_address = *m_target_instance->get_sysvar_string(
          "group_replication_local_address",
          mysqlshdk::mysql::Var_qualifier::GLOBAL);

      // Stop Group Replication and reset (persist) GR variables.
      mysqlsh::dba::leave_replicaset(*m_target_instance);

      // Update the replicaset members (group_seed variable) and remove the
      // replication users on the instance and other members.
      m_replicaset.get_cluster()
          ->get_default_replicaset()
          ->update_group_members_for_removed_member(local_gr_address,
                                                    *m_target_instance, true);

      log_info("Instance '%s' has left the group.", m_instance_address.c_str());
    } catch (std::exception &err) {
      log_error("Instance '%s' failed to leave the ReplicaSet: %s",
                m_instance_address.c_str(), err.what());
      // Only add the metadata back if the force option was not used.
      if (m_force.is_null() || *m_force == false) {
        // REVERT JOB: Remove instance from the MD (metadata).
        // If the removal of the instance from the replicaset failed
        // We must add it back to the MD if force is not used
        undo_remove_instance_metadata(instance_def);
        // If leave replicaset failed and force was not used then check the
        // possible cause of the failure and report it if found.
        find_failure_cause(err);
      }
      // If force is used do not add the instance back to the metadata,
      // and ignore any leave-replicaset error.
    }

    // Upgrade the prococol version if necessary
    if (m_upgrade_gr_protocol_version) {
      mysqlshdk::gr::set_group_protocol_version(
          *m_target_instance_protocol_upgrade,
          m_gr_protocol_version_to_upgrade);
    }
  }

  console->println();
  console->print_info("The instance '" + m_instance_address +
                      "' was successfully removed from the cluster.");
  console->println();

  return shcore::Value();
}

void Remove_instance::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Remove_instance::finish() {
  // Close the instance session at the end if available.
  if (m_target_instance) {
    m_target_instance->close_session();
  }
}

}  // namespace dba
}  // namespace mysqlsh
