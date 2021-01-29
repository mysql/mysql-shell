/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/create_cluster_set.h"

#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/global_topology.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "mysql/group_replication.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "shellcore/console.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

Create_cluster_set::Create_cluster_set(
    Cluster_impl *cluster, const std::string &domain_name,
    const Create_cluster_set_options &options)
    : m_cluster(cluster), m_domain_name(domain_name), m_options(options) {
  assert(cluster);
}

Create_cluster_set::~Create_cluster_set() {}

void Create_cluster_set::check_version_requirement() {
  auto console = mysqlsh::current_console();
  std::vector<std::string> instances_addresses;

  auto lowest_instance_version =
      m_cluster->get_lowest_instance_version(&instances_addresses);

  if (lowest_instance_version < k_min_cs_version) {
    std::string instances_list = shcore::str_join(instances_addresses, ",");

    console->print_info("MySQL version " + lowest_instance_version.get_full() +
                        " detected at [" + instances_list +
                        "], but 8.0.27 is required for InnoDB ClusterSet.");
    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }
}

void Create_cluster_set::check_illegal_channels() {
  auto console = mysqlsh::current_console();

  // Get all cluster instances from the Metadata and their respective GR state.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instance_defs = m_cluster->get_instances_with_state();

  // Check if there are unknown replication channels running at
  // any member of the cluster
  for (const auto &instance_def : instance_defs) {
    mysqlshdk::gr::Member_state state = instance_def.second.state;

    if (state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Verify if the instance has a replication channel set and running
      std::string instance_address = instance_def.first.address;
      auto instance =
          m_cluster->get_session_to_cluster_instance(instance_address);
      log_debug(
          "Checking if instance '%s' has asynchronous (source-replica) "
          "replication configured.",
          instance_address.c_str());

      if (mysqlshdk::mysql::is_async_replication_configured(*instance)) {
        console->print_error(
            "Cluster member '" + instance_address +
            "' has asynchronous (source-replica) replication channel(s) "
            "configured which is not supported in InnoDB ClusterSet.");

        throw shcore::Exception(
            "Unsupported active replication channel.",
            SHERR_DBA_CLUSTER_UNSUPPORTED_REPLICATION_CHANNEL);
      }
    }
  }
}

void Create_cluster_set::check_gr_configuration() {
  auto console = mysqlsh::current_console();

  // Check if group_replication_view_change_uuid is set on the cluster
  std::string view_change_uuid =
      m_cluster->get_primary_master()
          ->get_sysvar_string("group_replication_view_change_uuid")
          .get_safe();

  if (view_change_uuid == "AUTOMATIC") {
    console->print_error(
        "The cluster is not configured to use "
        "group_replication_view_change_uuid. Please use <Cluster>.rescan() to "
        "repair the issue.");

    throw shcore::Exception(
        "group_replication_view_change_uuid not configured.",
        SHERR_DBA_CLUSTER_NOT_CONFIGURED_VIEW_UUID);
  } else {
    // Check if the value is stored in the Metadata
    if (m_cluster->get_view_change_uuid().empty()) {
      console->print_error(
          "The cluster's group_replication_view_change_uuid is not stored in "
          "the Metadata. Please use <Cluster>.rescan() to update the "
          "metadata.");

      throw shcore::Exception(
          "group_replication_view_change_uuid not configured.",
          SHERR_DBA_CLUSTER_NOT_CONFIGURED_VIEW_UUID);
    }
  }
}

