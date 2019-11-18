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

#include "modules/adminapi/replica_set/replica_set_impl.h"

#include <mysql.h>
#include <mysqld_error.h>
#include <future>
#include <thread>
#include <utility>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/async_utils.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/global_topology_check.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_monitoring.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/star_global_topology_manager.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/replica_set/replica_set_status.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

constexpr const char *k_async_cluster_channel_name = "";
constexpr const char *k_async_cluster_user_name = "mysql_innodb_rs";
constexpr const char *k_error_connecting_to_instance =
    "Unable to connect to the target instance %s. Please verify the connection "
    "settings, make sure the instance is available and try again.";

// # of seconds to wait until clone starts
constexpr const int k_clone_start_timeout = 30;

namespace {
std::unique_ptr<topology::Server_global_topology> discover_unmanaged_topology(
    Instance *instance) {
  auto console = current_console();

  std::unique_ptr<topology::Server_global_topology> topology(
      new topology::Server_global_topology(k_async_cluster_channel_name));

  // perform initial discovery
  topology->discover_from_unmanaged(instance);

  topology->check_gtid_consistency(false);

  return topology;
}

void validate_version(const Instance &target_server) {
  if (target_server.get_version() < mysqlshdk::utils::Version(8, 0, 11) ||
      target_server.get_version() >= mysqlshdk::utils::Version(8, 1, 0)) {
    current_console()->print_info(
        "MySQL version " + target_server.get_version().get_full() +
        " detected at " + target_server.get_canonical_address() +
        ", but 8.0 is required for InnoDB ReplicaSets.");
    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }
}

void validate_instance(Instance *target_server) {
  auto console = current_console();

  // Check instance configuration and state
  ensure_ar_instance_configuration_valid(target_server);

  // Check if GR is running
  mysqlshdk::gr::Member_state state;
  if (mysqlshdk::gr::get_group_information(*target_server, &state, nullptr,
                                           nullptr, nullptr) &&
      state != mysqlshdk::gr::Member_state::OFFLINE) {
    console->print_error(target_server->descr() +
                         " has Group Replication active, which cannot be mixed "
                         "with ReplicaSets.");

    throw shcore::Exception("group_replication active",
                            SHERR_DBA_INVALID_SERVER_CONFIGURATION);
  }
}

std::vector<std::pair<mysqlshdk::mysql::Slave_host, std::string>>
get_valid_slaves(mysqlshdk::mysql::IInstance *instance) {
  auto slaves = get_slaves(*instance);
  std::vector<std::pair<mysqlshdk::mysql::Slave_host, std::string>> real_slaves;
  auto ipool = current_ipool();

  for (const auto &ch : slaves) {
    // This is a bit unnecessary, but we do it to avoid false positives,
    // specially in tests. The problem is that SHOW SLAVE HOSTS will include
    // slaves that have stopped replicating, so we need to double-check that
    // the channel is still active to prevent false positives.

    std::string endpoint =
        mysqlshdk::utils::make_host_and_port(ch.host, ch.port);
    std::string channel_name;
    bool ghost_slave = true;

    try {
      Scoped_instance slave(ipool->connect_unchecked_endpoint(endpoint));

      auto slave_channels = mysqlshdk::mysql::get_incoming_channels(*slave);
      for (const auto &slave_channel : slave_channels) {
        if (slave_channel.source_uuid == instance->get_uuid()) {
          channel_name = slave_channel.channel_name;
          ghost_slave = false;
          break;
        }
      }
    } catch (const shcore::Exception &e) {
      log_info("Could not connect to %s, which is listed as a slave for %s: %s",
               endpoint.c_str(), instance->descr().c_str(), e.format().c_str());
      real_slaves.push_back({ch, ""});
      continue;
    }

    if (ghost_slave) {
      log_info("Ignoring stale slave host %s for %s", endpoint.c_str(),
               instance->descr().c_str());
      continue;
    }

    real_slaves.push_back({ch, "'" + channel_name + "'"});
  }

  return real_slaves;
}

void validate_instance_is_standalone(Instance *target_server,
                                     bool add_op = false) {
  auto console = current_console();

  // Look for unexpected replication channels
  auto channels = mysqlshdk::mysql::get_incoming_channels(*target_server);
  auto slaves = get_valid_slaves(target_server);

  if (!channels.empty() || !slaves.empty()) {
    console->print_error("Extraneous replication channels found at " +
                         target_server->descr() + ":");
    for (const auto &ch : channels) {
      console->print_info(
          "- channel '" + ch.channel_name + "' from " +
          mysqlshdk::utils::make_host_and_port(ch.host, ch.port));
    }
    for (const auto &sl : slaves) {
      if (sl.second.empty())
        console->print_info(
            "- " +
            mysqlshdk::utils::make_host_and_port(sl.first.host, sl.first.port) +
            " replicates from this instance");
      else
        console->print_info(
            "- " +
            mysqlshdk::utils::make_host_and_port(sl.first.host, sl.first.port) +
            " replicates from this instance (channel " + sl.second + ")");
    }
    console->print_info();
    std::string info_msg =
        "Unmanaged replication channels are not supported in a replicaset. If "
        "you'd like to manage an existing MySQL replication topology with the "
        "Shell, use the <<<createReplicaSet>>>() operation with the "
        "adoptFromAR option.";
    if (add_op) {
      info_msg.append(
          " If the <<<addInstance>>>() operation previously failed for the "
          "target instance and you are trying to add it again, then after "
          "fixing the issue you should reset the current replication settings "
          "before retrying to execute the operation. To reset the replication "
          "settings on the target instance execute the following statements: "
          "'STOP SLAVE' and 'RESET SLAVE ALL'.");
    }
    console->print_para(info_msg);
    throw shcore::Exception("Unexpected replication channel",
                            SHERR_DBA_INVALID_SERVER_CONFIGURATION);
  }
}

void validate_adopted_topology(Global_topology_manager *topology,
                               const topology::Node **out_primary) {
  auto console = current_console();
  auto ipool = current_ipool();

  // check config of all instances
  console->print_info("* Checking configuration of discovered instances...");
  for (const auto *server : topology->topology()->nodes()) {
    Scoped_instance instance(ipool->connect_unchecked_endpoint(
        server->get_primary_member()->endpoint));

    validate_version(*instance);

    validate_instance(instance.get());
  }
  console->print_info();

  // check topology
  topology->validate_adopt_cluster(out_primary);
  console->print_info();

  console->print_info("Validations completed successfully.");
  console->print_info();
}

void create_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &donor_account, bool dry_run) {
  log_info("Creating clone recovery user %s@%% at %s%s.",
           donor_account.user.c_str(), target_instance->descr().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    try {
      auto console = mysqlsh::current_console();

      mysqlshdk::mysql::Suppress_binary_log nobinlog(target_instance);
      // Create recovery user for clone equal to the donor user
      mysqlshdk::mysql::create_user_with_password(
          *target_instance, donor_account.user, {"%"},
          {std::make_tuple("CLONE_ADMIN, EXECUTE", "*.*", false),
           std::make_tuple("SELECT", "performance_schema.*", false)},
          *donor_account.password);
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  }
}

void drop_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &account) {
  log_info("Dropping account %s@%% at %s", account.user.c_str(),
           target_instance->descr().c_str());

  try {
    mysqlshdk::mysql::Suppress_binary_log nobinlog(target_instance);
    target_instance->drop_user(account.user, "%", true);
  } catch (const mysqlshdk::db::Error &e) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%%.", target_instance->descr().c_str(),
        account.user.c_str()));
    // ignore the error and move on
  }
}

void revert_clone_recovery(mysqlshdk::mysql::IInstance *recipient,
                           const mysqlshdk::mysql::Auth_options &clone_user) {
  // NOTE: disable binlog to avoid messing up with the GTID
  drop_clone_recovery_user_nobinlog(recipient, clone_user);

  // Reset clone settings
  recipient->set_sysvar_default("clone_valid_donor_list");

  // Uninstall the clone plugin
  log_info("Uninstalling the clone plugin on recipient '%s'.",
           recipient->get_canonical_address().c_str());
  mysqlshdk::mysql::uninstall_clone_plugin(*recipient, nullptr);
}
}  // namespace

Replica_set_impl::Replica_set_impl(
    const std::string &cluster_name, const std::shared_ptr<Instance> &target,
    const std::shared_ptr<MetadataStorage> &metadata_storage,
    Global_topology_type topology_type)
    : Base_cluster_impl(cluster_name, target, metadata_storage),
      m_topology_type(topology_type) {
  set_description("Default ReplicaSet");
}

Replica_set_impl::Replica_set_impl(
    const Cluster_metadata &cm, const std::shared_ptr<Instance> &target,
    const std::shared_ptr<MetadataStorage> &metadata_storage)
    : Base_cluster_impl(cm.cluster_name, target, metadata_storage),
      m_topology_type(cm.async_topology_type) {
  m_id = cm.cluster_id;
  m_description = cm.description;
}

