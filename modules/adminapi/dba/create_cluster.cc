/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#include "adminapi/common/cluster_types.h"
#include "adminapi/common/dba_errors.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/dba_utils.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

Create_cluster::Create_cluster(const std::shared_ptr<Instance> &target_instance,
                               const std::string &cluster_name,
                               const Create_cluster_options &options)
    : m_target_instance(target_instance),
      m_cluster_name(cluster_name),
      m_options(options) {}

Create_cluster::Create_cluster(const std::shared_ptr<MetadataStorage> &metadata,
                               const std::shared_ptr<Instance> &target_instance,
                               const std::string &cluster_name,
                               const Create_cluster_options &options)
    : m_metadata(metadata),
      m_target_instance(target_instance),
      m_cluster_name(cluster_name),
      m_options(options) {}

Create_cluster::~Create_cluster() = default;

void Create_cluster::validate_create_cluster_options() {
  auto console = mysqlsh::current_console();

  if (!m_options.gr_options.auto_rejoin_tries.is_null() &&
      *m_options.gr_options.auto_rejoin_tries != 0) {
    std::string warn_msg =
        "The member will only proceed according to its exitStateAction if "
        "auto-rejoin fails (i.e. all retry attempts are exhausted).";
    console->print_warning(warn_msg);
    console->print_info();
  }

  // Get the instance GR state
  TargetType::Type instance_type = get_gr_instance_type(*m_target_instance);

  if (instance_type == mysqlsh::dba::TargetType::GroupReplication) {
    if (!m_options.adopt_from_gr && m_options.interactive()) {
      if (console->confirm(
              "You are connected to an instance that belongs to an unmanaged "
              "replication group.\nDo you want to setup an InnoDB Cluster "
              "based on this replication group?",
              mysqlsh::Prompt_answer::YES) == mysqlsh::Prompt_answer::YES) {
        m_options.adopt_from_gr = true;
      }
    }

    if (!m_retrying && !m_options.get_adopt_from_gr())
      throw shcore::Exception(
          "Creating a cluster on an unmanaged replication group requires "
          "adoptFromGR option to be true",
          SHERR_DBA_BADARG_INSTANCE_ALREADY_IN_GR);
  }

  if (m_options.get_adopt_from_gr() &&
      m_options.gr_options.ssl_mode != Cluster_ssl_mode::NONE) {
    throw shcore::Exception::argument_error(
        "Cannot use memberSslMode option if adoptFromGR is set to true.");
  }

  if (m_options.get_adopt_from_gr() && m_options.multi_primary) {
    throw shcore::Exception::argument_error(
        "Cannot use multiPrimary (or multiMaster) option if adoptFromGR is set "
        "to true. Using adoptFromGR mode will adopt the primary mode in use by "
        "the Cluster.");
  }

  if (m_options.get_adopt_from_gr() &&
      instance_type != TargetType::GroupReplication) {
    throw shcore::Exception::argument_error(
        "The adoptFromGR option is set to true, but there is no replication "
        "group to adopt");
  }

  // Using communicationStack and adoptFromGR and/or ipAllowlist/ipWhitelist at
  // the same time is forbidden
  if (m_options.adopt_from_gr &&
      !m_options.gr_options.communication_stack.is_null()) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Cannot use the '%s' option if '%s' is set "
                           "to true.",
                           kCommunicationStack, kAdoptFromGR));
  }

  if (m_options.gr_options.communication_stack.get_safe() ==
          kCommunicationStackMySQL &&
      !m_options.gr_options.ip_allowlist.is_null()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use '%s' when setting the '%s' option to '%s'",
        m_options.gr_options.ip_allowlist_option_name.c_str(),
        kCommunicationStack, kCommunicationStackMySQL));
  }

  // Set multi-primary to false by default.
  if (!m_options.multi_primary) m_options.multi_primary = false;

  if (*m_options.multi_primary && !m_options.force.get_safe()) {
    if (m_options.interactive()) {
      console->print_para(
          "The MySQL InnoDB Cluster is going to be setup in advanced "
          "Multi-Primary Mode. Before continuing you have to confirm that you "
          "understand the requirements and limitations of Multi-Primary Mode. "
          "For more information see "
          "https://dev.mysql.com/doc/refman/en/"
          "group-replication-limitations.html before proceeding.");

      console->print_para(
          "I have read the MySQL InnoDB Cluster manual and I understand the "
          "requirements and limitations of advanced Multi-Primary Mode.");

      if (console->confirm("Confirm", mysqlsh::Prompt_answer::NO) ==
          mysqlsh::Prompt_answer::NO) {
        console->print_info();
        throw shcore::cancelled("Cancelled");
      } else {
        console->print_info();
        m_options.force = true;
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
  bool have_ssl;
  bool require_secure_transport =
      m_target_instance->get_sysvar_bool("require_secure_transport").get_safe();

  auto resolved_ssl_mode = mysqlsh::dba::resolve_ssl_mode(
      *m_target_instance, m_options.gr_options.ssl_mode, &have_ssl);

  if (resolved_ssl_mode.is_null()) {
    throw std::logic_error("Unable to resolve SSL mode");
  }

  if (have_ssl) {
    // sslMode is DISABLED but instance requires SSL
    if (m_options.gr_options.ssl_mode == Cluster_ssl_mode::DISABLED &&
        require_secure_transport) {
      throw shcore::Exception::argument_error(
          "The instance '" + m_target_instance->descr() +
          "' requires secure connections, to create the "
          "cluster either turn off require_secure_transport or use the "
          "memberSslMode option with 'REQUIRED', 'VERIFY_CA' or "
          "'VERIFY_IDENTITY' value.");
    }

    // sslMode is VERIFY_CA or VERIFY_IDENTITY, verify the certificates
    if (m_options.gr_options.ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
        m_options.gr_options.ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
      ensure_certificates_set(*m_target_instance,
                              m_options.gr_options.ssl_mode);
    }
  } else {  // The instance does not support SSL
    // sslMode is REQUIRED, VERIFY_CA or VERIFY_IDENTITY
    if (m_options.gr_options.ssl_mode == Cluster_ssl_mode::REQUIRED ||
        m_options.gr_options.ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
        m_options.gr_options.ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
      throw shcore::Exception::argument_error(
          "The instance '" + m_target_instance->descr() +
          "' does not have SSL enabled, to create the "
          "cluster either use an instance with SSL enabled, remove the "
          "memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.");
    }
  }

  m_options.gr_options.ssl_mode = resolved_ssl_mode.get_safe();

  log_info("SSL mode used to configure the cluster: '%s'",
           to_string(m_options.gr_options.ssl_mode).c_str());
}

void Create_cluster::prepare() {
  auto console = mysqlsh::current_console();

  std::string msg = "A new InnoDB Cluster will be created ";

  if (m_options.adopt_from_gr) {
    msg += "based on the existing replication group ";
  }

  msg += "on instance '" + m_target_instance->descr();
  msg += "'.";

  console->print_info(msg);
  console->print_info();

  // Validate the cluster_name
  mysqlsh::dba::validate_cluster_name(m_cluster_name,
                                      Cluster_type::GROUP_REPLICATION);

  // Validate values given for GR options.
  m_options.gr_options.check_option_values(
      m_target_instance->get_version(),
      m_target_instance->get_canonical_port());

  // Validate the Clone options.
  m_options.clone_options.check_option_values(m_target_instance->get_version());

  // Check if we're retrying a previously failed create attempt
  if (mysqlshdk::mysql::check_indicator_tag(
          *m_target_instance, metadata::kClusterSetupIndicatorTag)) {
    log_info(
        "createCluster() detected a previously failed attempt, retrying...");
    m_retrying = true;
  }

  // Validate create cluster options (combinations).
  validate_create_cluster_options();

  // The default value for communicationStack must be 'mysql' if the target
  // instance is running 8.0.27+
  // NOTE: When adoptFromGR is used, we don't set the communication stack since
  // the one in use by the unmanaged GR group must be kept
  if (m_options.gr_options.communication_stack.is_null() &&
      !m_options.adopt_from_gr &&
      m_target_instance->get_version() >=
          k_mysql_communication_stack_initial_version) {
    m_options.gr_options.communication_stack = kCommunicationStackMySQL;

    // Verify if the allowlist is used when the communication stack is MySQL by
    // default
    if (!m_options.gr_options.ip_allowlist.is_null()) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Cannot use '%s' when the '%s' is '%s'",
          m_options.gr_options.ip_allowlist_option_name.c_str(),
          kCommunicationStack, kCommunicationStackMySQL));
    }

    // Validate localAddress if the communication stack is MySQL by default
    if (!m_options.gr_options.local_address.is_null()) {
      validate_local_address_option(
          *m_options.gr_options.local_address,
          m_options.gr_options.communication_stack.get_safe(),
          m_target_instance->get_canonical_port());
    }
  }

  // Verify if the instance is running asynchronous
  // replication. Skip if 'force' is enabled.
  if (!m_options.force.get_safe()) {
    validate_async_channels(*m_target_instance, {}, checks::Check_type::CREATE);
  } else {
    log_debug(
        "Skipping verification to check if instance '%s' is running "
        "asynchronous replication.",
        m_target_instance->descr().c_str());
  }

  // If adopting, set the target_instance to the primary member of the group:
  //
  // Adopting a cluster can be performed using any member of the group
  // regardless if it's a primary or a secondary. However, all the operations
  // performed during the adoption must be done in the primary member of the
  // group. We should not bypass GR's mechanism to avoid writes on the secondary
  // by disabling super_read_only. Apart from possible GTID inconsistencies, a
  // secondary will end up having super_read_only disabled.
  if (m_options.get_adopt_from_gr()) {
    m_target_instance = get_primary_member_from_group(m_target_instance);
  }

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
    if (!m_options.get_adopt_from_gr()) {
      // Check instance configuration and state, like dba.checkInstance
      // but skip if we're adopting, since in that case the target is obviously
      // already configured
      ensure_gr_instance_configuration_valid(m_target_instance.get());

      // exitStateAction default value should only be set if supported in
      // the target instance and must be READ_ONLY for versions < 8.0.16 since
      // the default was ABORT_SERVER which was considered to be too drastic
      // NOTE: In 8.0.16 the default value became READ_ONLY so we must not
      // change it
      auto instance_version = m_target_instance->get_version();
      if (m_options.gr_options.exit_state_action.is_null() &&
          is_option_supported(instance_version, kExpelTimeout,
                              k_global_cluster_supported_options) &&
          instance_version < mysqlshdk::utils::Version("8.0.16")) {
        m_options.gr_options.exit_state_action = "READ_ONLY";
      }

      // Check replication filters before creating the Metadata.
      validate_replication_filters(*m_target_instance,
                                   Cluster_type::GROUP_REPLICATION);

      // Resolve the SSL Mode to use to configure the instance.
      resolve_ssl_mode();

      // Make sure the target instance does not already belong to a cluster.
      if (!m_retrying)
        mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
            m_target_instance, m_target_instance, {});

      // Get the address used by GR for the added instance (used in MD).
      m_address_in_metadata = m_target_instance->get_canonical_address();

      // Resolve GR local address.
      // NOTE: Must be done only after getting the report_host used by GR and
      // for the metadata;
      m_options.gr_options.local_address =
          mysqlsh::dba::resolve_gr_local_address(
              m_options.gr_options.local_address,
              m_options.gr_options.communication_stack,
              m_target_instance->get_canonical_hostname(),
              m_target_instance->get_canonical_port(), !m_retrying, false);

      // Validate that the group_replication_local_address is valid for the
      // version we are using
      // Note this has to be done here after the resolve_gr_local_address method
      // since we need to validate both a gr_local_address value that was passed
      // by the user or the one that is automatically chosen.
      validate_local_address_ip_compatibility();

      // Generate the GR group name (if needed).
      if (m_options.gr_options.group_name.is_null()) {
        m_options.gr_options.group_name =
            mysqlshdk::gr::generate_uuid(*m_target_instance);
      }

      // Generate the GR view-change UUID
      if (m_target_instance->get_version() >= k_min_cs_version) {
        m_options.gr_options.view_change_uuid =
            mysqlshdk::gr::generate_uuid(*m_target_instance);
      }
    }

    // Determine the GR mode if the cluster is to be adopt from GR.
    if (m_options.get_adopt_from_gr()) {
      // Get the primary UUID value to determine GR mode:
      // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
      std::string gr_primary_uuid =
          mysqlshdk::gr::get_group_primary_uuid(*m_target_instance, nullptr);

      m_options.multi_primary = gr_primary_uuid.empty();
    } else {
      // Set the internal configuration object: read/write configs from the
      // server.
      m_cfg = mysqlsh::dba::create_server_config(
          m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);
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

  // Check if the operation is being called from create_replica_cluster():
  // if the Metadata belongs to a ClusterSet we can assume that
  Cluster_metadata cluster_md;
  if (m_metadata &&
      m_metadata->get_cluster_for_server_uuid(
          m_metadata->get_md_server()->get_uuid(), &cluster_md) &&
      !cluster_md.cluster_set_id.empty()) {
    m_create_replica_cluster = true;
  }
}

