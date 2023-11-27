/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#include <mysqld_error.h>

#include <tuple>
#include <utility>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/async_utils.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/connectivity_check.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/global_topology_check.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/star_global_topology_manager.h"
#include "modules/adminapi/common/topology_executor.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/replica_set/describe.h"
#include "modules/adminapi/replica_set/dissolve.h"
#include "modules/adminapi/replica_set/rescan.h"
#include "modules/adminapi/replica_set/status.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "scripting/types.h"

namespace mysqlsh::dba {

namespace {

// Constants with the names used to lock the cluster
constexpr std::string_view k_lock_ns{"AdminAPI_replicaset"};
constexpr std::string_view k_lock_name{"AdminAPI_lock"};

constexpr const char k_replica_set_attribute_ssl_mode[] =
    "opt_replicationSslMode";

// async replication channel options
inline constexpr std::string_view k_instance_attribute_replConnectRetry{
    "opt_replConnectRetry"};
inline constexpr std::string_view k_instance_attribute_replRetryCount{
    "opt_replRetryCount"};
inline constexpr std::string_view k_instance_attribute_replHeartbeatPeriod{
    "opt_replHeartbeatPeriod"};
inline constexpr std::string_view
    k_instance_attribute_replCompressionAlgorithms{
        "opt_replCompressionAlgorithms"};
inline constexpr std::string_view k_instance_attribute_replZstdCompressionLevel{
    "opt_replZstdCompressionLevel"};
inline constexpr std::string_view k_instance_attribute_replBind{"opt_replBind"};
inline constexpr std::string_view k_instance_attribute_replNetworkNamespace{
    "opt_replNetworkNamespace"};

std::unique_ptr<topology::Server_global_topology> discover_unmanaged_topology(
    Instance *instance) {
  auto topology = std::make_unique<topology::Server_global_topology>(
      k_replicaset_channel_name);

  // perform initial discovery
  topology->discover_from_unmanaged(instance);

  topology->check_gtid_consistency(false);

  return topology;
}

void validate_version(const Instance &target_server) {
  if (target_server.get_version() < mysqlshdk::utils::Version(8, 0, 11)) {
    current_console()->print_info(
        "MySQL version " + target_server.get_version().get_full() +
        " detected at " + target_server.get_canonical_address() +
        ", but 8.0.11 is required for InnoDB ReplicaSet.");
    throw shcore::Exception("Unsupported MySQL version",
                            SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED);
  }
}

void validate_instance(Instance *target_server) {
  // Check instance configuration and state
  ensure_ar_instance_configuration_valid(target_server);

  // Check if GR is running
  if (mysqlshdk::gr::Member_state state;
      !mysqlshdk::gr::get_group_information(*target_server, &state, nullptr,
                                            nullptr, nullptr) ||
      state == mysqlshdk::gr::Member_state::OFFLINE)
    return;

  current_console()->print_error(
      shcore::str_format("%s has Group Replication active, which cannot be "
                         "mixed with ReplicaSets.",
                         target_server->descr().c_str()));

  throw shcore::Exception("group_replication active",
                          SHERR_DBA_INVALID_SERVER_CONFIGURATION);
}

void validate_instance_is_standalone(Instance *target_server,
                                     bool add_op = false) {
  // Look for unexpected replication channels
  auto channels = mysqlshdk::mysql::get_incoming_channels(*target_server);
  auto slaves = Replica_set_impl::get_valid_slaves(*target_server, nullptr);
  if (channels.empty() && slaves.empty()) return;

  auto console = current_console();
  console->print_error(
      shcore::str_format("Extraneous replication channels found at '%s':",
                         target_server->descr().c_str()));
  for (const auto &ch : channels) {
    console->print_info(shcore::str_format(
        "- channel '%s' from %s", ch.channel_name.c_str(),
        mysqlshdk::utils::make_host_and_port(ch.host, ch.port).c_str()));
  }

  for (const auto &[slave, channel_name] : slaves) {
    if (channel_name.empty())
      console->print_info(shcore::str_format(
          "- %s replicates from this instance",
          mysqlshdk::utils::make_host_and_port(slave.host, slave.port)
              .c_str()));
    else
      console->print_info(shcore::str_format(
          "- %s replicates from this instance (channel '%s')",
          mysqlshdk::utils::make_host_and_port(slave.host, slave.port).c_str(),
          channel_name.c_str()));
  }

  console->print_info();
  std::string info_msg =
      "Unmanaged replication channels are not supported in a replicaset. If "
      "you'd like to manage an existing MySQL replication topology with the "
      "Shell, use the <<<createReplicaSet>>>() operation with the "
      "adoptFromAR option.";

  if (add_op) {
    std::string replica_term =
        mysqlshdk::mysql::get_replica_keyword(target_server->get_version());
    info_msg
        .append(
            " If the <<<addInstance>>>() operation previously failed for the "
            "target instance and you are trying to add it again, then after "
            "fixing the issue you should reset the current replication "
            "settings before retrying to execute the operation. To reset the "
            "replication settings on the target instance execute the following "
            "statements: 'STOP ")
        .append(replica_term)
        .append(";' and 'RESET ")
        .append(replica_term)
        .append(" ALL;'.");
  }

  console->print_para(info_msg);
  throw shcore::Exception("Unexpected replication channel",
                          SHERR_DBA_INVALID_SERVER_CONFIGURATION);
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
    const Create_replicaset_options &options) {
  // Acquire required locks on target instance.
  // No "write" operation allowed to be executed concurrently on the target
  // instance.
  auto i_lock = target_server->get_lock_exclusive();

  // Validate the cluster_name
  mysqlsh::dba::validate_cluster_name(full_cluster_name,
                                      Cluster_type::ASYNC_REPLICATION);

  // if adopting, memberAuth only support "password"
  if (options.adopt && (options.member_auth_options.member_auth_type !=
                        Replication_auth_type::PASSWORD))
    throw shcore::Exception::argument_error(
        shcore::str_format("Cannot set '%s' to a value other than 'PASSWORD' "
                           "if '%s' is set to true.",
                           kMemberAuthType, kAdoptFromAR));

  std::string domain_name;
  std::string cluster_name;
  parse_fully_qualified_cluster_name(full_cluster_name, &domain_name, nullptr,
                                     &cluster_name);
  if (domain_name.empty()) domain_name = k_default_domain_name;

  auto console = current_console();
  if (options.adopt) {
    console->print_info("A new replicaset with the topology visible from '" +
                        target_server->descr() + "' will be created.\n");
  } else {
    console->print_info("A new replicaset with instance '" +
                        target_server->descr() + "' will be created.\n");
  }
  if (options.dry_run) {
    console->print_note(
        "dryRun option was specified. Validations will be executed, "
        "but no changes will be applied.");
  }

  auto metadata = std::make_shared<MetadataStorage>(target_server);

  auto cluster = std::make_shared<Replica_set_impl>(cluster_name, target_server,
                                                    metadata, topology_type);

  if (options.adopt) {
    console->print_info("* Scanning replication topology...");
    auto topology = std::make_unique<Star_global_topology_manager>(
        0, discover_unmanaged_topology(target_server.get()));

    console->print_info();
    cluster->adopt(topology.get(), options, options.dry_run);
  } else {
    cluster->create(options, options.dry_run);
  }

  if (!options.dry_run) {
    metadata->update_cluster_attribute(
        cluster->get_id(), k_cluster_attribute_assume_gtid_set_complete,
        options.gtid_set_is_complete ? shcore::Value::True()
                                     : shcore::Value::False());
  }

  return cluster;
}

std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
Replica_set_impl::get_valid_slaves(
    const mysqlshdk::mysql::IInstance &instance,
    std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
        *ghost_slaves) {
  auto slaves = get_slaves(instance);
  if (slaves.empty()) return {};

  std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
      real_slaves;
  real_slaves.reserve(slaves.size());

  auto ipool = current_ipool();

  for (const auto &ch : slaves) {
    // This is a bit unnecessary, but we do it to avoid false positives,
    // specially in tests. The problem is that SHOW SLAVE HOSTS will include
    // slaves that have stopped replicating, so we need to double-check that
    // the channel is still active to prevent false positives.

    auto endpoint = mysqlshdk::utils::make_host_and_port(ch.host, ch.port);
    std::string channel_name;
    bool ghost_slave = true;

    try {
      Scoped_instance slave(ipool->connect_unchecked_endpoint(endpoint));

      auto slave_channels = mysqlshdk::mysql::get_incoming_channels(*slave);
      for (const auto &slave_channel : slave_channels) {
        if (slave_channel.source_uuid != instance.get_uuid()) continue;

        channel_name = slave_channel.channel_name;
        ghost_slave = false;
        break;
      }
    } catch (const shcore::Exception &e) {
      log_info(
          "Could not connect to %s, which is listed as a replica for %s: %s",
          endpoint.c_str(), instance.descr().c_str(), e.format().c_str());
      real_slaves.push_back({ch, ""});
      continue;
    }

    if (ghost_slave) {
      log_info("Ignoring stale replica host %s for %s", endpoint.c_str(),
               instance.descr().c_str());
      if (ghost_slaves)
        ghost_slaves->push_back(
            {ch, shcore::str_format("'%s'", channel_name.c_str())});
      continue;
    }

    real_slaves.push_back(
        {ch, shcore::str_format("'%s'", channel_name.c_str())});
  }

  return real_slaves;
}

void Replica_set_impl::create(const Create_replicaset_options &options,
                              bool dry_run) {
  auto console = current_console();

  console->print_info("* Checking MySQL instance at " +
                      m_cluster_server->descr());

  validate_instance(m_cluster_server.get());
  validate_instance_is_standalone(m_cluster_server.get());
  console->print_info();

  log_info("Unfencing PRIMARY %s", m_cluster_server->descr().c_str());
  if (!dry_run) unfence_instance(m_cluster_server.get(), true);

  // target is the primary
  m_primary_master = m_cluster_server;
  m_primary_master->retain();

  Cluster_ssl_mode ssl_mode = options.ssl_mode;
  resolve_ssl_mode_option("replicationSslMode", "ReplicaSet", *m_cluster_server,
                          &ssl_mode);

  // check if member auth request mode is supported
  validate_instance_member_auth_type(
      *m_primary_master, false, ssl_mode, "replicationSslMode",
      options.member_auth_options.member_auth_type);

  // check if SSL options are valid
  validate_instance_member_auth_options(
      "replicaset", options.member_auth_options.member_auth_type,
      options.member_auth_options.cert_issuer,
      options.member_auth_options.cert_subject);

  if (current_shell_options()->get().dba_connectivity_checks) {
    console->print_info("* Checking connectivity and SSL configuration...");
    test_self_connection(*m_cluster_server, "", ssl_mode,
                         options.member_auth_options.member_auth_type,
                         options.member_auth_options.cert_issuer,
                         options.member_auth_options.cert_subject, "");
  }

  console->print_info("* Updating metadata...");

  try {
    // First we need to create the Metadata Schema
    prepare_metadata_schema(m_cluster_server, dry_run);

    // Update metadata
    log_info("Creating replicaset metadata...");
    Cluster_id id;
    if (!dry_run) {
      MetadataStorage::Transaction trx(m_metadata_storage);

      id = m_metadata_storage->create_async_cluster_record(this, false);

      m_metadata_storage->update_cluster_attribute(
          id, k_cluster_attribute_replication_allowed_host,
          options.replication_allowed_host.empty()
              ? shcore::Value("%")
              : shcore::Value(options.replication_allowed_host));

      m_metadata_storage->update_cluster_attribute(
          id, k_cluster_attribute_member_auth_type,
          shcore::Value(
              to_string(options.member_auth_options.member_auth_type)));
      m_metadata_storage->update_cluster_attribute(
          id, k_cluster_attribute_cert_issuer,
          shcore::Value(options.member_auth_options.cert_issuer));

      m_metadata_storage->update_cluster_attribute(
          id, k_replica_set_attribute_ssl_mode,
          shcore::Value(to_string(ssl_mode)));

      trx.commit();
    }

    // create repl user to be used in the future
    auto user = create_replication_user(
        m_cluster_server.get(), options.member_auth_options.cert_subject,
        dry_run);

    log_info("Recording metadata for %s", m_cluster_server->descr().c_str());
    if (!dry_run) {
      assert(!id.empty());

      manage_instance(m_cluster_server.get(), {user.first.user, user.second},
                      options.instance_label, 0, true);

      m_metadata_storage->update_instance_attribute(
          m_cluster_server.get()->get_uuid(), k_instance_attribute_cert_subject,
          shcore::Value(options.member_auth_options.cert_subject));

      // Set and store the default Routing Options
      {
        MetadataStorage::Transaction trx(m_metadata_storage);

        for (const auto &[option, value] :
             k_default_replicaset_router_options) {
          m_metadata_storage->set_global_routing_option(
              Cluster_type::ASYNC_REPLICATION, id, option, value);
        }

        trx.commit();
      }
    }
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
                             const Create_replicaset_options &options,
                             bool dry_run) {
  auto console = current_console();

  console->print_info(
      "* Discovering async replication topology starting with " +
      m_cluster_server->descr());

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
  if (!dry_run) unfence_instance(primary_instance.get(), true);

  // target is the primary
  m_primary_master = m_cluster_server;
  m_primary_master->retain();

  console->print_info("* Updating metadata...");

  try {
    // First we need to create the Metadata Schema
    prepare_metadata_schema(primary_instance, dry_run);

    // Update metadata
    log_info("Creating replicaset metadata...");
    if (!dry_run) {
      MetadataStorage::Transaction trx(m_metadata_storage);
      auto id = m_metadata_storage->create_async_cluster_record(this, true);

      {
        auto allowed_host = options.replication_allowed_host.empty()
                                ? "%"
                                : options.replication_allowed_host;

        m_metadata_storage->update_cluster_attribute(
            id, k_cluster_attribute_replication_allowed_host,
            shcore::Value(std::move(allowed_host)));
      }

      m_metadata_storage->update_instance_attribute(
          id, k_instance_attribute_cert_subject,
          shcore::Value(options.member_auth_options.cert_subject));

      trx.commit();
    }

    // create rpl user for primary
    Instance_id primary_id = 0;
    {
      const auto [auth_options, host] = create_replication_user(
          m_cluster_server.get(), options.member_auth_options.cert_subject,
          dry_run);

      if (!dry_run)
        primary_id = manage_instance(primary_instance.get(),
                                     {auth_options.user, host}, "", 0, true);
    }

    console->print_info();

    std::string unused_repl_users;

    for (const auto *server : topology->topology()->nodes()) {
      if (server == primary) continue;

      Scoped_instance instance(ipool->connect_unchecked_endpoint(
          server->get_primary_member()->endpoint));

      std::string cert_subject;
      if (!dry_run)
        cert_subject = query_cluster_instance_auth_cert_subject(*instance);

      const auto [auth_options, host] =
          create_replication_user(instance.get(), cert_subject, dry_run);

      log_info("Fencing SECONDARY %s", server->label.c_str());

      if (!dry_run) fence_instance(instance.get());

      log_info("Recording metadata for %s", server->label.c_str());
      if (!dry_run) {
        manage_instance(instance.get(), {auth_options.user, host}, "",
                        primary_id, false);

        auto repl_user_old = mysqlshdk::mysql::get_replication_user(
            *instance, k_replicaset_channel_name);

        try {
          mysqlshdk::mysql::stop_replication_receiver(
              *instance, k_replicaset_channel_name);

          shcore::Scoped_callback start_channel([&instance]() {
            mysqlshdk::mysql::start_replication_receiver(
                *instance, k_replicaset_channel_name);
          });

          mysqlshdk::mysql::Replication_credentials_options repl_options;
          repl_options.password = auth_options.password.value_or("");

          mysqlshdk::mysql::change_replication_credentials(
              *instance, k_replicaset_channel_name, auth_options.user,
              repl_options);

        } catch (...) {
          console->print_error(
              shcore::str_format("An error occurred while trying to update the "
                                 "replication account for instance '%s'. "
                                 "Please try to invoke the command again.",
                                 instance->descr().c_str()));
          throw;
        }

        auto repl_user_new = mysqlshdk::mysql::get_replication_user(
            *instance, k_replicaset_channel_name);

        if (!repl_user_old.empty() && !repl_user_new.empty() &&
            (repl_user_old != repl_user_new) && (repl_user_old != "root")) {
          if (!unused_repl_users.empty()) unused_repl_users.append(", ");
          unused_repl_users.append("'").append(repl_user_old).append("'");
        }
      }
    }

    if (!unused_repl_users.empty())
      console->print_info(shcore::str_format(
          "A new replication user was created for each instance of the "
          "ReplicaSet, which means that the following users are unused and "
          "should be removed: %s",
          unused_repl_users.c_str()));

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

void Replica_set_impl::read_replication_options(
    std::string_view instance_uuid, Async_replication_options *ar_options,
    bool *has_null_options) {
  assert(ar_options);

  bool null_options{false};

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replConnectRetry, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->connect_retry = value.as_int();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replRetryCount, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->retry_count = value.as_int();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replHeartbeatPeriod, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if ((value.get_type() == shcore::Integer) ||
        (value.get_type() == shcore::Float))
      ar_options->heartbeat_period = value.as_double();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replCompressionAlgorithms,
          &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->compression_algos = value.as_string();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replZstdCompressionLevel,
          &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->zstd_compression_level = value.as_int();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replBind, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->bind = value.as_string();
  }

  if (shcore::Value value; get_metadata_storage()->query_instance_attribute(
          instance_uuid, k_instance_attribute_replNetworkNamespace, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->network_namespace = value.as_string();
  }

  if (has_null_options) *has_null_options = null_options;
}

void Replica_set_impl::read_replication_options(
    const mysqlshdk::mysql::IInstance *instance,
    Async_replication_options *ar_options, bool *has_null_options) {
  assert(ar_options);

  // Get the ClusterSet configured SSL mode
  {
    shcore::Value value;
    get_metadata_storage()->query_cluster_attribute(
        get_id(), k_replica_set_attribute_ssl_mode, &value);

    ar_options->ssl_mode = value ? to_cluster_ssl_mode(value.as_string())
                                 : Cluster_ssl_mode::DISABLED;
  }

  // get replication options
  if (instance)
    read_replication_options(instance->get_uuid(), ar_options,
                             has_null_options);

  // get the recovery auth mode
  ar_options->auth_type = query_cluster_auth_type();
}

void Replica_set_impl::cleanup_replication_options(
    const mysqlshdk::mysql::IInstance &instance) {
  auto remove_attrib = [this, &instance](std::string_view attrib_name) {
    shcore::Value value;
    if (!m_metadata_storage->query_instance_attribute(instance.get_uuid(),
                                                      attrib_name, &value))
      return;

    if (value.get_type() == shcore::Null)
      m_metadata_storage->remove_instance_attribute(instance.get_uuid(),
                                                    attrib_name);
  };

  remove_attrib(k_instance_attribute_replConnectRetry);
  remove_attrib(k_instance_attribute_replRetryCount);
  remove_attrib(k_instance_attribute_replHeartbeatPeriod);
  remove_attrib(k_instance_attribute_replCompressionAlgorithms);
  remove_attrib(k_instance_attribute_replZstdCompressionLevel);
  remove_attrib(k_instance_attribute_replBind);
  remove_attrib(k_instance_attribute_replNetworkNamespace);
}

void Replica_set_impl::validate_add_instance(
    Global_topology_manager *topology, mysqlshdk::mysql::IInstance * /*master*/,
    Instance *target_instance, Async_replication_options *ar_options,
    Clone_options *clone_options, const std::string &cert_subject,
    bool interactive) {
  auto validate_not_managed = [this](const Instance &target) {
    // Check if the instance is already a member of this replicaset
    Instance_metadata md;
    bool already_managed = false;
    try {
      md = m_metadata_storage->get_instance_by_uuid(target.get_uuid());
      already_managed = true;
    } catch (const shcore::Exception &e) {
      log_info("Error querying metadata for %s: %s\n",
               target.get_uuid().c_str(), e.what());
    }
    bool is_invalidated = false;
    if (!already_managed) {
      // The instance is not a member of this replicaset, but it has
      // the metadata schema. Check if this isn't an invalidated member.
      MetadataStorage metadata(target);

      if (metadata.check_version()) {
        try {
          md = metadata.get_instance_by_uuid(target.get_uuid());

          // Check whether the other instance was once part of the same rs
          if (md.cluster_id == get_id()) is_invalidated = true;
        } catch (const std::exception &e) {
          log_info("Error querying local copy of metadata for %s: %s",
                   target.get_uuid().c_str(), e.what());
        }
      }
    }
    if (is_invalidated) {
      // If this is a removed invalidated member of the replicaset, we can
      // only add it back if the GTID set has not diverged.
      // The GTID check will happen later on, as part of the regular checks.
      log_info("%s appears to be an invalidated ex-member of the replicaset",
               target.descr().c_str());
    } else {
      // Check whether the instance we found is really the target one
      // or if it's just a case of duplicate UUID
      if (!md.cluster_id.empty() &&
          !mysqlshdk::utils::are_endpoints_equal(
              md.address, target.get_canonical_address())) {
        log_info("%s: UUID %s already in used by %s", target.descr().c_str(),
                 target.get_uuid().c_str(), md.address.c_str());
        throw shcore::Exception("server_uuid in " + target.descr() +
                                    " is expected to be unique, but " +
                                    md.address + " already uses the same value",
                                SHERR_DBA_INVALID_SERVER_CONFIGURATION);
      }

      if (md.cluster_id == get_id()) {
        throw shcore::Exception(
            target.descr() + " is already a member of this replicaset.",
            SHERR_DBA_BADARG_INSTANCE_ALREADY_MANAGED);

      } else if (!md.cluster_id.empty()) {
        std::string label = md.group_name.empty() ? "replicaset." : "cluster.";
        throw shcore::Exception(
            target.descr() + " is already managed by a " + label,
            SHERR_DBA_BADARG_INSTANCE_ALREADY_MANAGED);
      }
    }
  };

  auto validate_unique_server_id = [](Instance *target,
                                      Global_topology_manager *topology_mgr) {
    std::string server_uuid = target->get_uuid();
    uint32_t server_id = target->get_server_id();

    for (const auto &node : topology_mgr->topology()->nodes()) {
      for (const auto &m : node->members()) {
        // uniqueness of uuid is checked in validate_not_managed()
        if (m.server_id.value_or(0) == server_id) {
          throw shcore::Exception("server_id in " + target->descr() +
                                      " is expected to be unique, but " +
                                      m.label + " already uses the same value",
                                  SHERR_DBA_INVALID_SERVER_CONFIGURATION);
        }
      }
    }
  };

  // Validate the Clone options.
  clone_options->check_option_values(target_instance->get_version());

  // Check version early on, so that other checks don't need to care about it
  validate_version(*target_instance);

  // Check if the target is already managed by us or some other cluster
  validate_not_managed(*target_instance);

  // regular instance checks
  validate_instance(target_instance);
  validate_instance_is_standalone(target_instance, true);

  validate_unique_server_id(target_instance, topology);

  validate_instance_ssl_mode(Cluster_type::ASYNC_REPLICATION, *target_instance,
                             ar_options->ssl_mode);

  auto console = current_console();
  // check consistency of the global topology
  console->print_info();
  topology->validate_add_replica(nullptr, target_instance, *ar_options);

  std::shared_ptr<Instance> donor_instance = get_cluster_server();

  console->print_info();

  if (current_shell_options()->get().dba_connectivity_checks) {
    console->print_info("* Checking connectivity and SSL configuration...");
    mysqlshdk::mysql::Set_variable sro(*target_instance, "super_read_only", 0,
                                       true);

    auto member_auth = query_cluster_auth_type();
    std::string cert_issuer = query_cluster_auth_cert_issuer();

    test_peer_connection(
        *target_instance, "", cert_subject, *m_primary_master, "",
        query_cluster_instance_auth_cert_subject(m_primary_master->get_uuid()),
        ar_options->ssl_mode, member_auth, cert_issuer, "");
  }
  console->print_info();

  console->print_info("* Checking transaction state of the instance...");

  clone_options->recovery_method = validate_instance_recovery(
      Member_op_action::ADD_INSTANCE, *donor_instance, *target_instance,
      *clone_options->recovery_method, get_gtid_set_is_complete(), interactive);

  DBUG_EXECUTE_IF("dba_abort_async_add_replica",
                  { throw std::logic_error("debug"); });
}

void Replica_set_impl::add_instance(
    const std::string &instance_def,
    const Async_replication_options &ar_options_,
    const Clone_options &clone_options_, const std::string &label,
    const std::string &auth_cert_subject,
    Recovery_progress_style progress_style, int sync_timeout, bool interactive,
    bool dry_run) {
  check_preconditions_and_primary_availability("addInstance");

  Async_replication_options ar_options(ar_options_);
  Clone_options clone_options(clone_options_);

  // Testing hack to set a replication delay. This should be removed
  // if/when a replication delay option is ever added.
  DBUG_EXECUTE_IF("dba_add_instance_master_delay", { ar_options.delay = 3; });

  // Connect to the target Instance.
  const Scoped_instance target_instance(connect_target_instance(instance_def));
  const auto target_uuid = target_instance->get_uuid();

  // Acquire required locks on target instance (acquired on primary after).
  // No "write" operation allowed to be executed concurrently on the target
  // instance, but the primary can be "shared" by other operations on different
  // target instances.
  auto i_lock = target_instance->get_lock_exclusive();

  // NOTE: Acquire a shared lock on the primary only if the UUID is different
  // from the target instance.
  mysqlshdk::mysql::Lock_scoped plock;
  if (target_uuid.empty() || target_uuid != get_primary_master()->get_uuid())
    plock = get_primary_master()->get_lock_shared();

  auto topology = get_topology_manager();

  auto console = current_console();
  console->print_info("Adding instance to the replicaset...");
  console->print_info();

  read_replication_options(nullptr, &ar_options);

  // recovery auth checks
  {
    auto auth_type = query_cluster_auth_type();

    // check if member auth request mode is supported
    validate_instance_member_auth_type(*target_instance, false,
                                       ar_options.ssl_mode,
                                       "replicationSslMode", auth_type);

    // check if certSubject was correctly supplied
    validate_instance_member_auth_options("replicaset", false, auth_type,
                                          auth_cert_subject);
  }

  // check repl options
  {
    auto version = target_instance->get_version();

    if ((ar_options.compression_algos.has_value() ||
         ar_options.zstd_compression_level.has_value()) &&
        !supports_repl_channel_compression(version))
      throw shcore::Exception::runtime_error(
          shcore::str_format("Instance '%s' does not support the "
                             "\"replicationCompressionAlgorithms\" or "
                             "\"replicationZstdCompressionLevel\" options.",
                             target_instance->descr().c_str()));

    if (ar_options.network_namespace.has_value() &&
        !supports_repl_channel_network_namespace(version))
      throw shcore::Exception::runtime_error(
          shcore::str_format("Instance '%s' does not support the "
                             "\"replicationNetworkNamespace\" option.",
                             target_instance->descr().c_str()));
  }

  console->print_info("* Performing validation checks");
  validate_add_instance(topology.get(), get_primary_master().get(),
                        target_instance.get(), &ar_options, &clone_options,
                        auth_cert_subject, interactive);

  console->print_info("* Updating topology");

  // Create the recovery account
  auto user = create_replication_user(target_instance.get(), auth_cert_subject,
                                      dry_run);
  ar_options.repl_credentials = user.first;

  try {
    if (*clone_options.recovery_method == Member_recovery_method::CLONE) {
      //  Pick a valid donor (unless cloneDonor is set). By default, the donor
      //  must be an ONLINE SECONDARY member. If not available, then must be the
      //  PRIMARY.
      std::string donor;
      if (clone_options.clone_donor.has_value()) {
        donor = *clone_options.clone_donor;
      } else {
        donor = pick_clone_donor(target_instance.get());
      }

      const auto donor_instance =
          Scoped_instance(connect_target_instance(donor));

      ensure_compatible_clone_donor(donor_instance, target_instance);

      // Do and monitor the clone
      Base_cluster_impl::handle_clone_provisioning(
          target_instance, donor_instance, ar_options, user.second,
          query_cluster_auth_cert_issuer(), auth_cert_subject, progress_style,
          sync_timeout, dry_run);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      target_instance->reconnect_if_needed("Target");
      m_metadata_storage->get_md_server()->reconnect_if_needed("Metadata");

      // Clone will copy all tables, including the replication settings stored
      // in mysql.slave_master_info. MySQL will start replication by default if
      // the replication setting are not empty, so in a fast system or if
      // --skip-slave-start is not enabled replication will start and the slave
      // threads will be up-and-running before we issue the new CHANGE MASTER.
      // This will result in an error: MySQL Error 3081 (HY000): (...) This
      // operation cannot be performed with running replication threads; run
      // STOP SLAVE FOR CHANNEL '' first (BUG#30632029)
      //
      // To avoid this situation, we must stop the slave and reset the
      // replication channels.
      remove_channel(*target_instance, k_replicaset_channel_name, dry_run);
    }

    // Update the global topology first, which means setting replication
    // channel to the primary in the master replica, so we can know if that
    // works. Async replication between DCs has many things that can go wrong
    // (bad address, private vs public address, network issue, account issues,
    // firewalls etc), so we do this first to keep an eventual rollback to a
    // minimum while retries would also be simpler to handle.
    async_add_replica(get_primary_master().get(), target_instance.get(),
                      k_replicaset_channel_name, ar_options, true, dry_run);

    console->print_info(
        "** Waiting for new instance to synchronize with PRIMARY...");
    if (!dry_run) {
      try {
        // Sync and check whether the slave started OK
        sync_transactions(*target_instance, {k_replicaset_channel_name},
                          sync_timeout);
      } catch (const shcore::Exception &e) {
        if (e.code() != SHERR_DBA_GTID_SYNC_TIMEOUT) throw;
        console->print_info(
            "You may increase or disable the transaction sync timeout with "
            "the timeout option for <<<addInstance>>>()");
      } catch (const cancel_sync &) {
        throw;
      }
    }

    Instance_id master_id = static_cast<const topology::Server *>(
                                topology->topology()->get_primary_master_node())
                                ->instance_id;

    log_info("Recording metadata for %s", target_instance->descr().c_str());
    if (!dry_run) {
      manage_instance(target_instance.get(), {user.first.user, user.second},
                      label, master_id, false);

      get_metadata_storage()->update_instance_attribute(
          target_instance->get_uuid(), k_instance_attribute_cert_subject,
          shcore::Value(auth_cert_subject));

      {
        MetadataStorage::Transaction trx(get_metadata_storage());

        auto update_repl_attrib = [this, &target_instance](
                                      const auto &value,
                                      std::string_view attrib_name) {
          if (!value.has_value()) return;
          get_metadata_storage()->update_instance_attribute(
              target_instance->get_uuid(), attrib_name,
              shcore::Value(value.value()));
        };

        update_repl_attrib(ar_options.connect_retry,
                           k_instance_attribute_replConnectRetry);
        update_repl_attrib(ar_options.retry_count,
                           k_instance_attribute_replRetryCount);
        update_repl_attrib(ar_options.heartbeat_period,
                           k_instance_attribute_replHeartbeatPeriod);
        update_repl_attrib(ar_options.compression_algos,
                           k_instance_attribute_replCompressionAlgorithms);
        update_repl_attrib(ar_options.zstd_compression_level,
                           k_instance_attribute_replZstdCompressionLevel);
        update_repl_attrib(ar_options.bind, k_instance_attribute_replBind);
        update_repl_attrib(ar_options.network_namespace,
                           k_instance_attribute_replNetworkNamespace);

        trx.commit();
      }
    }
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
      get_primary_master()->get_canonical_address().c_str()));

  // Wait for the new replica to catch up metadata state
  // Errors after this point don't rollback.
  if (target_instance && !dry_run && sync_timeout >= 0) {
    console->print_info(
        "* Waiting for instance '" + target_instance->descr() +
        "' to synchronize the Metadata updates with the PRIMARY...");
    sync_transactions(*target_instance, {k_replicaset_channel_name},
                      sync_timeout);
  }

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

topology::Node_status Replica_set_impl::validate_rejoin_instance(
    Global_topology_manager *topology_mng, Instance *target,
    Clone_options *clone_options, Instance_metadata *out_instance_md,
    bool interactive) {
  // Validate the Clone options.
  clone_options->check_option_values(target->get_version());

  // Check if the target instance belongs to the replicaset (MD).
  std::string target_address = target->get_canonical_address();
  try {
    *out_instance_md =
        m_metadata_storage->get_instance_by_address(target_address);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
      current_console()->print_error(
          "Cannot rejoin an instance that does not belong to the replicaset. "
          "Please confirm the specified address or execute the operation "
          "against the correct ReplicaSet object.");

      throw shcore::Exception(
          shcore::str_format("Instance %s does not belong to the replicaset",
                             target_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }
    throw;
  }

  // regular instance checks
  validate_instance(target);

  // Check instance status and consistency with the global topology
  auto status = topology_mng->validate_rejoin_replica(target);

  std::shared_ptr<Instance> donor_instance = get_cluster_server();

  current_console()->print_info(
      "** Checking transaction state of the instance...");

  clone_options->recovery_method = validate_instance_recovery(
      Member_op_action::REJOIN_INSTANCE, *donor_instance, *target,
      *clone_options->recovery_method, get_gtid_set_is_complete(), interactive);

  return status;
}

void Replica_set_impl::rejoin_instance(const std::string &instance_def,
                                       const Clone_options &clone_options_,
                                       Recovery_progress_style progress_style,
                                       int sync_timeout, bool interactive,
                                       bool dry_run) {
  log_debug("Checking rejoin instance preconditions.");

  check_preconditions_and_primary_availability("rejoinInstance");

  Clone_options clone_options(clone_options_);

  log_debug("Connecting to target instance.");
  const auto target_instance =
      Scoped_instance(connect_target_instance(instance_def));

  // Acquire required locks on target instance (already acquired on primary).
  // No "write" operation allowed to be executed concurrently on the target
  // instance, but the primary can be "shared" by other operations on different
  // target instances.
  auto i_lock = target_instance->get_lock_exclusive();

  log_debug("Setting up topology manager.");
  auto topology_mng = get_topology_manager(nullptr, true);

  log_debug("Get current PRIMARY and ensure it is healthy (updatable).");

  // NOTE: Acquire a shared lock on the primary only if the UUID is different
  // from the target instance.
  mysqlshdk::mysql::Lock_scoped plock;
  if (const auto target_uuid = target_instance->get_uuid();
      target_uuid.empty() || target_uuid != get_primary_master()->get_uuid())
    plock = get_primary_master()->get_lock_shared();

  auto console = current_console();
  console->print_info("* Validating instance...");

  Instance_metadata instance_md;
  auto instance_status =
      validate_rejoin_instance(topology_mng.get(), target_instance.get(),
                               &clone_options, &instance_md, interactive);

  if (instance_status == topology::Node_status::ONLINE) {
    bool reset_async_channel{false};
    Async_replication_options ar_options;
    read_replication_options(target_instance.get(), &ar_options,
                             &reset_async_channel);

    if (!reset_async_channel) {
      mysqlshdk::mysql::Replication_channel_master_info channel_info;
      mysqlshdk::mysql::get_channel_info(
          *target_instance, k_replicaset_channel_name, &channel_info, nullptr);

      auto options_to_update =
          async_merge_repl_options(ar_options, channel_info);

      // we don't need to reset the repl channel, so if we also don't need to
      // update any options, we can just leave
      if (!options_to_update.has_value()) {
        if (get_primary_master()->get_uuid() == target_instance->get_uuid())
          console->print_info(
              shcore::str_format("The instance '%s' is ONLINE and the primary "
                                 "of the ReplicaSet.\n",
                                 target_instance->descr().c_str()));
        else
          console->print_info(shcore::str_format(
              "The instance '%s' is ONLINE and replicating from '%s'.\n",
              target_instance->descr().c_str(),
              get_primary_master().get()->get_canonical_address().c_str()));

        if (dry_run) {
          console->print_info("dryRun finished.");
          console->print_info();
        }

        return;
      }

      // since the channel doesn't need to be reset but the options need to be
      // updated, just update the channel
      console->print_info("* Updating the replication channel...");

      if (!dry_run)
        async_change_channel_repl_options(
            target_instance, k_replicaset_channel_name, *options_to_update);

      console->print_info(shcore::str_format(
          "The replication channel of instance '%s' was updated.",
          target_instance->descr().c_str()));

      if (get_primary_master()->get_uuid() != target_instance->get_uuid())
        console->print_info(shcore::str_format(
            "The instance is replicating from '%s'.",
            get_primary_master().get()->get_canonical_address().c_str()));

      if (dry_run) {
        console->print_info("dryRun finished.");
        console->print_info();
      }

      return;
    }

    // reaching this point, the instance is valid but the replication
    // channel needs to be reset (and updated). Since we need to reset it,
    // just let the command proceed as usual, with the exception of cloning,
    // which is not needed.
    clone_options.recovery_method = Member_recovery_method::AUTO;
  }

  // Update target instance replication settings and restart replication.
  console->print_info("* Rejoining instance to replicaset...");

  DBUG_EXECUTE_IF("dba_abort_async_rejoin_replica",
                  { throw std::logic_error("debug"); });

  // NOTE: Replication user needs to be refreshed in case we are rejoining the
  // old PRIMARY from the replicaset.
  Async_replication_options ar_options;

  bool reset_async_channel{false};
  read_replication_options(target_instance.get(), &ar_options,
                           &reset_async_channel);

  std::string repl_account_host;
  std::tie(ar_options.repl_credentials, repl_account_host) =
      refresh_replication_user(target_instance.get(), dry_run);

  try {
    if (*clone_options.recovery_method == Member_recovery_method::CLONE) {
      auto cert_subject =
          query_cluster_instance_auth_cert_subject(*target_instance);

      // since we're in a rejoin, check if the value is valid
      switch (auto auth_type = query_cluster_auth_type(); auth_type) {
        case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
        case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
          if (cert_subject.empty())
            throw shcore::Exception::runtime_error(shcore::str_format(
                "The replicaset's SSL mode is set to %s but the instance being "
                "rejoined doesn't have a valid 'certSubject' option. Please "
                "stop GR on that instance and then add it back using "
                "cluster.addInstance() with the appropriate authentication "
                "options.",
                to_string(auth_type).c_str()));
          break;
        default:
          break;
      }

      //  Pick a valid donor (unless cloneDonor is set). By default, the donor
      //  must be an ONLINE SECONDARY member. If not available, then must be the
      //  PRIMARY.
      std::string donor;
      if (clone_options.clone_donor.has_value()) {
        donor = *clone_options.clone_donor;
      } else {
        donor = pick_clone_donor(target_instance.get());
      }

      const auto donor_instance =
          Scoped_instance(connect_target_instance(donor));

      ensure_compatible_clone_donor(donor_instance, target_instance);

      // Do and monitor the clone
      Base_cluster_impl::handle_clone_provisioning(
          target_instance, donor_instance, ar_options, repl_account_host,
          query_cluster_auth_cert_issuer(), cert_subject, progress_style,
          sync_timeout, dry_run);

      // When clone is used, the target instance will restart and all
      // connections are closed so we need to test if the connection to the
      // target instance and MD are closed and re-open if necessary
      target_instance->reconnect_if_needed("Target");
      m_metadata_storage->get_md_server()->reconnect_if_needed("Metadata");

      // Clone will copy all tables, including the replication settings stored
      // in mysql.slave_master_info. MySQL will start replication by default if
      // the replication setting are not empty, so in a fast system or if
      // --skip-slave-start is not enabled replication will start and the slave
      // threads will be up-and-running before we issue the new CHANGE MASTER.
      // This will result in an error: MySQL Error 3081 (HY000): (...) This
      // operation cannot be performed with running replication threads; run
      // STOP SLAVE FOR CHANNEL '' first (BUG#30632029)
      //
      // To avoid this situation, we must stop the slave and reset the
      // replication channels.
      remove_channel(*target_instance, k_replicaset_channel_name, dry_run);
      reset_channel(*target_instance, k_replicaset_channel_name, true, dry_run);

      // no need to clear the async channel again
      reset_async_channel = false;
    }

    if (!dry_run) {
      async_rejoin_replica(get_primary_master().get(), target_instance.get(),
                           k_replicaset_channel_name, ar_options,
                           reset_async_channel);

      console->print_info(
          "** Waiting for rejoined instance to synchronize with PRIMARY...");

      try {
        // Sync and check whether the slave started OK
        sync_transactions(*target_instance, {k_replicaset_channel_name},
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

    console->print_info("* Updating the Metadata...");

    // Set new instance information and store in MD.
    if (!dry_run) {
      MetadataStorage::Transaction trx(m_metadata_storage);

      if (reset_async_channel) cleanup_replication_options(*target_instance);

      instance_md.master_id =
          static_cast<const topology::Server *>(
              topology_mng->topology()->get_primary_master_node())
              ->instance_id;
      instance_md.primary_master = false;
      m_metadata_storage->record_async_member_rejoined(instance_md);

      ensure_metadata_has_server_uuid(*target_instance);

      trx.commit();
    }
  } catch (const cancel_sync &) {
    stop_channel(*target_instance, k_replicaset_channel_name, true, false);

    console->print_info();
    console->print_info("Changes successfully reverted.");

    return;
  } catch (const std::exception &e) {
    console->print_error("Error rejoining instance to replicaset: " +
                         format_active_exception());
    log_warning("While rejoining async instance: %s", e.what());

    stop_channel(*target_instance, k_replicaset_channel_name, true, false);

    console->print_error(target_instance->descr() +
                         " could not be rejoined to the replicaset");

    throw;
  }

  console->print_info(shcore::str_format(
      "The instance '%s' rejoined the replicaset and is replicating "
      "from '%s'.\n",
      target_instance->descr().c_str(),
      get_primary_master().get()->get_canonical_address().c_str()));

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
                                       std::optional<bool> force, int timeout) {
  log_debug("Checking remove instance preconditions.");

  check_preconditions_and_primary_availability("removeInstance");

  auto console = current_console();
  auto ipool = current_ipool();

  // Normalize what's given from the argument, to strip out username, pwd
  // and other garbage
  std::string instance_def = Connection_options(instance_def_).uri_endpoint();

  log_debug("Setting up topology manager.");
  auto topology = get_topology_manager();

  log_debug("Connecting to target instance.");
  Scoped_instance target_server;
  try {
    // Do not print the ERROR message here (in connect_target_instance())
    target_server =
        Scoped_instance(connect_target_instance(instance_def_, false));
  } catch (const shcore::Exception &) {
    // Check if instance belongs to the replicaset (to send a more user-friendly
    // message to users)
    bool belong_to_md = true;
    std::string target_address =
        target_server ? target_server->get_canonical_address() : instance_def;

    if (Connection_options(target_address).has_port()) {
      try {
        m_metadata_storage->get_instance_by_address(target_address);
      } catch (const shcore::Exception &err) {
        if (err.code() == SHERR_DBA_MEMBER_METADATA_MISSING) {
          belong_to_md = false;
        }
        // Ignore any error here: when checking if belongs to metadata.
      }
    } else {
      // user didn't provide a port, we cannot check if instance is in metadata
      belong_to_md = false;
    }

    if (!force.has_value() || !force.value()) {
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

  // Acquire required locks on target instance and primary.
  // No "write" operation allowed to be executed concurrently on the target
  // instance, but the primary can be "shared" by other operations on different
  // target instances.
  // NOTE: Do not lock target instance if unreachable, and skip primary if the
  //       same as the target instance.
  mysqlshdk::mysql::Lock_scoped slock, plock;
  {
    std::string target_uuid;
    if (target_server) {
      slock = target_server->get_lock_exclusive();
      target_uuid = target_server->get_uuid();
    }

    // NOTE: Acquire a shared lock on the primary only if the UUID is different
    // from the target instance.
    if (target_uuid.empty() || target_uuid != get_primary_master()->get_uuid())
      plock = get_primary_master()->get_lock_shared();
  }

  Instance_metadata md;
  bool repl_working = false;

  validate_remove_instance(
      topology.get(), get_primary_master().get(),
      target_server.get() ? target_server->get_canonical_address()
                          : instance_def,
      target_server.get(), force.value_or(false), &md, &repl_working);

  if (md.invalidated) {
    console->print_note(md.label +
                        " is invalidated, replication sync will be skipped.");
    timeout = -1;
  }

  // sync transactions before making changes (if not invalidated)
  if (target_server && repl_working && timeout >= 0) {
    try {
      console->print_info("* Waiting for instance '" + target_server->descr() +
                          "' to synchronize with the PRIMARY...");
      sync_transactions(*target_server, {k_replicaset_channel_name}, timeout);
    } catch (const shcore::Exception &e) {
      if (force.value_or(false)) {
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

  // drop user first since we need to query MD for it - this will ignore DB
  // errors
  drop_replication_user(md.uuid, target_server.get());

  // update metadata
  log_debug("Removing instance from the Metadata.");
  {
    MetadataStorage::Transaction trx(get_metadata_storage());
    m_metadata_storage->record_async_member_removed(md.cluster_id, md.id);
    trx.commit();
  }

  if (target_server.get()) {
    if (repl_working && timeout >= 0) {
      // If replication is working, sync once again so that the drop user and
      // metadata update are caught up with
      try {
        console->print_info(
            "* Waiting for instance '" + target_server->descr() +
            "' to synchronize the Metadata updates with the PRIMARY...");
        sync_transactions(*target_server, {k_replicaset_channel_name}, timeout);
      } catch (const shcore::Exception &e) {
        if (force.value_or(false)) {
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
      async_remove_replica(target_server.get(), k_replicaset_channel_name,
                           false);
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
  if (m_cluster_server->get_uuid() == md.uuid) {
    target_server_invalidated();
  }
}

void Replica_set_impl::set_primary_instance(const std::string &instance_def,
                                            uint32_t timeout, bool dry_run) {
  auto console = current_console();
  auto ipool = current_ipool();

  check_preconditions_and_primary_availability("setPrimaryInstance");

  // NOTE: Acquire an exclusive lock on the primary.
  auto plock = get_primary_master()->get_lock_exclusive();

  topology::Server_global_topology *srv_topology = nullptr;
  auto topology = get_topology_manager(&srv_topology);
  // topology->set_sync_timeout(timeout);

  const topology::Server *promoted =
      check_target_member(srv_topology, instance_def);
  if (!promoted)
    throw shcore::Exception(
        "Unable to find instance '" + instance_def + "' in the topology.",
        SHERR_DBA_ASYNC_MEMBER_TOPOLOGY_MISSING);

  const topology::Server *demoted = static_cast<const topology::Server *>(
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
  Scoped_instance_list instances(connect_all_members(0, false, &unreachable));

  if (!unreachable.empty()) {
    throw shcore::Exception("One or more instances are unreachable",
                            SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);
  }

  // Acquire required locks on all (alive) replica set instances (except the
  // primary, already acquired). No "write" operation allowed to be executed
  // concurrently on the instances.
  auto i_locks = get_instance_lock_exclusive(instances.list(),
                                             std::chrono::seconds::zero(),
                                             get_primary_master()->get_uuid());

  // another set of connections for locks
  // Note: give extra margin for the connection read timeout, so that it doesn't
  // get triggered before server-side timeouts
  Scoped_instance_list lock_instances(
      connect_all_members(timeout + 5, false, &unreachable));

  if (!unreachable.empty()) {
    throw shcore::Exception("One or more instances are unreachable",
                            SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);
  }

  std::shared_ptr<Instance> master;
  std::shared_ptr<Instance> new_master;
  {
    auto it = std::find_if(instances.list().begin(), instances.list().end(),
                           [promoted](const std::shared_ptr<Instance> &i) {
                             return i->get_uuid() ==
                                    promoted->get_primary_member()->uuid;
                           });
    if (it == instances.list().end()) {
      throw shcore::Exception::runtime_error(promoted->label +
                                             " cannot be promoted");
    }

    new_master = *it;

    it = std::find_if(instances.list().begin(), instances.list().end(),
                      [demoted](const std::shared_ptr<Instance> &i) {
                        return i->get_uuid() ==
                               demoted->get_primary_member()->uuid;
                      });
    if (it == instances.list().end()) {
      throw std::logic_error("Internal error: couldn't find primary");
    }
    master = *it;
  }
  console->print_info();

  console->print_info("* Performing validation checks");
  topology->validate_switch_primary(master.get(), new_master.get(),
                                    instances.list());
  console->print_info();

  // Pre-synchronize the promoted primary before making any changes
  // This should ensure that there's nothing left that could cause long delays
  // or timeouts (locks, replication lag etc). Once the metadata is updated,
  // the router will begin sending RW traffic to the new primary. We won't lose
  // consistency because they should fail with SRO errors, but we should try
  // to minimize the amount of time spent in that state.
  console->print_info("* Synchronizing transaction backlog at " +
                      new_master->descr());
  if (!dry_run) {
    sync_transactions(*new_master, {k_replicaset_channel_name}, timeout);
  }

  console->print_info("* Updating metadata");

  // Re-generate a new password for the master being demoted.
  Async_replication_options master_ar_options;
  read_replication_options(master.get(), &master_ar_options);

  std::string repl_account_host;
  std::tie(master_ar_options.repl_credentials, repl_account_host) =
      refresh_replication_user(master.get(), dry_run);

  // Update the metadata with the state the replicaset is supposed to be in
  log_info("Updating metadata at %s",
           m_metadata_storage->get_md_server()->descr().c_str());
  if (!dry_run) {
    MetadataStorage::Transaction trx(get_metadata_storage());
    m_metadata_storage->record_async_primary_switch(promoted->instance_id);
    trx.commit();
  }
  console->print_info();

  // Synchronize all slaves and lock all instances.
  Global_locks global_locks;
  try {
    global_locks.acquire(lock_instances.list(),
                         demoted->get_primary_member()->uuid, timeout, dry_run);
  } catch (const std::exception &e) {
    console->print_error(
        shcore::str_format("An error occurred while preparing replicaset "
                           "instances for a PRIMARY switch: %s",
                           e.what()));

    console->print_note("Reverting metadata changes");
    if (!dry_run) {
      try {
        MetadataStorage::Transaction trx(get_metadata_storage());
        m_metadata_storage->record_async_primary_switch(demoted->instance_id);
        trx.commit();
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
    do_set_primary_instance(master.get(), new_master.get(), instances.list(),
                            master_ar_options, dry_run);
  } catch (...) {
    console->print_note("Reverting metadata changes");
    if (!dry_run) {
      MetadataStorage::Transaction trx(get_metadata_storage());
      m_metadata_storage->record_async_primary_switch(demoted->instance_id);
      trx.commit();
    }
    throw;
  }
  console->print_info();

  // This will update the MD object to use the new primary
  if (!dry_run) {
    primary_instance_did_change(new_master);
    new_master->steal();
  }

  // since both async channels were cleared / reset, we can take this
  // opportunity to clear any null replication options
  if (!dry_run) {
    MetadataStorage::Transaction trx(get_metadata_storage());
    cleanup_replication_options(*master);
    cleanup_replication_options(*new_master);
    trx.commit();
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
    const std::list<std::shared_ptr<Instance>> &instances,
    const Async_replication_options &master_ar_options, bool dry_run) {
  auto console = current_console();
  shcore::Scoped_callback_list undo_list;

  try {
    // First, promote the new primary by stopping slave, unfencing it and making
    // the old primary a slave of the new one. Topology changes are reverted on
    // exception.
    async_swap_primary(master, new_master, k_replicaset_channel_name,
                       master_ar_options, &undo_list, dry_run);

    // NOTE: Skip old master, already setup previously by async_swap_primary().
    auto consume_instances =
        [this, instances_list = instances](
            mysqlshdk::mysql::IInstance **instance,
            Async_replication_options *instance_repl_options) mutable -> bool {
      assert(instance && instance_repl_options);
      if (instances_list.empty()) return false;  // no more instances

      *instance = instances_list.front().get();
      instances_list.pop_front();

      *instance_repl_options = Async_replication_options{};

      bool reset_async_channel{false};
      read_replication_options(*instance, instance_repl_options,
                               &reset_async_channel);

      return true;
    };

    async_change_primary(new_master, master, consume_instances,
                         k_replicaset_channel_name, &undo_list, dry_run);
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
    reset_channel(*new_master, k_replicaset_channel_name, true, dry_run);
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
  try {
    check_preconditions("forcePrimaryInstance");
  } catch (const shcore::Exception &e) {
    // When forcing the primary, there's no primary available
    if (e.code() != SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE) throw;
    log_debug("No PRIMARY member available: %s", e.what());
  }

  auto console = current_console();
  auto ipool = current_ipool();

  topology::Server_global_topology *srv_topology = nullptr;
  auto topology = get_topology_manager(&srv_topology);

  const topology::Server *demoted = static_cast<const topology::Server *>(
      srv_topology->get_primary_master_node());

  const topology::Server *promoted = nullptr;

  // Check if the specified target instance is available.
  if (!instance_def.empty()) {
    promoted = check_target_member(srv_topology, instance_def);

    if (promoted == demoted) {
      throw shcore::Exception(promoted->label + " is already the PRIMARY",
                              SHERR_DBA_BAD_ASYNC_PRIMARY_CANDIDATE);
    }
  }

  std::vector<Instance_metadata> instances_md =
      get_metadata_storage()->get_all_instances(get_id());
  std::list<std::shared_ptr<Instance>> instances;
  std::list<Instance_id> invalidate_ids;
  std::list<std::string> applier_off_uuids;

  {
    std::list<Instance_metadata> unreachable;

    console->print_info("* Connecting to replicaset instances");
    // give extra margin for the connection read timeout, so that it doesn't
    // get triggered before server-side timeouts
    instances = connect_all_members(timeout + 5, true, &unreachable);

    if (!unreachable.empty() && !invalidate_error_instances) {
      console->print_error(
          "Could not connect to one or more SECONDARY instances. Use the "
          "'invalidateErrorInstances' option to perform the failover anyway by "
          "skipping and invalidating unreachable instances.");
      throw shcore::Exception("One or more instances are unreachable",
                              SHERR_DBA_UNREACHABLE_INSTANCES);
    }

    for (const auto &i : unreachable) {
      console->print_note(shcore::str_format(
          "%s will be invalidated and must be removed from the replicaset.",
          i.label.c_str()));

      invalidate_ids.push_back(i.id);

      // Remove invalidated instance from the instance metadata list.
      instances_md.erase(
          std::remove_if(
              instances_md.begin(), instances_md.end(),
              [&i](const auto &i_md) { return i_md.uuid == i.uuid; }),
          instances_md.end());
    }

    console->print_info();
  }

  // Acquire required locks on all (alive) replica set instances.
  // No "write" operation allowed to be executed concurrently on the instances.
  auto i_locks = get_instance_lock_exclusive(instances);

  // Check for replication applier errors on all (online) instances, in order
  // to anticipate issues when applying retrieved transaction and try to
  // minimize the consequences of BUG#30148247.
  // NOTE: Instances with replication errors will be invalidated and skipped if
  //       invalidate_error_instances = true.
  check_replication_applier_errors(srv_topology, &instances,
                                   invalidate_error_instances, &instances_md,
                                   &invalidate_ids, &applier_off_uuids);

  // Wait for all instances to apply retrieved transactions (relay log) first.
  // NOTE: Otherwise GTID_EXECUTED set might be missing trx when checking most
  //       up-to-date instances.
  {
    // we can only wait for instances that *don't* have the applier OFF
    std::list<std::shared_ptr<Instance>> instances_wait;
    std::copy_if(instances.begin(), instances.end(),
                 std::back_inserter(instances_wait),
                 [&applier_off_uuids](const auto &i) {
                   auto it = std::find(applier_off_uuids.begin(),
                                       applier_off_uuids.end(), i->get_uuid());
                   return (it == applier_off_uuids.end());
                 });

    if (!instances_wait.empty()) {
      auto instances_wait_original = instances_wait;
      wait_all_apply_retrieved_trx(
          &instances_wait, std::chrono::seconds{timeout},
          invalidate_error_instances, &instances_md, &invalidate_ids);

      // the wait_all_apply_retrieved_trx removed instances from instances_wait
      // so we should remove them from the main instance list
      for (const auto &i_wait : instances_wait_original) {
        auto it = std::find_if(instances_wait.begin(), instances_wait.end(),
                               [&i_wait](const auto &i) {
                                 return (i_wait->get_id() == i->get_id());
                               });
        if (it != instances_wait.end()) continue;

        instances.remove_if([&i_wait](const auto &i) {
          return i->get_uuid() == i_wait->get_uuid();
        });
      }
    }
  }

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
    console->print_info(promoted->label +
                        " will be promoted to PRIMARY of the replicaset and "
                        "the former PRIMARY will be invalidated.");
    console->print_info();
  }

  if (promoted->invalidated)
    throw shcore::Exception(promoted->label + " was invalidated by a failover",
                            SHERR_DBA_ASYNC_MEMBER_INVALIDATED);

  std::shared_ptr<Instance> new_master;
  {
    auto it = std::find_if(instances.begin(), instances.end(),
                           [promoted](const std::shared_ptr<Instance> &i) {
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
    console->print_info();
  }

  try {
    console->print_info("* Promoting " + new_master->descr() +
                        " to a PRIMARY...");
    async_force_primary(new_master.get(), k_replicaset_channel_name, dry_run);
    console->print_info();

    // MD update has to happen after the failover, since there's no PRIMARY
    // before that
    console->print_info("* Updating metadata...");
    if (!dry_run) {
      new_master->steal();

      primary_instance_did_change(new_master);

      try {
        MetadataStorage::Transaction trx(get_metadata_storage());

        m_metadata_storage->record_async_primary_forced_switch(
            promoted->instance_id, invalidate_ids);

        // remove null options (since the channel will be reset)
        cleanup_replication_options(*new_master);

        trx.commit();
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
        undo_async_force_primary(target.get(), k_replicaset_channel_name,
                                 dry_run);
        // fence_instance(target.get());

        invalidate_handle();

        throw shcore::Exception("Error during failover: " + e.format(),
                                SHERR_DBA_FAILOVER_ERROR);
      }
    }
    console->print_info();

    console->print_info(shcore::str_format("%s was force-promoted to PRIMARY.",
                                           promoted->label.c_str()));
    console->print_note(
        shcore::str_format("Former PRIMARY %s is now invalidated and must be "
                           "removed from the replicaset.",
                           demoted->label.c_str()));
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_FAILOVER_ERROR)
      log_warning("While forcing primary instance: %s", e.what());
    throw;
  } catch (const std::exception &e) {
    log_warning("While forcing primary instance: %s", e.what());
    throw;
  }

  console->print_info("* Updating source of remaining SECONDARY instances");
  {
    shcore::Scoped_callback_list undo_list;
    try {
      auto consume_instances =
          [this, &instances](
              mysqlshdk::mysql::IInstance **instance,
              Async_replication_options *instance_repl_options) -> bool {
        assert(instance && instance_repl_options);
        if (instances.empty()) return false;  // no more instances

        *instance = instances.front().get();
        instances.pop_front();

        *instance_repl_options = Async_replication_options{};

        bool reset_async_channel{false};
        read_replication_options(*instance, instance_repl_options,
                                 &reset_async_channel);
        return true;
      };

      async_change_primary(new_master.get(), nullptr, consume_instances,
                           k_replicaset_channel_name, &undo_list, dry_run);

      assert(instances.empty());

    } catch (...) {
      console->print_error("Error changing replication source: " +
                           format_active_exception());

      console->print_note("Reverting replication changes");
      undo_list.call();

      throw shcore::Exception("Error during switchover",
                              SHERR_DBA_SWITCHOVER_ERROR);
    }
    undo_list.cancel();
  }

  console->print_info();

  // Clear replication configs from the promoted instance. Do it after
  // everything is done, to make reverting easier.
  reset_channel(*new_master, k_replicaset_channel_name, true, dry_run);

  console->print_info("Failover finished successfully.");
  console->print_info();

  if (dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Replica_set_impl::dissolve(const replicaset::Dissolve_options &options) {
  check_preconditions("dissolve");

  auto rs_lock = get_lock_exclusive();

  // TODO: remove these when BUG#35913824 is implemented and all other commands
  // in ReplicaSet implements replicaset-wide locks (otherwise, the previously
  // acquired replicaset lock doesn't work).
  Scoped_instance_list current_members(
      connect_all_members(0, false, nullptr, true));
  auto i_locks = get_instance_lock_exclusive(current_members.list(),
                                             std::chrono::seconds::zero());

  Topology_executor<replicaset::Dissolve>{*this}.run(options.force,
                                                     options.timeout());
}

void Replica_set_impl::rescan(const replicaset::Rescan_options &options) {
  check_preconditions_and_primary_availability("rescan");

  auto rs_lock = get_lock_exclusive();

  // TODO: remove these when BUG#35913824 is implemented and all other commands
  // in ReplicaSet implements replicaset-wide locks (otherwise, the previously
  // acquired replicaset lock doesn't work).
  Scoped_instance_list current_members(
      connect_all_members(0, false, nullptr, true));
  auto i_locks = get_instance_lock_exclusive(current_members.list(),
                                             std::chrono::seconds::zero());

  Topology_executor<replicaset::Rescan>{*this}.run(options.add_unmanaged,
                                                   options.remove_obsolete);
}

shcore::Value Replica_set_impl::describe() {
  check_preconditions_and_primary_availability("describe", false);

  return Topology_executor<replicaset::Describe>{*this}.run();
}

shcore::Value Replica_set_impl::status(int extended) {
  check_preconditions_and_primary_availability("status", false);

  return Topology_executor<replicaset::Status>{*this}.run(extended);
}

std::shared_ptr<Global_topology_manager> Replica_set_impl::get_topology_manager(
    topology::Server_global_topology **out_topology, bool deep) {
  Cluster_metadata cmd;
  if (!get_metadata_storage()->get_cluster(get_id(), &cmd))
    throw shcore::Exception("Metadata not found for replicaset " + get_name(),
                            SHERR_DBA_METADATA_MISSING);

  auto topology = topology::scan_global_topology(
      get_metadata_storage().get(), cmd, k_replicaset_channel_name, deep);

  auto gtm =
      std::make_shared<Star_global_topology_manager>(0, std::move(topology));

  if (out_topology)
    *out_topology =
        dynamic_cast<topology::Server_global_topology *>(gtm->topology());

  return gtm;
}

mysqlshdk::mysql::Lock_scoped Replica_set_impl::get_lock_shared(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::SHARED, timeout);
}

mysqlshdk::mysql::Lock_scoped Replica_set_impl::get_lock_exclusive(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout);
}

std::vector<Instance_metadata> Replica_set_impl::get_instances_from_metadata()
    const {
  return get_metadata_storage()->get_replica_set_instances(get_id());
}

/*
 * Acquire the primary.
 *
 * Find the primary and lock it if needed, ensuring the replicaset object can
 * perform update operations.
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
std::tuple<mysqlsh::dba::Instance *, mysqlshdk::mysql::Lock_scoped>
Replica_set_impl::acquire_primary_locked(mysqlshdk::mysql::Lock_mode mode,
                                         std::string_view skip_lock_uuid) {
  auto check_not_invalidated = [this](Instance *primary,
                                      Instance_metadata *out_new_primary) {
    uint64_t view_id;
    auto members =
        m_metadata_storage->get_replica_set_members(get_id(), &view_id);
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
        if (alt_view_id <= view_id) continue;

        log_info("view_id at %s is %s, target %s is %s", m.endpoint.c_str(),
                 std::to_string(alt_view_id).c_str(), primary->descr().c_str(),
                 std::to_string(view_id).c_str());

        for (const auto &am : alt_members) {
          if (!am.primary_master) continue;
          log_info("Primary according to target %s is %s", m.label.c_str(),
                   am.label.c_str());
          *out_new_primary = am;
          break;
        }

        return false;

      } catch (const std::exception &e) {
        log_warning("Could not connect to member %s: %s", m.endpoint.c_str(),
                    e.what());
      }
    }

    return true;
  };

  // Auxiliary lambda function to find the primary.
  auto find_primary = [&]() {
    auto ipool = current_ipool();
    try {
      std::shared_ptr<Instance> primary =
          ipool->connect_async_cluster_primary(get_id());

      primary->steal();
      m_primary_master = primary;
      m_metadata_storage = std::make_shared<MetadataStorage>(m_primary_master);
      ipool->set_metadata(m_metadata_storage);

      log_info("Connected to ReplicaSet PRIMARY instance '%s'",
               m_primary_master->descr().c_str());
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_ASYNC_MEMBER_INVALIDATED) {
        current_console()->print_error(
            get_name() +
            " was invalidated by a failover. Please "
            "repair it or remove it from the ReplicaSet.");
      }
      throw shcore::Exception(e.what(), SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE);
    }
  };

  // Always search for the primary (no cache, since it might have changed).
  uint64_t view_id, new_view_id;
  std::string uuid, new_uuid;
  bool primary_found = false;

  // Make sure the primary did not changed while determining it and acquiring
  // the lock on it, avoiding any race condition.
  mysqlshdk::mysql::Lock_scoped plock;

  while (!primary_found) {
    // Get the primary info (before acquiring lock);
    m_metadata_storage->get_replica_set_primary_info(get_id(), &uuid, &view_id);

    find_primary();

    // acquire the needed lock on the primary

    if (skip_lock_uuid.empty() ||
        skip_lock_uuid != m_primary_master->get_uuid()) {
      if (mode == mysqlshdk::mysql::Lock_mode::SHARED)
        plock = m_primary_master->get_lock_shared();
      else if (mode == mysqlshdk::mysql::Lock_mode::EXCLUSIVE)
        plock = m_primary_master->get_lock_exclusive();
    }

    // Get the primary info again (after acquiring lock);
    m_metadata_storage->get_replica_set_primary_info(get_id(), &new_uuid,
                                                     &new_view_id);

    if (uuid == new_uuid) {
      primary_found = true;
      continue;
    }

    // Primary changed while finding it and acquiring lock on it:
    // - Release any acquired lock on the old primary and try again.
    plock = nullptr;  // forces a lock release (if any)
  }

  // ensure that the primary isn't invalidated
  Instance_metadata new_primary;
  if (!check_not_invalidated(m_primary_master.get(), &new_primary)) {
    // Throw an exception so that the error can bubble up all the way up to the
    // top of the stack. All assumptions made by the code path up to this point
    // were based on an invalidated members view of the replicaset, which
    // is no good.
    shcore::Exception exc("Target " + m_cluster_server->descr() +
                              " was invalidated in a failover",
                          SHERR_DBA_ASYNC_MEMBER_INVALIDATED);

    exc.error()->set("new_primary_endpoint",
                     shcore::Value(new_primary.endpoint));
    throw exc;
  }

  return {m_primary_master.get(), std::move(plock)};
}

mysqlsh::dba::Instance *Replica_set_impl::acquire_primary(bool, bool) {
  // since acquire_primary_locked() has a lock mode NONE, to avoid duplicating
  // code, we can simply call it with NONE
  auto [instance, lock] =
      acquire_primary_locked(mysqlshdk::mysql::Lock_mode::NONE);

  assert(!lock);  // make sure the lock is empty

  return instance;
}

Cluster_metadata Replica_set_impl::get_metadata() const {
  Cluster_metadata cmd;
  if (!get_metadata_storage()->get_cluster(get_id(), &cmd)) {
    throw shcore::Exception(
        "ReplicaSet metadata could not be loaded for " + get_name(),
        SHERR_DBA_METADATA_MISSING);
  }
  return cmd;
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

void Replica_set_impl::ensure_metadata_has_server_uuid(
    const mysqlsh::dba::Instance &instance) {
  try {
    auto &target_uuid = instance.get_uuid();
    m_metadata_storage->get_instance_by_uuid(target_uuid);
    return;  // uuid is in metadata
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;
  }

  log_info("Updating instance '%s' server UUID in metadata.",
           instance.descr().c_str());

  auto instance_md = query_instance_info(instance, false);
  instance_md.cluster_id = get_id();
  m_metadata_storage->update_instance(instance_md);
}

void Replica_set_impl::ensure_compatible_clone_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient) {
  /*
   * A donor is compatible if:
   *
   *   - It's an ONLINE ReplicaSet member
   *   - The target (recipient) and donor instances support clone (version
   *     >= 8.0.17)
   *   - It has the same version of the recipient
   *   - It has the same operating system as the recipient
   */

  // Check if the target belongs to the ReplicaSet (MD)
  std::string target_address = donor.get_canonical_address();
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
  {
    auto topology_mng = get_topology_manager();

    auto topology_node =
        topology_mng->topology()->try_get_node_for_uuid(donor.get_uuid());
    if (!topology_node) {
      topology_node = topology_mng->topology()->try_get_node_for_endpoint(
          donor.get_canonical_address());
    }
    if (!topology_node) {
      throw shcore::Exception(
          "Unable to find instance '" + donor.descr() + "' in the topology.",
          SHERR_DBA_ASYNC_MEMBER_TOPOLOGY_MISSING);
    }

    if (topology_node->status() != topology::Node_status::ONLINE) {
      throw shcore::Exception("Instance " + target_address +
                                  " is not an ONLINE member of the ReplicaSet.",
                              SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
    }
  }

  Base_cluster_impl::ensure_compatible_clone_donor(donor, recipient);
}

std::string Replica_set_impl::pick_clone_donor(
    mysqlshdk::mysql::IInstance *recipient) {
  std::string r;
  auto topology_mng = get_topology_manager();
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
      if (recipient->get_uuid() == s->get_primary_member()->uuid) continue;

      std::string instance_def = s->get_primary_member()->endpoint;

      try {
        if (mysqlshdk::utils::Net::is_ipv6(
                mysqlshdk::utils::split_host_and_port(instance_def).first))
          throw shcore::Exception::runtime_error(
              "Instance hostname/report_host is an IPv6 address, which is "
              "not supported for cloning");

        const auto donor_instance =
            Scoped_instance(connect_target_instance(instance_def));

        ensure_compatible_clone_donor(donor_instance, *recipient);

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

  // If no secondary is suitable, use the primary
  if (r.empty()) {
    std::string instance_def = primary->get_primary_member()->endpoint;

    try {
      if (mysqlshdk::utils::Net::is_ipv6(
              mysqlshdk::utils::split_host_and_port(instance_def).first))
        throw shcore::Exception::runtime_error(
            "Instance hostname/report_host is an IPv6 address, which is "
            "not supported for cloning");

      const auto donor_instance =
          Scoped_instance(connect_target_instance(instance_def));

      ensure_compatible_clone_donor(donor_instance, *recipient);

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
    auto console = current_console();
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
      drop_replication_user(target_server->get_uuid(), target_server);
    }

    async_remove_replica(target_server, k_replicaset_channel_name, dry_run);
  } catch (const std::exception &) {
    console->print_error(
        std::string("Error while reverting replication changes: ") +
        format_active_exception());
  }
}

Member_recovery_method Replica_set_impl::validate_instance_recovery(
    Member_op_action op_action,
    const mysqlshdk::mysql::IInstance &donor_instance,
    const mysqlshdk::mysql::IInstance &target_instance,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive) {
  auto check_recoverable =
      [&donor_instance](const mysqlshdk::mysql::IInstance &tgt_instance) {
        // Get the gtid state in regards to the donor
        mysqlshdk::mysql::Replica_gtid_state state =
            mysqlshdk::mysql::check_replica_gtid_state(
                donor_instance, tgt_instance, nullptr, nullptr);

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

Instance_id Replica_set_impl::manage_instance(
    Instance *instance, const std::pair<std::string, std::string> &repl_user,
    const std::string &instance_label, Instance_id master_id, bool is_primary) {
  MetadataStorage::Transaction trx(get_metadata_storage());
  Instance_metadata inst = query_instance_info(*instance, false);

  if (!instance_label.empty()) inst.label = instance_label;
  inst.cluster_id = get_id();
  if (master_id > 0) {
    inst.master_id = master_id;
  }

  inst.primary_master = is_primary;

  inst.instance_type = Instance_type::ASYNC_MEMBER;

  auto instance_id = get_metadata_storage()->record_async_member_added(inst);

  if (!repl_user.first.empty())
    get_metadata_storage()->update_instance_repl_account(
        inst.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE,
        repl_user.first, repl_user.second);

  trx.commit();

  return instance_id;
}

const topology::Server *Replica_set_impl::check_target_member(
    topology::Server_global_topology *topology,
    const std::string &instance_def) {
  // we can't print instance_def directly because it may contain credentials
  Scoped_instance target_instance(connect_target_instance(instance_def));

  try {
    return topology->get_server(target_instance->get_uuid());
  } catch (const shcore::Exception &e) {
    std::string instance_label =
        Connection_options(instance_def).uri_endpoint();
    log_warning("%s: %s", instance_label.c_str(), e.format().c_str());

    if (e.code() == SHERR_DBA_CLUSTER_METADATA_MISSING) {
      throw shcore::Exception(
          "Target instance " + instance_label + " is not a managed instance.",
          SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
    }
    throw;
  }
}

std::list<std::shared_ptr<Instance>> Replica_set_impl::connect_all_members(
    uint32_t read_timeout, bool skip_primary,
    std::list<Instance_metadata> *out_unreachable, bool silent) {
  std::vector<Instance_metadata> instances =
      get_metadata_storage()->get_all_instances(get_id());

  auto console = current_console();
  std::list<std::shared_ptr<Instance>> r;
  auto ipool = current_ipool();

  for (const auto &i : instances) {
    if ((i.primary_master && skip_primary) || i.invalidated) continue;

    try {
      try {
        mysqlshdk::db::Connection_options opts(i.endpoint);
        ipool->default_auth_opts().set(&opts);
        // The read timeout will allow commands that block at the server but
        // have no server-side timeouts to not block the shell indefinitely.
        if (read_timeout > 0) opts.set_net_read_timeout(read_timeout * 1000);

        if (!silent)
          console->print_info(
              shcore::str_format("** Connecting to %s", i.label.c_str()));

        r.emplace_back(ipool->connect_unchecked(opts));
      }
      CATCH_AND_THROW_CONNECTION_ERROR(i.endpoint)
    } catch (const shcore::Exception &e) {
      // Client errors are likely because the server is unreachable/crashed
      if (e.is_mysql() && mysqlshdk::db::is_mysql_client_error(e.code())) {
        if (out_unreachable) out_unreachable->push_back(i);

        if (i.primary_master)
          console->print_warning(shcore::str_format(
              "Could not connect to PRIMARY instance: %s", e.format().c_str()));
        else
          console->print_warning(
              shcore::str_format("Could not connect to SECONDARY instance: %s",
                                 e.format().c_str()));
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
 * @param out_ids_applier_off List with the IDs of the instances which have the
 * applier in a state other than ON
 */
void Replica_set_impl::check_replication_applier_errors(
    topology::Server_global_topology *srv_topology,
    std::list<std::shared_ptr<Instance>> *out_online_instances,
    bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids,
    std::list<std::string> *out_ids_applier_off) const {
  auto console = current_console();

  std::list<std::string> error_uuids;

  for (const auto &instance : *out_online_instances) {
    // Get the matching instance info from the Server Global Topology.
    auto srv_info =
        srv_topology->get_server(instance->get_uuid())->get_primary_member();

    if (!srv_info->master_channel) continue;

    // Get the applier status.
    // NOTE: Ignore receiver status since it is expected to have errors
    //       if the primary failed and some expected receiver status
    //       (e.g., CONNECTION_ERROR) have precedence over other applier
    //       status (e.g., APPLIER_OFF).
    mysqlshdk::mysql::Replication_channel channel =
        srv_info->master_channel->info;
    mysqlshdk::mysql::Replication_channel::Status applier_status =
        channel.applier_status();

    if (out_ids_applier_off &&
        (applier_status != mysqlshdk::mysql::Replication_channel::Status::ON)) {
      auto it = std::find_if(out_instances_md->begin(), out_instances_md->end(),
                             [&instance](const Instance_metadata &i_md) {
                               return i_md.uuid == instance->get_uuid();
                             });

      if (it != out_instances_md->end())
        out_ids_applier_off->push_back(it->uuid);
    }

    // Issue error if the replication applier has errors.
    if (applier_status ==
        mysqlshdk::mysql::Replication_channel::Status::APPLIER_ERROR) {
      for (const auto &applier : channel.appliers) {
        if (applier.last_error.code == 0) continue;

        std::string err_msg{"Replication applier error at " + srv_info->label +
                            ": " +
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
      error_uuids.push_back(srv_info->uuid);
    }
  }

  if (error_uuids.empty()) return;

  if (!invalidate_error_instances) {
    console->print_error(
        "Replication errors found for one or more SECONDARY instances. Use "
        "the 'invalidateErrorInstances' option to perform the failover "
        "anyway by skipping and invalidating instances with errors.");
    throw shcore::Exception(
        "One or more instances have replication applier errors.",
        SHERR_DBA_REPLICATION_APPLIER_ERROR);
  }

  console->print_info();

  // Update instances lists according to invalidated instances.
  for (const auto &uuid : error_uuids) {
    // Add instance to invalidate to list.
    {
      auto it = std::find_if(
          out_instances_md->begin(), out_instances_md->end(),
          [&uuid](const Instance_metadata &i_md) { return i_md.uuid == uuid; });
      if (it != out_instances_md->end()) {
        out_invalidate_ids->push_back(it->id);
        out_instances_md->erase(it);
      }
    }

    // Remove instance to invalidate from lists.
    out_online_instances->remove_if(
        [&uuid](const std::shared_ptr<Instance> &i) {
          return i->get_uuid() == uuid;
        });
  }
}

shcore::Value Replica_set_impl::list_routers(bool only_upgrade_required) {
  try {
    check_preconditions("listRouters");
  } catch (const shcore::Exception &e) {
    // The primary might not be available, but listRouters() must work anyway
    if (e.code() == SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE) {
      current_console()->print_warning(e.format());
    } else {
      throw;
    }
  }

  shcore::Value r = Base_cluster_impl::list_routers(only_upgrade_required);

  (*r.as_map())["replicaSetName"] = shcore::Value(get_name());

  return r;
}

void Replica_set_impl::remove_router_metadata(const std::string &router) {
  check_preconditions_and_primary_availability("removeRouterMetadata");

  // Acquire a shared lock on the primary. The metadata instance (primary)
  // can be "shared" by other operations executing concurrently on other
  // instances.
  auto primary = get_primary_master();
  auto plock = primary->get_lock_shared();

  Base_cluster_impl::remove_router_metadata(router);
}

void Replica_set_impl::setup_admin_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  check_preconditions_and_primary_availability("setupAdminAccount");

  Base_cluster_impl::setup_admin_account(username, host, options);
}

void Replica_set_impl::setup_router_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  check_preconditions_and_primary_availability("setupRouterAccount");

  Base_cluster_impl::setup_router_account(username, host, options);
}

std::pair<mysqlshdk::mysql::Auth_options, std::string>
Replica_set_impl::refresh_replication_user(mysqlshdk::mysql::IInstance *slave,
                                           bool dry_run) {
  assert(m_primary_master);

  mysqlshdk::mysql::Auth_options creds;

  auto account = get_metadata_storage()->get_instance_repl_account(
      slave->get_uuid(), Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);
  if (account.first.empty()) {
    account = {make_replication_user_name(slave->get_server_id(),
                                          k_async_cluster_user_name),
               "%"};
  }
  creds.user = account.first;

  try {
    // Create replication accounts for this instance at the master
    // replicaset unless the user provided one.

    auto console = mysqlsh::current_console();

    log_info("Resetting password for %s@%s at %s", creds.user.c_str(),
             account.second.c_str(), m_primary_master->descr().c_str());
    // re-create replication with a new generated password
    if (!dry_run) {
      std::string repl_password;
      mysqlshdk::mysql::set_random_password(*m_primary_master, creds.user,
                                            {account.second}, &repl_password);
      creds.password = repl_password;
    }
  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while resetting password for replication account: %s",
        e.what()));
  }

  return {creds, account.second};
}

void Replica_set_impl::drop_replication_user(
    const std::string &server_uuid, mysqlshdk::mysql::IInstance *slave) {
  assert(m_primary_master);

  auto account = get_metadata_storage()->get_instance_repl_account(
      server_uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);

  if (account.first.empty()) {
    if (slave) {
      account = {make_replication_user_name(slave->get_server_id(),
                                            k_async_cluster_user_name),
                 ""};
    } else {
      // If the instance is unreachable and the replication account info is not
      // stored in the MD schema we cannot attempt to obtain the instance's
      // server_id to determine the account username. In this particular
      // scenario, we must just log that the account couldn't be removed
      log_info(
          "Unable to drop instance's replication account from the ReplicaSet: "
          "Instance '%s' is unreachable, unable to determine its replication "
          "account.",
          server_uuid.c_str());
    }
  }

  log_info("Dropping account %s@%s at %s", account.first.c_str(),
           account.second.c_str(), m_primary_master->descr().c_str());
  try {
    if (account.second.empty())
      mysqlshdk::mysql::drop_all_accounts_for_user(*m_primary_master,
                                                   account.first);
    else
      m_primary_master->drop_user(account.first, account.second, true);
  } catch (const shcore::Error &e) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%s: %s",
        m_primary_master->descr().c_str(), account.first.c_str(),
        account.second.c_str(), e.format().c_str()));
    // ignore the error and move on
  }
}

void Replica_set_impl::drop_all_replication_users() {
  auto primary = get_primary_master();
  assert(primary);

  std::vector<std::string> accounts_to_drop;
  {
    auto result = primary->queryf(
        "SELECT DISTINCT user from mysql.user WHERE user LIKE ?",
        shcore::str_format("%s%%", k_async_cluster_user_name).c_str());

    while (auto row = result->fetch_one())
      accounts_to_drop.push_back(row->get_string(0));
  }

  for (const auto &account : accounts_to_drop) {
    log_info("Removing account '%s'", account.c_str());
    try {
      mysqlshdk::mysql::drop_all_accounts_for_user(*primary, account);
    } catch (const std::exception &e) {
      mysqlsh::current_console()->print_warning(
          shcore::str_format("Error dropping accounts for user '%s': %s",
                             account.c_str(), e.what()));
    }
  }
}

std::pair<mysqlshdk::mysql::Auth_options, std::string>
Replica_set_impl::create_replication_user(mysqlshdk::mysql::IInstance *slave,
                                          std::string_view auth_cert_subject,
                                          bool dry_run,
                                          mysqlshdk::mysql::IInstance *master) {
  if (!master) master = m_primary_master.get();

  std::string host = "%";
  if (shcore::Value allowed_host;
      !dry_run &&
      get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_replication_allowed_host,
          &allowed_host) &&
      allowed_host.get_type() == shcore::String &&
      !allowed_host.as_string().empty()) {
    host = allowed_host.as_string();
  }

  mysqlshdk::mysql::Auth_options creds;
  creds.user = make_replication_user_name(slave->get_server_id(),
                                          k_async_cluster_user_name);

  try {
    // Create replication accounts for this instance at the master
    // replicaset unless the user provided one.

    // Accounts are created at the master replicaset regardless of who will use
    // them, since they'll get replicated everywhere.

    log_info("Creating replication user %s@%s at %s%s", creds.user.c_str(),
             host.c_str(), master->descr().c_str(), dry_run ? " (dryRun)" : "");

    if (dry_run) return {creds, host};

    // re-create replication with a new generated password

    // Check if the replication user already exists and delete it if it does,
    // before creating it again.
    mysqlshdk::mysql::drop_all_accounts_for_user(*master, creds.user);

    mysqlshdk::mysql::IInstance::Create_user_options user_options;
    user_options.grants.push_back({"REPLICATION SLAVE", "*.*", false});

    auto auth_type = query_cluster_auth_type();

    // setup password
    bool requires_password{false};
    switch (auth_type) {
      case Replication_auth_type::PASSWORD:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        requires_password = true;
        break;
      default:
        break;
    }

    // setup cert issuer and/or subject
    switch (auth_type) {
      case Replication_auth_type::CERT_SUBJECT:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        user_options.cert_subject = auth_cert_subject;
        [[fallthrough]];
      case Replication_auth_type::CERT_ISSUER:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
        user_options.cert_issuer = query_cluster_auth_cert_issuer();
        break;
      default:
        break;
    }

    if (requires_password) {
      std::string repl_password;
      mysqlshdk::mysql::create_user_with_random_password(
          *master, creds.user, {host}, user_options, &repl_password);

      creds.password = std::move(repl_password);
    } else {
      mysqlshdk::mysql::create_user(*master, creds.user, {host}, user_options);
    }

  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while setting up replication account: %s", e.what()));
  }

  return {creds, host};
}

shcore::Dictionary_t Replica_set_impl::get_topology_options() {
  shcore::Dictionary_t ret = shcore::make_dict();

  // Get the topology
  auto topo = topology::scan_global_topology(get_metadata_storage().get(),
                                             get_metadata(),
                                             k_replicaset_channel_name, true);

  for (const topology::Server &server :
       static_cast<topology::Server_global_topology *>(topo.get())->servers()) {
    const topology::Instance *instance = server.get_primary_member();

    if (!instance->connect_error.empty()) {
      shcore::Dictionary_t connect_error = shcore::make_dict();
      (*connect_error)["connectError"] = shcore::Value(instance->connect_error);
      ret->set(server.label, shcore::Value(std::move(connect_error)));
      continue;
    }

    shcore::Array_t array = shcore::make_array();

    // Get the parallel-appliers options
    for (const auto &[name, value] : instance->parallel_appliers) {
      shcore::Dictionary_t option = shcore::make_dict();
      (*option)["variable"] = shcore::Value(name);
      (*option)["value"] =
          value.has_value() ? shcore::Value(*value) : shcore::Value::Null();
      array->push_back(shcore::Value(std::move(option)));
    }

    // cert_subject
    {
      auto cert_subject =
          query_cluster_instance_auth_cert_subject(instance->uuid);

      shcore::Dictionary_t option = shcore::make_dict();
      (*option)["option"] = shcore::Value(kCertSubject);
      (*option)["value"] = shcore::Value(std::move(cert_subject));

      array->push_back(shcore::Value(std::move(option)));
    }

    // replication options
    {
      std::array<std::tuple<std::string_view, std::string_view>, 7> options{
          std::make_tuple(k_instance_attribute_replConnectRetry,
                          kReplicationConnectRetry),
          std::make_tuple(k_instance_attribute_replRetryCount,
                          kReplicationRetryCount),
          std::make_tuple(k_instance_attribute_replHeartbeatPeriod,
                          kReplicationHeartbeatPeriod),
          std::make_tuple(k_instance_attribute_replCompressionAlgorithms,
                          kReplicationCompressionAlgorithms),
          std::make_tuple(k_instance_attribute_replZstdCompressionLevel,
                          kReplicationZstdCompressionLevel),
          std::make_tuple(k_instance_attribute_replBind, kReplicationBind),
          std::make_tuple(k_instance_attribute_replNetworkNamespace,
                          kReplicationNetworkNamespace)};

      for (const auto &[option_attrib, option_name] : options) {
        if ((option_name == kReplicationCompressionAlgorithms) &&
            (instance->version < mysqlshdk::utils::Version(8, 0, 18)))
          continue;
        if ((option_name == kReplicationZstdCompressionLevel) &&
            (instance->version < mysqlshdk::utils::Version(8, 0, 18)))
          continue;
        if ((option_name == kReplicationNetworkNamespace) &&
            (instance->version < mysqlshdk::utils::Version(8, 0, 22)))
          continue;

        shcore::Value option_variable;
        if (!get_metadata_storage()->query_instance_attribute(
                instance->uuid, option_attrib, &option_variable))
          option_variable = shcore::Value::Null();

        auto option_value = shcore::Value::Null();
        if (!server.primary_master) {
          if (option_name == kReplicationConnectRetry)
            option_value = shcore::Value(
                instance->master_channel->master_info.connect_retry);
          else if (option_name == kReplicationRetryCount)
            option_value = shcore::Value(
                instance->master_channel->master_info.retry_count);
          else if (option_name == kReplicationHeartbeatPeriod)
            option_value = shcore::Value(
                instance->master_channel->master_info.heartbeat_period);
          else if (option_name == kReplicationCompressionAlgorithms)
            option_value =
                shcore::Value(instance->master_channel->master_info
                                  .compression_algorithm.value_or(""));
          else if (option_name == kReplicationZstdCompressionLevel)
            option_value =
                shcore::Value(instance->master_channel->master_info
                                  .zstd_compression_level.value_or(0));
          else if (option_name == kReplicationBind)
            option_value =
                shcore::Value(instance->master_channel->master_info.bind);
          else if (option_name == kReplicationNetworkNamespace)
            option_value = shcore::Value(instance->master_channel->master_info
                                             .network_namespace.value_or(""));
        }

        shcore::Dictionary_t option = shcore::make_dict();
        (*option)["option"] =
            shcore::Value("replication" + std::string{option_name});
        (*option)["value"] = std::move(option_value);
        (*option)["variable"] = std::move(option_variable);

        array->push_back(shcore::Value(std::move(option)));
      }
    }

    ret->set(server.label, shcore::Value(std::move(array)));
  }

  return ret;
}

shcore::Value Replica_set_impl::options() {
  try {
    check_preconditions("options");
  } catch (const shcore::Exception &e) {
    // The primary might not be available, but options() must work anyway
    if (e.code() != SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE) throw;
    current_console()->print_warning(e.format());
  }

  shcore::Dictionary_t inner_dict = shcore::make_dict();
  (*inner_dict)["name"] = shcore::Value(get_name());

  // get the topology options
  (*inner_dict)["topology"] = shcore::Value(get_topology_options());

  // Get the instance options

  // get the tags
  (*inner_dict)[kTags] = Base_cluster_impl::get_cluster_tags();

  // read replica attributes
  {
    std::array<std::tuple<std::string_view, std::string_view>, 4> attribs{
        std::make_tuple(k_cluster_attribute_replication_allowed_host,
                        kReplicationAllowedHost),
        std::make_tuple(k_cluster_attribute_member_auth_type, kMemberAuthType),
        std::make_tuple(k_cluster_attribute_cert_issuer, kCertIssuer),
        std::make_tuple(k_replica_set_attribute_ssl_mode, kReplicationSslMode)};

    shcore::Array_t options = shcore::make_array();
    for (const auto &[attrib_name, attrib_desc] : attribs) {
      shcore::Value attrib_value;
      if (!get_metadata_storage()->query_cluster_attribute(
              get_id(), attrib_name, &attrib_value))
        attrib_value = shcore::Value::Null();

      options->emplace_back(
          shcore::Value(shcore::make_dict("option", shcore::Value(attrib_desc),
                                          "value", std::move(attrib_value))));
    }

    (*inner_dict)["globalOptions"] = shcore::Value(std::move(options));
  }

  shcore::Dictionary_t res = shcore::make_dict();
  (*res)["replicaSet"] = shcore::Value(std::move(inner_dict));
  return shcore::Value(std::move(res));
}

void Replica_set_impl::_set_instance_option(const std::string &instance_def,
                                            const std::string &option,
                                            const shcore::Value &value) {
  std::string target_uuid;
  std::string target_descr;
  mysqlshdk::utils::Version target_version;
  {
    const auto target_instance =
        Scoped_instance(connect_target_instance(instance_def));

    target_uuid = target_instance->get_uuid();
    target_descr = target_instance->descr();
    target_version = target_instance->get_version();
  }

  constexpr std::string_view ReplicationPrefix{"replication"};
  if (shcore::str_beginswith(option, ReplicationPrefix)) {
    auto check_value_type = [&option, &value](shcore::Value_type type,
                                              bool supports_null) {
      if (value.get_type() == type) return;
      if (supports_null && (value.get_type() == shcore::Null)) return;

      // for convenience
      if (type == shcore::Float && (value.get_type() == shcore::Integer))
        return;

      if (type == shcore::Integer)
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' only accepts an integer value%s.", option.c_str(),
            supports_null ? " or null" : ""));

      if (type == shcore::Float)
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' only accepts a numeric value%s.", option.c_str(),
            supports_null ? " or null" : ""));

      if (type == shcore::String)
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' only accepts a string value%s.", option.c_str(),
            supports_null ? " or null" : ""));

      if (type == shcore::Null)
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' only accepts a null value.", option.c_str()));

      throw shcore::Exception::argument_error(
          shcore::str_format("Option '%s' doesn't support %s values.",
                             option.c_str(), type_name(type).c_str()));
    };

    auto option_name =
        std::string_view(option).substr(ReplicationPrefix.size());

    bool instance_is_primary{false};
    {
      topology::Server_global_topology *srv_topology = nullptr;
      auto topology = get_topology_manager(&srv_topology);

      instance_is_primary = (srv_topology->get_server(target_uuid)->role() ==
                             topology::Node_role::PRIMARY);
    }

    bool known_option{false};

    if (shcore::str_endswith(option_name, kReplicationConnectRetry)) {
      check_value_type(shcore::Integer, !instance_is_primary);

      if ((value.get_type() == shcore::Integer) && (value.as_int() < 0))
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' doesn't support negative values.", option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replConnectRetry, value, true);
    }

    if (shcore::str_endswith(option_name, kReplicationRetryCount)) {
      check_value_type(shcore::Integer, !instance_is_primary);

      if ((value.get_type() == shcore::Integer) && (value.as_int() < 0))
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' doesn't support negative values.", option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replRetryCount, value, true);
    }

    if (shcore::str_endswith(option_name, kReplicationHeartbeatPeriod)) {
      check_value_type(shcore::Float, !instance_is_primary);

      if (((value.get_type() == shcore::Integer) ||
           (value.get_type() == shcore::Float)) &&
          (value.as_double() < 0.0))
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' doesn't support negative values.", option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replHeartbeatPeriod, value, true);
    }

    if (shcore::str_endswith(option_name, kReplicationCompressionAlgorithms)) {
      check_value_type(shcore::String, !instance_is_primary);

      if (target_version < mysqlshdk::utils::Version(8, 0, 18))
        throw shcore::Exception::runtime_error(shcore::str_format(
            "Instance '%s' does not support the \"%s\" option.",
            target_descr.c_str(), option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replCompressionAlgorithms, value,
          true);
    }

    if (shcore::str_endswith(option_name, kReplicationZstdCompressionLevel)) {
      check_value_type(shcore::Integer, !instance_is_primary);

      if (target_version < mysqlshdk::utils::Version(8, 0, 18))
        throw shcore::Exception::runtime_error(shcore::str_format(
            "Instance '%s' does not support the \"%s\" option.",
            target_descr.c_str(), option.c_str()));

      if ((value.get_type() == shcore::Integer) && (value.as_int() < 0))
        throw shcore::Exception::argument_error(shcore::str_format(
            "Option '%s' doesn't support negative values.", option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replZstdCompressionLevel, value,
          true);
    }

    if (shcore::str_endswith(option_name, kReplicationBind)) {
      check_value_type(shcore::String, !instance_is_primary);

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replBind, value, true);
    }

    if (shcore::str_endswith(option_name, kReplicationNetworkNamespace)) {
      check_value_type(shcore::String, !instance_is_primary);

      if (target_version < mysqlshdk::utils::Version(8, 0, 22))
        throw shcore::Exception::runtime_error(shcore::str_format(
            "Instance '%s' does not support the \"%s\" option.",
            target_descr.c_str(), option.c_str()));

      known_option = true;
      get_metadata_storage()->update_instance_attribute(
          target_uuid, k_instance_attribute_replNetworkNamespace, value, true);
    }

    if (known_option) {
      auto console = current_console();
      if (value.get_type() == shcore::Null)
        console->print_info(shcore::str_format(
            "Successfully set the value of '%s' to NULL for the cluster "
            "member: '%s'.",
            option.c_str(), target_descr.c_str()));
      else
        console->print_info(shcore::str_format(
            "Successfully set the value of '%s' to '%s' for the cluster "
            "member: '%s'.",
            option.c_str(), value.descr().c_str(), target_descr.c_str()));

      if (instance_is_primary) {
        console->print_warning(
            "The new value won't take effect while the instance remains the "
            "primary instance of the ReplicaSet.");
      } else {
        console->print_warning(
            "The new value won't take effect until <<<rejoinInstance>>>() is "
            "called on the instance.");
      }

      return;
    }
  }

  throw shcore::Exception::argument_error(
      shcore::str_format("Option '%s' not supported.", option.c_str()));
}

void Replica_set_impl::update_replication_allowed_host(
    const std::string &host) {
  for (const Instance_metadata &instance : get_instances_from_metadata()) {
    auto account = get_metadata_storage()->get_instance_repl_account(
        instance.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);
    bool maybe_adopted = false;

    if (account.first.empty()) {
      account = {make_replication_user_name(instance.server_id,
                                            k_async_cluster_user_name),
                 "%"};
      maybe_adopted = true;
    }

    if (account.second != host) {
      log_info("Re-creating account for %s: %s@%s -> %s@%s",
               instance.endpoint.c_str(), account.first.c_str(),
               account.second.c_str(), account.first.c_str(), host.c_str());
      clone_user(*get_primary_master(), account.first, account.second,
                 account.first, host);

      get_primary_master()->drop_user(account.first, account.second, true);

      get_metadata_storage()->update_instance_repl_account(
          instance.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE,
          account.first, host);
    } else {
      log_info("Skipping account recreation for %s: %s@%s == %s@%s",
               instance.endpoint.c_str(), account.first.c_str(),
               account.second.c_str(), account.first.c_str(), host.c_str());
    }

    if (maybe_adopted) {
      auto ipool = current_ipool();

      Scoped_instance target(
          ipool->connect_unchecked_endpoint(instance.endpoint));

      // If the replicaset could have been adopted, then also ensure the channel
      // is using the right account
      std::string repl_user = mysqlshdk::mysql::get_replication_user(
          *target, k_replicaset_channel_name);
      if (repl_user != account.first && !instance.primary_master) {
        current_console()->print_info(shcore::str_format(
            "Changing replication user at %s to %s", instance.endpoint.c_str(),
            account.first.c_str()));

        std::string password;
        mysqlshdk::mysql::set_random_password(*get_primary_master(),
                                              account.first, {host}, &password);

        mysqlshdk::mysql::stop_replication_receiver(*target,
                                                    k_replicaset_channel_name);

        mysqlshdk::mysql::Replication_credentials_options options;
        options.password = password;

        mysqlshdk::mysql::change_replication_credentials(
            *target, k_replicaset_channel_name, account.first, options);

        mysqlshdk::mysql::start_replication_receiver(*target,
                                                     k_replicaset_channel_name);
      }
    }
  }
}

void Replica_set_impl::_set_option(const std::string &option,
                                   const shcore::Value &value) {
  if (option != kReplicationAllowedHost)
    throw shcore::Exception::argument_error("Option '" + option +
                                            "' not supported.");

  if (value.get_type() != shcore::String || value.as_string().empty())
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for '%s': Argument #2 is expected "
                           "to be a string.",
                           option.c_str()));

  update_replication_allowed_host(value.as_string());

  get_metadata_storage()->update_cluster_attribute(
      get_id(), k_cluster_attribute_replication_allowed_host, value);

  current_console()->print_info(shcore::str_format(
      "Internally managed replication users updated for ReplicaSet '%s'",
      get_name().c_str()));
}

void Replica_set_impl::check_preconditions_and_primary_availability(
    const std::string &function_name, bool throw_if_primary_unavailable) {
  try {
    check_preconditions(function_name);
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE) throw;

    auto console = mysqlsh::current_console();

    auto err = shcore::str_format(
        "Unable to connect to the PRIMARY of the ReplicaSet '%s': %s",
        get_name().c_str(), e.format().c_str());

    if (throw_if_primary_unavailable) {
      console->print_error(err);
    } else {
      console->print_warning(err);
    }

    console->print_info(
        "ReplicaSet change operations will not be possible unless the PRIMARY "
        "can be reached.");
    console->print_info(
        "If the PRIMARY is unavailable, you must either repair it or perform a "
        "failover.");
    console->print_info(
        "See \\help <<<forcePrimaryInstance>>> for more information.");

    if (throw_if_primary_unavailable) {
      throw shcore::Exception("PRIMARY instance is unavailable", e.code());
    }
  }
}

mysqlshdk::mysql::Lock_scoped Replica_set_impl::get_lock(
    mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout) {
  if (!m_cluster_server->is_lock_service_installed()) {
    bool lock_service_installed = false;
    // if SRO is disabled, we have a chance to install the lock service
    if (bool super_read_only =
            m_cluster_server->get_sysvar_bool("super_read_only", false);
        !super_read_only) {
      // we must disable log_bin to prevent the installation from being
      // replicated
      mysqlshdk::mysql::Suppress_binary_log nobinlog(m_cluster_server.get());

      lock_service_installed =
          m_cluster_server->ensure_lock_service_is_installed(false);
    }

    if (!lock_service_installed) {
      log_warning(
          "The required MySQL Locking Service isn't installed on instance "
          "'%s'. The operation will continue without concurrent execution "
          "protection.",
          m_cluster_server->descr().c_str());
      return nullptr;
    }
  }

  DBUG_EXECUTE_IF("dba_locking_timeout_one",
                  { timeout = std::chrono::seconds{1}; });

  // Try to acquire the specified lock.
  //
  // NOTE: Only one lock per namespace is used because lock release is
  // performed by namespace.
  try {
    log_debug("Acquiring %s lock ('%.*s', '%.*s') on '%s'.",
              mysqlshdk::mysql::to_string(mode).c_str(),
              static_cast<int>(k_lock_ns.size()), k_lock_ns.data(),
              static_cast<int>(k_lock_name.size()), k_lock_name.data(),
              m_cluster_server->descr().c_str());
    mysqlshdk::mysql::get_lock(*m_cluster_server, k_lock_ns, k_lock_name, mode,
                               timeout.count());
  } catch (const shcore::Error &err) {
    // Abort the operation in case the required lock cannot be acquired.
    log_info("Failed to get %s lock ('%.*s', '%.*s') on '%s': %s",
             mysqlshdk::mysql::to_string(mode).c_str(),
             static_cast<int>(k_lock_ns.size()), k_lock_ns.data(),
             static_cast<int>(k_lock_name.size()), k_lock_name.data(),
             m_cluster_server->descr().c_str(), err.what());

    if (err.code() == ER_LOCKING_SERVICE_TIMEOUT) {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "ReplicaSet lock through primary member '%s'. Another operation "
          "requiring access to the member is still in progress, please wait "
          "for it to finish and try again.",
          m_cluster_server->descr().c_str()));
      throw shcore::Exception(
          shcore::str_format(
              "Failed to acquire ReplicaSet lock through primary member '%s'",
              m_cluster_server->descr().c_str()),
          SHERR_DBA_LOCK_GET_FAILED);
    } else {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "ReplicaSet lock through primary member '%s': %s",
          m_cluster_server->descr().c_str(), err.what()));

      throw;
    }
  }

  auto release_cb = [instance = m_cluster_server]() {
    // Release all instance locks in the k_lock_ns namespace.
    //
    // NOTE: Only perform the operation if the lock service is
    // available, otherwise do nothing (ignore if concurrent execution is not
    // supported, e.g., lock service plugin not available).
    try {
      log_debug("Releasing locks for '%.*s' on %s.",
                static_cast<int>(k_lock_ns.size()), k_lock_ns.data(),
                instance->descr().c_str());
      mysqlshdk::mysql::release_lock(*instance, k_lock_ns);

    } catch (const shcore::Error &error) {
      // Ignore any error trying to release locks (e.g., might have not
      // been previously acquired due to lack of permissions).
      log_error("Unable to release '%.*s' locks for '%s': %s",
                static_cast<int>(k_lock_ns.size()), k_lock_ns.data(),
                instance->descr().c_str(), error.what());
    }
  };

  return mysqlshdk::mysql::Lock_scoped{std::move(release_cb)};
}

}  // namespace mysqlsh::dba