std::shared_ptr<Replica_set_impl> Replica_set_impl::create(
    const std::string &full_cluster_name, Global_topology_type topology_type,
    const std::shared_ptr<Instance> &target_server,
    const std::string &instance_label,
    const Async_replication_options & /*ar_options*/, bool adopt, bool dry_run,
    bool gtid_set_is_complete) {
  auto console = current_console();

  // Validate the cluster_name
  mysqlsh::dba::validate_cluster_name(full_cluster_name,
                                      Cluster_type::ASYNC_REPLICATION);

  std::string domain_name;
  std::string cluster_name;
  parse_fully_qualified_cluster_name(full_cluster_name, &domain_name, nullptr,
                                     &cluster_name);
  if (domain_name.empty()) domain_name = k_default_domain_name;

  if (adopt) {
    console->print_info("A new replicaset with the topology visible from '" +
                        target_server->descr() + "' will be created.\n");
  } else {
    console->print_info("A new replicaset with instance '" +
                        target_server->descr() + "' will be created.\n");
  }
  if (dry_run) {
    console->print_note(
        "dryRun option was specified. Validations will be executed, "
        "but no changes will be applied.");
  }

  std::shared_ptr<MetadataStorage> metadata;
  metadata.reset(new MetadataStorage(target_server));

  auto cluster = std::make_shared<Replica_set_impl>(cluster_name, target_server,
                                                    metadata, topology_type);

  if (adopt) {
    console->print_info("* Scanning replication topology...");
    std::unique_ptr<Star_global_topology_manager> topology(
        new Star_global_topology_manager(
            0, discover_unmanaged_topology(target_server.get())));
    console->print_info();
    cluster->adopt(topology.get(), instance_label, dry_run);
  } else {
    cluster->create(instance_label, dry_run);
  }

  if (!dry_run) {
    metadata->update_cluster_attribute(
        cluster->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        gtid_set_is_complete ? shcore::Value::True() : shcore::Value::False());
  }

  return cluster;
}