void Create_cluster::log_used_gr_options() {
  auto console = mysqlsh::current_console();

  log_info("Using Group Replication single primary mode: %s",
           *m_options.multi_primary ? "FALSE" : "TRUE");

  if (m_options.gr_options.ssl_mode != Cluster_ssl_mode::NONE) {
    log_info("Using Group Replication SSL mode: %s",
             to_string(m_options.gr_options.ssl_mode).c_str());
  }

  if (!m_options.gr_options.group_name.is_null()) {
    log_info("Using Group Replication group name: %s",
             m_options.gr_options.group_name->c_str());
  }

  if (!m_options.gr_options.local_address.is_null()) {
    log_info("Using Group Replication local address: %s",
             m_options.gr_options.local_address->c_str());
  }

  if (!m_options.gr_options.group_seeds.is_null()) {
    log_info("Using Group Replication group seeds: %s",
             m_options.gr_options.group_seeds->c_str());
  }

  if (!m_options.gr_options.exit_state_action.is_null()) {
    log_info("Using Group Replication exit state action: %s",
             m_options.gr_options.exit_state_action->c_str());
  }

  if (!m_options.gr_options.member_weight.is_null()) {
    log_info("Using Group Replication member weight: %s",
             std::to_string(*m_options.gr_options.member_weight).c_str());
  }

  if (!m_options.gr_options.consistency.is_null()) {
    log_info("Using Group Replication failover consistency: %s",
             m_options.gr_options.consistency->c_str());
  }

  if (!m_options.gr_options.expel_timeout.is_null()) {
    log_info("Using Group Replication expel timeout: %s",
             std::to_string(*m_options.gr_options.expel_timeout).c_str());
  }

  if (!m_options.gr_options.auto_rejoin_tries.is_null()) {
    log_info("Using Group Replication auto-rejoin tries: %s",
             std::to_string(*m_options.gr_options.auto_rejoin_tries).c_str());
  }

  if (!m_options.gr_options.communication_stack.is_null()) {
    log_info("Using Group Replication Communication Stack: %s",
             m_options.gr_options.communication_stack->c_str());
  }

  if (!m_options.gr_options.transaction_size_limit.is_null()) {
    log_info(
        "Using Group Replication transaction size limit: %s",
        std::to_string(*m_options.gr_options.transaction_size_limit).c_str());
  }
}

