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

#include "modules/adminapi/dba/create_cluster.h"

#include <memory>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

Create_cluster::Create_cluster(
    mysqlshdk::mysql::IInstance *target_instance,
    const std::string &cluster_name,
    const Group_replication_options &gr_options,
    mysqlshdk::utils::nullable<bool> multi_primary, bool adopt_from_gr,
    bool force, const mysqlshdk::utils::nullable<bool> &clear_read_only,
    bool interactive)
    : m_target_instance(target_instance),
      m_cluster_name(cluster_name),
      m_gr_opts(gr_options),
      m_multi_primary(multi_primary),
      m_adopt_from_gr(adopt_from_gr),
      m_force(force),
      m_clear_read_only(clear_read_only),
      m_interactive(interactive) {}

Create_cluster::~Create_cluster() {}

void Create_cluster::validate_create_cluster_options() {
  auto console = mysqlsh::current_console();

  if (!m_gr_opts.auto_rejoin_tries.is_null() &&
      *m_gr_opts.auto_rejoin_tries != 0) {
    std::string warn_msg =
        "The member will only proceed according to its exitStateAction if "
        "auto-rejoin fails (i.e. all retry attempts are exhausted).";
    console->print_warning(warn_msg);
    console->println();
  }

  // Get the instance GR state
  GRInstanceType instance_type =
      get_gr_instance_type(m_target_instance->get_session());

  if (instance_type == mysqlsh::dba::GRInstanceType::GroupReplication &&
      !m_adopt_from_gr) {
    if (m_interactive) {
      if (console->confirm(
              "You are connected to an instance that belongs to an unmanaged "
              "replication group.\nDo you want to setup an InnoDB cluster "
              "based on this replication group?",
              mysqlsh::Prompt_answer::YES) == mysqlsh::Prompt_answer::YES) {
        m_adopt_from_gr = true;
      }
    }
    if (!m_adopt_from_gr)
      throw shcore::Exception::argument_error(
          "Creating a cluster on an unmanaged replication group requires "
          "adoptFromGR option to be true");
  }

  if (m_adopt_from_gr && !m_gr_opts.ssl_mode.is_null()) {
    throw shcore::Exception::argument_error(
        "Cannot use memberSslMode option if adoptFromGR is set to true.");
  }

  if (m_adopt_from_gr && !m_multi_primary.is_null()) {
    throw shcore::Exception::argument_error(
        "Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set "
        "to true. Using adoptFromGR mode will adopt the primary mode in use by "
        "the Cluster.");
  }

  if (m_adopt_from_gr && instance_type != GRInstanceType::GroupReplication) {
    throw shcore::Exception::argument_error(
        "The adoptFromGR option is set to true, but there is no replication "
        "group to adopt");
  }

  // Set multi-primary to false by default.
  if (m_multi_primary.is_null()) m_multi_primary = false;

  if (!m_adopt_from_gr && *m_multi_primary && !m_force) {
    if (m_interactive) {
      console->print_info(fit_screen(
          "The MySQL InnoDB cluster is going to be setup in advanced "
          "Multi-Primary Mode. Before continuing you have to confirm that you "
          "understand the requirements and limitations of Multi-Primary Mode. "
          "For more information see "
          "https://dev.mysql.com/doc/refman/en/"
          "group-replication-limitations.html before proceeding."));
      console->println();
      console->print_info(fit_screen(
          "I have read the MySQL InnoDB cluster manual and I understand the "
          "requirements and limitations of advanced Multi-Primary Mode."));

      if (console->confirm("Confirm", mysqlsh::Prompt_answer::NO) ==
          mysqlsh::Prompt_answer::NO) {
        console->println();
        throw shcore::cancelled("Cancelled");
      } else {
        console->println();
        m_force = true;
      }
    } else {
      throw shcore::Exception::argument_error(
          "Use of multiPrimary mode is not recommended unless you understand "
          "the limitations. Please use the 'force' option if you understand "
          "and accept them.");
    }
  }
}

/**
 * Resolves SSL mode based on the given options.
 *
 * Determine the SSL mode for the cluster based on the provided SSL option and
 * if SSL is enabled on the target instance used to bootstrap the cluster (seed
 * instance).
 */
void Create_cluster::resolve_ssl_mode() {
  // Set SSL Mode to AUTO by default (not defined).
  if (m_gr_opts.ssl_mode.is_null()) {
    m_gr_opts.ssl_mode = dba::kMemberSSLModeAuto;
  }

  std::string new_ssl_mode = resolve_cluster_ssl_mode(
      m_target_instance->get_session(), *m_gr_opts.ssl_mode);

  if (new_ssl_mode != *m_gr_opts.ssl_mode) {
    m_gr_opts.ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the cluster: '%s'",
                m_gr_opts.ssl_mode->c_str());
  }
}