void Replica_set_impl::create(const std::string &instance_label, bool dry_run) {
  auto console = current_console();

  console->print_info("* Checking MySQL instance at " +
                      m_target_server->descr());

  validate_instance(m_target_server.get());
  validate_instance_is_standalone(m_target_server.get());
  console->print_info();

  log_info("Unfencing PRIMARY %s", m_target_server->descr().c_str());
  if (!dry_run) unfence_instance(m_target_server.get());

  // target is the primary
  m_primary_master = m_target_server;
  m_primary_master->retain();

  // create repl user to be used in the future
  create_replication_user(m_target_server.get(), k_async_cluster_user_name,
                          dry_run);

  console->print_info("* Updating metadata...");

  try {
    // First we need to create the Metadata Schema (if it doesn't already exist)
    prepare_metadata_schema(m_target_server, dry_run);

    // Update metadata
    log_info("Creating replicaset metadata...");
    if (!dry_run) m_metadata_storage->create_async_cluster_record(this, false);

    log_info("Recording metadata for %s", m_target_server->descr().c_str());
    if (!dry_run)
      manage_instance(m_target_server.get(), instance_label, 0, true);
    console->print_info();
  } catch (...) {
    console->print_error(
        "Failed to update the metadata. Please fix the issue and drop the "
        "metadata using dba.<<<dropMetadataSchema>>>() before retrying to "
        "execute the operation.");
    throw;
  }

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Replica_set_impl::adopt(Global_topology_manager *topology,
                             const std::string & /*instance_label*/,
                             bool dry_run) {
  auto console = current_console();

  console->print_info(
      "* Discovering async replication topology starting with " +
      m_target_server->descr());

  console->print_info("Discovered topology:");
  topology->topology()->show_raw();
  console->print_info();

  const topology::Node *primary = nullptr;
  validate_adopted_topology(topology, &primary);

  // Only allow adopt from the primary, to avoid surprises where we accidentally
  // adopt the wrong instance because of some secondary replication channel
  if (primary->get_primary_member()->uuid !=
      m_metadata_storage->get_md_server()->get_uuid()) {
    console->print_error(
        "Active connection must be to the PRIMARY when adopting an "
        "existing replication topology.");

    throw shcore::Exception("Target instance is not the PRIMARY",
                            SHERR_DBA_BADARG_INSTANCE_NOT_PRIMARY);
  }

  auto ipool = current_ipool();
  Scoped_instance primary_instance_scoped(ipool->connect_unchecked_endpoint(
      primary->get_primary_member()->endpoint));
  std::shared_ptr<Instance> primary_instance = primary_instance_scoped;

  log_info("Unfencing PRIMARY %s", primary_instance->descr().c_str());
  if (!dry_run) unfence_instance(primary_instance.get());

  // target is the primary
  m_primary_master = m_target_server;
  m_primary_master->retain();

  // Create rpl user to be used in the future (for primary).
  create_replication_user(m_target_server.get(), k_async_cluster_user_name,
                          dry_run);

  console->print_info("* Updating metadata...");

  try {
    // First we need to create the Metadata Schema
    prepare_metadata_schema(primary_instance, dry_run);

    // Update metadata
    log_info("Creating replicaset metadata...");
    if (!dry_run) m_metadata_storage->create_async_cluster_record(this, true);

    Instance_id primary_id = 0;

    if (!dry_run)
      primary_id = manage_instance(primary_instance.get(), "", 0, true);
    console->print_info();

    for (const auto *server : topology->topology()->nodes()) {
      if (server != primary) {
        Scoped_instance instance(ipool->connect_unchecked_endpoint(
            server->get_primary_member()->endpoint));

        // Create rpl user to be used in the future (for secondary).
        create_replication_user(instance.get(), k_async_cluster_user_name,
                                dry_run);

        log_info("Fencing SECONDARY %s", server->label.c_str());

        if (!dry_run) fence_instance(instance.get());

        log_info("Recording metadata for %s", server->label.c_str());
        if (!dry_run) manage_instance(instance.get(), "", primary_id, false);
      }
    }
  } catch (...) {
    console->print_error(
        "Failed to update the metadata. Please fix the issue and drop the "
        "metadata using dba.<<<dropMetadataSchema>>>() before retrying to "
        "execute the operation.");
    throw;
  }

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

std::shared_ptr<Instance> Replica_set_impl::validate_add_instance(
    Global_topology_manager *topology, const std::string &target_def,
    const Async_replication_options &ar_options, Clone_options *clone_options,
    bool interactive) {
  auto console = current_console();

  auto validate_not_managed = [this](const std::shared_ptr<Instance> &target) {
    // Check if the instance is already a member of this replicaset
    Instance_metadata md;
    bool already_managed = false;
    try {
      md = m_metadata_storage->get_instance_by_uuid(target->get_uuid());
      already_managed = true;
    } catch (const shcore::Exception &e) {
      log_info("Error querying metadata for %s: %s\n",
               target->get_uuid().c_str(), e.what());
    }
    bool is_invalidated = false;
    if (!already_managed) {
      // The instance is not a member of this replicaset, but it has
      // the metadata schema. Check if this isn't an invalidated member.
      MetadataStorage metadata(target);

      if (metadata.check_version()) {
        try {
          md = metadata.get_instance_by_uuid(target->get_uuid());

          // Check whether the other instance was once part of the same rs
          if (md.cluster_id == get_id()) is_invalidated = true;
        } catch (const std::exception &e) {
          log_info("Error querying local copy of metadata for %s: %s",
                   target->get_uuid().c_str(), e.what());
        }
      }
    }
    if (is_invalidated) {
      // If this is a removed invalidated member of the replicaset, we can
      // only add it back if the GTID set has not diverged.
      // The GTID check will happen later on, as part of the regular checks.
      log_info("%s appears to be an invalidated ex-member of the replicaset",
               target->descr().c_str());
    } else {
      // Check whether the instance we found is really the target one
      // or if it's just a case of duplicate UUID
      if (!md.cluster_id.empty() &&
          md.address != target->get_canonical_address()) {
        log_info("%s: UUID %s already in used by %s", target->descr().c_str(),
                 target->get_uuid().c_str(), md.address.c_str());
        throw shcore::Exception("server_uuid in " + target->descr() +
                                    " is expected to be unique, but " +
                                    md.address + " already uses the same value",
                                SHERR_DBA_INVALID_SERVER_CONFIGURATION);
      }

      if (md.cluster_id == get_id()) {
        throw shcore::Exception(
            target->descr() + " is already a member of this replicaset.",
            SHERR_DBA_BADARG_INSTANCE_ALREADY_MANAGED);

      } else if (!md.cluster_id.empty()) {
        std::string label = md.group_name.empty() ? "replicaset." : "cluster.";
        throw shcore::Exception(
            target->descr() + " is already managed by a " + label,
            SHERR_DBA_BADARG_INSTANCE_ALREADY_MANAGED);
      }
    }
  };

  auto validate_unique_server_id = [](Instance *target,
                                      Global_topology_manager *topology_mgr) {
    std::string server_uuid = target->get_uuid();
    uint32_t server_id = target->queryf_one_int(0, 0, "SELECT @@server_id");

    for (const auto &node : topology_mgr->topology()->nodes()) {
      for (const auto &m : node->members()) {
        // uniqueness of uuid is checked in validate_not_managed()
        if (m.server_id.get_safe() == server_id) {
          throw shcore::Exception("server_id in " + target->descr() +
                                      " is expected to be unique, but " +
                                      m.label + " already uses the same value",
                                  SHERR_DBA_INVALID_SERVER_CONFIGURATION);
        }
      }
    }
  };

  log_debug("Connecting to target instance: %s", target_def.c_str());
  Scoped_instance target;
  try {
    target = Scoped_instance(connect_target_instance(target_def));
  } catch (const shcore::Exception &e) {
    current_console()->print_error(shcore::str_format(
        k_error_connecting_to_instance,
        Connection_options(target_def).uri_endpoint().c_str()));
    throw;
  }

  // Validate the Clone options.
  clone_options->check_option_values(target->get_version());

  // Check version early on, so that other checks don't need to care about it
  validate_version(*target);

  // Check if the target is already managed by us or some other cluster
  validate_not_managed(target);

  // regular instance checks
  validate_instance(target.get());
  validate_instance_is_standalone(target.get(), true);

  validate_unique_server_id(target.get(), topology);

  // check consistency of the global topology
  console->print_info();
  topology->validate_add_replica(nullptr, target.get(), ar_options);

  std::shared_ptr<Instance> donor_instance = get_target_server();

  console->print_info();
  console->print_info("* Checking transaction state of the instance...");

  clone_options->recovery_method = validate_instance_recovery(
      Member_op_action::ADD_INSTANCE, donor_instance.get(), target.get(),
      *clone_options->recovery_method, get_gtid_set_is_complete(), interactive);

  DBUG_EXECUTE_IF("dba_abort_async_add_replica",
                  { throw std::logic_error("debug"); });

  target->retain();
  return target;
}

void Replica_set_impl::add_instance(
    const std::string &instance_def,
    const Async_replication_options &ar_options_,
    const Clone_options &clone_options_, const mysqlshdk::null_string &label,
    Recovery_progress_style progress_style, int sync_timeout, bool interactive,
    bool dry_run) {
  check_preconditions("addInstance");

  Async_replication_options ar_options(ar_options_);
  Clone_options clone_options(clone_options_);

  // Testing hack to set a replication delay. This should be removed
  // if/when a replication delay option is ever added.
  DBUG_EXECUTE_IF("dba_add_instance_master_delay",
                  { ar_options.master_delay = 3; });

  auto console = current_console();

  auto active_master = acquire_primary();
  auto finally = shcore::on_leave_scope([this]() { release_primary(); });

  auto topology = setup_topology_manager();

  console->print_info("Adding instance to the replicaset...");
  console->println();

  console->print_info("* Performing validation checks");
  Scoped_instance target_instance(validate_add_instance(
      topology.get(), instance_def, ar_options, &clone_options, interactive));

  console->print_info("* Updating topology");

  // Create the recovery account
  ar_options.repl_credentials = create_replication_user(
      target_instance.get(), k_async_cluster_user_name, dry_run);

  try {
    if (*clone_options.recovery_method == Member_recovery_method::CLONE) {
      // Do and monitor the clone
      handle_clone(target_instance, clone_options, ar_options, progress_style,
                   sync_timeout, dry_run);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      refresh_target_connections(target_instance.get());
    }
    // Update the global topology first, which means setting replication
    // channel to the primary in the master replica, so we can know if that
    // works. Async replication between DCs has many things that can go wrong
    // (bad address, private vs public address, network issue, account issues,
    // firewalls etc), so we do this first to keep an eventual rollback to a
    // minimum while retries would also be simpler to handle.

    async_add_replica(active_master, target_instance.get(), ar_options,
                      dry_run);

    console->print_info(
        "** Waiting for new instance to synchronize with PRIMARY...");
    if (!dry_run) {
      try {
        // Sync and check whether the slave started OK
        sync_transactions(*target_instance, k_async_cluster_channel_name,
                          sync_timeout);
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
          console->print_info(
              "You may increase or disable the transaction sync timeout with "
              "the timeout option for <<<addInstance>>>()");
        }
        throw;
      } catch (const cancel_sync &) {
        // Throw it up
        throw;
      }
    }
    console->print_info();

    Instance_id master_id = static_cast<const topology::Server *>(
                                topology->topology()->get_primary_master_node())
                                ->instance_id;

    log_info("Recording metadata for %s", target_instance->descr().c_str());
    if (!dry_run)
      manage_instance(target_instance.get(), *label, master_id, false);
  } catch (const cancel_sync &) {
    if (!dry_run) {
      revert_topology_changes(target_instance.get(), true, false);
    }

    console->print_info();
    console->print_info("Changes successfully reverted.");

    return;
  } catch (const std::exception &e) {
    console->print_error("Error adding instance to replicaset: " +
                         format_active_exception());
    log_warning("While adding async instance: %s", e.what());

    if (!dry_run) {
      revert_topology_changes(target_instance.get(), true, false);
    }

    console->print_info();
    console->print_info("Changes successfully reverted.");

    console->print_error(target_instance->descr() +
                         " could not be added to the replicaset");

    throw;
  }

  console->print_info(shcore::str_format(
      "The instance '%s' was added to the replicaset and is replicating "
      "from %s.\n",
      target_instance->descr().c_str(),
      active_master->get_canonical_address().c_str()));

  // Wait for the new replica to catch up metadata state
  // Errors after this point don't rollback.
  if (target_instance && !dry_run && sync_timeout >= 0) {
    sync_transactions(*target_instance, k_async_cluster_channel_name,
                      sync_timeout);
  }

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

std::shared_ptr<Instance> Replica_set_impl::validate_rejoin_instance(
    Global_topology_manager *topology_mng, const std::string &target_def,
    Clone_options *clone_options, Instance_metadata *out_instance_md,
    bool interactive) {
  auto console = current_console();

  log_debug("Connecting to target instance: %s", target_def.c_str());
  Scoped_instance target;
  try {
    target = Scoped_instance(connect_target_instance(target_def));
  } catch (const shcore::Exception &e) {
    current_console()->print_error(shcore::str_format(
        k_error_connecting_to_instance,
        Connection_options(target_def).uri_endpoint().c_str()));
    throw;
  }

  // Validate the Clone options.
  clone_options->check_option_values(target->get_version());

  // Check if the target instance belongs to the replicaset (MD).
  std::string target_address = target->get_canonical_address();
  try {
    *out_instance_md =
        m_metadata_storage->get_instance_by_address(target_address);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      console->print_error(
          "Cannot rejoin an instance that does not belong to the replicaset. "
          "Please confirm the specified address or execute the operation "
          "against the correct ReplicaSet object.");
      throw shcore::Exception(
          "Instance " + target_address + " does not belong to the replicaset",
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }
    throw;
  }

  // Check instance status and consistency with the global topology
  topology_mng->validate_rejoin_replica(target.get());

  std::shared_ptr<Instance> donor_instance = get_target_server();

  console->print_info("** Checking transaction state of the instance...");

  clone_options->recovery_method = validate_instance_recovery(
      Member_op_action::REJOIN_INSTANCE, donor_instance.get(), target.get(),
      *clone_options->recovery_method, get_gtid_set_is_complete(), interactive);

  target->retain();
  return target.ptr;
}

void Replica_set_impl::rejoin_instance(const std::string &instance_def,
                                       const Clone_options &clone_options_,
                                       Recovery_progress_style progress_style,
                                       int sync_timeout, bool interactive,
                                       bool dry_run) {
  log_debug("Checking rejoin instance preconditions.");
  check_preconditions("rejoinInstance");

  Clone_options clone_options(clone_options_);

  auto console = current_console();

  log_debug("Setting up topology manager.");
  auto topology_mng = setup_topology_manager(nullptr, true);

  log_debug("Get current PRIMARY and ensure it is healthy (updatable).");
  auto active_primary = acquire_primary();
  auto finally = shcore::on_leave_scope([this]() { release_primary(); });

  console->print_info("* Validating instance...");
  Instance_metadata instance_md;

  Scoped_instance target_instance(
      validate_rejoin_instance(topology_mng.get(), instance_def, &clone_options,
                               &instance_md, interactive));

  // Update target instance replication settings and restart replication.
  console->print_info("* Rejoining instance to replicaset...");

  DBUG_EXECUTE_IF("dba_abort_async_rejoin_replica",
                  { throw std::logic_error("debug"); });

  // NOTE: Replication user needs to be refreshed in case we are rejoining the
  // old PRIMARY from the replicaset.
  Async_replication_options ar_options;
  ar_options.repl_credentials = refresh_replication_user(
      target_instance.get(), k_async_cluster_user_name, dry_run);

  try {
    if (*clone_options.recovery_method == Member_recovery_method::CLONE) {
      // Do and monitor the clone
      handle_clone(target_instance, clone_options, ar_options, progress_style,
                   sync_timeout, dry_run);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      refresh_target_connections(target_instance.get());
    }

    if (!dry_run) {
      async_rejoin_replica(active_primary, target_instance.get(), ar_options);

      console->print_info(
          "** Waiting for rejoined instance to synchronize with PRIMARY...");

      try {
        // Sync and check whether the slave started OK
        sync_transactions(*target_instance, k_async_cluster_channel_name,
                          sync_timeout);
      } catch (const cancel_sync &) {
        log_info("Operating canceled during transactions sync at %s.",
                 target_instance->descr().c_str());
        // Throw it up
        throw;
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
          console->print_info(
              "You may increase or disable the transaction sync timeout with "
              "the timeout option for <<<rejoinInstance>>>()");
        }
        throw e;
      }
    }

    console->print_info();

    console->print_info("* Updating the Metadata...");
    // Set new instance information and store in MD.
    if (!dry_run) {
      instance_md.master_id =
          static_cast<const topology::Server *>(
              topology_mng->topology()->get_primary_master_node())
              ->instance_id;
      instance_md.primary_master = false;
      m_metadata_storage->record_async_member_rejoined(instance_md);
    }
  } catch (const cancel_sync &) {
    stop_channel(target_instance.get());

    console->print_info();
    console->print_info("Changes successfully reverted.");

    return;
  } catch (const std::exception &e) {
    console->print_error("Error rejoining instance to replicaset: " +
                         format_active_exception());
    log_warning("While rejoining async instance: %s", e.what());

    stop_channel(target_instance.get());

    console->print_error(target_instance->descr() +
                         " could not be rejoined to the replicaset");

    throw;
  }

  console->print_info(shcore::str_format(
      "The instance '%s' rejoined the replicaset and is replicating "
      "from %s.\n",
      target_instance->descr().c_str(),
      active_primary->get_canonical_address().c_str()));

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Replica_set_impl::validate_remove_instance(
    Global_topology_manager *topology, mysqlshdk::mysql::IInstance *master,
    const std::string &target_address, Instance *target, bool force,
    Instance_metadata *out_instance_md, bool *out_repl_working) {
  auto console = current_console();

  // check if belongs to the replicaset
  try {
    *out_instance_md =
        m_metadata_storage->get_instance_by_address(target_address);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      console->print_error(
          "Instance " + target_address +
          " cannot be removed because it does not belong to the replicaset "
          "(not found in the metadata). If you really want to remove this "
          "instance because it is still using replication then it must be "
          "stopped manually.");
      throw shcore::Exception(
          target_address + " does not belong to the replicaset",
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }
    throw;
  }

  if (out_instance_md->primary_master) {
    console->print_error(target_address +
                         " is a PRIMARY and cannot be removed.");
    throw shcore::Exception(
        "PRIMARY instance cannot be removed from the replicaset.",
        SHERR_DBA_BADARG_INSTANCE_REMOVE_NOT_ALLOWED);
  }

  if (out_instance_md->cluster_id != get_id())
    throw shcore::Exception(
        target_address + " does not belong to the replicaset",
        SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);

  // check consistency of the global topology
  if (target) {
    topology->validate_remove_replica(master, target, force, out_repl_working);
  }
}

void Replica_set_impl::remove_instance(const std::string &instance_def_,
                                       const mysqlshdk::null_bool &force,
                                       int timeout) {
  log_debug("Checking remove instance preconditions.");
  check_preconditions("removeInstance");

  auto console = current_console();
  auto ipool = current_ipool();

  // Normalize what's given from the argument, to strip out username, pwd
  // and other garbage
  std::string instance_def = Connection_options(instance_def_).uri_endpoint();

  log_debug("Setting up topology manager.");
  auto topology = setup_topology_manager();

  log_debug("Connecting to target instance.");
  mysqlshdk::mysql::IInstance *master_server = acquire_primary();
  auto finally = shcore::on_leave_scope([this]() { release_primary(); });

  Scoped_instance target_server;
  try {
    target_server = Scoped_instance(connect_target_instance(instance_def_));
  } catch (const shcore::Exception &e) {
    // Check if instance belongs to the replicaset (to send a more user-friendly
    // message to users)
    bool belong_to_md = true;
    std::string target_address =
        target_server ? target_server->get_canonical_address() : instance_def;
    try {
      m_metadata_storage->get_instance_by_address(target_address);
    } catch (const shcore::Exception &err) {
      if (err.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
        belong_to_md = false;
      }
      // Ignore any error here: when checking if belongs to metadata.
    }

    if (force.is_null() || *force == false) {
      console->print_error(
          "Unable to connect to the target instance " + target_address +
          ". Please make sure the instance is available and try again. If the "
          "instance is permanently not reachable, use the 'force' option to "
          "remove it from the replicaset metadata and skip reconfiguration of "
          "that instance.");
      throw;
    } else {
      if (belong_to_md) {
        console->print_note(
            "Unable to connect to the target instance " + target_address +
            ". The instance will only be removed from the metadata, but its "
            "replication configuration cannot be updated. Please, take any "
            "necessary actions to make sure that the instance will not "
            "replicate from the replicaset if brought back online.");
      } else {
        console->print_error(
            "Instance " + target_address +
            " is unreachable and was not found in the replicaset metadata. "
            "The exact address of the instance as recorded in the metadata "
            "must be used in cases where the target is unreachable.");
        throw shcore::Exception(
            target_address + " does not belong to the replicaset",
            SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
      }
    }
  }

  Instance_metadata md;
  bool repl_working = false;

  validate_remove_instance(
      topology.get(), master_server,
      target_server.get() ? target_server->get_canonical_address()
                          : instance_def,
      target_server.get(), force.get_safe(), &md, &repl_working);

  if (md.invalidated) {
    console->print_note(md.label +
                        " is invalidated, replication sync will be skipped.");
    timeout = -1;
  }

  // sync transactions before making changes (if not invalidated)
  if (target_server && repl_working && timeout >= 0) {
    try {
      sync_transactions(*target_server, k_async_cluster_channel_name, timeout);
    } catch (const shcore::Exception &e) {
      if (force.get_safe()) {
        console->print_warning(
            "Transaction sync failed but ignored because of 'force' option: " +
            e.format());
      } else {
        console->print_error(
            "Transaction sync failed. Use the 'force' option to remove "
            "anyway.");
        throw;
      }
    }
  }

  // update metadata first
  log_debug("Removing instance from the Metadata.");
  m_metadata_storage->record_async_member_removed(md.cluster_id, md.id);

  // drop user - this will ignore DB errors
  if (target_server.get())
    drop_replication_user(target_server.get(), k_async_cluster_user_name);

  if (target_server.get()) {
    if (repl_working && timeout >= 0) {
      // If replication is working, sync once again so that the drop user and
      // metadata update are caught up with
      try {
        sync_transactions(*target_server, k_async_cluster_channel_name,
                          timeout);
      } catch (const shcore::Exception &e) {
        if (force.get_safe()) {
          console->print_warning(
              "Transaction sync failed but ignored because of 'force' "
              "option: " +
              e.format());
        } else {
          console->print_error(
              "Transaction sync failed. Use the 'force' option to remove "
              "anyway.");
          throw;
        }
      }
    }

    // actual stop slave happens last, so that other changes can get propagated
    // to the instance being removed 1st
    log_debug("Updating replication settings on the target instance.");
    try {
      async_remove_replica(target_server.get(), false);
    } catch (const shcore::Exception &e) {
      console->print_error("Error updating replication settings: " +
                           e.format());

      console->print_info(
          shcore::str_format("Metadata for instance '%s' was deleted, but "
                             "replication clean up failed with an error.\n",
                             target_server->descr().c_str()));

      log_warning("While removing async instance: %s", e.what());
      throw;
    }

    console->print_info(shcore::str_format(
        "The instance '%s' was removed from the replicaset.\n",
        md.label.c_str()));
  } else {
    console->print_info(shcore::str_format(
        "Metadata for instance '%s' was deleted, but instance "
        "configuration could not be updated.\n",
        instance_def.c_str()));
  }

  // If target server was the removed one, invalidate
  if (m_target_server->get_uuid() == md.uuid) {
    target_server_invalidated();
  }
}

void Replica_set_impl::set_primary_instance(const std::string &instance_def,
                                            uint32_t timeout, bool dry_run) {
  auto console = current_console();
  auto ipool = current_ipool();

  check_preconditions("setPrimaryInstance");

  acquire_primary();
  auto finally = shcore::on_leave_scope([this]() { release_primary(); });

  topology::Server_global_topology *srv_topology = nullptr;
  auto topology = setup_topology_manager(&srv_topology);
  // topology->set_sync_timeout(timeout);

  const topology::Server *promoted =
      check_target_member(srv_topology, instance_def);

  const topology::Server *demoted = dynamic_cast<const topology::Server *>(
      srv_topology->get_primary_master_node());

  if (promoted == demoted) {
    console->print_info("Target instance " + promoted->label +
                        " is already the PRIMARY.");
    return;
  }

  validate_node_status(promoted);

  console->print_info(shcore::str_format(
      "%s will be promoted to PRIMARY of '%s'.\nThe current PRIMARY is %s.\n",
      promoted->label.c_str(), get_name().c_str(), demoted->label.c_str()));

  console->print_info("* Connecting to replicaset instances");
  std::list<Instance_metadata> unreachable;
  std::list<Scoped_instance> instances(
      connect_all_members(0, false, &unreachable));
  if (!unreachable.empty()) {
    throw shcore::Exception("One or more instances are unreachable",
                            SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);
  }

  // another set of connections for locks
  // Note: give extra margin for the connection read timeout, so that it doesn't
  // get triggered before server-side timeouts
  std::list<Scoped_instance> lock_instances(
      connect_all_members(timeout + 5, false, &unreachable));
  if (!unreachable.empty()) {
    throw shcore::Exception("One or more instances are unreachable",
                            SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);
  }

  Scoped_instance master;
  Scoped_instance new_master;
  {
    auto it = std::find_if(instances.begin(), instances.end(),
                           [promoted](const Scoped_instance &i) {
                             return i->get_uuid() ==
                                    promoted->get_primary_member()->uuid;
                           });
    if (it == instances.end())
      throw shcore::Exception::runtime_error(promoted->label +
                                             " cannot be promoted");

    new_master = *it;

    it = std::find_if(instances.begin(), instances.end(),
                      [demoted](const Scoped_instance &i) {
                        return i->get_uuid() ==
                               demoted->get_primary_member()->uuid;
                      });
    if (it == instances.end())
      throw std::logic_error("Internal error: couldn't find primary");
    master = *it;
  }
  console->print_info();

  console->print_info("* Performing validation checks");
  topology->validate_switch_primary(master.get(), new_master.get(), instances);
  console->print_info();

  // Pre-synchronize the promoted primary before making any changes
  // This should ensure that there's nothing left that could cause long delays
  // or timeouts (locks, replication lag etc). Once the metadata is updated,
  // the router will begin sending RW traffic to the new primary. We won't lose
  // consistency because they should fail with SRO errors, but we should try
  // to minimize the amount of time spent in that state.
  console->print_info("* Synchronizing transaction backlog at " +
                      new_master->descr());
  if (!dry_run)
    sync_transactions(*new_master, k_async_cluster_channel_name, timeout);
  console->print_info();

  console->print_info("* Updating metadata");
  // Re-generate a new password for the master being demoted.
  Async_replication_options ar_options;
  ar_options.repl_credentials = refresh_replication_user(
      master.get(), k_async_cluster_user_name, dry_run);

  // Update the metadata with the state the replicaset is supposed to be in
  log_info("Updating metadata at %s",
           m_metadata_storage->get_md_server()->descr().c_str());
  if (!dry_run)
    m_metadata_storage->record_async_primary_switch(promoted->instance_id);
  console->print_info();

  // Synchronize all slaves and lock all instances.
  Global_locks global_locks;
  try {
    global_locks.acquire(lock_instances, demoted->get_primary_member()->uuid,
                         timeout, dry_run);
  } catch (const std::exception &e) {
    console->print_error(shcore::str_format(
        "An error occurred while preparing replicaset instances for a PRIMARY "
        "switch: %s",
        e.what()));

    console->print_note("Reverting metadata changes");
    if (!dry_run) {
      try {
        m_metadata_storage->record_async_primary_switch(demoted->instance_id);
      } catch (const std::exception &err) {
        console->print_warning(shcore::str_format(
            "Failed to revert metadata changes on the PRIMARY: %s",
            err.what()));
      }
    }
    throw;
  }

  console->print_info("* Updating replication topology");
  // Update the topology but revert if it fails
  try {
    do_set_primary_instance(master.get(), new_master.get(), instances,
                            ar_options, dry_run);
  } catch (...) {
    console->print_note("Reverting metadata changes");
    if (!dry_run)
      m_metadata_storage->record_async_primary_switch(demoted->instance_id);
    throw;
  }
  console->print_info();

  // This will update the MD object to use the new primary
  if (!dry_run) {
    primary_instance_did_change(new_master.ptr);
    new_master.ptr->steal();
  }

  console->print_info(new_master->get_canonical_address() +
                      " was promoted to PRIMARY.");
  console->print_info();

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Replica_set_impl::do_set_primary_instance(
    Instance *master, Instance *new_master,
    const std::list<Scoped_instance> &instances,
    const Async_replication_options &ar_options, bool dry_run) {
  auto console = current_console();
  shcore::Scoped_callback_list undo_list;

  try {
    // First, promote the new primary by stopping slave, unfencing it and
    // making the old primary a slave of the new one.
    // Topology changes are reverted on exception.
    async_set_primary(master, new_master, ar_options, &undo_list, dry_run);

    // NOTE: Skip old master, already setup previously by async_set_primary().
    async_change_primary(new_master, instances, ar_options, master, &undo_list,
                         dry_run);
  } catch (...) {
    console->print_error("Error changing replication source: " +
                         format_active_exception());

    console->print_note("Reverting replication changes");
    undo_list.call();

    throw shcore::Exception("Error during switchover",
                            SHERR_DBA_SWITCHOVER_ERROR);
  }

  undo_list.cancel();

  // Clear replication configs from the promoted instance. Do it after
  // everything is done, to make reverting easier.
  try {
    reset_channel(new_master, true, dry_run);
  } catch (...) {
    // Failure to reset slave is not fatal
    console->print_warning("Error resetting replication configurations at " +
                           new_master->descr());
  }
}

void Replica_set_impl::force_primary_instance(const std::string &instance_def,
                                              uint32_t timeout,
                                              bool invalidate_error_instances,
                                              bool dry_run) {
  Async_replication_options ar_options;

  check_preconditions("forcePrimaryInstance");

  auto console = current_console();
  auto ipool = current_ipool();

  topology::Server_global_topology *srv_topology = nullptr;
  auto topology = setup_topology_manager(&srv_topology);

  const topology::Server *demoted = dynamic_cast<const topology::Server *>(
      srv_topology->get_primary_master_node());

  const topology::Server *promoted = nullptr;

  // Check if the specified target instance is available.
  if (!instance_def.empty()) {
    promoted = check_target_member(srv_topology, instance_def);

    if (promoted == demoted)
      throw shcore::Exception(promoted->label + " is already the PRIMARY",
                              SHERR_DBA_BAD_ASYNC_PRIMARY_CANDIDATE);
  }

  std::vector<Instance_metadata> instances_md =
      get_metadata_storage()->get_all_instances(get_id());
  std::list<Scoped_instance> instances;
  std::list<Instance_id> invalidate_ids;

  {
    std::list<Instance_metadata> unreachable;

    console->print_info("* Connecting to replicaset instances");
    // give extra margin for the connection read timeout, so that it doesn't
    // get triggered before server-side timeouts
    instances = connect_all_members(timeout + 5, true, &unreachable);

    if (!unreachable.empty()) {
      if (!invalidate_error_instances) {
        console->print_error(
            "Could not connect to one or more SECONDARY instances. Use the "
            "'invalidateErrorInstances' option to perform the failover anyway "
            "by skipping and invalidating unreachable instances.");
        throw shcore::Exception("One or more instances are unreachable",
                                SHERR_DBA_UNREACHABLE_INSTANCES);
      }
    }

    for (const auto &i : unreachable) {
      console->print_note(
          i.label +
          " will be invalidated and must be removed from the replicaset.");

      invalidate_ids.push_back(i.id);

      // Remove invalidated instance from the instance metadata list.
      instances_md.erase(
          std::remove_if(instances_md.begin(), instances_md.end(),
                         [&i](const Instance_metadata &i_md) {
                           return i_md.uuid == i.uuid;
                         }),
          instances_md.end());
    }
    console->print_info();
  }

  // Check for replication applier errors on all (online) instances, in order
  // to anticipate issues when applying retrieved transaction and try to
  // minimize the consequences of BUG#30148247.
  // NOTE: Instances with replication errors will be invalidated and skipped if
  //       invalidate_error_instances = true.
  check_replication_applier_errors(srv_topology, &instances,
                                   invalidate_error_instances, &instances_md,
                                   &invalidate_ids);

  // Wait for all instances to apply retrieved transactions (relay log) first.
  // NOTE: Otherwise GTID_EXECUTED set might be missing trx when checking most
  //       up-to-date instances.
  wait_all_apply_retrieved_trx(&instances, timeout, invalidate_error_instances,
                               &instances_md, &invalidate_ids);

  // Find a candidate to be promoted.
  // NOTE: Use updated (current) GTID_EXECUTED set from instance and not the
  //       "cached" value in the srv_topology.
  if (instance_def.empty()) {
    console->print_info(
        "* Searching instance with the most up-to-date transaction set");
    promoted = srv_topology->find_failover_candidate(instances);
  }

  if (!promoted) {
    assert(instance_def.empty());

    throw shcore::Exception(
        "Could not find a suitable candidate to failover to",
        SHERR_DBA_NO_ASYNC_PRIMARY_CANDIDATES);
  } else {
    console->print_info(
        promoted->label +
        " will be promoted to PRIMARY of the replicaset and the "
        "former PRIMARY will be invalidated.");
    console->print_info();
  }

  if (promoted->invalidated)
    throw shcore::Exception(promoted->label + " was invalidated by a failover",
                            SHERR_DBA_ASYNC_MEMBER_INVALIDATED);

  Scoped_instance new_master;
  {
    auto it = std::find_if(instances.begin(), instances.end(),
                           [promoted](const Scoped_instance &i) {
                             return i->get_uuid() ==
                                    promoted->get_primary_member()->uuid;
                           });
    // either an invalid target or the target is unavailable
    if (it == instances.end())
      throw shcore::Exception::argument_error(promoted->label +
                                              " cannot be promoted");
    new_master = *it;
  }

  // Validate instance to be promoted.
  topology->validate_force_primary(new_master.get(), instances);

  if (invalidate_ids.size() > 0) {
    console->print_note(shcore::str_format(
        "%zi instances will be skipped and invalidated during the failover",
        invalidate_ids.size()));
    console->println();
  }

  try {
    console->print_info("* Promoting " + new_master->descr() +
                        " to a PRIMARY...");
    async_force_primary(new_master.get(), ar_options, dry_run);
    console->print_info();

    // MD update has to happen after the failover, since there's no PRIMARY
    // before that
    console->print_info("* Updating metadata...");
    if (!dry_run) {
      new_master->steal();

      primary_instance_did_change(new_master);

      try {
        m_metadata_storage->record_async_primary_forced_switch(
            promoted->instance_id, invalidate_ids);
      } catch (const shcore::Exception &e) {
        // CR_SERVER_LOST will happen if the connection timeouts while waiting
        // for the INSERT/UPDATE to execute, which could happen if there's
        // a lock, for example.
        console->print_error("Could not update metadata: " + e.format());
        if (e.code() == CR_SERVER_LOST) {
          console->print_info(
              "Metadata update may have failed because of a timeout.");
        }
        // Restart replication on the promoted instance and re-enable read-only,
        // i.e., revert all previous changes from async_force_primary().
        Scoped_instance target(ipool->connect_unchecked(promoted));
        undo_async_force_primary(target.get(), dry_run);
        // fence_instance(target.get());

        invalidate_handle();

        throw shcore::Exception("Error during failover: " + e.format(),
                                SHERR_DBA_FAILOVER_ERROR);
      }
    }
    console->print_info();

    console->print_info(promoted->label + " was force-promoted to PRIMARY.");
    console->print_note(
        "Former PRIMARY " + demoted->label +
        " is now invalidated and must be removed from the replicaset.");
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_FAILOVER_ERROR) throw;

    log_warning("While forcing primary instance: %s", e.what());
    throw;
  } catch (const std::exception &e) {
    log_warning("While forcing primary instance: %s", e.what());
    throw;
  }

  console->print_info("* Updating source of remaining SECONDARY instances");
  shcore::Scoped_callback_list undo_list;
  try {
    async_change_primary(new_master.get(), instances, ar_options, nullptr,
                         &undo_list, dry_run);
  } catch (...) {
    console->print_error("Error changing replication source: " +
                         format_active_exception());

    console->print_note("Reverting replication changes");
    undo_list.call();

    throw shcore::Exception("Error during switchover",
                            SHERR_DBA_SWITCHOVER_ERROR);
  }
  undo_list.cancel();

  console->print_info();

  // Clear replication configs from the promoted instance. Do it after
  // everything is done, to make reverting easier.
  reset_channel(new_master.get(), true, dry_run);

  console->print_info("Failover finished successfully.");
  console->print_info();

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

shcore::Value Replica_set_impl::status(int extended) {
  check_preconditions("status");

  // Status checks should go through the primary if possible
  try {
    acquire_primary();
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE)
      current_console()->print_warning(e.format());
    else
      throw;
  }
  auto finally = shcore::on_leave_scope([this]() { release_primary(); });

  Cluster_metadata cmd = get_metadata();

  std::unique_ptr<topology::Global_topology> topo(
      topology::scan_global_topology(get_metadata_storage().get(), cmd,
                                     k_async_cluster_channel_name, true));

  Status_options opts;
  opts.show_members = true;
  opts.show_details = extended;

  shcore::Dictionary_t status = replica_set_status(
      *dynamic_cast<topology::Server_global_topology *>(topo.get()), opts);

  status->get_map("replicaSet")->set("name", shcore::Value(get_name()));

  // Gets the metadata version
  if (opts.show_details >= 1) {
    auto version = mysqlsh::dba::metadata::installed_version(
        m_metadata_storage->get_md_server());
    status->set("metadataVersion", shcore::Value(version.get_base()));
  }

  return shcore::Value(status);
}

Cluster_check_info Replica_set_impl::check_preconditions(
    const std::string &function_name) const {
  return check_function_preconditions("ReplicaSet." + function_name,
                                      get_target_server());
}

std::shared_ptr<Global_topology_manager>
Replica_set_impl::setup_topology_manager(
    topology::Server_global_topology **out_topology, bool deep) {
  Cluster_metadata cmd;
  if (!get_metadata_storage()->get_cluster(get_id(), &cmd))
    throw shcore::Exception("Metadata not found for replicaset " + get_name(),
                            SHERR_DBA_METADATA_MISSING);

  std::unique_ptr<topology::Global_topology> topology(
      topology::scan_global_topology(get_metadata_storage().get(), cmd,
                                     k_async_cluster_channel_name, deep));

  auto gtm =
      std::make_shared<Star_global_topology_manager>(0, std::move(topology));

  if (out_topology)
    *out_topology =
        dynamic_cast<topology::Server_global_topology *>(gtm->topology());

  return gtm;
}

Cluster_metadata Replica_set_impl::get_metadata() const {
  Cluster_metadata cmd;
  if (!get_metadata_storage()->get_cluster(get_id(), &cmd)) {
    throw shcore::Exception(
        "Cluster metadata could not be loaded for " + get_name(),
        SHERR_DBA_METADATA_MISSING);
  }
  return cmd;
}

std::vector<Instance_metadata> Replica_set_impl::get_instances_from_metadata()
    const {
  return get_metadata_storage()->get_replica_set_instances(get_id());
}

/*
 * Acquire the primary.
 *
 * Ensures the replicaset object can perform update operations.
 *
 * For a replicaset to be updatable, it's necessary that:
 * - the MD object is connected to the PRIMARY of the primary master of the
 * primary replicaset, so that the MD can be updated (and is also not lagged)
 * - the primary master of the replicaset is reachable, so that replicaset
 * accounts can be created there.
 *
 * An exception is thrown if not possible to connect to the primary master.
 *
 * An Instance object connected to the primary master is returned. The session
 * is owned by the replicaset object.
 */
mysqlshdk::mysql::IInstance *Replica_set_impl::acquire_primary() {
  auto console = current_console();

  auto check_not_invalidated = [&](Instance *primary,
                                   Instance_metadata *out_new_primary) {
    uint64_t view_id;
    auto members(
        m_metadata_storage->get_replica_set_members(get_id(), &view_id));
    assert(!members.empty());

    auto ipool = current_ipool();

    // Check if some other member has a higher view_id
    for (const auto &m : members) {
      try {
        Scoped_instance i(ipool->connect_unchecked_uuid(m.uuid));
        MetadataStorage md(*i);
        uint64_t alt_view_id = 0;
        std::vector<Instance_metadata> alt_members =
            md.get_replica_set_members(get_id(), &alt_view_id);
        if (alt_view_id > view_id) {
          log_info("view_id at %s is %s, target %s is %s", m.endpoint.c_str(),
                   std::to_string(alt_view_id).c_str(),
                   primary->descr().c_str(), std::to_string(view_id).c_str());

          for (const auto &am : alt_members) {
            if (am.primary_master) {
              log_info("Primary according to target %s is %s", m.label.c_str(),
                       am.label.c_str());
              *out_new_primary = am;
              break;
            }
          }
          return false;
        }
      } catch (const std::exception &e) {
        log_warning("Could not connect to member %s: %s", m.endpoint.c_str(),
                    e.what());
      }
    }
    return true;
  };

  // Always search for the primary (no cache, since it might have changed).
  auto ipool = current_ipool();
  try {
    std::shared_ptr<Instance> primary(
        ipool->connect_async_cluster_primary(get_id()));

    primary->steal();
    m_primary_master = primary;
    m_metadata_storage = std::make_shared<MetadataStorage>(m_primary_master);
    ipool->set_metadata(m_metadata_storage);

    log_info("Connected to replicaset PRIMARY instance %s",
             m_primary_master->descr().c_str());
  } catch (const shcore::Exception &e) {
    console->print_error("Unable to connect to the PRIMARY of the replicaset " +
                         get_name() + ": " + e.what());

    if (e.code() == SHERR_DBA_ASYNC_MEMBER_INVALIDATED) {
      console->print_error(get_name() +
                           " was invalidated by a failover. Please "
                           "repair it or remove it from the replicaset.");
    } else {
      console->print_info(
          "Cluster change operations will not be possible unless the PRIMARY "
          "can be reached.");
      console->print_info(
          "If the PRIMARY is unavailable, you must either repair it or "
          "perform a forced failover.");
      console->print_info(
          "See \\help <<<forcePrimaryInstance>>> for more information.");

      throw shcore::Exception("PRIMARY instance is unavailable",
                              SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE);
    }
    throw;
  }

  // ensure that the primary isn't invalidated
  Instance_metadata new_primary;
  if (!check_not_invalidated(m_primary_master.get(), &new_primary)) {
    // Throw an exception so that the error can bubble up all the way up to the
    // top of the stack. All assumptions made by the code path up to this point
    // were based on an invalidated members view of the replicaset, which
    // is no good.
    shcore::Exception exc(
        "Target " + m_target_server->descr() + " was invalidated in a failover",
        SHERR_DBA_ASYNC_MEMBER_INVALIDATED);

    exc.error()->set("new_primary_endpoint",
                     shcore::Value(new_primary.endpoint));
    throw exc;
  }

  return m_primary_master.get();
}

void Replica_set_impl::release_primary() { m_primary_master.reset(); }

void Replica_set_impl::primary_instance_did_change(
    const std::shared_ptr<Instance> &new_primary) {
  if (m_primary_master) m_primary_master->release();
  m_primary_master.reset();

  if (new_primary) {
    m_primary_master = new_primary;
    new_primary->retain();

    m_metadata_storage = std::make_shared<MetadataStorage>(new_primary);
  }
}

void Replica_set_impl::invalidate_handle() {
  if (m_primary_master) m_primary_master->release();
  m_primary_master.reset();
}

void Replica_set_impl::ensure_compatible_donor(
    const std::string &instance_def, mysqlshdk::mysql::IInstance *recipient) {
  /*
   * A donor is compatible if:
   *
   *   - It's an ONLINE ReplicaSet member
   *   - The target (recipient) and donor instances support clone (version
   *     >= 8.0.17)
   *   - It has the same version of the recipient
   *   - It has the same operating system as the recipient
   */

  auto console = current_console();

  Scoped_instance target;
  try {
    target = Scoped_instance(connect_target_instance(instance_def));
  } catch (const shcore::Exception &e) {
    console->print_error(shcore::str_format(k_error_connecting_to_instance,
                                            instance_def.c_str()));
    throw;
  }

  // Check if the target belongs to the ReplicaSet (MD)
  std::string target_address = target->get_canonical_address();
  try {
    m_metadata_storage->get_instance_by_address(target_address);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      throw shcore::Exception(
          "Instance " + target_address + " does not belong to the replicaset",
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }
    throw;
  }

  // Check if the instance is ONLINE
  auto topology_mng = setup_topology_manager();
  auto topology_node =
      topology_mng->topology()->try_get_node_for_uuid(target->get_uuid());
  topology::Node_status status = topology_node->status();

  if (status != topology::Node_status::ONLINE) {
    throw shcore::Exception("Instance " + target_address +
                                " is not an ONLINE member of the ReplicaSet.",
                            SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
  }

  // Check if the instance support clone (the recipient was already checked)
  if (target->get_version() <
      mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
    throw shcore::Exception(
        "Instance " + target_address + " does not support MySQL Clone.",
        SHERR_DBA_CLONE_NO_SUPPORT);
  }

  // Check if the instance has the same version of the recipient
  if (target->get_version() != recipient->get_version()) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "version than the recipient.",
                            SHERR_DBA_CLONE_DIFF_VERSION);
  }

  // Check if the instance has the same OS the recipient
  if (target->get_version_compile_os() != recipient->get_version_compile_os()) {
    throw shcore::Exception("Instance " + target_address +
                                " cannot be a donor because it has a different "
                                "Operating System than the recipient.",
                            SHERR_DBA_CLONE_DIFF_OS);
  }

  // Check if the instance is running on the same platform of the recipient
  if (target->get_version_compile_machine() !=
      recipient->get_version_compile_machine()) {
    throw shcore::Exception(
        "Instance " + target_address +
            " cannot be a donor because it is running on a different "
            "platform than the recipient.",
        SHERR_DBA_CLONE_DIFF_PLATFORM);
  }
}

std::string Replica_set_impl::pick_clone_donor(
    mysqlshdk::mysql::IInstance *recipient) {
  auto console = current_console();

  std::string r;
  auto topology_mng = setup_topology_manager();
  auto topology = topology_mng->topology();

  // Get the ReplicaSet primary member
  const topology::Node *primary = topology->get_primary_master_node();

  // Get the ReplicaSet secondary members
  std::list<const topology::Node *> secondaries =
      topology_mng->topology()->get_slave_nodes(primary);

  std::string full_msg;

  // By default, the donor must be an online secondary member. If not available,
  // then it must be the primary
  if (!secondaries.empty()) {
    for (const auto *s : secondaries) {
      if (recipient->get_uuid() != s->get_primary_member()->uuid) {
        std::string instance_def = s->get_primary_member()->endpoint;

        try {
          if (mysqlshdk::utils::Net::is_ipv6(
                  mysqlshdk::utils::split_host_and_port(instance_def).first))
            throw shcore::Exception::runtime_error(
                "Instance hostname/report_host is an IPv6 address, which is "
                "not supported for cloning");

          ensure_compatible_donor(instance_def, recipient);
          r = instance_def;

          // No need to continue looking for more
          break;
        } catch (const shcore::Exception &e) {
          std::string msg = "SECONDARY '" + instance_def +
                            "' is not a suitable clone donor: " + e.what();
          log_info("%s", msg.c_str());
          full_msg += msg + "\n";
          continue;
        }
      }
    }
  }

  // If no secondary is suitable, use the primary
  if (r.empty()) {
    std::string instance_def = primary->get_primary_member()->endpoint;

    try {
      if (mysqlshdk::utils::Net::is_ipv6(
              mysqlshdk::utils::split_host_and_port(instance_def).first))
        throw shcore::Exception::runtime_error(
            "Instance hostname/report_host is an IPv6 address, which is "
            "not supported for cloning");

      ensure_compatible_donor(instance_def, recipient);
      r = instance_def;
    } catch (const shcore::Exception &e) {
      std::string msg = "PRIMARY '" + instance_def +
                        "' is not a suitable clone donor: " + e.what();
      log_info("%s", msg.c_str());
      full_msg += msg + "\n";
    }
  }

  // If nobody is compatible...
  if (r.empty()) {
    console->print_error(
        "None of the members in the replicaSet are compatible to be used as "
        "clone donors for " +
        recipient->descr());
    console->print_info(full_msg);

    throw shcore::Exception("The ReplicaSet has no compatible clone donors.",
                            SHERR_DBA_CLONE_NO_DONORS);
  }

  return r;
}

void Replica_set_impl::revert_topology_changes(
    mysqlshdk::mysql::IInstance *target_server, bool remove_user,
    bool dry_run) {
  auto console = current_console();
  // revert changes by deleting the account we created for it and
  // clearing replication
  console->print_info("Reverting topology changes...");
  try {
    if (remove_user) {
      drop_replication_user(target_server, k_async_cluster_user_name);
    }

    async_remove_replica(target_server, dry_run);
  } catch (const std::exception &e) {
    console->print_error(
        std::string("Error while reverting replication changes: ") +
        format_active_exception());
  }
}

void Replica_set_impl::handle_clone(
    const std::shared_ptr<mysqlsh::dba::Instance> &recipient,
    const Clone_options &clone_options,
    const Async_replication_options &ar_options,
    const Recovery_progress_style &progress_style, int sync_timeout,
    bool dry_run) {
  auto console = current_console();
  Scoped_instance donor_instance;
  /*
   * Clone handling:
   *
   * 1.  Pick a valid donor (unless cloneDonor is set). By default, the donor
   *     must be an ONLINE SECONDARY member. If not available, then must be the
   *     PRIMARY.
   * 2.  Install the Clone plugin on the donor and recipient (if not installed
   *     already)
   * 3.  Set the donor to the selected donor: SET GLOBAL clone_valid_donor_list
   *     = "donor_host:donor_port";
   * 4.  Create or update a recovery account with the required
   *     privileges for replicaSets management + clone usage (BACKUP_ADMIN)
   * 5.  Ensure the donor's recovery account has the clone usage required
   *     privileges: BACKUP_ADMIN
   * 6.  Grant the CLONE_ADMIN privilege to the recovery account
   *     at the recipient
   * 7.  Create the SQL clone command based on the above
   * 8.  Execute the clone command
   */

  // Pick a valid donor
  std::string donor;
  if (!clone_options.clone_donor.is_null()) {
    ensure_compatible_donor(*clone_options.clone_donor, recipient.get());
    donor = *clone_options.clone_donor;
  } else {
    donor = pick_clone_donor(recipient.get());
  }

  // Install the clone plugin on the recipient and donor
  try {
    donor_instance = Scoped_instance(connect_target_instance(donor));
  } catch (const shcore::Exception &e) {
    console->print_error(
        shcore::str_format(k_error_connecting_to_instance,
                           Connection_options(donor).uri_endpoint().c_str()));
    throw;
  }

  log_info("Installing the clone plugin on donor '%s'%s.",
           donor_instance.get()->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*donor_instance, nullptr);
  }

  log_info("Installing the clone plugin on recipient '%s'%s.",
           recipient->get_canonical_address().c_str(),
           dry_run ? " (dryRun)" : "");

  if (!dry_run) {
    mysqlshdk::mysql::install_clone_plugin(*recipient, nullptr);
  }

  // Set the donor to the selected donor on the recipient
  if (!dry_run) {
    recipient->set_sysvar("clone_valid_donor_list", donor);
  }

  // Create or update a recovery account with the required privileges for
  // replicaSets management + clone usage (BACKUP_ADMIN) on the recipient

  // Check if super_read_only is enabled. If so it must be disabled to create
  // the account
  if (recipient->get_sysvar_bool("super_read_only").get_safe()) {
    recipient->set_sysvar("super_read_only", false);
  }

  // Clone requires a user in both donor and recipient:
  //
  // On the donor, the clone user requires the BACKUP_ADMIN privilege for
  // accessing and transferring data from the donor, and for blocking DDL
  // during the cloning operation.
  //
  // On the recipient, the clone user requires the CLONE_ADMIN privilege for
  // replacing recipient data, blocking DDL during the cloning operation, and
  // automatically restarting the server. The CLONE_ADMIN privilege includes
  // BACKUP_ADMIN and SHUTDOWN privileges implicitly.
  //
  // For that reason, we create a user in the recipient with the same username
  // and password as the replication user created in the donor.
  create_clone_recovery_user_nobinlog(recipient.get(),
                                      *ar_options.repl_credentials, dry_run);

  if (!dry_run) {
    // Ensure the donor's recovery account has the clone usage required
    // privileges: BACKUP_ADMIN
    get_primary_master()->executef("GRANT BACKUP_ADMIN ON *.* TO ?@?",
                                   ar_options.repl_credentials->user, "%");

    // If the donor instance is processing transactions, it may have the
    // clone-user handling (create + grant) still in the backlog waiting to be
    // applied. For that reason, we must wait for it to be in sync with the
    // primary before starting the clone itself (BUG#30628746)
    std::string primary_address = get_primary_master()->get_canonical_address();

    std::string donor_address = donor_instance->get_canonical_address();

    if (primary_address != donor_address) {
      console->print_info(
          "* Waiting for the donor to synchronize with PRIMARY...");
      if (!dry_run) {
        try {
          // Sync the donor with the primary
          sync_transactions(*donor_instance, k_async_cluster_channel_name,
                            sync_timeout);
        } catch (const shcore::Exception &e) {
          if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
            console->print_error(
                "The donor instance failed to synchronize its transaction set "
                "with the PRIMARY.");
          }
          throw;
        } catch (const cancel_sync &) {
          // Throw it up
          throw;
        }
      }
      console->print_info();
    }

    // Create a new connection to the recipient and run clone asynchronously
    Scoped_instance recipient_clone;
    std::string instance_def =
        recipient->get_connection_options().uri_endpoint();
    try {
      recipient_clone = Scoped_instance(connect_target_instance(instance_def));
    } catch (const shcore::Exception &e) {
      current_console()->print_error(shcore::str_format(
          k_error_connecting_to_instance, instance_def.c_str()));
      throw;
    }

    mysqlshdk::db::Connection_options clone_donor_opts(donor);

    std::string begin_time =
        recipient->queryf_one_string(0, "", "SELECT NOW(6)");

    std::exception_ptr error_ptr;

    std::thread clone_thread = std::thread(
        [&recipient_clone, clone_donor_opts, ar_options, &error_ptr]() {
          mysqlsh::thread_init();

          try {
            mysqlshdk::mysql::do_clone(recipient_clone, clone_donor_opts,
                                       *ar_options.repl_credentials);
          } catch (const mysqlshdk::db::Error &err) {
            // Clone canceled
            if (err.code() == ER_QUERY_INTERRUPTED) {
              log_info("Clone canceled: %s", err.format().c_str());
            } else {
              log_info("Error cloning from instance '%s': %s",
                       clone_donor_opts.uri_endpoint().c_str(),
                       err.format().c_str());
              error_ptr = std::current_exception();
            }
          } catch (const std::exception &err) {
            log_info("Error cloning from instance '%s': %s",
                     clone_donor_opts.uri_endpoint().c_str(), err.what());
            error_ptr = std::current_exception();
          }

          mysqlsh::thread_end();
        });

    shcore::Scoped_callback join([&clone_thread, error_ptr]() {
      if (clone_thread.joinable()) clone_thread.join();
    });

    try {
      monitor_clone_instance(
          recipient->get_connection_options(), begin_time, progress_style,
          k_clone_start_timeout,
          mysqlshdk::mysql::k_server_recovery_restart_timeout);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      refresh_target_connections(recipient.get());

      // Remove the BACKUP_ADMIN grant from the recovery account
      get_primary_master()->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?",
                                     ar_options.repl_credentials->user);
    } catch (const stop_monitoring &) {
      console->print_info();
      console->print_note("Recovery process canceled. Reverting changes...");

      // Cancel the clone query
      mysqlshdk::mysql::cancel_clone(*recipient);

      log_info("Clone canceled.");

      log_debug("Waiting for clone thread...");
      // wait for the clone thread to finish
      clone_thread.join();
      log_debug("Clone thread joined");

      // When clone is canceled, the target instance will restart and all
      // connections are closed so we need to wait for the server to start-up
      // and re-establish the session. Also we need to test if the connection
      // to the target instance and MD are closed and re-open if necessary
      *recipient = *wait_server_startup(
          recipient->get_connection_options(),
          mysqlshdk::mysql::k_server_recovery_restart_timeout,
          Recovery_progress_style::NOWAIT);

      refresh_target_connections(recipient.get());

      // Remove the BACKUP_ADMIN grant from the recovery account
      get_primary_master()->executef("REVOKE BACKUP_ADMIN ON *.* FROM ?",
                                     ar_options.repl_credentials->user);

      revert_clone_recovery(recipient.get(), *ar_options.repl_credentials);

      // Thrown the exception cancel_sync up
      throw cancel_sync();
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          e.what(), e.code(), e.sqlstate());
    }
  }
}