void Create_cluster::prepare_metadata_schema() {
  // Ensure that the metadata schema is ready for creating a new cluster in it
  // If the metadata schema does not exist, we create it
  // If the metadata schema already exists:
  // - clear it of old data
  // - ensure it has the right version
  // If we're retrying from a previous createCluster attempt, drop and recreate
  // We ensure both by always dropping the old schema and re-creating it from
  // scratch.

  mysqlsh::dba::prepare_metadata_schema(m_target_instance, false);
}

void Create_cluster::create_recovery_account(
    mysqlshdk::mysql::IInstance *target, Cluster_impl *cluster,
    std::string *out_username) {
  // Setup recovery account for this seed instance, which will be used
  // when/if it needs to rejoin.
  std::string repl_account_host;
  mysqlshdk::mysql::Auth_options repl_account;

  if (cluster) {
    std::tie(repl_account, repl_account_host) =
        cluster->create_replication_user(target);
  } else {
    repl_account_host = m_options.replication_allowed_host.empty()
                            ? "%"
                            : m_options.replication_allowed_host;

    repl_account.user = mysqlshdk::gr::k_group_recovery_user_prefix +
                        std::to_string(m_target_instance->get_server_id());

    log_info("Creating recovery account '%s'@'%s' for instance '%s'",
             repl_account.user.c_str(), repl_account_host.c_str(),
             m_target_instance->descr().c_str());

    if (m_target_instance->user_exists(repl_account.user, repl_account_host)) {
      mysqlsh::current_console()->print_note(shcore::str_format(
          "User '%s'@'%s' already existed at instance '%s'. It will be "
          "deleted and created again with a new password.",
          repl_account.user.c_str(), repl_account_host.c_str(),
          m_target_instance->descr().c_str()));
      m_target_instance->drop_user(repl_account.user, repl_account_host);
    }

    auto target_version = m_target_instance->get_version();

    bool clone_available_on_target =
        target_version >=
        mysqlshdk::mysql::k_mysql_clone_plugin_initial_version;

    bool mysql_communication_stack_available_on_target =
        target_version >= k_mysql_communication_stack_initial_version;

    repl_account = mysqlshdk::gr::create_recovery_user(
        repl_account.user, *m_target_instance, {repl_account_host}, {},
        clone_available_on_target, false,
        mysql_communication_stack_available_on_target);
  }

  if (out_username) {
    *out_username = repl_account.user;
  }

  // Configure GR's recovery channel with the newly created account
  log_debug("Changing GR recovery user details for %s",
            target->descr().c_str());

  try {
    mysqlshdk::mysql::change_replication_credentials(
        *target, repl_account.user, *repl_account.password,
        mysqlshdk::gr::k_gr_recovery_channel);
  } catch (const std::exception &e) {
    current_console()->print_warning(
        shcore::str_format("Error updating recovery credentials for %s: %s",
                           target->descr().c_str(), e.what()));

    cluster->drop_replication_user(target);
  }
}