void Create_cluster::prompt_super_read_only() {
  auto console = mysqlsh::current_console();

  // Get the status of super_read_only in order to verify if we need to
  // prompt the user to disable it
  mysqlshdk::utils::nullable<bool> super_read_only =
      m_target_instance->get_sysvar_bool(
          "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  if (*super_read_only) {
    console->println();
    console->print_info(mysqlsh::fit_screen(
        "The MySQL instance at '" + m_target_instance->descr() +
        "' currently has the super_read_only system variable set to protect it "
        "from inadvertent updates from applications. You must first unset it "
        "to be able to perform any changes to this instance.\n"
        "For more information see: https://dev.mysql.com/doc/refman/"
        "en/server-system-variables.html#sysvar_super_read_only."));
    console->println();

    // Get the list of open session to the instance
    std::vector<std::pair<std::string, int>> open_sessions;
    open_sessions =
        mysqlsh::dba::get_open_sessions(m_target_instance->get_session());

    if (!open_sessions.empty()) {
      console->print_note(
          "There are open sessions to '" + m_target_instance->descr() +
          "'.\n"
          "You may want to kill these sessions to prevent them from "
          "performing unexpected updates: \n");

      for (auto &value : open_sessions) {
        std::string account = value.first;
        int num_open_sessions = value.second;

        console->println("" + std::to_string(num_open_sessions) +
                         " open session(s) of '" + account + "'. \n");
      }
    }

    if (console->confirm("Do you want to disable super_read_only and continue?",
                         mysqlsh::Prompt_answer::NO) ==
        mysqlsh::Prompt_answer::NO) {
      console->println();
      throw shcore::cancelled("Cancelled");
    } else {
      m_clear_read_only = true;
      console->println();
    }
  }
}

void Create_cluster::prepare() {
  auto console = mysqlsh::current_console();
  console->println(
      std::string{"A new InnoDB cluster will be created"} +
      (m_adopt_from_gr ? " based on the existing replication group" : "") +
      " on instance '" + m_target_instance->descr() + "'.\n");

  // Validate the cluster_name
  mysqlsh::dba::validate_cluster_name(m_cluster_name);

  // Validate values given for GR options.
  m_gr_opts.check_option_values(m_target_instance->get_version());

  // Validate create cluster options (combinations).
  validate_create_cluster_options();

  if (!m_adopt_from_gr) {
    // Check instance configuration and state, like dba.checkInstance
    // but skip if we're adopting, since in that case the target is obviously
    // already configured
    ensure_instance_configuration_valid(*m_target_instance);

    // Print warning if auto-rejoin is set (not 0).
    if (!m_gr_opts.auto_rejoin_tries.is_null() &&
        *(m_gr_opts.auto_rejoin_tries) != 0) {
      console->print_warning(
          "The member will only proceed according to its exitStateAction if "
          "auto-rejoin fails (i.e. all retry attempts are exhausted).");
      console->println();
    }

    // BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
    // - exitStateAction default value must be READ_ONLY
    // - exitStateAction default value should only be set if supported in
    // the target instance
    if (m_gr_opts.exit_state_action.is_null() &&
        is_group_replication_option_supported(m_target_instance->get_version(),
                                              kExpelTimeout)) {
      m_gr_opts.exit_state_action = "READ_ONLY";
    }

    // Check replication filters before creating the Metadata.
    validate_replication_filters(m_target_instance->get_session());

    // Resolve the SSL Mode to use to configure the instance.
    resolve_ssl_mode();

    // Make sure the target instance does not already belong to a cluster.
    mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
        *m_target_instance, m_target_instance->get_session());

    // Get the address used by GR for the added instance (used in MD).
    int instance_port = m_target_instance->get_connection_options().get_port();
    std::string host_in_metadata =
        mysqlshdk::mysql::get_report_host(*m_target_instance);
    m_address_in_metadata =
        host_in_metadata + ":" + std::to_string(instance_port);

    // Resolve GR local address.
    // NOTE: Must be done only after getting the report_host used by GR and for
    //       the metadata;
    m_gr_opts.local_address = mysqlsh::dba::resolve_gr_local_address(
        m_gr_opts.local_address, host_in_metadata, instance_port);

    // Generate the GR group name (if needed).
    if (m_gr_opts.group_name.is_null()) {
      m_gr_opts.group_name =
          mysqlshdk::gr::generate_group_name(*m_target_instance);
    }
  }

  // Determine the GR mode if the cluster is to be adopt from GR.
  if (m_adopt_from_gr) {
    // Get the primary UUID value to determine GR mode:
    // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
    std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
        m_target_instance->get_session(), nullptr);

    m_multi_primary = gr_primary_uuid.empty();
  }

  // Verify the status of super_read_only and ask the user if wants
  // to disable it
  // NOTE: this is left for last to avoid setting super_read_only to true
  // and right before some execution failure of the command leaving the
  // instance in an incorrect state
  if (m_interactive && m_clear_read_only.is_null()) {
    prompt_super_read_only();
  }

  // Check if super_read_only is turned off and disable it if required
  // NOTE: this is left for last to avoid setting super_read_only to true
  // and right before some execution failure of the command leaving the
  // instance in an incorrect state
  validate_super_read_only(
      m_target_instance->get_session(),
      m_clear_read_only.is_null() ? false : *m_clear_read_only);

  if (!m_adopt_from_gr) {
    // Set the internal configuration object: read/write configs from the
    // server.
    m_cfg = mysqlsh::dba::create_server_config(
        m_target_instance, mysqlshdk::config::k_dft_cfg_server_handler);
  }
}

void Create_cluster::log_used_gr_options() {
  auto console = mysqlsh::current_console();

  log_info("Using Group Replication single primary mode: %s",
           *m_multi_primary ? "FALSE" : "TRUE");

  if (!m_gr_opts.ssl_mode.is_null()) {
    log_info("Using Group Replication SSL mode: %s",
             m_gr_opts.ssl_mode->c_str());
  }

  if (!m_gr_opts.group_name.is_null()) {
    log_info("Using Group Replication group name: %s",
             m_gr_opts.group_name->c_str());
  }

  if (!m_gr_opts.local_address.is_null()) {
    log_info("Using Group Replication local address: %s",
             m_gr_opts.local_address->c_str());
  }

  if (!m_gr_opts.group_seeds.is_null()) {
    log_info("Using Group Replication group seeds: %s",
             m_gr_opts.group_seeds->c_str());
  }

  if (!m_gr_opts.exit_state_action.is_null()) {
    log_info("Using Group Replication exit state action: %s",
             m_gr_opts.exit_state_action->c_str());
  }

  if (!m_gr_opts.member_weight.is_null()) {
    log_info("Using Group Replication member weight: %s",
             std::to_string(*m_gr_opts.member_weight).c_str());
  }

  if (!m_gr_opts.consistency.is_null()) {
    log_info("Using Group Replication failover consistency: %s",
             m_gr_opts.consistency->c_str());
  }

  if (!m_gr_opts.expel_timeout.is_null()) {
    log_info("Using Group Replication expel timeout: %s",
             std::to_string(*m_gr_opts.expel_timeout).c_str());
  }

  if (!m_gr_opts.auto_rejoin_tries.is_null()) {
    log_info("Using Group Replication auto-rejoin tries: %s",
             std::to_string(*m_gr_opts.auto_rejoin_tries).c_str());
  }
}

void Create_cluster::prepare_metadata_schema() {
  // Ensure that the metadata schema is ready for creating a new cluster in it
  // If the metadata schema does not exist, we create it
  // If the metadata schema already exists:
  // - clear it of old data
  // - ensure it has the right version
  // We ensure both by always dropping the old schema and re-creating it from
  // scratch.

  mysqlsh::dba::metadata::uninstall(m_target_instance->get_session());

  mysqlsh::dba::metadata::install(m_target_instance->get_session());
}