Member_recovery_method Replica_set_impl::validate_instance_recovery(
    Member_op_action op_action, mysqlshdk::mysql::IInstance *donor_instance,
    mysqlshdk::mysql::IInstance *target_instance,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive) {
  auto check_recoverable =
      [donor_instance](mysqlshdk::mysql::IInstance *tgt_instance) {
        // Get the gtid state in regards to the donor
        mysqlshdk::mysql::Replica_gtid_state state =
            mysqlshdk::mysql::check_replica_gtid_state(
                *donor_instance, *tgt_instance, nullptr, nullptr);

        if (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE)
          return true;
        else
          return false;
      };

  Member_recovery_method recovery_method =
      mysqlsh::dba::validate_instance_recovery(
          Cluster_type::ASYNC_REPLICATION, op_action, donor_instance,
          target_instance, check_recoverable, opt_recovery_method,
          gtid_set_is_complete, interactive);

  return recovery_method;
}

void Replica_set_impl::refresh_target_connections(
    mysqlshdk::mysql::IInstance *recipient) {
  Connection_options opts = recipient->get_connection_options();

  try {
    recipient->query("SELECT 1");
  } catch (const mysqlshdk::db::Error &e) {
    if (mysqlshdk::db::is_mysql_client_error(e.code())) {
      log_debug(
          "Target instance connection lost: %s. Re-establishing a "
          "connection.",
          e.format().c_str());

      recipient->get_session()->connect(opts);
    } else {
      throw;
    }
  }

  try {
    m_metadata_storage->get_md_server()->query("SELECT 1");
  } catch (const mysqlshdk::db::Error &e) {
    if (mysqlshdk::db::is_mysql_client_error(e.code())) {
      log_debug(
          "Metadata connection lost: %s. Re-establishing a "
          "connection.",
          e.format().c_str());

      auto md_session = m_metadata_storage->get_md_server()->get_session();

      md_session->connect(md_session->get_connection_options());
    } else {
      throw;
    }
  }
}