void Create_cluster_set::resolve_ssl_mode() {
  bool have_ssl;
  bool require_secure_transport =
      m_cluster->get_cluster_server()
          ->get_sysvar_bool("require_secure_transport")
          .get_safe();

  // Resolve the Replication Channel SSL Mode
  auto resolved_ssl_mode = mysqlsh::dba::resolve_ssl_mode(
      *m_cluster->get_cluster_server(), m_options.ssl_mode, &have_ssl);

  if (resolved_ssl_mode.is_null()) {
    throw std::logic_error("Unable to resolve SSL mode");
  }

  if (have_ssl) {
    // sslMode is DISABLED but instance requires SSL
    if (m_options.ssl_mode == Cluster_ssl_mode::DISABLED &&
        require_secure_transport) {
      throw shcore::Exception::argument_error(
          "The instance '" + m_cluster->get_cluster_server()->descr() +
          "' requires secure connections, either turn off "
          "require_secure_transport or use the "
          "clusterSetReplicationSslMode option with 'REQUIRED', 'VERIFY_CA' "
          "or 'VERIFY_IDENTITY' value.");
    }

    // sslMode is VERIFY_CA or VERIFY_IDENTITY, verify the certificates
    if (m_options.ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
        m_options.ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
      ensure_certificates_set(*m_cluster->get_cluster_server(),
                              m_options.ssl_mode);
    }
  } else {
    if (m_options.ssl_mode == Cluster_ssl_mode::REQUIRED ||
        m_options.ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
        m_options.ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
      throw shcore::Exception::argument_error(
          "The instance '" + m_cluster->get_cluster_server()->descr() +
          "' does not have SSL enabled, either use an instance with SSL "
          "enabled, remove the clusterSetReplicationSslMode option or use it "
          "with any of 'AUTO' or 'DISABLED'.");
    }
  }

  m_options.ssl_mode = resolved_ssl_mode.get_safe();

  log_info(
      "SSL mode used to configure the ClusterSet replication channels: '%s'",
      to_string(m_options.ssl_mode).c_str());
}

void Create_cluster_set::prepare() {
  auto console = current_console();

  console->print_info(
      "A new ClusterSet will be created based on the Cluster '" +
      m_cluster->get_name() + "'.\n");

  console->print_info("* Validating Cluster '" + m_cluster->get_name() +
                      "' for ClusterSet compliance.\n");

  if (m_options.dry_run) {
    console->print_note(
        "dryRun option was specified. Validations will be executed, "
        "but no changes will be applied.");
  }

  // Verify if the Metadata version is compatible, i.e. >= 2.1.0.
  // NOTE: not made a precondition because this is the only use case
  if (!m_cluster->get_metadata_storage()->supports_cluster_set()) {
    throw std::runtime_error(
        "Operation not allowed. The installed metadata version is lower than "
        "the version required by Shell. Upgrade the metadata to execute this "
        "operation. See \\? dba.<<<upgradeMetadata>>> for additional details.");
  }

  // Validate the domain_name
  mysqlsh::dba::validate_cluster_name(m_domain_name,
                                      Cluster_type::REPLICATED_CLUSTER);

  // The target cluster must comply with the following requirements to become a
  // ClusterSet:
  //
  //  - Must be ONLINE (OK, OK_PARTIAL, OK_NO_TOLERANCE,
  //  OK_NO_TOLERANCE_PARTIAL)
  //  - The primary instance must be reachable/ONLINE
  //  - The cluster must not be part of a clusterSet already (check in
  //  preconditions check)
  //  - All cluster members must be running MySQL >= 8.0.27
  //  - The cluster's metadata version must be compatible: >= 2.1.0 (check in
  //  preconditions check)
  //  - Unknown replication channels are not allowed.
  //  - The Cluster is not running in multi-primary mode.
  //  - The Cluster must have its group_replication_view_change_uuid set and
  //  stored in the Metadata

  // Validate that the target cluster is not running in multi-primary mode
  if (m_cluster->get_cluster_topology_type() ==
      mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    console->print_error(
        "The cluster is running in multi-primary mode that is not "
        "supported in InnoDB ClusterSet.");
    throw shcore::Exception("Unsupported topology-mode: multi-primary.",
                            SHERR_DBA_CLUSTER_MULTI_PRIMARY_MODE_NOT_SUPPORTED);
  }

  // Check if the target cluster is ONLINE and if the primary instance is
  // reachable/ONLINE
  try {
    // If the primary is unavailable/unreachable this will throw
    m_cluster->acquire_primary();
  } catch (const shcore::Exception &e) {
    // NOTE: Not validating quorumless as it was validated already in the
    // preconditions check
    if (e.code() == SHERR_DBA_GROUP_UNAVAILABLE) {
      console->print_error("Target cluster is unavailable.");
      throw shcore::Exception("Invalid Cluster status: OFFLINE",
                              SHERR_DBA_CLUSTER_STATUS_INVALID);
    } else if (e.code() == SHERR_DBA_GROUP_HAS_NO_PRIMARY) {
      console->print_error(
          "The target cluster's primary instance is unavailable.");
      throw shcore::Exception("Invalid Cluster status: OFFLINE",
                              SHERR_DBA_CLUSTER_PRIMARY_UNAVAILABLE);
    } else {
      throw;
    }
  }

  // Validate the cluster members versions
  check_version_requirement();

  // Check for illegal replication channels
  check_illegal_channels();

  // Validate the Group Replication configurations
  check_gr_configuration();

  // Resolve the ClusterSet SSL mode
  resolve_ssl_mode();

  // Disable super_read_only mode if it is enabled.
  bool super_read_only = m_cluster->get_primary_master()
                             ->get_sysvar_bool("super_read_only")
                             .get_safe();
  if (super_read_only) {
    log_info("Disabling super_read_only mode on instance '%s'.",
             m_cluster->get_primary_master()->descr().c_str());
    m_cluster->get_primary_master()->set_sysvar("super_read_only", false);
  }
}