void Create_cluster::store_recovery_account_metadata(
    mysqlshdk::mysql::IInstance *target, const Cluster_impl &cluster) {
  std::string repl_account_host = "%";
  mysqlshdk::mysql::Auth_options repl_account;

  shcore::Value allowed_host;
  if (cluster.get_metadata_storage()->query_cluster_attribute(
          cluster.get_id(), k_cluster_attribute_replication_allowed_host,
          &allowed_host) &&
      allowed_host.type == shcore::String &&
      !allowed_host.as_string().empty()) {
    repl_account_host = allowed_host.as_string();
  }

  repl_account.user = mysqlshdk::mysql::get_replication_user(
      *target, mysqlshdk::gr::k_gr_recovery_channel);

  // Insert the recovery account on the Metadata Schema.
  cluster.get_metadata_storage()->update_instance_repl_account(
      target->get_uuid(), Cluster_type::GROUP_REPLICATION, repl_account.user,
      repl_account_host);
}

void Create_cluster::reset_recovery_all(Cluster_impl *cluster) {
  // Iterate all members of the group and reset recovery credentials
  auto console = mysqlsh::current_console();
  console->print_info(
      "Resetting distributed recovery credentials across the cluster...");

  std::set<std::string> new_users;
  std::set<std::string> old_users;

  cluster->execute_in_members(
      {}, cluster->get_cluster_server()->get_connection_options(), {},
      [&](const std::shared_ptr<Instance> &target,
          const mysqlshdk::gr::Member &) {
        old_users.insert(mysqlshdk::mysql::get_replication_user(
            *target, mysqlshdk::gr::k_gr_recovery_channel));

        std::string new_user;

        create_recovery_account(target.get(), cluster, &new_user);
        store_recovery_account_metadata(target.get(), *cluster);

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
            *cluster->get_cluster_server(), old_user);
      } catch (const std::exception &e) {
        console->print_warning(
            "Error dropping old recovery accounts for user " + old_user + ": " +
            e.what());
      }
    }
  }
}