Instance_id Replica_set_impl::manage_instance(Instance *instance,
                                              const std::string &instance_label,
                                              Instance_id master_id,
                                              bool is_primary) {
  Instance_metadata inst = query_instance_info(*instance);

  if (!instance_label.empty()) inst.label = instance_label;
  inst.cluster_id = get_id();
  if (master_id > 0) {
    inst.master_id = master_id;
  }

  inst.primary_master = is_primary;

  // XXX record the recoveryUser in the MD

  return get_metadata_storage()->record_async_member_added(inst);
}

const topology::Server *Replica_set_impl::check_target_member(
    topology::Server_global_topology *topology,
    const std::string &instance_def) {
  auto console = current_console();
  // we can't print instance_def directly because it may contain credentials
  std::string instance_label = Connection_options(instance_def).uri_endpoint();

  Scoped_instance target_instance;
  try {
    target_instance = Scoped_instance(connect_target_instance(instance_def));
  } catch (const shcore::Exception &e) {
    current_console()->print_error(shcore::str_format(
        k_error_connecting_to_instance, instance_label.c_str()));
    throw;
  }

  try {
    return topology->get_server(target_instance->get_uuid());
  } catch (const shcore::Exception &e) {
    log_warning("%s: %s", instance_label.c_str(), e.format().c_str());

    if (e.code() == SHERR_DBA_CLUSTER_METADATA_MISSING) {
      throw shcore::Exception(
          "Target instance " + instance_label + " is not a managed instance.",
          SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
    }
    throw;
  }
}

std::list<Scoped_instance> Replica_set_impl::connect_all_members(
    uint32_t read_timeout, bool skip_primary,
    std::list<Instance_metadata> *out_unreachable) {
  std::vector<Instance_metadata> instances =
      get_metadata_storage()->get_all_instances(get_id());

  auto console = current_console();
  std::list<Scoped_instance> r;
  auto ipool = current_ipool();

  for (const auto &i : instances) {
    if ((i.primary_master && skip_primary) || i.invalidated) continue;

    try {
      mysqlshdk::db::Connection_options opts(i.endpoint);
      ipool->default_auth_opts().set(&opts);
      // The read timeout will allow commands that block at the server but
      // have no server-side timeouts to not block the shell indefinitely.
      if (read_timeout > 0)
        opts.set(mysqlshdk::db::kNetReadTimeout,
                 {std::to_string(read_timeout * 1000)});

      console->print_info("** Connecting to " + i.label);
      r.emplace_back(ipool->connect_unchecked(opts));
    } catch (const shcore::Exception &e) {
      // Client errors are likely because the server is unreachable/crashed
      if (e.is_mysql() && mysqlshdk::db::is_mysql_client_error(e.code())) {
        if (out_unreachable) out_unreachable->push_back(i);

        if (i.primary_master)
          console->print_warning("Could not connect to PRIMARY instance: " +
                                 e.format());
        else
          console->print_warning("Could not connect to SECONDARY instance: " +
                                 e.format());
      } else {
        throw;
      }
    }
  }
  return r;
}

/**
 * Check for replication applier errors on all online instances.
 *
 * NOTE: If invalidate_error_instances is true, then instances with applier
 *       errors or stopped will be skipped (removed from the instances lists)
 *       and their ID added to the invalidated list.
 *
 * @param srv_topology Server_global_topology object with initial information
 *                     about the status of each instance.
 * @param out_online_instances List of online instances.
 * @param invalidate_error_instances Boolean indicate if instances with errors
 *                                   must be invalidated.
 * @param out_instances_md Vector with the instances metadata information.
 * @param out_invalidate_ids List with the IDs of the invalidated instances.
 */
void Replica_set_impl::check_replication_applier_errors(
    topology::Server_global_topology *srv_topology,
    std::list<Scoped_instance> *out_online_instances,
    bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids) const {
  auto console = current_console();
  int error_count = 0;

  std::list<std::string> error_uuids;

  for (const auto &instance : *out_online_instances) {
    // Get the matching instance info from the Server Global Topology.
    auto srv_info =
        srv_topology->get_server(instance->get_uuid())->get_primary_member();

    if (srv_info->master_channel) {
      // Get the applier status.
      // NOTE: Ignore receiver status since it is expected to have errors
      //       if the primary failed  and some expected receiver status
      //       (e.g., CONNECTION_ERROR) have precedence over other applier
      //       status (e.g., APPLIER_OFF).
      mysqlshdk::mysql::Replication_channel channel =
          srv_info->master_channel->info;
      mysqlshdk::mysql::Replication_channel::Status applier_status =
          channel.applier_status();

      // Issue error if the replication applier is stopped or has errors.
      if (applier_status ==
          mysqlshdk::mysql::Replication_channel::Status::APPLIER_OFF) {
        std::string err_msg{"Replication applier is OFF at instance " +
                            srv_info->label + "."};
        if (!invalidate_error_instances) {
          console->print_error(err_msg);
        } else {
          log_warning("%s", err_msg.c_str());
          console->print_note(
              srv_info->label +
              " will be invalidated (replication applier is OFF) and must be "
              "fixed or removed from the replicaset.");
        }
        error_count++;
        error_uuids.push_back(srv_info->uuid);
      } else if (applier_status ==
                 mysqlshdk::mysql::Replication_channel::Status::APPLIER_ERROR) {
        for (const auto &applier : channel.appliers) {
          if (applier.last_error.code != 0) {
            std::string err_msg{
                "Replication applier error at " + srv_info->label + ": " +
                mysqlshdk::mysql::to_string(applier.last_error)};
            if (!invalidate_error_instances) {
              current_console()->print_error(err_msg);
            } else {
              log_warning("%s", err_msg.c_str());
              console->print_note(
                  srv_info->label +
                  " will be invalidated (replication applier error) and must "
                  "be fixed or removed from the replicaset.");
            }
          }
        }
        error_uuids.push_back(srv_info->uuid);
        error_count++;
      }
    }
  }

  if (error_count > 0) {
    if (!invalidate_error_instances) {
      console->print_error(
          "Replication errors found for one or more SECONDARY instances. Use "
          "the 'invalidateErrorInstances' option to perform the failover "
          "anyway by skipping and invalidating instances with errors.");
      throw shcore::Exception(
          "One or more instances have replication applier errors.",
          SHERR_DBA_REPLICATION_APPLIER_ERROR);
    } else {
      console->println();

      // Update instances lists according to invalidated instances.
      for (const auto &uuid : error_uuids) {
        // Add instance to invalidate to list.
        auto it =
            std::find_if(out_instances_md->begin(), out_instances_md->end(),
                         [&uuid](const Instance_metadata &i_md) {
                           return i_md.uuid == uuid;
                         });
        if (it != out_instances_md->end()) {
          out_invalidate_ids->push_back(it->id);
        }

        // Remove instance to invalidate from lists.
        out_online_instances->remove_if([&uuid](const Scoped_instance &i) {
          return i->get_uuid() == uuid;
        });
        out_instances_md->erase(it);
      }
    }
  }
}

shcore::Value Replica_set_impl::list_routers(bool only_upgrade_required) {
  shcore::Value r = Base_cluster_impl::list_routers(only_upgrade_required);

  (*r.as_map())["replicaSetName"] = shcore::Value(get_name());

  return r;
}

}  // namespace dba
}  // namespace mysqlsh