shcore::Value Create_cluster::execute() {
  auto console = mysqlsh::current_console();
  console->print_info("Creating InnoDB cluster '" + m_cluster_name + "' on '" +
                      m_target_instance->descr() + "'...\n");

  // Common informative logging
  if (m_adopt_from_gr) {
    log_info(
        "Adopting cluster from existing Group Replication and using its "
        "settings.");
  } else {
    log_used_gr_options();
  }

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  mysqlshdk::gr::install_plugin(*m_target_instance, nullptr, true);

  if (*m_multi_primary && !m_adopt_from_gr) {
    log_info(
        "The MySQL InnoDB cluster is going to be setup in advanced "
        "Multi-Primary Mode. Consult its requirements and limitations in "
        "https://dev.mysql.com/doc/refman/en/"
        "group-replication-limitations.html");
  }

  shcore::Value ret_val;
  std::string replication_user;
  std::string replication_pwd;
  try {
    console->print_info("Adding Seed Instance...");

    // BUG#28064729: Do not create Metadata and recovery users before
    // bootstrapping the GR group.
    if (!m_adopt_from_gr) {
      // Start replicaset before creating the recovery user and metadata.
      mysqlsh::dba::start_replicaset(*m_target_instance, m_gr_opts,
                                     m_multi_primary, m_cfg.get());

      // Creates the replication account to for the primary instance.
      // BUG#28054500: Do not create new users, not needed, when creating a
      // cluster with adoptFromGR.
      mysqlshdk::gr::create_replication_random_user_pass(
          *m_target_instance, &replication_user,
          convert_ipwhitelist_to_netmask(m_gr_opts.ip_whitelist.get_safe()),
          &replication_pwd);
      log_debug("Created replication user '%s'", replication_user.c_str());

      // Set the GR replication (recovery) user.
      log_debug("Setting Group Replication recovery user to '%s'.",
                replication_user.c_str());
      mysqlshdk::gr::change_recovery_credentials(
          *m_target_instance, replication_user, replication_pwd);
    }

    // Create the Metadata Schema.
    prepare_metadata_schema();

    // Create the cluster and insert it into the Metadata.
    std::shared_ptr<MetadataStorage> metadata =
        std::make_shared<MetadataStorage>(m_target_instance->get_session());
    MetadataStorage::Transaction tx(metadata);

    std::shared_ptr<Cluster> cluster = std::make_shared<Cluster>(
        m_cluster_name, m_target_instance->get_session(), metadata);

    // Update the properties
    // For V1.0, let's see the Cluster's description to "default"
    cluster->impl()->set_description("Default Cluster");

    cluster->impl()->set_attribute(ATT_DEFAULT, shcore::Value::True());

    // Insert Cluster on the Metadata Schema.
    metadata->insert_cluster(cluster->impl());

    // Create the default replicaset in the Metadata and set it to the cluster.
    cluster->impl()->insert_default_replicaset(*m_multi_primary,
                                               m_adopt_from_gr);

    tx.commit();

    // Set default_replicaset group name
    std::string group_replication_group_name =
        get_gr_replicaset_group_name(m_target_instance->get_session());
    cluster->impl()->get_default_replicaset()->set_group_name(
        group_replication_group_name);

    // Insert instance into the metadata.
    if (m_adopt_from_gr) {
      // Adoption from an existing GR group is performed after creating/updating
      // the metadata (since it is used internally by adopt_from_gr()).
      cluster->impl()->get_default_replicaset()->adopt_from_gr();
    } else {
      // Check if instance address already belong to replicaset (metadata).
      bool is_instance_on_md =
          cluster->impl()->get_metadata_storage()->is_instance_on_replicaset(
              cluster->impl()->get_default_replicaset()->get_id(),
              m_address_in_metadata);

      log_debug(
          "ReplicaSet %s: Instance '%s' %s",
          std::to_string(cluster->impl()->get_default_replicaset()->get_id())
              .c_str(),
          m_target_instance->descr().c_str(),
          is_instance_on_md ? "is already in the Metadata."
                            : "is being added to the Metadata...");

      // If the instance is not on the Metadata, we must add it.
      if (!is_instance_on_md)
        cluster->impl()->get_default_replicaset()->add_instance_metadata(
            m_target_instance->get_connection_options());
    }

    // If it reaches here, it means there are no exceptions
    ret_val =
        shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(cluster));

  } catch (const std::exception &) {
    // Remove the replication user if previously created.
    if (!replication_user.empty()) {
      log_debug("Removing replication user '%s'", replication_user.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(
            m_target_instance->get_session(), replication_user);
      } catch (const std::exception &err) {
        console->print_warning("Failed to remove replication user '" +
                               replication_user + "': " + err.what());
      }
    }
    // Re-throw the original exception
    throw;
  }

  std::string message =
      m_adopt_from_gr ? "Cluster successfully created based on existing "
                        "replication group."
                      : "Cluster successfully created. Use "
                        "Cluster.<<<addInstance>>>() to add MySQL instances.\n"
                        "At least 3 instances are needed for the cluster to be "
                        "able to withstand up to\n"
                        "one server failure.";
  console->print_info(message);
  console->println();

  return ret_val;
}

void Create_cluster::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Create_cluster::finish() {
  // Do nothing.
}

}  // namespace dba
}  // namespace mysqlsh