void Create_cluster::persist_sro_all(Cluster_impl *cluster) {
  log_info("Persisting super_read_only=1 across the cluster...");

  auto config = cluster->create_config_object({}, false, true);

  config->set("super_read_only", mysqlshdk::utils::nullable<bool>(true));

  config->apply();
}

void Create_cluster::validate_local_address_ip_compatibility() const {
  // local_address must have some value
  assert(!m_options.gr_options.local_address.is_null() &&
         !m_options.gr_options.local_address.get_safe().empty());

  std::string local_address = m_options.gr_options.local_address.get_safe();

  // Validate that the group_replication_local_address is valid for the version
  // of the target instance.
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
          local_address, m_target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot create cluster on instance '" +
                         m_target_instance->descr() +
                         "': unsupported localAddress value.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 and the target instance version is %s.",
        local_address.c_str(),
        m_target_instance->get_version().get_base().c_str()));
  }
}

shcore::Value Create_cluster::execute() {
  auto console = mysqlsh::current_console();
  console->print_info("Creating InnoDB Cluster '" + m_cluster_name + "' on '" +
                      m_target_instance->descr() + "'...\n");

  //  Common informative logging
  if (m_options.get_adopt_from_gr()) {
    log_info(
        "Adopting cluster from existing Group Replication and using its "
        "settings.");
  } else {
    log_used_gr_options();
  }

  // Make sure the GR plugin is installed (only installed if needed).
  // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
  //       Disable read-only temporarily to install the plugin if needed.
  if (!m_options.dry_run) {
    mysqlshdk::gr::install_group_replication_plugin(*m_target_instance,
                                                    nullptr);
  }

  // Reset GR member actions to avoid BUG#34464526 "Missing
  // mysql_start_failover_channels_if_primary after upgrade to 8.0.27+"
  //
  // When a Cluster is upgraded from 8.0.26 to 8.0.27+, every instance that is
  // taken out and rejoined back will have the list of available member
  // actions reduced to the ones available in 8.0.26 (the current primary).
  // In this particular scenario, the action
  // 'mysql_start_failover_channels_if_primary' is removed since it's only
  // available in 8.0.27+.
  //
  // As soon as the primary is taken out, upgraded, and rejoined, the member
  // actions available in the group are the "reduced" ones, meaning
  // 'mysql_start_failover_channels_if_primary' is not available.
  //
  // This results in an error while creating a Replica Cluster since that
  // action is configured and enabled. Resetting the member actions beforehand
  // ensures this issue is not seen.
  if (!m_options.dry_run && m_create_replica_cluster) {
    try {
      mysqlshdk::gr::reset_member_actions(*m_target_instance);
    } catch (...) {
      log_error("Error resetting Group Replication member actions at %s: %s",
                m_target_instance->descr().c_str(),
                format_active_exception().c_str());
      throw;
    }
  }

  if (*m_options.multi_primary && !m_options.get_adopt_from_gr()) {
    console->print_info(
        "The MySQL InnoDB Cluster is going to be setup in advanced "
        "Multi-Primary Mode. Consult its requirements and limitations in "
        "https://dev.mysql.com/doc/refman/en/"
        "group-replication-limitations.html");
  }

  // Get the current value of group_replication_transaction_size_limit
  int64_t original_transaction_size_limit =
      m_target_instance->get_sysvar_int(kGrTransactionSizeLimit).get_safe();
  ;

  shcore::Scoped_callback_list undo_list;

  shcore::Value ret_val;
  try {
    console->print_info("Adding Seed Instance...");

    if (!m_options.get_adopt_from_gr()) {
      if (m_retrying && !m_options.dry_run) {
        // If we're retrying, make sure GR is stopped
        console->print_note("Stopping GR started by a previous failed attempt");
        mysqlshdk::gr::stop_group_replication(*m_target_instance);
        // stop GR will leave the server in SRO mode
        m_target_instance->set_sysvar("super_read_only", false);
      }

      // Set a guard to indicate that a createCluster() has started.
      // This has to happen before start GR, otherwise we can't tell whether
      // a running GR was started by us or the user.
      // This hack is used as an indicator that createCluster is running in an
      // instance If the instance already has master_user set to this value when
      // we enter createCluster(), it means a previous attempt failed.
      // Note: this shouldn't be set if we're adopting
      assert(!m_options.get_adopt_from_gr());
      mysqlshdk::mysql::create_indicator_tag(
          *m_target_instance, metadata::kClusterSetupIndicatorTag);

      // When using the communicationStack 'MySQL' the recovery channel must be
      // set-up before starting GR because GR requires that the recovery channel
      // is configured in order to obtain the credentials information
      if (m_options.gr_options.communication_stack.get_safe() ==
          kCommunicationStackMySQL) {
        // Create the Recovery account and configure GR's recovery channel.
        // If creating a Replica Cluster, suppress the binary log otherwise
        // we're creating an errant transaction in the Replica Cluster.
        // When a standalone Cluster we must not suppress the binary log
        // otherwise the transaction will never be replicated to the Cluster
        // members

        // Check if super_read_only is enabled. If so it must be disabled to
        // create the account. This might happen if super_read_only is persisted
        // and we're creating a Replica Cluster
        if (m_target_instance->get_sysvar_bool("super_read_only").get_safe()) {
          m_target_instance->set_sysvar("super_read_only", false);
        }

        std::string repl_account;

        if (m_create_replica_cluster) {
          mysqlshdk::mysql::Suppress_binary_log nobinlog(
              m_target_instance.get());

          create_recovery_account(m_target_instance.get(), nullptr,
                                  &repl_account);
        } else {
          create_recovery_account(m_target_instance.get(), nullptr,
                                  &repl_account);
        }

        // Drop the recovery account if GR fails to start
        undo_list.push_front([=]() {
          std::string repl_account_host =
              m_options.replication_allowed_host.empty()
                  ? "%"
                  : m_options.replication_allowed_host;
          m_target_instance->drop_user(repl_account, repl_account_host);
        });
      }

      // GR has to be stopped if this fails
      undo_list.push_front([this, console]() {
        log_info("revert: Stopping group_replication");
        try {
          mysqlshdk::gr::stop_group_replication(*m_target_instance);
        } catch (const shcore::Error &e) {
          console->print_warning(
              "Could not stop group_replication while aborting changes: " +
              std::string(e.what()));
        }
        log_info("revert: Clearing super_read_only");
        try {
          m_target_instance->set_sysvar("super_read_only", false);
        } catch (const shcore::Error &e) {
          console->print_warning(
              "Could not clear super_read_only while aborting changes: " +
              std::string(e.what()));
        }
      });
      // Start GR before creating the recovery user and metadata, so that
      // all transactions executed by us have the same GTID prefix
      // NOTE: When the communicationStack chosen is "MySQL", there's one
      // transaction that will have a different GTID prefix that is the "CREATE
      // USER" (see above)
      mysqlsh::dba::start_cluster(*m_target_instance, m_options.gr_options,
                                  m_options.multi_primary, m_cfg.get());
    }

    std::string group_name = m_target_instance->get_group_name();

    undo_list.push_front([this]() { metadata::uninstall(m_target_instance); });

    std::shared_ptr<MetadataStorage> metadata;
    if (!m_metadata) {
      // Create the Metadata Schema.
      prepare_metadata_schema();
      metadata = std::make_shared<MetadataStorage>(m_target_instance);
    } else {
      metadata = m_metadata;
    }

    // Create the cluster and insert it into the Metadata.

    std::string domain_name;
    std::string cluster_name;
    parse_fully_qualified_cluster_name(m_cluster_name, &domain_name, nullptr,
                                       &cluster_name);
    if (domain_name.empty()) domain_name = k_default_domain_name;

    mysqlshdk::gr::Topology_mode topology_mode =
        (*m_options.multi_primary)
            ? mysqlshdk::gr::Topology_mode::MULTI_PRIMARY
            : mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY;

    auto cluster_impl = std::make_shared<Cluster_impl>(
        cluster_name, group_name, m_target_instance, metadata, topology_mode);

    // Update the properties
    // For V1.0, let's see the Cluster's description to "default"
    cluster_impl->set_description("Default Cluster");

    // Insert Cluster (and ReplicaSet) on the Metadata Schema.
    MetadataStorage::Transaction trx(metadata);
    undo_list.push_front([&trx]() { trx.rollback(); });

    metadata->create_cluster_record(cluster_impl.get(),
                                    m_options.get_adopt_from_gr(), false);

    metadata->update_cluster_attribute(
        cluster_impl->get_id(), k_cluster_attribute_replication_allowed_host,
        m_options.replication_allowed_host.empty()
            ? shcore::Value("%")
            : shcore::Value(m_options.replication_allowed_host));

    // Store the communicationStack in the Metadata as a Cluster capability
    // TODO(miguel): build and add the list of allowed operations
    metadata->update_cluster_capability(
        cluster_impl->get_id(), kCommunicationStack,
        m_options.gr_options.communication_stack.get_safe(), {});

    metadata->update_cluster_attribute(
        cluster_impl->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        m_options.clone_options.gtid_set_is_complete ? shcore::Value::True()
                                                     : shcore::Value::False());

    metadata->update_cluster_attribute(
        cluster_impl->get_id(), k_cluster_attribute_manual_start_on_boot,
        m_options.gr_options.manual_start_on_boot.get_safe(false)
            ? shcore::Value::True()
            : shcore::Value::False());

    metadata->update_cluster_attribute(cluster_impl->get_id(),
                                       k_cluster_attribute_default,
                                       shcore::Value::True());

    if (!m_options.gr_options.view_change_uuid.is_null()) {
      metadata->update_cluster_attribute(
          cluster_impl->get_id(), "group_replication_view_change_uuid",
          shcore::Value(m_options.gr_options.view_change_uuid.get_safe()));
    }

    // Store in the Metadata the value of
    // group_replication_transaction_size_limit
    if (m_create_replica_cluster) {
      metadata->update_cluster_attribute(
          cluster_impl->get_id(), k_cluster_attribute_transaction_size_limit,
          shcore::Value(original_transaction_size_limit));
    } else {
      // Get and store the current value. If the option was used, the value was
      // already set by now so we're good
      metadata->update_cluster_attribute(
          cluster_impl->get_id(), k_cluster_attribute_transaction_size_limit,
          shcore::Value(
              m_target_instance->get_sysvar_int(kGrTransactionSizeLimit)
                  .get_safe()));
    }

    // Insert instance into the metadata.
    if (m_options.get_adopt_from_gr()) {
      // Adoption from an existing GR group is performed after creating/updating
      // the metadata (since it is used internally by adopt_from_gr()).
      cluster_impl->adopt_from_gr();

      // Reset recovery channel in all instances to use our own account
      reset_recovery_all(cluster_impl.get());

      persist_sro_all(cluster_impl.get());
    } else {
      // Check if instance address already belong to cluster (metadata).
      // TODO(alfredo) - this check seems redundant?
      bool is_instance_on_md =
          cluster_impl->contains_instance_with_address(m_address_in_metadata);

      log_debug("Cluster %s: Instance '%s' %s",
                cluster_impl->get_name().c_str(),
                m_target_instance->descr().c_str(),
                is_instance_on_md ? "is already in the Metadata."
                                  : "is being added to the Metadata...");

      // If the instance is not in the Metadata, we must add it.
      if (!is_instance_on_md) {
        cluster_impl->add_metadata_for_instance(
            m_target_instance->get_connection_options());
      }

      // Create the recovery account and update the Metadata
      // NOTE: If the Cluster is a REPLICA the account must be created at the
      // PRIMARY Cluster too (create_recovery_account will create it at the MD
      // server)
      if (m_create_replica_cluster ||
          m_options.gr_options.communication_stack.is_null() ||
          m_options.gr_options.communication_stack.get_safe() ==
              kCommunicationStackXCom) {
        std::string repl_account;
        create_recovery_account(m_target_instance.get(), cluster_impl.get(),
                                &repl_account);

        // Drop the recovery account if GR fails to start
        undo_list.push_front([=]() {
          std::string repl_account_host =
              m_options.replication_allowed_host.empty()
                  ? "%"
                  : m_options.replication_allowed_host;
          m_target_instance->drop_user(repl_account, repl_account_host);
        });

        store_recovery_account_metadata(m_target_instance.get(), *cluster_impl);
      } else {
        // The recovery account was already created, just updated the Metadata:
        // Standalone cluster using MySQL comm stack
        store_recovery_account_metadata(m_target_instance.get(), *cluster_impl);
      }
    }

    // Handle the clone options
    {
      // If disableClone: true, uninstall the plugin and update the Metadata
      if (!m_options.clone_options.disable_clone.is_null() &&
          *m_options.clone_options.disable_clone) {
        mysqlshdk::mysql::uninstall_clone_plugin(*m_target_instance, nullptr);
        cluster_impl->set_disable_clone_option(
            *m_options.clone_options.disable_clone);
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

            cluster_impl->set_disable_clone_option(true);
          }
        } else {
          // If the target instance is < 8.0.17 it does not support clone, so we
          // must disable it by default
          cluster_impl->set_disable_clone_option(true);
        }
      }
    }

    // Everything after this should not affect the result of the createCluster()
    // even if an exception is thrown.
    trx.commit();
    undo_list.cancel();

    // If it reaches here, it means there are no exceptions
    std::shared_ptr<Cluster> cluster = std::make_shared<Cluster>(cluster_impl);

    ret_val =
        shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(cluster));
  } catch (...) {
    try {
      log_info("createCluster() failed: Trying to revert changes...");

      undo_list.call();

      mysqlshdk::mysql::drop_indicator_tag(*m_target_instance,
                                           metadata::kClusterSetupIndicatorTag);
    } catch (const std::exception &e) {
      console->print_error("Error aborting changes: " + std::string(e.what()));
    }
    throw;
  }

  mysqlshdk::mysql::drop_indicator_tag(*m_target_instance,
                                       metadata::kClusterSetupIndicatorTag);

  std::string message =
      m_options.get_adopt_from_gr()
          ? "Cluster successfully created based on existing "
            "replication group."
          : "Cluster successfully created. Use "
            "Cluster.<<<addInstance>>>() to add MySQL instances.\n"
            "At least 3 instances are needed for the cluster to be "
            "able to withstand up to\n"
            "one server failure.";
  console->print_info(message);
  console->print_info();

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
