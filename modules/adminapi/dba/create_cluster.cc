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
#include <set>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

Create_cluster::Create_cluster(mysqlshdk::mysql::IInstance *target_instance,
                               const std::string &cluster_name,
                               const Group_replication_options &gr_options,
                               const Clone_options &clone_options,
                               mysqlshdk::utils::nullable<bool> multi_primary,
                               bool adopt_from_gr, bool force, bool interactive)
    : m_target_instance(target_instance),
      m_cluster_name(cluster_name),
      m_gr_opts(gr_options),
      m_clone_opts(clone_options),
      m_multi_primary(multi_primary),
      m_adopt_from_gr(adopt_from_gr),
      m_force(force),
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

  std::string new_ssl_mode =
      resolve_cluster_ssl_mode(*m_target_instance, *m_gr_opts.ssl_mode);

  if (new_ssl_mode != *m_gr_opts.ssl_mode) {
    m_gr_opts.ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the cluster: '%s'",
                m_gr_opts.ssl_mode->c_str());
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

  // Validate the Clone options.
  m_clone_opts.check_option_values(m_target_instance->get_version());

  // Validate create cluster options (combinations).
  validate_create_cluster_options();

  // Disable super_read_only mode if it is enabled.
  bool super_read_only =
      m_target_instance->get_sysvar_bool("super_read_only").get_safe();
  if (super_read_only) {
    console->print_info("Disabling super_read_only mode on instance '" +
                        m_target_instance->descr() + "'.");
    log_debug(
        "Disabling super_read_only mode on instance '%s' to run createCluster.",
        m_target_instance->descr().c_str());
    m_target_instance->set_sysvar("super_read_only", false);
  }

  try {
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
          is_option_supported(m_target_instance->get_version(), kExpelTimeout,
                              k_global_replicaset_supported_options)) {
        m_gr_opts.exit_state_action = "READ_ONLY";
      }

      // Check replication filters before creating the Metadata.
      validate_replication_filters(*m_target_instance);

      // Resolve the SSL Mode to use to configure the instance.
      resolve_ssl_mode();

      // Make sure the target instance does not already belong to a cluster.
      mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
          *m_target_instance, m_target_instance->get_session());

      // Get the address used by GR for the added instance (used in MD).
      m_address_in_metadata = m_target_instance->get_canonical_address();

      // Resolve GR local address.
      // NOTE: Must be done only after getting the report_host used by GR and
      // for the metadata;
      m_gr_opts.local_address = mysqlsh::dba::resolve_gr_local_address(
          m_gr_opts.local_address, m_target_instance->get_canonical_hostname(),
          m_target_instance->get_canonical_port());

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

    if (!m_adopt_from_gr) {
      // Set the internal configuration object: read/write configs from the
      // server.
      m_cfg = mysqlsh::dba::create_server_config(
          m_target_instance, mysqlshdk::config::k_dft_cfg_server_handler);
    }
  } catch (...) {
    // catch any exception that is thrown, restore super read-only-mode if it
    // was enabled and re-throw the caught exception.
    if (super_read_only) {
      log_debug(
          "Restoring super_read_only mode on instance '%s' to 'ON' since it "
          "was enabled before createCluster operation started.",
          m_target_instance->descr().c_str());
      console->print_info(
          "Restoring super_read_only mode on instance '" +
          m_target_instance->descr() +
          "' since dba.<<<createCluster>>>() operation failed.");
      m_target_instance->set_sysvar("super_read_only", true);
    }
    throw;
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

void Create_cluster::setup_recovery(Cluster_impl *cluster,
                                    mysqlshdk::mysql::IInstance *target,
                                    std::string *out_username) {
  // Setup recovery account for this seed instance, which will be used
  // when/if it needs to rejoin.
  mysqlshdk::mysql::Auth_options repl_account =
      cluster->create_replication_user(target);

  if (out_username) *out_username = repl_account.user;

  // Insert the recovery account on the Metadata Schema.
  cluster->get_metadata_storage()->update_instance_recovery_account(
      target->get_uuid(), repl_account.user, "%");

  // Set the GR replication (recovery) user.
  log_debug("Changing GR recovery user details for %s",
            target->descr().c_str());

  try {
    mysqlshdk::gr::change_recovery_credentials(*target, repl_account.user,
                                               *repl_account.password);
  } catch (const std::exception &e) {
    current_console()->print_warning(
        shcore::str_format("Error updating recovery credentials for %s: %s",
                           target->descr().c_str(), e.what()));

    cluster->drop_replication_user(target);
  }
}

void Create_cluster::reset_recovery_all(Cluster_impl *cluster) {
  // Iterate all members of the group and reset recovery credentials
  auto console = mysqlsh::current_console();
  console->print_info(
      "Resetting distributed recovery credentials across the cluster...");

  std::set<std::string> new_users;
  std::set<std::string> old_users;

  cluster->get_default_replicaset()->execute_in_members(
      {}, cluster->get_group_session()->get_connection_options(), {},
      [&](const std::shared_ptr<mysqlshdk::db::ISession> &session) {
        mysqlshdk::mysql::Instance target(session);

        old_users.insert(mysqlshdk::gr::get_recovery_user(target));

        std::string new_user;
        setup_recovery(cluster, &target, &new_user);
        new_users.insert(new_user);
        return true;
      });

  // Drop old recovery accounts if they look like they were created by
  // the shell, since this group could have been a InnoDB cluster
  // previously, with its metadata dropped by the user.
  std::set<std::string> unused_users;
  std::set_difference(old_users.begin(), old_users.end(), new_users.begin(),
                      new_users.end(),
                      std::inserter(unused_users, unused_users.begin()));

  for (const auto &old_user : unused_users) {
    if (shcore::str_beginswith(
            old_user, mysqlshdk::gr::k_group_recovery_old_user_prefix)) {
      log_info("Removing old replication user '%s'", old_user.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(
            mysqlshdk::mysql::Instance(cluster->get_group_session()), old_user);
      } catch (const std::exception &e) {
        console->print_warning(
            "Error dropping old recovery accounts for user " + old_user + ": " +
            e.what());
      }
    }
  }
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
  mysqlshdk::gr::install_group_replication_plugin(*m_target_instance, nullptr);

  if (*m_multi_primary && !m_adopt_from_gr) {
    log_info(
        "The MySQL InnoDB cluster is going to be setup in advanced "
        "Multi-Primary Mode. Consult its requirements and limitations in "
        "https://dev.mysql.com/doc/refman/en/"
        "group-replication-limitations.html");
  }

  shcore::Value ret_val;
  {
    console->print_info("Adding Seed Instance...");

    if (!m_adopt_from_gr) {
      // Start GR before creating the recovery user and metadata, so that
      // all transactions executed by us have the same GTID prefix
      mysqlsh::dba::start_replicaset(*m_target_instance, m_gr_opts,
                                     m_multi_primary, m_cfg.get());
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

    // Insert Cluster on the Metadata Schema.
    metadata->insert_cluster(cluster->impl());
    metadata->update_cluster_attribute(
        cluster->impl()->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        m_clone_opts.gtid_set_is_complete ? shcore::Value::True()
                                          : shcore::Value::False());
    metadata->update_cluster_attribute(cluster->impl()->get_id(),
                                       k_cluster_attribute_default,
                                       shcore::Value::True());

    // Create the default replicaset in the Metadata and set it to the cluster.
    cluster->impl()->insert_default_replicaset(*m_multi_primary,
                                               m_adopt_from_gr);

    tx.commit();

    // Set default_replicaset group name
    cluster->impl()->get_default_replicaset()->set_group_name(
        m_target_instance->get_group_name());

    // Insert instance into the metadata.
    if (m_adopt_from_gr) {
      // Adoption from an existing GR group is performed after creating/updating
      // the metadata (since it is used internally by adopt_from_gr()).
      cluster->impl()->get_default_replicaset()->adopt_from_gr();

      // Reset recovery channel in all instances to use our own account
      reset_recovery_all(cluster->impl().get());
    } else {
      // Check if instance address already belong to replicaset (metadata).
      // TODO(alfredo) - this check seems redundant?
      bool is_instance_on_md =
          cluster->impl()->get_metadata_storage()->is_instance_on_replicaset(
              cluster->impl()->get_default_replicaset()->get_id(),
              m_address_in_metadata);

      log_debug(
          "Cluster %s: Instance '%s' %s",
          std::to_string(cluster->impl()->get_default_replicaset()->get_id())
              .c_str(),
          m_target_instance->descr().c_str(),
          is_instance_on_md ? "is already in the Metadata."
                            : "is being added to the Metadata...");

      // If the instance is not in the Metadata, we must add it.
      if (!is_instance_on_md) {
        cluster->impl()->get_default_replicaset()->add_instance_metadata(
            m_target_instance->get_connection_options());
      }
      setup_recovery(cluster->impl().get(), m_target_instance);
    }

    // Handle the clone options
    {
      // If disableClone: true, uninstall the plugin and update the Metadata
      if (!m_clone_opts.disable_clone.is_null() &&
          *m_clone_opts.disable_clone) {
        mysqlshdk::mysql::uninstall_clone_plugin(*m_target_instance, nullptr);
        cluster->impl()->set_disable_clone_option(*m_clone_opts.disable_clone);
      } else {
        // Install the plugin if the target instance is >= 8.0.17
        if (m_target_instance->get_version() >=
            mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
          // Try to install the plugin, if it fails or it's disabled, then
          // disable the clone usage on the cluster but proceed
          try {
            mysqlshdk::mysql::install_clone_plugin(*m_target_instance, nullptr);
          } catch (const std::exception &e) {
            console->print_warning(
                "Failed to install/activate the clone plugin: " +
                std::string(e.what()));
            console->print_info("Disabling the clone usage on the cluster.");
            console->print_info();

            cluster->impl()->set_disable_clone_option(true);
          }
        } else {
          // If the target instance is < 8.0.17 it does not support clone, so we
          // must disable it by default
          cluster->impl()->set_disable_clone_option(true);
        }
      }
    }

    // If it reaches here, it means there are no exceptions
    ret_val =
        shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(cluster));
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