shcore::Value Create_cluster_set::execute() {
  auto console = current_console();
  std::unique_ptr<mysqlshdk::config::Config> config;
  shcore::Scoped_callback_list undo_list;
  std::shared_ptr<Cluster_set_impl> cs;

  try {
    console->print_info("* Creating InnoDB ClusterSet '" + m_domain_name +
                        "' on '" + m_cluster->get_name() + "'...\n");

    // Acquire primary at target cluster
    m_cluster->acquire_primary();

    // Create ClusterSet object
    cs = std::make_shared<Cluster_set_impl>(
        m_domain_name, m_cluster->get_cluster_server(),
        m_cluster->get_metadata_storage(),
        Global_topology_type::SINGLE_PRIMARY_TREE);

    // Invoke Primary promotion primitive
    // NOTE: No need to handle a failure with the undo_list in here since all
    // the tasks done in the promotion are just to ensure the right member
    // actions are set.
    cs->promote_to_primary(m_cluster->get_primary_master().get(),
                           m_options.dry_run);

    if (!m_options.dry_run) {
      // Enable skip_replica_start
      log_info("Persisting skip_replica_start=1 across the cluster...");

      config = m_cluster->create_config_object({}, false, true);

      config->set("skip_replica_start", mysqlshdk::utils::nullable<bool>(true));

      config->apply();

      undo_list.push_front([&config]() {
        log_info("Revert: Clearing skip_replica_start");
        config->set("skip_replica_start",
                    mysqlshdk::utils::nullable<bool>(false));
        config->apply();
      });

      // Set debug trap to test reversion of primary promotion
      DBUG_EXECUTE_IF("dba_create_cluster_set_fail_post_primary_promotion",
                      { throw std::logic_error("debug"); });
    }

    auto seed_cluster_id = m_cluster->get_id();

    console->print_info("* Updating metadata...");

    if (!m_options.dry_run) {
      // create and record the replication user for this cluster
      auto repl_user = cs->create_cluster_replication_user(
          m_cluster->get_primary_master().get(), m_options.dry_run);
      cs->record_cluster_replication_user(m_cluster, repl_user);

      undo_list.push_front([=]() {
        log_info("Revert: Dropping replication account '%s'",
                 repl_user.user.c_str());
        cs->drop_cluster_replication_user(m_cluster);
      });

      // Set debug trap to test reversion of replication user creation
      DBUG_EXECUTE_IF(
          "dba_create_cluster_set_fail_post_create_replication_user",
          { throw std::logic_error("debug"); });

      // Store the new ClusterSet in the Metadata schema
      cs->record_in_metadata(seed_cluster_id, m_options);
    }

    undo_list.cancel();

    if (m_options.dry_run) {
      console->print_info("dryRun finished.");
      console->print_info();

      return shcore::Value::Null();
    } else {
      console->print_info();
      console->print_info(
          "ClusterSet successfully created. Use "
          "ClusterSet.<<<createReplicaCluster>>>() to add Replica Clusters to "
          "it.");
      console->print_info();

      return shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(
          std::make_shared<ClusterSet>(cs)));
    }
  } catch (...) {
    console->print_error("Error creating ClusterSet: " +
                         format_active_exception());

    console->print_note("Reverting changes...");

    undo_list.call();

    console->print_info();
    console->print_info("Changes successfully reverted.");

    throw;
  }
}

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh
