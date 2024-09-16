/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster_set/cluster_set_impl.h"

#include <algorithm>
#include <exception>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "adminapi/common/metadata_storage.h"
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster/create_cluster_set.h"
#include "modules/adminapi/cluster_set/create_replica_cluster.h"
#include "modules/adminapi/cluster_set/status.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/async_utils.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/undo.h"
#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/libs/mysql/binlog_utils.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "scripting/types.h"

namespace mysqlsh::dba {

namespace {
constexpr int k_cluster_set_master_connect_retry = 3;
constexpr int k_cluster_set_master_retry_count = 10;

constexpr int64_t k_primary_weight_default = 80;
constexpr int64_t k_secondary_weight_default = 60;

// Constants with the names used to lock the cluster set
constexpr char k_lock_ns[] = "AdminAPI_clusterset";
constexpr char k_lock_name[] = "AdminAPI_lock";

void cleanup_replication_options(MetadataStorage *metadata,
                                 const Cluster_id &cluster_id) {
  auto remove_attrib = [&metadata, &cluster_id](std::string_view attrib_name) {
    shcore::Value value;
    if (!metadata->query_cluster_attribute(cluster_id, attrib_name, &value))
      return;

    if (value.get_type() == shcore::Null)
      metadata->remove_cluster_attribute(cluster_id, attrib_name);
  };

  remove_attrib(k_cluster_attribute_repl_connect_retry);
  remove_attrib(k_cluster_attribute_repl_retry_count);
  remove_attrib(k_cluster_attribute_repl_heartbeat_period);
  remove_attrib(k_cluster_attribute_repl_compression_algorithms);
  remove_attrib(k_cluster_attribute_repl_zstd_compression_level);
  remove_attrib(k_cluster_attribute_repl_bind);
  remove_attrib(k_cluster_attribute_repl_network_namespace);
}

}  // namespace

Cluster_set_impl::Cluster_set_impl(
    const std::string &domain_name,
    const std::shared_ptr<Instance> &target_instance,
    const std::shared_ptr<MetadataStorage> &metadata,
    Global_topology_type topology_type)
    : Base_cluster_impl(domain_name, target_instance, metadata),
      m_topology_type(topology_type) {}

Cluster_set_impl::Cluster_set_impl(
    const Cluster_set_metadata &csmd,
    const std::shared_ptr<Instance> &target_instance,
    const std::shared_ptr<MetadataStorage> &metadata)
    : Base_cluster_impl(csmd.domain_name, target_instance, metadata),
      m_topology_type(Global_topology_type::SINGLE_PRIMARY_TREE) {
  m_id = csmd.id;
}

void Cluster_set_impl::disconnect() {
  Base_cluster_impl::disconnect();

  m_primary_cluster.reset();
}

shcore::Value Cluster_set_impl::create_cluster_set(
    Cluster_impl *cluster, const std::string &domain_name,
    const clusterset::Create_cluster_set_options &options) {
  // Create the Create_cluster_set command and execute it.
  clusterset::Create_cluster_set op_create_cluster_set(cluster, domain_name,
                                                       options);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_create_cluster_set]() { op_create_cluster_set.finish(); });

  // Prepare the Create_cluster_set command execution (validations).
  op_create_cluster_set.prepare();

  // Execute Create_cluster_set operations.
  return op_create_cluster_set.execute();
}

std::pair<std::string, std::string> Cluster_set_impl::get_cluster_repl_account(
    Cluster_impl *cluster) const {
  return get_metadata_storage()->get_cluster_repl_account(cluster->get_id());
}

std::pair<mysqlshdk::mysql::Auth_options, std::string>
Cluster_set_impl::create_cluster_replication_user(
    Instance *cluster_primary, const std::string &account_host,
    Replication_auth_type member_auth_type, const std::string &auth_cert_issuer,
    const std::string &auth_cert_subject, bool dry_run) {
  // we use server_id as an arbitrary unique number within the clusterset. Once
  // the user is created, no relationship between the server_id and the number
  // should be assumed. The only way to obtain the replication user name for a
  // cluster is through the metadata.
  auto repl_user = make_replication_user_name(
      cluster_primary->get_server_id(), k_cluster_set_async_user_name, true);
  std::string repl_host = "%";

  if (account_host.empty()) {
    shcore::Value allowed_host;
    if (get_metadata_storage()->query_cluster_set_attribute(
            get_id(), k_cluster_attribute_replication_allowed_host,
            &allowed_host) &&
        allowed_host.get_type() == shcore::String &&
        !allowed_host.as_string().empty()) {
      repl_host = allowed_host.as_string();
    }
  } else {
    repl_host = account_host;
  }

  log_info("Creating async replication account '%s'@'%s' for new cluster at %s",
           repl_user.c_str(), repl_host.c_str(),
           cluster_primary->descr().c_str());

  auto primary = get_metadata_storage()->get_md_server();

  mysqlshdk::mysql::Auth_options repl_account;

  if (!dry_run) {
    // If the user already exists, it may have a different password causing an
    // authentication error. Also, it may have the wrong or incomplete set of
    // required grants. For those reasons, we must simply drop the account if
    // already exists to ensure a new one is created.
    mysqlshdk::mysql::drop_all_accounts_for_user(*primary, repl_user);

    std::vector<std::string> hosts;
    hosts.push_back(repl_host);

    mysqlshdk::gr::Create_recovery_user_options options;
    options.clone_supported = true;
    options.auto_failover = true;

    switch (member_auth_type) {
      case Replication_auth_type::PASSWORD:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        options.requires_password = true;
        break;
      default:
        options.requires_password = false;
        break;
    }

    switch (member_auth_type) {
      case Replication_auth_type::CERT_SUBJECT:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        options.cert_subject = auth_cert_subject;
        [[fallthrough]];
      case Replication_auth_type::CERT_ISSUER:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
        options.cert_issuer = auth_cert_issuer;
        break;
      default:
        break;
    }

    repl_account = mysqlshdk::gr::create_recovery_user(repl_user, *primary,
                                                       hosts, options);
  } else {
    repl_account.user = repl_user;
  }

  return {repl_account, repl_host};
}

void Cluster_set_impl::record_cluster_replication_user(
    Cluster_impl *cluster, const mysqlshdk::mysql::Auth_options &repl_user,
    const std::string &repl_user_host) {
  get_metadata_storage()->update_cluster_repl_account(
      cluster->get_id(), repl_user.user, repl_user_host.c_str());
}

void Cluster_set_impl::drop_cluster_replication_user(Cluster_impl *cluster,
                                                     Sql_undo_list *undo) {
  // auto primary = get_primary_master();
  auto primary = get_metadata_storage()->get_md_server();

  std::string repl_user;
  std::string repl_user_host;

  std::tie(repl_user, repl_user_host) = get_cluster_repl_account(cluster);

  /*
    Since clone copies all the data, including mysql.slave_master_info (where
    the CHANGE MASTER credentials are stored) an instance may be using the
    info stored in the donor's mysql.slave_master_info.

    To avoid such situation, we re-issue the CHANGE MASTER query after
    clone to ensure the right account is used. However, if the monitoring
    process is interrupted or waitRecovery = 0, that won't be done.

    The approach to tackle this issue is to store the donor recovery account
    in the target instance MD.instances table before doing the new CHANGE
    and only update it to the right account after a successful CHANGE MASTER.
    With this approach we can ensure that the account is not removed if it is
    being used.

   As so were we must query the Metadata to check whether the
    recovery user of that instance is registered for more than one instance
    and if that's the case then it won't be dropped.
  */
  if (get_metadata_storage()->count_recovery_account_uses(repl_user, true) ==
      1) {
    log_info("Dropping replication account '%s'@'%s' for cluster '%s'",
             repl_user.c_str(), repl_user_host.c_str(),
             cluster->get_name().c_str());

    try {
      if (undo)
        undo->add_snapshot_for_drop_user(*primary, repl_user, repl_user_host);

      primary->drop_user(repl_user, repl_user_host.c_str(), true);
    } catch (const std::exception &) {
      current_console()->print_warning(shcore::str_format(
          "Error dropping replication account '%s'@'%s' for cluster '%s'",
          repl_user.c_str(), repl_user_host.c_str(),
          cluster->get_name().c_str()));
    }

    // Also update metadata
    try {
      Transaction_undo *trx_undo = nullptr;
      if (undo) {
        auto u = Transaction_undo::create();
        trx_undo = u.get();
        undo->add(std::move(u));
      }

      get_metadata_storage()->update_cluster_repl_account(cluster->get_id(), "",
                                                          "", trx_undo);
    } catch (const std::exception &e) {
      log_warning("Could not update replication account metadata for '%s': %s",
                  cluster->get_name().c_str(), e.what());
    }
  }
}

mysqlshdk::mysql::Auth_options
Cluster_set_impl::refresh_cluster_replication_user(Cluster_impl *cluster,
                                                   bool dry_run) {
  return refresh_cluster_replication_user(*get_primary_master(), cluster,
                                          dry_run);
}

mysqlshdk::mysql::Auth_options
Cluster_set_impl::refresh_cluster_replication_user(
    const mysqlsh::dba::Instance &primary, Cluster_impl *cluster,
    bool dry_run) {
  std::string repl_user;
  std::string repl_user_host;
  std::string repl_password;

  std::tie(repl_user, repl_user_host) = get_cluster_repl_account(cluster);

  try {
    log_info("Resetting password for %s@%s at %s", repl_user.c_str(),
             repl_user_host.c_str(), primary.descr().c_str());
    // re-create replication with a new generated password
    if (!dry_run) {
      mysqlshdk::mysql::set_random_password(primary, repl_user,
                                            {repl_user_host}, &repl_password);
    }
  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while resetting password for replication account: %s",
        e.what()));
  }

  return mysqlshdk::mysql::Auth_options{repl_user, repl_password};
}

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_primary_cluster() const {
  return m_primary_cluster;
}

Cluster_set_member_metadata Cluster_set_impl::get_primary_cluster_metadata()
    const {
  std::vector<Cluster_set_member_metadata> cs_members;

  if (!m_metadata_storage->get_cluster_set(m_id, false, nullptr, &cs_members)) {
    throw std::runtime_error("Metadata not found");
  }

  for (const auto &m : cs_members) {
    if (m.primary_cluster) {
      log_debug("Primary cluster is '%s'", m.cluster.cluster_name.c_str());
      return m;
    }
  }

  throw std::runtime_error("No primary cluster found");
}

/**
 * Connect to PRIMARY of PRIMARY Cluster
 */
mysqlsh::dba::Instance *Cluster_set_impl::connect_primary() {
  Cluster_set_member_metadata primary_cluster = get_primary_cluster_metadata();

  current_ipool()->set_metadata(m_metadata_storage);

  m_primary_cluster = get_cluster_object(primary_cluster, true);

  m_metadata_storage = m_primary_cluster->get_metadata_storage();

  m_primary_master = m_primary_cluster->get_primary_master();

  return m_primary_master.get();
}

bool Cluster_set_impl::reconnect_target_if_invalidated(bool print_warnings) {
  // Connect to all REPLICA Clusters and ensure none of them have a higher
  // view_id generation than the global primary (which would it was
  // invalidated). If a higher view_id generation is found, the cluster_server
  // and MD object are replaced with it.

  // The current clusterset topology must be discoverable even when
  // getClusterSet() is called from an instance of an invalidated cluster, IFF
  // there is at least one available member of the current topology
  // that was also in the topology at the time of the invalidation.

  Cluster_set_metadata cs;
  std::vector<Cluster_set_member_metadata> cs_members;

  uint64_t primary_view_id;

  if (!m_metadata_storage->get_cluster_set(m_id, false, &cs, &cs_members,
                                           &primary_view_id)) {
    throw std::runtime_error("Metadata not found");
  }

  log_debug(
      "Checking if primary cluster '%s' (id=%s, view_id_generation=%x) has the "
      "most recent clusterset metadata",
      get_primary_cluster()->get_name().c_str(), cs.id.c_str(),
      cluster_set_view_id_generation(primary_view_id));

  auto ipool = current_ipool();

  std::shared_ptr<Instance> best_candidate;
  std::shared_ptr<MetadataStorage> best_candidate_md;
  uint32_t best_candidate_generation = 0;

  for (const auto &m : cs_members) {
    if (m.primary_cluster || m.invalidated) continue;

    // Can't use get_cluster_object(), because we need the MD object to point
    // to itself and not the primary cluster

    Cluster_availability availability;
    std::shared_ptr<Instance> group_server;
    try {
      group_server = ipool->try_connect_cluster_primary_with_fallback(
          m.cluster.cluster_id, &availability);
    } catch (const shcore::Exception &e) {
      if (e.code() != SHERR_DBA_METADATA_MISSING) throw;
      log_debug("%s", e.what());
    }

    if (!group_server) continue;

    auto md = std::make_shared<MetadataStorage>(group_server);
    uint64_t view_id = 0;
    Cluster_set_id csid;

    if (md->check_cluster_set(group_server.get(), &view_id, nullptr, &csid)) {
      log_debug("Metadata copy at %s has view_id=%x, csid=%s",
                group_server->descr().c_str(),
                cluster_set_view_id_generation(view_id), cs.id.c_str());

      if (csid != cs.id.c_str()) {
        if (print_warnings) {
          current_console()->print_note(shcore::str_format(
              "Metadata at instance %s is not consistent with the clusterSet.",
              group_server->descr().c_str()));
        }
      } else if (cluster_set_view_id_generation(view_id) >
                 cluster_set_view_id_generation(primary_view_id)) {
        if (print_warnings) {
          current_console()->print_note(shcore::str_format(
              "Instance %s has more recent metadata than %s (generation %x vs "
              "%x), which suggests %s has been invalidated",
              group_server->descr().c_str(),
              get_primary_master()->descr().c_str(),
              cluster_set_view_id_generation(view_id),
              cluster_set_view_id_generation(primary_view_id),
              get_primary_cluster()->get_name().c_str()));
        }

        if (cluster_set_view_id_generation(view_id) >
            best_candidate_generation) {
          best_candidate = group_server;
          best_candidate_md = md;
          best_candidate_generation = cluster_set_view_id_generation(view_id);
        }
      }
    } else {
      if (print_warnings) {
        current_console()->print_note(shcore::str_format(
            "Metadata at instance %s is not consistent with the clusterSet.",
            group_server->descr().c_str()));
      }
    }
  }

  if (best_candidate) {
    if (print_warnings) {
      current_console()->print_note(shcore::str_format(
          "Cluster %s appears to have been invalidated, reconnecting to %s.",
          get_primary_cluster()->get_name().c_str(),
          best_candidate->descr().c_str()));
    }

    m_cluster_server = best_candidate;
    m_metadata_storage = best_candidate_md;

    m_primary_cluster.reset();
    m_primary_master.reset();

    return true;
  }

  return false;
}

/**
 * Connect to PRIMARY of PRIMARY Cluster, throw exception if not possible
 */
mysqlsh::dba::Instance *Cluster_set_impl::acquire_primary(
    bool primary_required, bool check_primary_status) {
  connect_primary();

  auto primary_cluster = get_primary_cluster();
  assert(primary_cluster);

  try {
    m_primary_cluster->acquire_primary(primary_required, check_primary_status);
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_GROUP_HAS_NO_QUORUM) throw;

    log_debug("Cluster has no quorum and cannot process write transactions: %s",
              e.what());
  }

  return m_primary_cluster->get_primary_master().get();
}

void Cluster_set_impl::release_primary() { m_primary_master.reset(); }

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_cluster(
    const std::string &name, bool allow_unavailable, bool allow_invalidated) {
  if (m_primary_cluster && name == m_primary_cluster->get_name()) {
    return m_primary_cluster;
  }

  Cluster_set_member_metadata cluster_md;

  if (!m_metadata_storage->get_cluster_set_member_for_cluster_name(
          name, &cluster_md, allow_invalidated)) {
    throw shcore::Exception(
        shcore::str_format("The cluster with the name '%s' does not exist.",
                           name.c_str()),
        SHERR_DBA_METADATA_MISSING);
  }

  auto cluster = get_cluster_object(cluster_md, allow_unavailable);

  if (!cluster->is_cluster_set_member()) {
    throw shcore::Exception(
        shcore::str_format(
            "The cluster with the name '%s' is not a part of the ClusterSet",
            name.c_str()),
        SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET);
  }

  if (cluster && !allow_unavailable) {
    cluster->check_cluster_online();
  }

  return cluster;
}

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_cluster_object(
    const Cluster_set_member_metadata &cluster_md, bool allow_unavailable) {
  auto md = get_metadata_storage();

  auto ipool = current_ipool();
  try {
    std::shared_ptr<Instance> group_server;
    Cluster_availability availability = Cluster_availability::ONLINE;

    if (allow_unavailable) {
      group_server = ipool->try_connect_cluster_primary_with_fallback(
          cluster_md.cluster.cluster_id, &availability);
    } else {
      group_server =
          ipool->connect_group_primary(cluster_md.cluster.group_name);
    }
    if (group_server) {
      group_server->steal();

      if (cluster_md.primary_cluster && !cluster_md.invalidated) {
        auto md_server =
            Instance::connect(group_server->get_connection_options());

        md = std::make_shared<MetadataStorage>(md_server);
      }
    }
    return std::make_shared<Cluster_impl>(
        shared_from_this(), cluster_md.cluster, group_server, md, availability);
  } catch (const shcore::Exception &e) {
    if (!allow_unavailable) throw;

    current_console()->print_warning(shcore::str_format(
        "Could not connect to any member of cluster '%s': %s",
        cluster_md.cluster.cluster_name.c_str(), e.format().c_str()));

    return std::make_shared<Cluster_impl>(shared_from_this(),
                                          cluster_md.cluster, nullptr, md,
                                          Cluster_availability::UNREACHABLE);
  }
}

Cluster_channel_status Cluster_set_impl::get_replication_channel_status(
    const Cluster_impl &cluster) const {
  // Get the ClusterSet member metadata
  auto cluster_primary = cluster.get_cluster_server();
  if (!cluster_primary) return Cluster_channel_status::UNKNOWN;

  mysqlshdk::mysql::Replication_channel channel;
  if (!mysqlshdk::mysql::get_channel_status(
          *cluster_primary, k_clusterset_async_channel_name, &channel))
    return Cluster_channel_status::MISSING;

  if (channel.status() != mysqlshdk::mysql::Replication_channel::ON) {
    log_info("Channel '%s' at %s not ON: %s", k_clusterset_async_channel_name,
             cluster_primary->descr().c_str(),
             mysqlshdk::mysql::format_status(channel, true).c_str());
  } else if (cluster.is_primary_cluster()) {
    log_info("Unexpected channel '%s' at %s: %s",
             k_clusterset_async_channel_name, cluster_primary->descr().c_str(),
             mysqlshdk::mysql::format_status(channel, true).c_str());
  }

  switch (channel.status()) {
    case mysqlshdk::mysql::Replication_channel::CONNECTING:
      return Cluster_channel_status::CONNECTING;
    case mysqlshdk::mysql::Replication_channel::ON: {
      auto primary_cluster = get_primary_cluster();
      if (!primary_cluster || primary_cluster->cluster_availability() !=
                                  Cluster_availability::ONLINE)
        return Cluster_channel_status::OK;

      if (auto primary = get_primary_master();
          primary && (channel.source_uuid != primary->get_uuid()))
        return Cluster_channel_status::MISCONFIGURED;

      return Cluster_channel_status::OK;
    }

    case mysqlshdk::mysql::Replication_channel::OFF:
    case mysqlshdk::mysql::Replication_channel::RECEIVER_OFF:
    case mysqlshdk::mysql::Replication_channel::APPLIER_OFF:
      return Cluster_channel_status::STOPPED;
    case mysqlshdk::mysql::Replication_channel::CONNECTION_ERROR:
    case mysqlshdk::mysql::Replication_channel::APPLIER_ERROR:
      return Cluster_channel_status::ERROR;
  }

  return Cluster_channel_status::UNKNOWN;
}

namespace {
void handle_user_warnings(const Cluster_global_status &global_status,
                          const std::string &replica_cluster_name) {
  if (global_status == Cluster_global_status::OK_MISCONFIGURED ||
      global_status == Cluster_global_status::OK_NOT_REPLICATING) {
    auto console = mysqlsh::current_console();

    console->print_warning(
        "The Cluster's Replication Channel is misconfigured or stopped, "
        "topology changes will not be allowed on the InnoDB Cluster "
        "'" +
        replica_cluster_name + "'");
    console->print_note(
        "To restore the Cluster and ClusterSet operations, the Replication "
        "Channel must be fixed using <<<rejoinCluster>>>(). If there are "
        "invalid options, those must be corrected using <<<setOption>>>() "
        "before.");
  }
}

void ensure_no_router_uses_cluster(const std::vector<Router_metadata> &routers,
                                   const std::string &cluster_name) {
  if (!routers.empty()) {
    std::vector<std::string> routers_name_list;
    std::transform(
        routers.begin(), routers.end(), std::back_inserter(routers_name_list),
        [](const Router_metadata &r) { return r.hostname + "::" + r.name; });

    mysqlsh::current_console()->print_error(
        "The following Routers are using the Cluster '" + cluster_name +
        "' as a target cluster: [" + shcore::str_join(routers_name_list, ", ") +
        "]. Please ensure no Routers are using the cluster as target with "
        ".<<<setRoutingOption>>>()");

    throw shcore::Exception(
        "The Cluster '" + cluster_name +
            "' is an active Target Cluster for Routing operations.",
        SHERR_DBA_CLUSTER_IS_ACTIVE_ROUTING_TARGET);
  }
}

}  // namespace

Cluster_global_status Cluster_set_impl::get_cluster_global_status(
    Cluster_impl *cluster) const {
  {
    auto cluster_set_member = cluster->is_cluster_set_member();
    assert(cluster_set_member);
    if (!cluster_set_member) return Cluster_global_status::UNKNOWN;
  }

  if (cluster->is_invalidated()) return Cluster_global_status::INVALIDATED;

  switch (cluster->cluster_availability()) {
    case Cluster_availability::UNREACHABLE:
    case Cluster_availability::SOME_UNREACHABLE:
      return Cluster_global_status::UNKNOWN;
    case Cluster_availability::NO_QUORUM:
    case Cluster_availability::OFFLINE:
      return Cluster_global_status::NOT_OK;
    default:
      break;
  }

  if (!cluster->get_cluster_server()) return Cluster_global_status::UNKNOWN;

  if (cluster->is_primary_cluster()) {
    auto cl_status = cluster->cluster_status();

    switch (cl_status) {
      case Cluster_status::NO_QUORUM:
      case Cluster_status::OFFLINE:
      case Cluster_status::UNKNOWN:
        return Cluster_global_status::NOT_OK;
      default:
        break;
    }

    switch (get_replication_channel_status(*cluster)) {
      case Cluster_channel_status::OK:
      case Cluster_channel_status::CONNECTING:
      case Cluster_channel_status::MISCONFIGURED:
      case Cluster_channel_status::ERROR:
        // unexpected at primary cluster
        return Cluster_global_status::OK_MISCONFIGURED;

      case Cluster_channel_status::UNKNOWN:
      case Cluster_channel_status::STOPPED:
      case Cluster_channel_status::MISSING:
        if (cl_status == Cluster_status::FENCED_WRITES)
          return Cluster_global_status::OK_FENCED_WRITES;
        return Cluster_global_status::OK;
    }

    return Cluster_global_status::UNKNOWN;
  }

  // it's a replica cluster

  auto ret = Cluster_global_status::UNKNOWN;

  switch (cluster->cluster_status()) {
    case Cluster_status::NO_QUORUM:
    case Cluster_status::OFFLINE:
    case Cluster_status::UNKNOWN:
      ret = Cluster_global_status::NOT_OK;
      break;
    default:
      switch (get_replication_channel_status(*cluster)) {
        case Cluster_channel_status::OK:
        case Cluster_channel_status::CONNECTING:
          ret = Cluster_global_status::OK;
          break;
        case Cluster_channel_status::STOPPED:
        case Cluster_channel_status::ERROR:
          ret = Cluster_global_status::OK_NOT_REPLICATING;
          break;
        case Cluster_channel_status::MISSING:
        case Cluster_channel_status::MISCONFIGURED:
          ret = Cluster_global_status::OK_MISCONFIGURED;
          break;
        case Cluster_channel_status::UNKNOWN:
          ret = Cluster_global_status::NOT_OK;
          break;
      }
      break;
  }

  handle_user_warnings(ret, cluster->get_name());

  // if the cluster is OK, check consistency
  if (ret == Cluster_global_status::OK) {
    auto primary_status =
        get_cluster_global_status(get_primary_cluster().get());

    if ((primary_status != Cluster_global_status::UNKNOWN) &&
        !check_gtid_consistency(cluster))
      ret = Cluster_global_status::OK_NOT_CONSISTENT;
  }

  return ret;
}

bool Cluster_set_impl::check_gtid_consistency(Cluster_impl *cluster) const {
  assert(get_primary_master());

  auto my_gtid_set = mysqlshdk::mysql::Gtid_set::from_gtid_executed(
      *cluster->get_cluster_server());
  auto my_view_change_uuid = cluster->get_cluster_server()->get_sysvar_string(
      "group_replication_view_change_uuid", "");
  auto my_received_gtid_set =
      mysqlshdk::mysql::Gtid_set::from_received_transaction_set(
          *cluster->get_cluster_server(), k_clusterset_async_channel_name);

  // exclude gtids from view changes
  my_gtid_set.subtract(my_gtid_set.get_gtids_from(my_view_change_uuid),
                       *cluster->get_cluster_server());

  // exclude gtids that were received by the async channel, just in case we got
  // GTIDs that haven't been exposed to GTID_EXECUTED in the source yet
  my_gtid_set.subtract(my_received_gtid_set, *cluster->get_cluster_server());

  // always query GTID_EXECUTED from source after replica
  auto source_gtid_set =
      mysqlshdk::mysql::Gtid_set::from_gtid_executed(*get_primary_master());

  auto errants = my_gtid_set;
  errants.subtract(source_gtid_set, *cluster->get_cluster_server());

  if (!errants.empty()) {
    log_warning(
        "Cluster %s has errant transactions: source=%s source_gtids=%s "
        "replica=%s replica_gtids=%s received_gtids=%s errants=%s",
        cluster->get_name().c_str(), get_primary_master()->descr().c_str(),
        source_gtid_set.str().c_str(),
        cluster->get_cluster_server()->descr().c_str(),
        my_gtid_set.str().c_str(), my_received_gtid_set.str().c_str(),
        errants.str().c_str());
  }

  return errants.empty();
}

mysqlshdk::mysql::Lock_scoped Cluster_set_impl::get_lock_shared(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::SHARED, timeout);
}

mysqlshdk::mysql::Lock_scoped Cluster_set_impl::get_lock_exclusive(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout);
}

Async_replication_options
Cluster_set_impl::get_clusterset_default_replication_options() const {
  Async_replication_options repl_options;

  repl_options.ssl_mode = query_clusterset_ssl_mode();
  repl_options.auth_type = query_clusterset_auth_type();

  if (!repl_options.connect_retry.has_value())
    repl_options.connect_retry = k_cluster_set_master_connect_retry;
  if (!repl_options.retry_count.has_value())
    repl_options.retry_count = k_cluster_set_master_retry_count;

  return repl_options;
}

Async_replication_options Cluster_set_impl::get_clusterset_replication_options(
    const Cluster_id &cluster_id, bool *out_needs_reset_repl_channel) const {
  Async_replication_options repl_options;

  repl_options.ssl_mode = query_clusterset_ssl_mode();
  repl_options.auth_type = query_clusterset_auth_type();

  read_cluster_replication_options(cluster_id, &repl_options,
                                   out_needs_reset_repl_channel);

  if (!repl_options.connect_retry.has_value())
    repl_options.connect_retry = k_cluster_set_master_connect_retry;
  if (!repl_options.retry_count.has_value())
    repl_options.retry_count = k_cluster_set_master_retry_count;

  return repl_options;
}

std::list<std::shared_ptr<Cluster_impl>> Cluster_set_impl::connect_all_clusters(
    uint32_t read_timeout, bool skip_primary_cluster,
    std::list<Cluster_id> *inout_unreachable) {
  // Connect to the PRIMARY of each cluster
  std::vector<Cluster_set_member_metadata> cs_members;

  if (!m_metadata_storage->get_cluster_set(m_id, false, nullptr, &cs_members)) {
    throw std::runtime_error("MD not found");
  }

  std::list<std::shared_ptr<Cluster_impl>> r;

  (void)read_timeout;
  for (const auto &m : cs_members) {
    if ((m.primary_cluster && skip_primary_cluster) || m.invalidated) {
      continue;
    }

    // don't try connecting again to clusters that are known to be unreachable
    if (inout_unreachable &&
        std::find_if(inout_unreachable->begin(), inout_unreachable->end(),
                     [m](const Cluster_id &id) {
                       return m.cluster.cluster_id == id;
                     }) != inout_unreachable->end()) {
      continue;
    }

    try {
      auto cluster = get_cluster_object(m);

      cluster->acquire_primary();

      r.emplace_back(std::move(cluster));
    } catch (const shcore::Error &e) {
      current_console()->print_error(
          "Could not connect to PRIMARY of cluster " + m.cluster.cluster_name +
          ": " + e.format());
      if (inout_unreachable) {
        inout_unreachable->emplace_back(m.cluster.cluster_id);
      } else {
        throw shcore::Exception(
            "Cluster " + m.cluster.cluster_name + " is not reachable",
            SHERR_DBA_ASYNC_MEMBER_UNREACHABLE);
      }
    }
  }
  return r;
}

Member_recovery_method Cluster_set_impl::validate_instance_recovery(
    Member_op_action op_action,
    const mysqlshdk::mysql::IInstance &donor_instance,
    const mysqlshdk::mysql::IInstance &target_instance,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive) {
  auto check_recoverable =
      [&donor_instance](const mysqlshdk::mysql::IInstance &tgt_instance) {
        // Get the gtid state in regards to the donor
        mysqlshdk::mysql::Replica_gtid_state state =
            check_replica_group_gtid_state(donor_instance, tgt_instance,
                                           nullptr, nullptr);

        return (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE);
      };

  Member_recovery_method recovery_method =
      mysqlsh::dba::validate_instance_recovery(
          Cluster_type::REPLICATED_CLUSTER, op_action, donor_instance,
          target_instance, check_recoverable, opt_recovery_method,
          gtid_set_is_complete, interactive,
          get_primary_cluster()->check_clone_availablity(donor_instance,
                                                         target_instance));

  return recovery_method;
}

std::vector<Cluster_set_member_metadata> Cluster_set_impl::get_clusters()
    const {
  std::vector<Cluster_set_member_metadata> all_clusters;

  get_metadata_storage()->get_cluster_set(get_id(), false, nullptr,
                                          &all_clusters);

  return all_clusters;
}

std::vector<Instance_metadata> Cluster_set_impl::get_instances_from_metadata()
    const {
  return get_metadata_storage()->get_all_instances();
}

void Cluster_set_impl::ensure_compatible_clone_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient) const {
  {
    auto donor_address = donor.get_canonical_address();

    // check if the donor belongs to the primary cluster (MD)
    try {
      auto primary = get_primary_cluster();
      primary->get_metadata_storage()->get_instance_by_address(
          donor_address, primary->get_id());
    } catch (const shcore::Exception &e) {
      if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' does not belong to the PRIMARY Cluster",
              donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }

    // check donor state
    if (mysqlshdk::gr::get_member_state(donor) !=
        mysqlshdk::gr::Member_state::ONLINE) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' is not an ONLINE member of the PRIMARY Cluster.",
              donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
    }
  }

  // further checks (related to the instances and unrelated to the cluster)
  Base_cluster_impl::check_compatible_clone_donor(donor, recipient);
}

shcore::Value Cluster_set_impl::create_replica_cluster(
    const std::string &instance_def, const std::string &cluster_name,
    Recovery_progress_style progress_style,
    const clusterset::Create_replica_cluster_options &options) {
  check_preconditions("createReplicaCluster");

  Scoped_instance target(connect_target_instance(instance_def, true, true));

  // put a shared lock on the clusterset and an exclusive one on the target
  // instance
  auto cs_lock = get_lock_shared();
  auto i_lock = target->get_lock_exclusive();

  // Create the replica cluster
  {
    // Create the Create_replica_cluster command and execute it.
    clusterset::Create_replica_cluster op_create_replica_cluster(
        this, get_primary_master().get(), target, cluster_name, progress_style,
        options);

    // Prepare the Create_replica_cluster command execution (validations).
    op_create_replica_cluster.prepare();

    // Execute Create_replica_cluster operations.
    return op_create_replica_cluster.execute();
  }
}

void Cluster_set_impl::remove_cluster(
    const std::string &cluster_name,
    const clusterset::Remove_cluster_options &options) {
  check_preconditions("removeCluster");

  std::shared_ptr<Cluster_impl> target_cluster;
  bool skip_channel_check = false;
  auto console = mysqlsh::current_console();

  // put an exclusive lock on the clusterset and one exclusive cluster lock on
  // the primary cluster
  auto cs_lock = get_lock_exclusive();
  auto pc_lock = m_primary_cluster->get_lock_exclusive();

  // Validations and initializations of variables
  {
    // Validate the Cluster name
    mysqlsh::dba::validate_cluster_name(cluster_name,
                                        Cluster_type::GROUP_REPLICATION);

    // Get the Cluster object
    // NOTE: This will throw an exception if the Cluster does not exist in the
    // metadata
    try {
      target_cluster = get_cluster(cluster_name, false, true);
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_METADATA_MISSING ||
          e.code() == SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET) {
        console->print_error(shcore::str_format(
            "The Cluster '%s' does not exist or does not belong to the "
            "ClusterSet.",
            cluster_name.c_str()));
        throw;
      } else {
        if (options.force.value_or(false)) {
          target_cluster = get_cluster(cluster_name, true, true);

          console->print_warning(
              "The target Cluster's Primary instance is unavailable so the "
              "Cluster cannot be cleanly removed from the ClusterSet. Removing "
              "anyway because 'force' is enabled.");

          skip_channel_check = true;
        } else {
          console->print_error(
              "The target Cluster's Primary instance is unavailable so the "
              "Cluster cannot be cleanly removed from the ClusterSet. Use the "
              "'force' option to remove anyway from the replication topology.");
          throw shcore::Exception(
              shcore::str_format(
                  "PRIMARY instance of Cluster '%s' is unavailable: '%s'",
                  cluster_name.c_str(), e.format().c_str()),
              SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE);
        }
      }
    }

    // Check if the target cluster is a PRIMARY Cluster
    if (target_cluster->is_primary_cluster()) {
      console->print_error(
          "Cannot remove the PRIMARY Cluster of the ClusterSet.");
      throw shcore::Exception("The Cluster '" + cluster_name +
                                  "' is the PRIMARY Cluster of the ClusterSet.",
                              SHERR_DBA_CLUSTER_CANNOT_REMOVE_PRIMARY_CLUSTER);
    }

    // Check if there is any Router instance registered in the ClusterSet using
    // the Cluster as Target Cluster
    auto routers = get_metadata_storage()->get_routers_using_cluster_as_target(
        target_cluster->get_group_name());

    ensure_no_router_uses_cluster(routers, target_cluster->get_name());
  }

  // put an exclusive cluster lock on the target cluster (if reachable)
  mysqlshdk::mysql::Lock_scoped tc_lock;
  if (!skip_channel_check) tc_lock = target_cluster->get_lock_exclusive();

  // Execute the operation

  Undo_tracker undo_tracker;
  Undo_tracker::Undo_entry *drop_cluster_undo = nullptr;
  Undo_tracker::Undo_entry *replication_user_undo = nullptr;
  std::vector<Scoped_instance> cluster_reachable_members,
      cluster_reachable_read_replicas;
  std::vector<std::pair<std::string, std::string>> cluster_unreachable_members;
  bool cluster_has_unreachable_read_replicas = false;

  try {
    console->print_info("The Cluster '" + cluster_name +
                        "' will be removed from the InnoDB ClusterSet.");
    console->print_info();

    // Check if the channel exists and its OK (not ERROR) first
    mysqlshdk::mysql::Replication_channel channel;
    if (!skip_channel_check) {
      if (!get_channel_status(*target_cluster->get_cluster_server(),
                              {k_clusterset_async_channel_name}, &channel)) {
        if (!options.force.value_or(false)) {
          console->print_error(
              "The ClusterSet Replication channel could not be found at the "
              "Cluster '" +
              cluster_name + "'. Use the 'force' option to ignore this check.");

          throw shcore::Exception("Replication channel does not exist",
                                  SHERR_DBA_REPLICATION_OFF);
        } else {
          skip_channel_check = true;
          console->print_warning(
              "Ignoring non-existing ClusterSet Replication channel because "
              "of 'force' option");
        }
      } else {
        if (channel.status() !=
            mysqlshdk::mysql::Replication_channel::Status::ON) {
          if (!options.force.value_or(false)) {
            console->print_error(
                "The ClusterSet Replication channel has an invalid state '" +
                to_string(channel.status()) +
                "'. Use the 'force' option to ignore this check.");

            throw shcore::Exception(
                "ClusterSet Replication Channel not in expected state",
                SHERR_DBA_REPLICATION_INVALID);
          } else {
            skip_channel_check = true;
            console->print_warning(
                "ClusterSet Replication channel has invalid state '" +
                to_string(channel.status()) +
                "'. Ignoring because of 'force' option");
          }
        }
      }
    }

    // Get the list of the reachable and unreachable members of the cluster
    target_cluster->execute_in_members(
        [&cluster_reachable_members](
            const std::shared_ptr<Instance> &instance,
            const Cluster_impl::Instance_md_and_gr_member &) {
          Scoped_instance reachable_member(instance);
          cluster_reachable_members.emplace_back(reachable_member);
          return true;
        },
        [&cluster_unreachable_members](
            const shcore::Error &connection_error,
            const Cluster_impl::Instance_md_and_gr_member &info) {
          cluster_unreachable_members.emplace_back(info.first.endpoint,
                                                   connection_error.format());
          return true;
        });

    // Get the list of the reachable and unreachable read-replicas of the
    // cluster
    target_cluster->execute_in_read_replicas(
        [&cluster_reachable_read_replicas](
            const std::shared_ptr<Instance> &instance,
            const Instance_metadata &) {
          cluster_reachable_read_replicas.emplace_back(
              Scoped_instance(instance));
          return true;
        },
        [&cluster_has_unreachable_read_replicas](const shcore::Error &,
                                                 const Instance_metadata &) {
          cluster_has_unreachable_read_replicas = true;
          return true;
        });

    // Check if there are unreachable Read-Replicas
    if (cluster_has_unreachable_read_replicas) {
      if (!options.force) {
        console->print_error(
            "The Cluster has unreachable Read-Replicas so the command cannot "
            "remove them. Please bring the instances back ONLINE and try to "
            "remove the Cluster again. If the instances are permanently not "
            "reachable, then you can choose to proceed with the operation and "
            "only remove the instances from the Cluster Metadata by using the "
            "'force' option.");

        throw shcore::Exception("Unreachable Read-Replicas detected",
                                SHERR_DBA_UNREACHABLE_INSTANCES);
      } else {
        console->print_note(
            "The Cluster has unreachable Read-Replicas. Ignored because of "
            "'force' option");
      }
    }

    // Sync transactions before making any changes
    console->print_info(
        "* Waiting for the Cluster to synchronize with the PRIMARY "
        "Cluster...");

    if (!target_cluster->get_primary_master()) {
      console->print_note(
          "Transaction sync was skipped because cluster is unavailable");
    } else if (!skip_channel_check) {
      try {
        sync_transactions(*target_cluster->get_cluster_server(),
                          {k_clusterset_async_channel_name}, options.timeout);
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
          console->print_error(
              "The Cluster failed to synchronize its transaction set "
              "with the PRIMARY Cluster. You may increase the "
              "transaction sync timeout with the option 'timeout' or use "
              "the 'force' option to ignore the timeout.");
          throw;
        } else if (options.force.value_or(false)) {
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

    // Restore the transaction_size_limit value to the original one
    if (target_cluster->get_cluster_server()) {
      restore_transaction_size_limit(target_cluster.get(), options.dry_run);
    }

    // Disable skip_replica_start
    if (!options.dry_run && target_cluster->get_cluster_server() &&
        target_cluster->cluster_availability() ==
            Cluster_availability::ONLINE) {
      log_info("Persisting skip_replica_start=0 across the cluster...");

      auto config = target_cluster->create_config_object({}, true, true, true);

      config->set("skip_replica_start", std::optional<bool>(false));
      config->apply(true);

      undo_tracker.add("", [=]() {
        log_info("Revert: Enabling skip_replica_start");
        auto config_undo =
            target_cluster->create_config_object({}, true, true, true);

        config_undo->set("skip_replica_start", std::optional<bool>(true));
        config_undo->apply();
      });

      // Set debug trap to test reversion of the ClusterSet setting set-up
      DBUG_EXECUTE_IF("dba_remove_cluster_fail_disable_skip_replica_start",
                      { throw std::logic_error("debug"); });
    }

    // Update Metadata
    console->print_info("* Updating topology");

    log_debug("Removing Cluster from the Metadata.");

    auto metadata = get_metadata_storage();

    struct {
      mysqlsh::dba::Replication_auth_type cluster_auth_type;
      std::string cluster_auth_cert_issuer;
      std::optional<std::string> primary_cert_subject;
    } auth_data_backup;

    // store auth data of the cluster to be removed (in case reverted is
    // executed)
    auth_data_backup.cluster_auth_type =
        target_cluster->query_cluster_auth_type();
    auth_data_backup.cluster_auth_cert_issuer =
        target_cluster->query_cluster_auth_cert_issuer();

    if (auto primary = metadata->get_md_server(); primary) {
      auth_data_backup.primary_cert_subject =
          query_cluster_instance_auth_cert_subject(*primary);
    }

    if (!options.dry_run) {
      {
        MetadataStorage::Transaction trx(metadata);

        metadata->record_cluster_set_member_removed(get_id(),
                                                    target_cluster->get_id());

        // Push the whole transaction and changes to the Undo list
        undo_tracker.add(
            "Recording back ClusterSet member removed", [=, this]() {
              Cluster_set_member_metadata cluster_md;
              cluster_md.cluster.cluster_id = target_cluster->get_id();
              cluster_md.cluster_set_id = get_id();
              cluster_md.master_cluster_id = get_primary_cluster()->get_id();
              cluster_md.primary_cluster = false;

              MetadataStorage::Transaction trx2(metadata);
              metadata->record_cluster_set_member_added(cluster_md);
              trx2.commit();
            });

        // Only commit transactions once everything is done
        trx.commit();
      }

      // Set debug trap to test reversion of Metadata topology updates to
      // remove the member
      DBUG_EXECUTE_IF("dba_remove_cluster_fail_post_cs_member_removed",
                      { throw std::logic_error("debug"); });

      // Drop cluster replication user
      {
        Sql_undo_list sql_undo;
        drop_cluster_replication_user(target_cluster.get(), &sql_undo);
        replication_user_undo = &undo_tracker.add(
            "Restore cluster replication account", std::move(sql_undo),
            [this]() { return get_metadata_storage()->get_md_server(); });
      }

      // Set debug trap to test reversion of replication user creation
      DBUG_EXECUTE_IF("dba_remove_cluster_fail_post_replication_user_removal",
                      { throw std::logic_error("debug"); });

      // Remove Cluster's recovery accounts
      if (!options.dry_run && target_cluster->get_cluster_server()) {
        Sql_undo_list undo_drop_users;

        // The accounts must be dropped from the ClusterSet
        // NOTE: If the replication channel is down and 'force' was used, the
        // accounts won't be dropped in the target cluster. This is expected,
        // otherwise, it wouldn't be possible to reboot the cluster from
        // complete outage later on
        target_cluster->drop_replication_users(&undo_drop_users);

        undo_tracker.add("Re-creating Cluster recovery accounts",
                         std::move(undo_drop_users),
                         [this]() { return get_primary_master(); });
      }

      // Remove the Cluster's Metadata and Cluster members' info from the
      // Metadata
      {
        auto drop_cluster_trx_undo = Transaction_undo::create();
        MetadataStorage::Transaction trx(metadata);
        metadata->drop_cluster(target_cluster->get_name(),
                               drop_cluster_trx_undo.get());
        trx.commit();
        log_debug("removeCluster() metadata updates done");

        drop_cluster_undo = &undo_tracker.add(
            "Re-creating Cluster's metadata",
            Sql_undo_list(std::move(drop_cluster_trx_undo)),
            [this]() { return get_metadata_storage()->get_md_server(); });
      }

      // Sync again to catch-up the drop user and metadata update
      if (target_cluster->cluster_availability() ==
              Cluster_availability::ONLINE &&
          !skip_channel_check && options.timeout >= 0) {
        try {
          console->print_info(
              "* Waiting for the Cluster to synchronize the Metadata updates "
              "with the PRIMARY Cluster...");
          sync_transactions(*target_cluster->get_cluster_server(),
                            {k_clusterset_async_channel_name}, options.timeout);
        } catch (const shcore::Exception &e) {
          if (options.force.value_or(false)) {
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

        // Sync read-replicas
        for (const auto &rr : cluster_reachable_read_replicas) {
          try {
            console->print_info("* Waiting for Read-Replica '" + rr->descr() +
                                "' to synchronize with the Cluster...");
            target_cluster->sync_transactions(*rr, Instance_type::READ_REPLICA,
                                              options.timeout);
          } catch (const shcore::Exception &e) {
            if (options.force) {
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
      }
    }

    // Stop replication
    // NOTE: This is done last so all other changes are propagated first to
    // the Cluster being removed
    console->print_info(
        "* Stopping and deleting ClusterSet managed replication channel...");

    // Store the replication options before stopping and deleting the
    // channel to use in the revert process in case of a failure
    auto ar_options =
        get_clusterset_replication_options(target_cluster->get_id(), nullptr);

    if (target_cluster->get_cluster_server() &&
        target_cluster->cluster_availability() ==
            Cluster_availability::ONLINE) {
      // Call the primitive to remove the replica, ensuring:
      //  - super_read_only management is enabled
      //  - the ClusterSet replication channel is stopped and reset
      //  - The managed connection failover configurations are deleted
      // ... on all members
      const auto &primary_uuid =
          target_cluster->get_cluster_server()->get_uuid();

      for (const auto &instance : cluster_reachable_members) {
        remove_replica(instance.get(), options.dry_run, primary_uuid);
      }

      // Revert in case of failure
      undo_tracker.add("Re-adding Cluster as Replica", [=, this]() {
        drop_cluster_undo->call();

        replication_user_undo->call();

        auto repl_source = get_primary_master();

        assert(auth_data_backup.primary_cert_subject.has_value());
        auto repl_credentials = create_cluster_replication_user(
            target_cluster->get_cluster_server().get(), "",
            auth_data_backup.cluster_auth_type,
            auth_data_backup.cluster_auth_cert_issuer,
            auth_data_backup.primary_cert_subject.value_or(""),
            options.dry_run);

        auto ar_channel_options = ar_options;
        ar_channel_options.repl_credentials = repl_credentials.first;

        // create async channel on all secondaries, update member actions
        target_cluster->execute_in_members(
            {mysqlshdk::gr::Member_state::ONLINE,
             mysqlshdk::gr::Member_state::RECOVERING},
            get_primary_master()->get_connection_options(), {},
            [=, this](const std::shared_ptr<Instance> &instance,
                      const mysqlshdk::gr::Member &) {
              if (target_cluster->get_cluster_server()->get_uuid() !=
                  instance->get_uuid()) {
                async_create_channel(instance.get(), repl_source.get(),
                                     k_clusterset_async_channel_name,
                                     ar_options, options.dry_run);
                update_replica_settings(
                    target_cluster->get_cluster_server().get(),
                    repl_source.get(), false, options.dry_run);
              }
              return true;
            });

        update_replica(target_cluster->get_cluster_server().get(),
                       repl_source.get(), ar_channel_options, true, false,
                       options.dry_run);
      });
    } else {
      // If the target cluster is OFFLINE, check if there are reachable
      // members in order to reset the settings in those instances
      for (const auto &reachable_member : cluster_reachable_members) {
        // Check if super_read_only is enabled. If so it must be
        // disabled to reset the ClusterSet settings
        if (reachable_member->get_sysvar_bool("super_read_only", false)) {
          reachable_member->set_sysvar("super_read_only", false);
        }

        // Reset the ClusterSet settings and replication channel
        try {
          if (cluster_topology_executor_ops::is_member_auto_rejoining(
                  reachable_member))
            cluster_topology_executor_ops::ensure_not_auto_rejoining(
                reachable_member);

          remove_replica(reachable_member.get(), options.dry_run);
        } catch (...) {
          if (options.force.value_or(false)) {
            current_console()->print_warning(
                "Could not reset replication settings for " +
                reachable_member->descr() + ": " + format_active_exception());
          } else {
            throw;
          }
        }

        // Disable skip_replica_start
        reachable_member->set_sysvar(
            "skip_replica_start", false,
            mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

        // Revert in case of failure
        undo_tracker.add("", [=, this]() {
          log_info("Revert: Re-adding Cluster as Replica");
          update_replica(reachable_member.get(), get_primary_master().get(),
                         ar_options, false, false, options.dry_run);

          log_info("Revert: Enabling skip_replica_start");
          reachable_member->set_sysvar(
              "skip_replica_start", true,
              mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
        });

        for (const auto &unreachable_member : cluster_unreachable_members) {
          console->print_warning(
              shcore::str_format("Configuration update of %s skipped: %s",
                                 unreachable_member.first.c_str(),
                                 unreachable_member.second.c_str()));
        }
      }
    }

    // Set debug trap to test reversion of replica removal
    DBUG_EXECUTE_IF("dba_remove_cluster_fail_post_replica_removal",
                    { throw std::logic_error("debug"); });

    // Dissolve the Cluster
    if (target_cluster->get_cluster_server() &&
        target_cluster->cluster_availability() ==
            Cluster_availability::ONLINE) {
      auto target_cluster_primary = target_cluster->get_cluster_server();

      console->print_info("* Dissolving the Cluster...");

      auto comm_stack = get_communication_stack(*target_cluster_primary);

      bool requires_certificates{false};
      switch (target_cluster->query_cluster_auth_type()) {
        case mysqlsh::dba::Replication_auth_type::CERT_ISSUER:
        case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
        case mysqlsh::dba::Replication_auth_type::CERT_ISSUER_PASSWORD:
        case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
          requires_certificates = true;
        default:
          break;
      }

      // Remove the read-replicas
      for (const auto &rr : cluster_reachable_read_replicas) {
        console->print_info("* Removing Read-Replica '" + rr->descr() +
                            "' from the cluster...");

        // Remove the async channel
        remove_channel(*rr, k_read_replica_async_channel_name, false);

        try {
          log_info("Disabling automatic failover management at %s",
                   rr->descr().c_str());

          reset_managed_connection_failover(*rr, false);
        } catch (const std::exception &e) {
          log_error("Error disabling automatic failover at %s: %s",
                    rr->descr().c_str(), e.what());
          console->print_warning(
              "An error occurred when trying to disable automatic failover on "
              "Read-Replica at '" +
              rr->descr() + "'");
          console->print_info();
        }
      }

      // Then the secondaries
      for (const auto &member : cluster_reachable_members) {
        if (member->get_uuid() == target_cluster_primary->get_uuid()) continue;

        try {
          // Stop Group Replication and reset GR variables
          log_debug("Stopping GR at %s", member->descr().c_str());

          if (!options.dry_run) {
            // Do not reset Group Replication's recovery channel credentials,
            // otherwise, when MySQL communication stack is used it won't be
            // possible to reboot the cluster from complete outage. That would
            // require re-creating the account, but that's a problem if the
            // cluster is a replica cluster since it would create an errant
            // transaction since the cluster is not yet rejoined back to the
            // clusterset and, on the other side, suppressing the binary log is
            // not an option since then the recovery account wouldn't be
            // replicated to the other cluster members
            if (comm_stack == kCommunicationStackMySQL) {
              mysqlsh::dba::leave_cluster(*member, false, false);
            } else {
              mysqlsh::dba::leave_cluster(*member);
            }

            undo_tracker.add("", [=]() {
              Group_replication_options gr_opts;
              std::unique_ptr<mysqlshdk::config::Config> cfg =
                  create_server_config(
                      member.get(),
                      mysqlshdk::config::k_dft_cfg_server_handler);

              mysqlsh::dba::join_cluster(*member, *target_cluster_primary,
                                         gr_opts, requires_certificates,
                                         cluster_reachable_members.size() - 1,
                                         cfg.get());
            });
          }
        } catch (const std::exception &err) {
          console->print_error(shcore::str_format(
              "Instance '%s' failed to leave the cluster: %s",
              member->get_canonical_address().c_str(), err.what()));
        }
      }

      // Reconcile the view change GTIDs generated when the Replica Cluster
      // was created and when members were added to it. Required to ensure
      // those transactions are not detected an errant in further operations
      // on those instances.
      // Do it before completing the dissolve of the Cluster (removal of the
      // Primary) to ensure no events are missing.
      console->print_info("* Reconciling internally generated GTIDs...");
      if (!options.dry_run) {
        reconcile_view_change_gtids(target_cluster_primary.get());
      }

      // Finally the primary
      try {
        log_debug("Stopping GR at %s", target_cluster_primary->descr().c_str());

        if (!options.dry_run) {
          // Do not reset Group Replication's recovery channel credentials
          if (comm_stack == kCommunicationStackMySQL) {
            mysqlsh::dba::leave_cluster(*target_cluster_primary, false, false);
          } else {
            mysqlsh::dba::leave_cluster(*target_cluster_primary);
          }
        }
      } catch (const std::exception &err) {
        console->print_error(shcore::str_format(
            "Instance '%s' failed to leave the cluster: %s",
            target_cluster_primary->get_canonical_address().c_str(),
            err.what()));
        throw;
      }
    }

    console->print_info();
    console->print_info("The Cluster '" + cluster_name +
                        "' was removed from the ClusterSet.");
    console->print_info();

    if (options.dry_run) {
      console->print_info("dryRun finished.");
      console->print_info();
    }
  } catch (...) {
    console->print_error("Error removing Replica Cluster: " +
                         format_active_exception());

    console->print_note("Reverting changes...");

    undo_tracker.execute();

    console->print_info();
    console->print_info("Changes successfully reverted.");

    throw;
  }
}

void Cluster_set_impl::update_replica_settings(Instance *instance,
                                               Instance *new_primary,
                                               bool is_primary, bool dry_run) {
  try {
    bool enabled;

    log_info(
        "Checking Group Action mysql_disable_super_read_only_if_primary %s",
        instance->descr().c_str());
    if (!dry_run) {
      if (mysqlshdk::gr::get_member_action_status(
              *instance, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
              &enabled) &&
          enabled) {
        // needs to be done before locks are acquired, where SRO is set
        log_info("Disabling automatic super_read_only management from %s",
                 instance->descr().c_str());
        mysqlshdk::gr::disable_member_action(
            *instance, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
            mysqlshdk::gr::k_gr_member_action_after_primary_election);
      } else {
        log_info("Already disabled");
      }
    }
  } catch (...) {
    log_error("Error disabling member action %s at %s: %s",
              mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
              instance->descr().c_str(), format_active_exception().c_str());
    throw;
  }

  // Ensure GR action 'mysql_start_failover_channels_if_primary' is enabled
  // to ensure the automatic connection failover for receiver failure.
  // NOTE: This is not needed on PRIMARY Clusters and won't have any effect,
  // but we should set it during the creation of the PRIMARY cluster to
  // avoid having to deal with it during switchover/failovers.
  try {
    bool enabled;

    log_info(
        "Checking Group Action mysql_start_failover_channels_if_primary %s",
        instance->descr().c_str());
    if (!dry_run) {
      if (!mysqlshdk::gr::get_member_action_status(
              *instance, mysqlshdk::gr::k_gr_start_failover_channels_if_primary,
              &enabled) ||
          !enabled) {
        log_info(
            "Enabling Group Action mysql_start_failover_channels_if_primary");

        mysqlshdk::gr::enable_member_action(
            *instance, mysqlshdk::gr::k_gr_start_failover_channels_if_primary,
            mysqlshdk::gr::k_gr_member_action_after_primary_election);
      }
    }
  } catch (...) {
    log_error(
        "Error enabling automatic connection failover for receiver failure "
        "handling at %s: %s",
        instance->descr().c_str(), format_active_exception().c_str());
    throw;
  }

  log_info("Configuring the managed connection failover configurations for %s",
           instance->descr().c_str());
  if (!dry_run) {
    try {
      // Delete any previous managed connection failover configurations
      delete_managed_connection_failover(
          *instance, k_clusterset_async_channel_name, dry_run);

      if (is_primary) {
        // Setup the new managed connection failover
        add_managed_connection_failover(
            *instance, *new_primary, k_clusterset_async_channel_name, "",
            k_primary_weight_default, k_secondary_weight_default, dry_run);
      }
    } catch (...) {
      log_error("Error setting up connection failover at %s: %s",
                instance->descr().c_str(), format_active_exception().c_str());
      throw;
    }
  }
}

void Cluster_set_impl::remove_replica_settings(Instance *instance,
                                               bool dry_run) {
  try {
    bool enabled;

    if (!mysqlshdk::gr::get_member_action_status(
            *instance, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
            &enabled) ||
        !enabled) {
      // must be done after locks are released
      log_info("Enabling automatic super_read_only management on %s",
               instance->descr().c_str());
      if (!dry_run) {
        mysqlshdk::gr::enable_member_action(
            *instance, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
            mysqlshdk::gr::k_gr_member_action_after_primary_election);
      }
    } else {
      log_info("Already enabled");
    }
  } catch (...) {
    current_console()->print_error(shcore::str_format(
        "Error enabling automatic super_read_only management at %s: %s",
        instance->descr().c_str(), format_active_exception().c_str()));
    throw;
  }

  // Delete the managed connection failover configurations
  try {
    // needs to be done before locks are acquired, where SRO is set
    log_info("Disabling automatic failover management at %s",
             instance->descr().c_str());

    delete_managed_connection_failover(
        *instance, k_clusterset_async_channel_name, dry_run);
  } catch (...) {
    current_console()->print_error(shcore::str_format(
        "Error disabling automatic failover at %s: %s",
        instance->descr().c_str(), format_active_exception().c_str()));
    throw;
  }
}

void Cluster_set_impl::ensure_replica_settings(Cluster_impl *replica,
                                               bool dry_run) {
  // Ensure SRO is enabled on all members of the Cluster
  replica->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE},
      replica->get_cluster_server()->get_connection_options(), {},
      [dry_run](const std::shared_ptr<Instance> &instance,
                const mysqlshdk::gr::Member &) {
        if (!instance->get_sysvar_bool("super_read_only", false)) {
          log_info("Enabling super_read_only at '%s'",
                   instance->descr().c_str());
          if (!dry_run) {
            instance->set_sysvar("super_read_only", true);
          }
        }

        return true;
      });
}

shcore::Value Cluster_set_impl::list_routers(const std::string &router) {
  check_preconditions("listRouters");

  auto routers =
      clusterset_list_routers(get_metadata_storage().get(), get_id(), router);

  if (router.empty()) {
    auto dict = shcore::make_dict();
    (*dict)["routers"] = routers;
    (*dict)["domainName"] = shcore::Value(get_name());
    return shcore::Value(std::move(dict));
  }

  return routers;
}

shcore::Dictionary_t Cluster_set_impl::routing_options(
    const std::string &router) {
  auto dict = Base_cluster_impl::routing_options(router);
  if (router.empty()) (*dict)["domainName"] = shcore::Value(get_name());

  return dict;
}

void Cluster_set_impl::delete_async_channel(Cluster_impl *cluster,
                                            bool dry_run) {
  cluster->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE},
      cluster->get_cluster_server()->get_connection_options(), {},
      [=](const std::shared_ptr<Instance> &instance,
          const mysqlshdk::gr::Member &) {
        try {
          reset_channel(*instance, k_clusterset_async_channel_name, true,
                        dry_run);
        } catch (const shcore::Exception &e) {
#ifndef ER_REPLICA_CHANNEL_DOES_NOT_EXIST
#define ER_REPLICA_CHANNEL_DOES_NOT_EXIST ER_SLAVE_CHANNEL_DOES_NOT_EXIST
#endif
          if (e.code() != ER_REPLICA_CHANNEL_DOES_NOT_EXIST) {
            throw;
          }
        }
        return true;
      });
}

void Cluster_set_impl::promote_to_primary(Cluster_impl *cluster,
                                          bool preserve_channel, bool dry_run) {
  auto primary = cluster->get_cluster_server();

  log_info("Promoting cluster %s (primary=%s)", cluster->get_name().c_str(),
           primary->descr().c_str());

  stop_channel(*primary, k_clusterset_async_channel_name, true, dry_run);

  // Stop the channel but don't delete it yet, since we need to be able to
  // revert in case something goes wrong, without knowing the password of
  // the replication user
  if (!preserve_channel) {
    delete_async_channel(cluster, dry_run);
  }

  // Unfence the primary. member_action changes need to have SRO=off
  if (!dry_run) {
    unfence_instance(primary.get(), false);
  }

  // Reset ClusterSet related configurations first:
  //  - Enable super_read_only management
  //  - Delete the managed connection failover configuration
  remove_replica_settings(primary.get(), dry_run);

  if (!dry_run) {
    // notify that the primary cluster of the clusterset changed
    shcore::ShellNotifications::get()->notify(
        kNotifyClusterSetPrimaryChanged, nullptr,
        shcore::make_dict(kNotifyDataClusterSetId, get_id()));
  }
}

void Cluster_set_impl::demote_from_primary(
    Cluster_impl *cluster, Instance *new_primary,
    const Async_replication_options &repl_options, bool dry_run) {
  auto cluster_primary = cluster->get_cluster_server();

  log_info("Demoting %s from primary", cluster_primary->descr().c_str());

  // create async channel on all secondaries, update member actions
  cluster->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE,
       mysqlshdk::gr::Member_state::RECOVERING},
      new_primary->get_connection_options(), {},
      [=](const std::shared_ptr<Instance> &instance,
          const mysqlshdk::gr::Member &) {
        if (cluster_primary->get_uuid() == instance->get_uuid()) return true;

        async_create_channel(instance.get(), new_primary,
                             k_clusterset_async_channel_name, repl_options,
                             dry_run);

        return true;
      });

  update_replica_settings(cluster_primary.get(), new_primary, false, dry_run);

  update_replica(cluster_primary.get(), new_primary, repl_options, true, false,
                 dry_run);

  set_maximum_transaction_size_limit(cluster, false, dry_run);
}

void Cluster_set_impl::update_replica(
    Cluster_impl *replica, Instance *master,
    const Async_replication_options &ar_options, bool reset_channel,
    bool dry_run) {
  // Update SECONDARY instances before the PRIMARY, because we need to ensure
  // the async channel exists everywhere before we can start the channel at the
  // PRIMARY with auto-failover enabled

  replica->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE}, master->get_connection_options(),
      {replica->get_cluster_server()->descr()},
      [=, this](const std::shared_ptr<Instance> &instance,
                const mysqlshdk::gr::Member &gr_member) {
        if (gr_member.role != mysqlshdk::gr::Member_role::PRIMARY)
          update_replica(instance.get(), master, ar_options, false,
                         reset_channel, dry_run);
        return true;
      });

  update_replica(replica->get_cluster_server().get(), master, ar_options, true,
                 reset_channel, dry_run);
}

void Cluster_set_impl::update_replica(
    Instance *replica, Instance *master,
    const Async_replication_options &ar_options, bool primary_instance,
    bool reset_channel, bool dry_run) {
  log_info("Updating replication source at %s", replica->descr().c_str());

  auto repl_options = ar_options;

  try {
    // Enable SOURCE_CONNECTION_AUTO_FAILOVER if the instance is the primary
    if (primary_instance) {
      repl_options.auto_failover = true;
    }

    // This will re-point the slave to the new master without changing any
    // other replication parameter if ar_options has no credentials.
    async_change_primary(replica, master, k_clusterset_async_channel_name,
                         repl_options, primary_instance ? true : false, dry_run,
                         reset_channel);

    log_info("Replication source changed for %sinstance %s",
             (primary_instance ? "primary " : ""), replica->descr().c_str());
  } catch (...) {
    current_console()->print_error("Error updating replication source for " +
                                   replica->descr() + ": " +
                                   format_active_exception());

    throw shcore::Exception(
        "Could not update replication source of " + replica->descr(),
        SHERR_DBA_SWITCHOVER_ERROR);
  }

  if (primary_instance)
    update_replica_settings(replica, master, primary_instance, dry_run);
}

void Cluster_set_impl::remove_replica(Instance *instance, bool dry_run,
                                      std::string_view primary_uuid) {
  // Stop and reset the replication channel configuration
  remove_channel(*instance, k_clusterset_async_channel_name, dry_run);

  // Enabling automatic super_read_only management is a Group Replication member
  // action and those must be executed on the primary member OR in OFFLINE
  // members. When executed in the primary member, Group Replication will then
  // propagate the action to the other members. We can assume that as soon as
  // the UDF returns successfully the action change was accepted in the Group
  // and will be processed, likewise a regular transaction.
  if (primary_uuid.empty() || instance->get_uuid() == primary_uuid) {
    remove_replica_settings(instance, dry_run);
  }

  if (!dry_run) fence_instance(instance);
}

void Cluster_set_impl::record_in_metadata(
    const Cluster_id &seed_cluster_id,
    const clusterset::Create_cluster_set_options &options,
    Replication_auth_type auth_type, std::string_view member_auth_cert_issuer) {
  auto metadata = get_metadata_storage();

  MetadataStorage::Transaction trx(metadata);

  // delete unrelated records if there are any
  metadata->cleanup_for_cluster(seed_cluster_id);

  m_id = metadata->create_cluster_set_record(this, seed_cluster_id,
                                             shcore::make_dict());

  // Record the ClusterSet SSL mode in the Metadata
  metadata->update_cluster_set_attribute(
      m_id, k_cluster_set_attribute_ssl_mode,
      shcore::Value(to_string(options.ssl_mode)));

  metadata->update_cluster_set_attribute(
      m_id, k_cluster_set_attribute_member_auth_type,
      shcore::Value(to_string(auth_type)));

  metadata->update_cluster_set_attribute(
      m_id, k_cluster_set_attribute_cert_issuer,
      shcore::Value(member_auth_cert_issuer));

  metadata->update_cluster_set_attribute(
      m_id, k_cluster_attribute_replication_allowed_host,
      options.replication_allowed_host.empty()
          ? shcore::Value("%")
          : shcore::Value(options.replication_allowed_host));

  // Migrate Routers registered in the Cluster to the ClusterSet
  log_info(
      "Migrating Metadata records of the Cluster Router instances to "
      "the ClusterSet");
  metadata->migrate_routers_to_clusterset(seed_cluster_id, get_id());

  // Set and store the default Routing Options
  for (const auto &[name, value] : k_default_clusterset_router_options) {
    if ((name == k_router_option_stats_updates_frequency) &&
        (metadata->installed_version() < mysqlshdk::utils::Version(2, 2))) {
      metadata->set_global_routing_option(Cluster_type::REPLICATED_CLUSTER,
                                          m_id, name, shcore::Value(0));
    } else {
      metadata->set_global_routing_option(Cluster_type::REPLICATED_CLUSTER,
                                          m_id, name, value);
    }
  }

  // Migrate the Read Only Targets setting from Cluster to ClusterSet
  log_info(
      "Adopting as the ClusterSet's global Read Only Targets the Cluster's "
      "current setting");
  get_metadata_storage()->migrate_read_only_targets_to_clusterset(
      seed_cluster_id, get_id());

  trx.commit();
}

shcore::Value Cluster_set_impl::status(int extended) {
  check_preconditions("status");

  return shcore::Value(clusterset::cluster_set_status(this, extended));
}

shcore::Value Cluster_set_impl::describe() {
  check_preconditions("describe");

  return shcore::Value(clusterset::cluster_set_describe(this));
}

shcore::Value Cluster_set_impl::options() {
  check_preconditions("options");

  shcore::Dictionary_t inner_dict = shcore::make_dict();
  (*inner_dict)["domainName"] = shcore::Value(get_name());

  // read clusterset attributes
  {
    std::array<std::tuple<std::string_view, std::string_view>, 2> attribs{
        std::make_tuple(k_cluster_attribute_replication_allowed_host,
                        kReplicationAllowedHost),
        std::make_tuple(k_cluster_set_attribute_ssl_mode,
                        kClusterSetReplicationSslMode)};

    shcore::Array_t options = shcore::make_array();
    for (const auto &[attrib_name, attrib_desc] : attribs) {
      shcore::Value attrib_value;
      if (!get_metadata_storage()->query_cluster_set_attribute(
              get_id(), attrib_name, &attrib_value))
        attrib_value = shcore::Value::Null();
      options->emplace_back(
          shcore::Value(shcore::make_dict("option", shcore::Value(attrib_desc),
                                          "value", std::move(attrib_value))));
    }

    (*inner_dict)["globalOptions"] = shcore::Value(std::move(options));
  }

  shcore::Dictionary_t res = shcore::make_dict();
  (*res)["clusterSet"] = shcore::Value(std::move(inner_dict));
  return shcore::Value(std::move(res));
}

void Cluster_set_impl::_set_instance_option(
    const std::string & /*instance_def*/, const std::string & /*option*/,
    const shcore::Value & /*value*/) {
  throw std::logic_error("Not implemented yet.");
}

void Cluster_set_impl::update_replication_allowed_host(
    const std::string &host) {
  for (const Cluster_set_member_metadata &cluster : get_clusters()) {
    auto account = get_metadata_storage()->get_cluster_repl_account(
        cluster.cluster.cluster_id);

    if (account.second != host) {
      log_info("Re-creating account for cluster %s: %s@%s -> %s@%s",
               cluster.cluster.cluster_name.c_str(), account.first.c_str(),
               account.second.c_str(), account.first.c_str(), host.c_str());
      clone_user(*get_primary_master(), account.first, account.second,
                 account.first, host);

      get_primary_master()->drop_user(account.first, account.second, true);

      get_metadata_storage()->update_cluster_repl_account(
          cluster.cluster.cluster_id, account.first, host);
    } else {
      log_info("Skipping account recreation for cluster %s: %s@%s == %s@%s",
               cluster.cluster.cluster_name.c_str(), account.first.c_str(),
               account.second.c_str(), account.first.c_str(), host.c_str());
    }
  }
}

void Cluster_set_impl::restore_transaction_size_limit(Cluster_impl *replica,
                                                      bool dry_run) {
  shcore::Value value;

  // Get the current transaction_size_limit
  int64_t current_transaction_size_limit =
      replica->get_cluster_server()
          ->get_sysvar_int(kGrTransactionSizeLimit)
          .value_or(0);

  // Get the transaction_size_limit value from the metadata
  if (get_metadata_storage()->query_cluster_attribute(
          replica->get_id(), k_cluster_attribute_transaction_size_limit,
          &value) &&
      value.get_type() == shcore::Integer) {
    int64_t transaction_size_limit = value.as_int();

    // Only restore if the current value is zero, meaning it was set by the
    // AdminAPI. Otherwise, it was either manually changed or it was picked
    // manually by the user using the option 'transactionSizeLimit'
    if (!dry_run && current_transaction_size_limit == 0) {
      log_info("Restoring the Cluster's transaction_size_limit to '%" PRId64
               "'.",
               transaction_size_limit);
      // The primary must be reachable at this point so it will always be
      // updated, but one of more secondaries might be unreachable and it's OK
      // if they are not updated. The value is always restored from the maximum
      // to a lower one so there's no risk of transactions being rejected. The
      // user will be warned about it in cluster.status() and can fix it with
      // .rescan()
      std::unique_ptr<mysqlshdk::config::Config> config =
          replica->create_config_object({}, false, false, true, true);

      config->set(kGrTransactionSizeLimit,
                  std::optional<std::int64_t>(transaction_size_limit));

      config->apply();
    }
  } else {
    std::string msg =
        "Unable to restore Cluster's transaction size limit: "
        "group_replication_transaction_size_limit is not stored in the "
        "Metadata. Please use <Cluster>.rescan() to update the metadata.";

    // IF the current value of transaction_size_limit from the Cluster's
    // primary member is zero we must not proceed, otherwise we're
    // leaving the value set to the maximum risking memory exhaustion in the
    // replica Clusters due to the lack of the transaction size limitation
    if (current_transaction_size_limit == 0) {
      mysqlsh::current_console()->print_error(msg);

      throw shcore::Exception(
          "group_replication_transaction_size_limit not configured",
          SHERR_DBA_CLUSTER_NOT_CONFIGURED_TRANSACTION_SIZE_LIMIT);
    } else {
      // Otherwise, it can be ignored and the user is just warned about it
      mysqlsh::current_console()->print_warning(msg);
    }
  }
}

void Cluster_set_impl::set_maximum_transaction_size_limit(Cluster_impl *cluster,
                                                          bool is_primary,
                                                          bool dry_run) {
  log_info(
      "Setting transaction_size_limit to the maximum accepted value in "
      "Cluster '%s'",
      cluster->get_name().c_str());

  if (dry_run) return;

  int64_t cluster_transaction_size_limit = 0;
  if (is_primary) {
    shcore::Value value;
    if (!cluster->get_metadata_storage()->query_cluster_attribute(
            cluster->get_id(), k_cluster_attribute_transaction_size_limit,
            &value))
      return;

    cluster_transaction_size_limit = value.as_int();
  }

  // If there are Cluster members that are reachable but group_replication is
  // either disabled or not installed, attempting to set
  // group_replication_transaction_size_limit will result in an error. To avoid
  // that, we check which are those instances to let the config_handler know
  // that those are to be ignored
  std::vector<std::string> ignore_instances_vec;
  {
    auto is_gr_active = [](const mysqlshdk::mysql::IInstance &instance) {
      std::optional<std::string> plugin_state =
          instance.get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
      if (!plugin_state.has_value() ||
          plugin_state.value_or("DISABLED") != "ACTIVE") {
        return false;
      }
      return true;
    };

    cluster->execute_in_members(
        {}, cluster->get_cluster_server()->get_connection_options(), {},
        [&ignore_instances_vec, &is_gr_active](
            const std::shared_ptr<Instance> &instance,
            const mysqlshdk::gr::Member &) {
          if (!is_gr_active(*instance)) {
            ignore_instances_vec.push_back(instance->get_canonical_address());
          }

          return true;
        });
  }

  // The primary must be reachable at this point so it will always be
  // updated, but one of more secondaries might be unreachable and it's OK
  // if they are not updated. Auto-rejoins might fail due to transactions
  // being rejected, but the user will be warned about it in cluster.status()
  // and can fix it with .rescan(). Also, manually rejoining instances with
  // .rejoinInstance() will overcome the problem
  std::unique_ptr<mysqlshdk::config::Config> config =
      cluster->create_config_object(ignore_instances_vec, false, false, true);

  config->set(kGrTransactionSizeLimit,
              std::optional<int64_t>(cluster_transaction_size_limit));
  config->apply();
}

void Cluster_set_impl::_set_option(const std::string &option,
                                   const shcore::Value &value) {
  // put an exclusive lock on the clusterset
  mysqlshdk::mysql::Lock_scoped_list api_locks;
  api_locks.push_back(get_lock_exclusive());

  if (option != kReplicationAllowedHost)
    throw shcore::Exception::argument_error("Option '" + option +
                                            "' not supported.");

  if (value.get_type() != shcore::String || value.as_string().empty())
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for '%s': Argument #2 is expected "
                           "to be a string.",
                           option.c_str()));

  // we also need an exclusive lock on the primary cluster
  api_locks.push_back(m_primary_cluster->get_lock_exclusive());

  update_replication_allowed_host(value.as_string());

  get_metadata_storage()->update_cluster_set_attribute(
      get_id(), k_cluster_attribute_replication_allowed_host, value);

  current_console()->print_info(shcore::str_format(
      "Internally managed replication users updated for ClusterSet '%s'",
      get_name().c_str()));
}

void Cluster_set_impl::set_primary_cluster(
    const std::string &cluster_name,
    const clusterset::Set_primary_cluster_options &options) {
  check_preconditions("setPrimaryCluster");

  // put an exclusive lock on the clusterset
  mysqlshdk::mysql::Lock_scoped_list api_locks;
  api_locks.push_back(get_lock_exclusive());

  auto console = current_console();
  console->print_info("Switching the primary cluster of the clusterset to '" +
                      cluster_name + "'");
  if (options.dry_run)
    console->print_note("dryRun enabled, no changes will be made");

  auto primary = get_primary_master();
  shcore::on_leave_scope release_primary_finally(
      [this]() { release_primary(); });

  auto primary_cluster = get_primary_cluster();
  std::shared_ptr<Cluster_impl> promoted_cluster;
  try {
    promoted_cluster = get_cluster(cluster_name);
  } catch (const shcore::Exception &e) {
    console->print_error("Cluster '" + cluster_name +
                         "' cannot be promoted: " + e.format());
    throw;
  }

  if (promoted_cluster == primary_cluster) {
    throw shcore::Exception(
        "Cluster '" + cluster_name + "' is already the PRIMARY cluster",
        SHERR_DBA_TARGET_ALREADY_PRIMARY);
  }

  auto promoted = promoted_cluster->get_cluster_server();

  log_info(
      "Switchover primary_cluster=%s primary_instance=%s "
      "promoted_cluster=%s promoted_instance=%s",
      primary_cluster->get_name().c_str(), primary->descr().c_str(),
      promoted_cluster->get_name().c_str(), promoted->descr().c_str());

  // if there any unreachable clusters we collect them in this list, which will
  // be used by connect_all_clusters() for skipping clusters
  std::list<Cluster_id> unreachable;

  console->print_info("* Verifying clusterset status");
  check_clusters_available(options.invalidate_replica_clusters, &unreachable);

  std::list<std::shared_ptr<Cluster_impl>> clusters(
      connect_all_clusters(0, false, &unreachable));
  shcore::on_leave_scope release_cluster_primaries([&clusters] {
    for (auto &c : clusters) c->release_primary();
  });

  // put an exclusive lock on each reachable cluster
  for (const auto &cluster : clusters)
    api_locks.push_back(cluster->get_lock_exclusive());

  // another set of connections for locks
  // get triggered before server-side timeouts
  std::list<std::shared_ptr<Cluster_impl>> lock_clusters(
      connect_all_clusters(options.timeout + 5, false, &unreachable));
  shcore::on_leave_scope release_lock_primaries(
      [this, &lock_clusters, options] {
        for (auto &c : lock_clusters) {
          if (!options.dry_run && options.timeout > 0) {
            c->get_cluster_server()->set_sysvar_default(
                "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::SESSION);
          }
          c->release_primary();
        }
        get_primary_master()->set_sysvar_default(
            "lock_wait_timeout", mysqlshdk::mysql::Var_qualifier::SESSION);
      });

  std::list<std::shared_ptr<Instance>> lock_instances;
  for (auto &c : lock_clusters) {
    if (!options.dry_run && options.timeout > 0) {
      c->get_cluster_server()->set_sysvar(
          "lock_wait_timeout", static_cast<int64_t>(options.timeout),
          mysqlshdk::mysql::Var_qualifier::SESSION);
    }

    lock_instances.emplace_back(c->get_primary_master());
  }

  get_primary_master()->set_sysvar("lock_wait_timeout",
                                   static_cast<int64_t>(options.timeout),
                                   mysqlshdk::mysql::Var_qualifier::SESSION);

  get_metadata_storage()->get_md_server()->set_sysvar(
      "lock_wait_timeout", static_cast<int64_t>(options.timeout),
      mysqlshdk::mysql::Var_qualifier::SESSION);

  console->print_info();

  // make sure that all instances have the most up-to-date GTID
  console->print_info(
      "** Waiting for the promoted cluster to apply pending received "
      "transactions...");

  if (!options.dry_run) {
    mysqlshdk::mysql::Replication_channel channel;
    if (get_channel_status(*promoted, mysqlshdk::gr::k_gr_applier_channel,
                           &channel)) {
      switch (channel.status()) {
        case mysqlshdk::mysql::Replication_channel::OFF:
        case mysqlshdk::mysql::Replication_channel::APPLIER_OFF:
        case mysqlshdk::mysql::Replication_channel::APPLIER_ERROR:
          break;
        default: {
          auto timeout =
              (options.timeout <= 0)
                  ? options.timeout
                  : current_shell_options()->get().dba_gtid_wait_timeout;
          wait_for_apply_retrieved_trx(*promoted,
                                       mysqlshdk::gr::k_gr_applier_channel,
                                       std::chrono::seconds{timeout}, true);
        }
      }
    }
  }

  ensure_transaction_set_consistent_and_recoverable(
      promoted.get(), primary.get(), primary_cluster.get(), false,
      options.dry_run, nullptr);

  // Get the current settings for the ClusterSet replication channel
  auto ar_options_demoted =
      get_clusterset_replication_options(primary_cluster->get_id(), nullptr);

  console->print_info("* Refreshing replication account of demoted cluster");
  // Re-generate a new password for the master being demoted.
  ar_options_demoted.repl_credentials =
      refresh_cluster_replication_user(primary_cluster.get(), options.dry_run);

  console->print_info("* Synchronizing transaction backlog at " +
                      promoted->descr());
  if (!options.dry_run) {
    sync_transactions(*promoted, k_clusterset_async_channel_name,
                      options.timeout);
  }
  console->print_info();

  // Restore the transaction_size_limit value to the original one
  restore_transaction_size_limit(promoted_cluster.get(), options.dry_run);

  Undo_tracker undo_tracker;

  try {
    console->print_info("* Updating metadata");

    // Update the metadata with the state the replicaset is supposed to be in
    log_info("Updating metadata at %s",
             m_metadata_storage->get_md_server()->descr().c_str());
    if (!options.dry_run) {
      {
        MetadataStorage::Transaction trx(m_metadata_storage);
        m_metadata_storage->record_cluster_set_primary_switch(
            get_id(), promoted_cluster->get_id(), unreachable);
        trx.commit();
      }

      undo_tracker.add("", [=, this]() {
        MetadataStorage::Transaction trx(m_metadata_storage);
        m_metadata_storage->record_cluster_set_primary_switch(
            get_id(), primary_cluster->get_id(), {});
        trx.commit();
      });

      for (const auto &c : options.invalidate_replica_clusters) {
        console->print_note(
            "Cluster '" + c +
            "' was INVALIDATED and no longer part of the clusterset.");
      }
    }
    console->print_info();

    console->print_info("* Updating topology");

    // Do the actual switchover:
    // - stop/delete channel from primary of promoted
    // - create channel in all members of demoted
    // - enable failover at primary of demoted
    // - start replica at demoted
    {
      auto repl_source = promoted_cluster->get_cluster_server();

      undo_tracker.add("", [this, primary_cluster, options]() {
        promote_to_primary(primary_cluster.get(), false, options.dry_run);
      });

      demote_from_primary(primary_cluster.get(), repl_source.get(),
                          ar_options_demoted, options.dry_run);
    }

    {
      // Synchronize all slaves and lock all instances.
      Global_locks global_locks;
      try {
        console->print_info("* Acquiring locks in ClusterSet instances");
        global_locks.acquire(lock_instances, primary->get_uuid(),
                             options.timeout, options.dry_run);
      } catch (const std::exception &e) {
        console->print_error(
            shcore::str_format("An error occurred while preparing "
                               "instances for a PRIMARY switch: %s",
                               e.what()));
        throw;
      }

      console->print_info(
          "* Synchronizing remaining transactions at promoted primary");
      if (!options.dry_run) {
        sync_transactions(*promoted, k_clusterset_async_channel_name,
                          options.timeout);
      }
      console->print_info();
    }

    undo_tracker.add(
        "", [this, promoted_cluster, primary, primary_cluster, options]() {
          auto ar_options = get_clusterset_replication_options(
              promoted_cluster->get_id(), nullptr);

          auto repl_source = primary_cluster->get_cluster_server();

          demote_from_primary(promoted_cluster.get(), repl_source.get(),
                              ar_options, options.dry_run);
        });

    promote_to_primary(promoted_cluster.get(), true, options.dry_run);

    console->print_info("* Updating replica clusters");

    DBUG_EXECUTE_IF("set_primary_cluster_skip_update_replicas", {
      std::cerr << "Skipping replica update "
                   "(set_primary_cluster_skip_update_replicas)\n";
      return;
    });

    // NOTE: up to this point, we already switched to the new primary, but the
    // objects (Cluster_set_impl / Base_cluster_impl) haven't been updated yet
    // (they will be the next time a command calls check_preconditions(...)).
    // But we can't update them here because there's still a possibility of
    // having to revert want the command did, which means that calling
    // primary_instance_did_change(), for example, isn't an option. In other
    // words, from this point forward, we need to be careful when calling APIs
    // on these objects.

    for (const auto &replica : clusters) {
      if (promoted_cluster->get_name() == replica->get_name() ||
          primary_cluster->get_name() == replica->get_name())
        continue;

      bool reset_repl_channel;
      auto ar_options = get_clusterset_replication_options(replica->get_id(),
                                                           &reset_repl_channel);

      auto repl_source = promoted_cluster->get_cluster_server();

      if (reset_repl_channel) {
        ar_options.repl_credentials = refresh_cluster_replication_user(
            *promoted_cluster->get_cluster_server(), replica.get(),
            options.dry_run);
      }

      undo_tracker.add("", [this, &replica, primary, ar_options,
                            reset_repl_channel, options, console]() {
        try {
          update_replica(replica.get(), primary.get(), ar_options,
                         reset_repl_channel, options.dry_run);
        } catch (...) {
          console->print_error("Could not revert changes to " +
                               replica->get_name() + ": " +
                               format_active_exception());
        }
      });

      update_replica(replica.get(), repl_source.get(), ar_options,
                     reset_repl_channel, options.dry_run);

      log_info("PRIMARY changed for cluster %s", replica->get_name().c_str());
    }

    // reset replication channel from the promoted primary after revert isn't
    // needed anymore
    delete_async_channel(promoted_cluster.get(), options.dry_run);

    // we reset the clusters repl channel, so we can clean the options (remove
    // nulls) from the metadata
    if (!options.dry_run) {
      auto metadata = std::make_shared<MetadataStorage>(
          promoted_cluster->get_cluster_server());

      MetadataStorage::Transaction trx(metadata);
      for (const auto &replica : clusters)
        cleanup_replication_options(metadata.get(), replica->get_id());

      trx.commit();
    }

  } catch (...) {
    current_console()->print_info(
        "* Could not complete operation, attempting to revert changes...");
    try {
      undo_tracker.execute();
    } catch (...) {
      current_console()->print_error("Could not revert changes: " +
                                     format_active_exception());
    }
    throw;
  }

  console->print_info("Cluster '" + promoted_cluster->get_name() +
                      "' was promoted to PRIMARY of the clusterset. The "
                      "PRIMARY instance is '" +
                      promoted->descr() + "'");
  console->print_info();

  if (options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Cluster_set_impl::reconcile_view_change_gtids(
    mysqlshdk::mysql::IInstance *replica,
    const mysqlshdk::mysql::Gtid_set &missing_view_gtids) {
  using mysqlshdk::mysql::Gtid_set;
  auto primary = get_primary_master();

  Gtid_set gtid_set = missing_view_gtids;
  if (gtid_set.empty()) {
    Gtid_set primary_gtid_set = Gtid_set::from_gtid_executed(*primary);
    auto view_change_uuid =
        replica->get_sysvar_string("group_replication_view_change_uuid", "");

    gtid_set =
        Gtid_set::from_gtid_executed(*replica).get_gtids_from(view_change_uuid);

    gtid_set.subtract(primary_gtid_set, *replica);
  }

  log_info(
      "Group of instance %s has %s VCLE GTIDs that will be reconciled by "
      "injecting them at %s: %s",
      replica->descr().c_str(), std::to_string(gtid_set.count()).c_str(),
      primary->descr().c_str(), gtid_set.str().c_str());

  inject_gtid_set(*primary, gtid_set);
}

void Cluster_set_impl::check_clusters_available(
    const std::list<std::string> &invalidate_clusters,
    std::list<Cluster_id> *inout_unreachable) {
  auto console = current_console();
  size_t num_unavailable_clusters = 0;

  std::vector<Cluster_set_member_metadata> clusters;
  m_metadata_storage->get_cluster_set(get_id(), true, nullptr, &clusters);

  // check for any invalid cluster names in invalidReplicaClusters
  for (const auto &name : invalidate_clusters) {
    if (std::find_if(clusters.begin(), clusters.end(),
                     [name](const Cluster_set_member_metadata &c) {
                       return c.cluster.cluster_name == name;
                     }) == clusters.end()) {
      throw shcore::Exception::argument_error(
          "Invalid cluster name '" + name +
          "' in option invalidateReplicaClusters");
    }
  }

  for (const auto &cluster_md : clusters) {
    if (cluster_md.invalidated) continue;

    if (std::find(inout_unreachable->begin(), inout_unreachable->end(),
                  cluster_md.cluster.cluster_id) == inout_unreachable->end()) {
      console->print_info("** Checking cluster " +
                          cluster_md.cluster.cluster_name);
      if (!check_cluster_available(cluster_md.cluster.cluster_name)) {
        inout_unreachable->emplace_back(cluster_md.cluster.cluster_id);

        if (std::find(invalidate_clusters.begin(), invalidate_clusters.end(),
                      cluster_md.cluster.cluster_name) !=
            invalidate_clusters.end()) {
          console->print_note("Cluster '" + cluster_md.cluster.cluster_name +
                              "' will be INVALIDATED as requested");
        } else {
          num_unavailable_clusters++;
        }
      } else {
        console->print_info("  Cluster '" + cluster_md.cluster.cluster_name +
                            "' is available");
        if (std::find(invalidate_clusters.begin(), invalidate_clusters.end(),
                      cluster_md.cluster.cluster_name) !=
            invalidate_clusters.end()) {
          console->print_error("Cluster '" + cluster_md.cluster.cluster_name +
                               "' is available and cannot be INVALIDATED");
          throw shcore::Exception::argument_error(
              "Invalid value for option invalidateReplicaClusters");
        }
      }
    }
  }

  if (num_unavailable_clusters > 0) {
    console->print_error(shcore::str_format(
        "Operation cannot be completed because %zi of %zi "
        "clusters are not available. Please correct these "
        "issues before proceeding. If you'd like to "
        "INVALIDATE these clusters and continue anyway, "
        "add the cluster name to the 'invalidateReplicaClusters' option.",
        num_unavailable_clusters, clusters.size()));

    throw shcore::Exception("One or more replica clusters are unavailable",
                            SHERR_DBA_UNAVAILABLE_REPLICA_CLUSTERS);
  }
}

bool Cluster_set_impl::check_cluster_available(
    const std::string &cluster_name) {
  auto console = current_console();

  std::shared_ptr<Cluster_impl> cluster;
  try {
    cluster = get_cluster(cluster_name);
  } catch (...) {
    console->print_warning("Could not get a handle to cluster '" +
                           cluster_name + "': " + format_active_exception());
    return false;
  }

  // check that the cluster has quorum and its primary is reachable
  try {
    cluster->acquire_primary();
    cluster->release_primary();
  } catch (...) {
    console->print_warning("Error in cluster '" + cluster_name +
                           "': " + format_active_exception());

    return false;
  }
  return true;
}

void Cluster_set_impl::verify_primary_cluster_not_recoverable() {
  auto console = current_console();

  auto primary_cluster = get_primary_cluster();

  switch (primary_cluster->cluster_availability()) {
    case Cluster_availability::ONLINE_NO_PRIMARY:
    case Cluster_availability::ONLINE:
      console->print_error("The PRIMARY cluster '" +
                           primary_cluster->get_name() +
                           "' is still available and has quorum.");

      throw shcore::Exception("PRIMARY cluster '" +
                                  primary_cluster->get_name() +
                                  "' is still available",
                              SHERR_DBA_CLUSTER_PRIMARY_STILL_AVAILABLE);

    case Cluster_availability::NO_QUORUM:
      console->print_error(
          "The PRIMARY cluster '" + primary_cluster->get_name() +
          "' has reachable members that can be used to restore it using the "
          "cluster.<<<forceQuorumUsingPartitionOf>>>() command.");

      throw shcore::Exception(
          "PRIMARY cluster '" + primary_cluster->get_name() +
              "' is in state NO_QUORUM but can still be restored",
          SHERR_DBA_CLUSTER_STILL_RESTORABLE);

    case Cluster_availability::OFFLINE:
    case Cluster_availability::SOME_UNREACHABLE:
      console->print_error(
          "Instances of the PRIMARY cluster '" + primary_cluster->get_name() +
          "' are reachable but OFFLINE. Use the "
          "dba.<<<rebootClusterFromCompleteOutage>>>() command to restore it.");

      throw shcore::Exception(
          "PRIMARY cluster '" + primary_cluster->get_name() +
              "' is in state OFFLINE but can still be restored",
          SHERR_DBA_CLUSTER_STILL_RESTORABLE);

    case Cluster_availability::UNREACHABLE:
      console->print_info("None of the instances of the PRIMARY cluster '" +
                          primary_cluster->get_name() + "' could be reached.");
      break;
  }
}

void Cluster_set_impl::check_gtid_set_most_recent(
    Instance *promoted,
    const std::list<std::shared_ptr<Cluster_impl>> &clusters) {
  auto get_filtered_gtid_set =
      [](mysqlshdk::mysql::IInstance *instance,
         mysqlshdk::mysql::Gtid_set *out_view_changes) {
        auto gtid_set =
            mysqlshdk::mysql::Gtid_set::from_gtid_executed(*instance);

        /*
        NOTE: this is not the correct way to handle this: if the query returns
        std::nullopt, then it will convert to an empty string: this compromises
        the execution of the command. Although this error is very very rare (and
        implies much bigger underlying issues), it should be treated as such
        (maybe raise an exception) so that it can be handled properly. That,
        however, means that logic to deal with the exception must be created
        further up the stack.
        */
        auto uuid = instance->get_sysvar_string(
            "group_replication_view_change_uuid", "");

        auto view_changes = gtid_set.get_gtids_from(uuid);
        if (out_view_changes) *out_view_changes = view_changes;
        return gtid_set.subtract(view_changes, *instance);
      };

  mysqlshdk::mysql::Gtid_set promoted_view_changes;
  auto promoted_gtid_set =
      get_filtered_gtid_set(promoted, &promoted_view_changes);

  auto console = current_console();

  bool up_to_date = true;

  for (auto i : clusters) {
    auto primary = i->get_cluster_server();

    if (primary->get_uuid() != promoted->get_uuid()) {
      auto gtid_set = get_filtered_gtid_set(primary.get(), nullptr);

      gtid_set.subtract(promoted_view_changes, *promoted);

      if (!promoted_gtid_set.contains(gtid_set, *promoted)) {
        console->print_note("Cluster " + i->get_name() +
                            " has a more up-to-date GTID set");

        promoted_gtid_set.subtract(gtid_set, *promoted);

        console->print_info(
            "The following GTIDs are missing from the target cluster: " +
            promoted_gtid_set.str());

        up_to_date = false;
      }
    }
  }

  if (!up_to_date) {
    console->print_error(
        "The selected target cluster is not the most up-to-date cluster "
        "available for failover.");
    throw shcore::Exception("Target cluster is behind other candidates",
                            SHERR_DBA_BADARG_INSTANCE_OUTDATED);
  }
}

void Cluster_set_impl::primary_instance_did_change(
    const std::shared_ptr<Instance> &new_primary) {
  if (m_primary_master) m_primary_master->release();
  m_primary_master.reset();

  if (new_primary) {
    m_primary_master = new_primary;
    new_primary->retain();

    m_metadata_storage = std::make_shared<MetadataStorage>(new_primary);
  }
}

void Cluster_set_impl::force_primary_cluster(
    const std::string &cluster_name,
    const clusterset::Force_primary_cluster_options &options) {
  check_preconditions("forcePrimaryCluster");

  auto console = current_console();
  auto primary_cluster_name = get_primary_cluster()->get_name();
  int lock_timeout = current_shell_options()->get().dba_gtid_wait_timeout;

  console->print_info("Failing-over primary cluster of the clusterset to '" +
                      cluster_name + "'");

  console->print_info("* Verifying primary cluster status");
  connect_primary();
  verify_primary_cluster_not_recoverable();

  std::shared_ptr<Cluster_impl> promoted_cluster;
  try {
    promoted_cluster = get_cluster(cluster_name);
  } catch (const shcore::Exception &e) {
    console->print_error("Could not reach cluster '" + cluster_name +
                         "': " + e.format());
    throw;
  }

  if (promoted_cluster->is_primary_cluster()) {
    throw shcore::Exception(
        "Target cluster '" + cluster_name + "' is the current PRIMARY",
        SHERR_DBA_TARGET_ALREADY_PRIMARY);
  }

  auto promoted = promoted_cluster->get_cluster_server();

  // if there any unreachable clusters we collect them in this list, which will
  // be used by connect_all_clusters() for skipping clusters
  std::list<Cluster_id> unreachable;

  // skip checking the current primary
  unreachable.push_back(get_primary_cluster_metadata().cluster.cluster_id);

  console->print_info("* Verifying clusterset status");
  check_clusters_available(options.invalidate_replica_clusters, &unreachable);

  std::list<std::shared_ptr<Cluster_impl>> clusters(
      connect_all_clusters(0, false, &unreachable));
  shcore::on_leave_scope release_cluster_primaries([&clusters] {
    for (auto &c : clusters) c->release_primary();
  });

  // put an exclusive lock on each reachable cluster
  mysqlshdk::mysql::Lock_scoped_list api_locks;
  for (const auto &cluster : clusters)
    api_locks.push_back(cluster->get_lock_exclusive());

  // another set of connections for locks
  // get triggered before server-side timeouts
  std::list<std::shared_ptr<Cluster_impl>> lock_clusters(
      connect_all_clusters(lock_timeout + 5, false, &unreachable));
  shcore::on_leave_scope release_lock_primaries([&lock_clusters] {
    for (auto &c : lock_clusters) c->release_primary();
  });
  std::list<std::shared_ptr<Instance>> lock_instances;
  for (auto &c : lock_clusters) {
    lock_instances.emplace_back(c->get_primary_master());
  }

  // make sure that all instances have the most up-to-date GTID
  {
    console->print_info(
        "** Waiting for instances to apply pending received transactions...");

    if (!options.dry_run) {
      for (const auto &c : lock_clusters) {
        mysqlshdk::mysql::Replication_channel channel;
        if (!get_channel_status(*c->get_cluster_server(),
                                mysqlshdk::gr::k_gr_applier_channel, &channel))
          continue;

        if (auto status = channel.status();
            (status == mysqlshdk::mysql::Replication_channel::OFF) ||
            (status == mysqlshdk::mysql::Replication_channel::APPLIER_OFF) ||
            (status == mysqlshdk::mysql::Replication_channel::APPLIER_ERROR))
          continue;

        wait_for_apply_retrieved_trx(*c->get_cluster_server(),
                                     mysqlshdk::gr::k_gr_applier_channel,
                                     options.get_timeout(), true);
      }
    }
  }

  console->print_info(
      "** Checking whether target cluster has the most recent GTID set");
  check_gtid_set_most_recent(promoted.get(), lock_clusters);

  if (lock_timeout > 0) {
    promoted->set_sysvar("lock_wait_timeout",
                         static_cast<int64_t>(lock_timeout),
                         mysqlshdk::mysql::Var_qualifier::SESSION);
  }
  shcore::on_leave_scope restore_lock_wait_timeout([promoted, lock_timeout]() {
    if (!promoted || (lock_timeout <= 0)) return;
    try {
      promoted->set_sysvar_default("lock_wait_timeout",
                                   mysqlshdk::mysql::Var_qualifier::SESSION);
    } catch (...) {
      log_info("Could not restore lock_wait_timeout: %s",
               format_active_exception().c_str());
    }
  });

  // Restore the transaction_size_limit value to the original one
  restore_transaction_size_limit(promoted_cluster.get(), options.dry_run);

  console->print_info("* Promoting cluster '" + promoted_cluster->get_name() +
                      "'");

  promote_to_primary(promoted_cluster.get(), false, options.dry_run);

  // This will update the MD object to use the new primary
  if (!options.dry_run) {
    primary_instance_did_change(promoted_cluster->get_primary_master());
  }

  console->print_info("* Updating metadata");

  // Update the metadata with the state the replicaset is supposed to be in
  log_info("Updating metadata at %s",
           m_metadata_storage->get_md_server()->descr().c_str());
  if (!options.dry_run) {
    MetadataStorage::Transaction trx(m_metadata_storage);
    m_metadata_storage->record_cluster_set_primary_failover(
        get_id(), promoted_cluster->get_id(), unreachable);
    trx.commit();

    for (const auto &c : options.invalidate_replica_clusters) {
      console->print_note(
          "Cluster '" + c +
          "' was INVALIDATED and is no longer part of the clusterset.");
    }
  }
  console->print_info();

  for (const auto &replica : clusters) {
    if (replica->get_id() == promoted_cluster->get_id()) continue;

    bool reset_repl_channel;
    auto ar_options = get_clusterset_replication_options(replica->get_id(),
                                                         &reset_repl_channel);

    auto repl_source = promoted_cluster->get_cluster_server();

    if (reset_repl_channel)
      ar_options.repl_credentials =
          refresh_cluster_replication_user(replica.get(), options.dry_run);

    update_replica(replica.get(), repl_source.get(), ar_options,
                   reset_repl_channel, options.dry_run);

    if (!options.dry_run) {
      MetadataStorage::Transaction trx(m_metadata_storage);
      cleanup_replication_options(m_metadata_storage.get(), replica->get_id());
      trx.commit();
    }

    log_info("PRIMARY changed for cluster %s", replica->get_name().c_str());
  }

  console->print_info("PRIMARY cluster failed-over to '" +
                      promoted_cluster->get_name() +
                      "'. The PRIMARY instance is '" + promoted->descr() + "'");
  console->print_info("Former PRIMARY cluster '" + primary_cluster_name +
                      "' was INVALIDATED, transactions that were not yet "
                      "replicated may be lost.");
  console->print_info(
      "In case of network partitions and similar, the former PRIMARY "
      "Cluster '" +
      primary_cluster_name +
      "' may still be online and a split-brain can happen. To avoid "
      "it, use <Cluster>.<<<fenceAllTraffic>>>() to fence the Cluster from "
      "all application traffic, or <Cluster>.<<<fenceWrites()>>> to fence it "
      "from write traffic only.");
  console->print_info();

  if (options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

void Cluster_set_impl::ensure_transaction_set_consistent_and_recoverable(
    mysqlshdk::mysql::IInstance *replica, mysqlshdk::mysql::IInstance *primary,
    Cluster_impl *primary_cluster, bool allow_unrecoverable, bool dry_run,
    bool *out_is_recoverable) {
  mysqlshdk::mysql::Gtid_set missing_view_gtids;
  mysqlshdk::mysql::Gtid_set errant_gtids;
  mysqlshdk::mysql::Gtid_set unrecoverable_gtids;

  {
    // purged GTIDs from all members of the primary
    std::vector<mysqlshdk::mysql::Gtid_set> purged_gtids;

    primary_cluster->execute_in_members(
        [&purged_gtids](const std::shared_ptr<Instance> &instance,
                        const Cluster_impl::Instance_md_and_gr_member &) {
          auto purged = mysqlshdk::mysql::Gtid_set::from_gtid_purged(*instance);

          log_debug("gtid_purged@%s=%s", instance->descr().c_str(),
                    purged.str().c_str());
          purged_gtids.emplace_back(purged);
          return true;
        },
        [](const shcore::Error &err,
           const Cluster_impl::Instance_md_and_gr_member &i) {
          log_debug("could not connect to %s:%i to query gtid_purged: %s",
                    i.second.host.c_str(), i.second.port, err.format().c_str());
          return true;
        });

    std::vector<Cluster_set_member_metadata> members;
    get_metadata_storage()->get_cluster_set(get_id(), true, nullptr, &members);

    std::vector<std::string> allowed_errant_uuids;
    allowed_errant_uuids.reserve(members.size());
    for (const auto &m : members) {
      allowed_errant_uuids.push_back(m.cluster.view_change_uuid);
    }

    mysqlshdk::mysql::Gtid_set missing_gtids;

    mysqlshdk::mysql::compute_joining_replica_gtid_state(
        *primary, mysqlshdk::mysql::Gtid_set::from_gtid_executed(*primary),
        purged_gtids, mysqlshdk::mysql::Gtid_set::from_gtid_executed(*replica),
        allowed_errant_uuids, &missing_gtids, &unrecoverable_gtids,
        &errant_gtids, &missing_view_gtids);

    log_info(
        "GTID check between '%s' and '%s' ('%s'): missing='%s' errant='%s' "
        "unrecoverable='%s' vcle='%s'",
        replica->descr().c_str(), primary_cluster->get_name().c_str(),
        primary->descr().c_str(), missing_gtids.str().c_str(),
        errant_gtids.str().c_str(), unrecoverable_gtids.str().c_str(),
        missing_view_gtids.str().c_str());
  }

  if (out_is_recoverable) *out_is_recoverable = true;

  if (!errant_gtids.empty()) {
    current_console()->print_info(shcore::str_format(
        "%s has the following errant transactions not present at %s: %s",
        replica->descr().c_str(), primary_cluster->get_name().c_str(),
        errant_gtids.str().c_str()));
    throw shcore::Exception(
        "Errant transactions detected at " + replica->descr(),
        SHERR_DBA_DATA_ERRANT_TRANSACTIONS);
  }

  auto console = current_console();

  if (!unrecoverable_gtids.empty()) {
    auto msg = shcore::str_format(
        "The following transactions were purged from %s and "
        "not present at %s: %s",
        primary_cluster->get_name().c_str(), replica->descr().c_str(),
        unrecoverable_gtids.str().c_str());
    if (allow_unrecoverable) {
      console->print_warning(msg);
      if (out_is_recoverable) *out_is_recoverable = false;
    } else {
      console->print_error(msg);

      throw shcore::Exception(
          "Cluster is unable to recover one or more transactions that have "
          "been purged from the PRIMARY cluster",
          SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
    }
  }

  if (!missing_view_gtids.empty()) {
    console->print_info(
        shcore::str_format("* Reconciling %u internally generated GTIDs",
                           static_cast<unsigned>(missing_view_gtids.count())));
    if (!dry_run) reconcile_view_change_gtids(replica, missing_view_gtids);
    console->print_info();
  }
}

void Cluster_set_impl::rejoin_cluster(
    const std::string &cluster_name,
    const clusterset::Rejoin_cluster_options &options, bool allow_unavailable) {
  check_preconditions("rejoinCluster");

  // put a shared lock on the clusterset and a shared cluster lock on the
  // primary cluster
  auto cs_lock = get_lock_shared();
  auto pc_lock = m_primary_cluster->get_lock_shared();

  auto console = current_console();
  console->print_info("Rejoining cluster '" + cluster_name +
                      "' to the clusterset");

  auto primary_cluster = get_primary_cluster();
  auto primary = get_primary_master();
  std::shared_ptr<Cluster_impl> rejoining_cluster;
  try {
    rejoining_cluster = get_cluster(cluster_name, allow_unavailable, true);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GROUP_HAS_NO_PRIMARY) {
      if (rejoining_cluster = get_cluster(cluster_name, true, true);
          rejoining_cluster->cluster_availability() ==
          Cluster_availability::OFFLINE) {
        console->print_error(shcore::str_format(
            "The Cluster '%s' is reachable but OFFLINE. Use the "
            "dba.<<<rebootClusterFromCompleteOutage>>>() command to restore "
            "it.",
            cluster_name.c_str()));

        throw shcore::Exception(
            shcore::str_format("The Cluster '%s' is reachable but OFFLINE.",
                               cluster_name.c_str()),
            SHERR_DBA_CLUSTER_OFFLINE);
      }
    }

    console->print_error(shcore::str_format("Could not reach cluster '%s': %s",
                                            cluster_name.c_str(),
                                            e.format().c_str()));
    throw;
  }

  // put an exclusive lock on the target cluster
  auto tc_lock = rejoining_cluster->get_lock_exclusive();

  auto rejoining_primary = rejoining_cluster->get_cluster_server();

  bool refresh_only = false;

  // check if the cluster is invalidated
  if (!rejoining_cluster->is_invalidated()) {
    refresh_only = true;
  } else {
    console->print_note("Cluster '" + rejoining_cluster->get_name() +
                        "' is invalidated");
    refresh_only = false;
  }

  // check gtid set
  try {
    ensure_transaction_set_consistent_and_recoverable(
        rejoining_primary.get(), primary.get(), primary_cluster.get(), false,
        options.dry_run, nullptr);
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE) {
      console->print_error(
          "Cluster '" + rejoining_cluster->get_name() +
          "' cannot be rejoined because it's missing transactions that have "
          "been purged from the binary log of '" +
          primary->descr() + "'");
      console->print_info(
          "The cluster must be re-provisioned with clone or some other "
          "method.");
    } else if (e.code() == SHERR_DBA_DATA_ERRANT_TRANSACTIONS) {
      console->print_error(
          "Cluster '" + rejoining_cluster->get_name() +
          "' cannot be rejoined because it contains transactions that did not "
          "originate from the primary of the clusterset.");
    }
    throw;
  }

  set_maximum_transaction_size_limit(
      rejoining_cluster.get(),
      rejoining_cluster->get_id() == primary_cluster->get_id(),
      options.dry_run);

  bool reset_repl_channel{false};
  mysqlsh::dba::Async_replication_options ar_options =
      get_clusterset_replication_options(rejoining_cluster->get_id(),
                                         &reset_repl_channel);

  auto repl_source = primary_cluster->get_cluster_server();

  if (refresh_only) {
    // update replica cluster if the primary isn't the one being "rejoined"
    if (rejoining_cluster->get_id() != primary_cluster->get_id()) {
      console->print_info("* Refreshing replication settings");

      ar_options.repl_credentials = refresh_cluster_replication_user(
          rejoining_cluster.get(), options.dry_run);

      update_replica(rejoining_cluster.get(), repl_source.get(), ar_options,
                     reset_repl_channel, options.dry_run);
    }
  } else {
    console->print_info("* Updating metadata");
    if (!options.dry_run) {
      auto metadata = get_metadata_storage();
      MetadataStorage::Transaction trx(metadata);
      metadata->record_cluster_set_member_rejoined(
          get_id(), rejoining_cluster->get_id(), primary_cluster->get_id());
      trx.commit();
    }
    console->print_info();

    console->print_info("* Rejoining cluster");

    // configure as a replica
    ar_options.repl_credentials = refresh_cluster_replication_user(
        rejoining_cluster.get(), options.dry_run);

    update_replica(rejoining_cluster.get(), repl_source.get(), ar_options,
                   reset_repl_channel, options.dry_run);
  }

  if (reset_repl_channel && !options.dry_run) {
    // cleanup NULL replication options (to avoid resetting the channel on
    // subsequent calls)
    auto metadata = get_metadata_storage();
    MetadataStorage::Transaction trx(metadata);

    cleanup_replication_options(metadata.get(), rejoining_cluster->get_id());

    trx.commit();
  }

  console->print_info();

  rejoining_cluster->invalidate_cluster_set_metadata_cache();

  console->print_info("Cluster '" + cluster_name +
                      "' was rejoined to the clusterset");
}

void Cluster_set_impl::setup_admin_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  check_preconditions("setupAdminAccount");

  // put a shared lock on the cluster
  auto c_lock = get_lock_shared();

  Base_cluster_impl::setup_admin_account(username, host, options);
}

void Cluster_set_impl::setup_router_account(
    const std::string &username, const std::string &host,
    const Setup_account_options &options) {
  check_preconditions("setupRouterAccount");

  // put a shared lock on the cluster
  auto c_lock = get_lock_shared();

  Base_cluster_impl::setup_router_account(username, host, options);
}

Cluster_ssl_mode Cluster_set_impl::query_clusterset_ssl_mode() const {
  if (shcore::Value value;
      get_metadata_storage()->query_cluster_set_attribute(
          get_id(), k_cluster_set_attribute_ssl_mode, &value) &&
      (value.get_type() == shcore::String))
    return to_cluster_ssl_mode(value.as_string());

  return Cluster_ssl_mode::NONE;
}

Replication_auth_type Cluster_set_impl::query_clusterset_auth_type() const {
  if (shcore::Value value;
      get_metadata_storage()->query_cluster_set_attribute(
          get_id(), k_cluster_set_attribute_member_auth_type, &value) &&
      (value.get_type() == shcore::String))
    return to_replication_auth_type(value.as_string());

  return Replication_auth_type::PASSWORD;
}

std::string Cluster_set_impl::query_clusterset_auth_cert_issuer() const {
  if (shcore::Value value;
      get_metadata_storage()->query_cluster_set_attribute(
          get_id(), k_cluster_set_attribute_cert_issuer, &value) &&
      (value.get_type() == shcore::String))
    return value.as_string();

  return {};
}

void Cluster_set_impl::read_cluster_replication_options(
    const Cluster_id &cluster_id, Async_replication_options *ar_options,
    bool *has_null_options) const {
  assert(ar_options);

  bool null_options{false};

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_connect_retry, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->connect_retry = static_cast<int>(value.as_int());
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_retry_count, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->retry_count = static_cast<int>(value.as_int());
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_heartbeat_period, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if ((value.get_type() == shcore::Integer) ||
        (value.get_type() == shcore::Float))
      ar_options->heartbeat_period = value.as_double();
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_compression_algorithms,
          &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->compression_algos = value.as_string();
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_zstd_compression_level,
          &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::Integer)
      ar_options->zstd_compression_level = static_cast<int>(value.as_int());
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_bind, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->bind = value.as_string();
  }

  if (shcore::Value value; get_metadata_storage()->query_cluster_attribute(
          cluster_id, k_cluster_attribute_repl_network_namespace, &value)) {
    null_options |= (value.get_type() == shcore::Null);
    if (value.get_type() == shcore::String)
      ar_options->network_namespace = value.as_string();
  }

  if (has_null_options) *has_null_options = null_options;
}

mysqlshdk::mysql::Lock_scoped Cluster_set_impl::get_lock(
    mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout) {
  if (!m_primary_master->is_lock_service_installed()) {
    bool lock_service_installed = false;
    // if SRO is disabled, we have a chance to install the lock service
    if (bool super_read_only =
            m_primary_master->get_sysvar_bool("super_read_only", false);
        !super_read_only) {
      // we must disable log_bin to prevent the installation from being
      // replicated
      mysqlshdk::mysql::Suppress_binary_log nobinlog(m_primary_master.get());

      lock_service_installed =
          m_primary_master->ensure_lock_service_is_installed(false);
    }

    if (!lock_service_installed) {
      log_warning(
          "The required MySQL Locking Service isn't installed on instance "
          "'%s'. The operation will continue without concurrent execution "
          "protection.",
          m_primary_master->descr().c_str());
      return nullptr;
    }
  }

  DBUG_EXECUTE_IF("dba_locking_timeout_one",
                  { timeout = std::chrono::seconds{1}; });

  // Try to acquire the specified lock.
  //
  // NOTE: Only one lock per namespace is used because lock release is performed
  // by namespace.
  try {
    log_debug("Acquiring %s lock ('%s', '%s') on '%s'.",
              mysqlshdk::mysql::to_string(mode).c_str(), k_lock_ns, k_lock_name,
              m_primary_master->descr().c_str());
    mysqlshdk::mysql::get_lock(*m_primary_master, k_lock_ns, k_lock_name, mode,
                               static_cast<unsigned int>(timeout.count()));
  } catch (const shcore::Error &err) {
    // Abort the operation in case the required lock cannot be acquired.
    log_info("Failed to get %s lock ('%s', '%s') on '%s': %s",
             mysqlshdk::mysql::to_string(mode).c_str(), k_lock_ns, k_lock_name,
             m_primary_master->descr().c_str(), err.what());

    if (err.code() == ER_LOCKING_SERVICE_TIMEOUT) {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "ClusterSet lock through global primary member '%s'. Another "
          "operation requiring access to the member is still in progress, "
          "please wait for it to finish and try again.",
          m_primary_master->descr().c_str()));
      throw shcore::Exception(
          shcore::str_format("Failed to acquire ClusterSet lock through global "
                             "primary member '%s'",
                             m_primary_master->descr().c_str()),
          SHERR_DBA_LOCK_GET_FAILED);
    } else {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "ClusterSet lock through global primary member '%s': %s",
          m_primary_master->descr().c_str(), err.what()));
      throw;
    }
  }

  auto release_cb = [instance = m_primary_master]() {
    // Release all instance locks in the k_lock_ns namespace.
    //
    // NOTE: Only perform the operation if the lock service is
    // available, otherwise do nothing (ignore if concurrent execution is not
    // supported, e.g., lock service plugin not available).
    try {
      log_debug("Releasing locks for '%s' on %s.", k_lock_ns,
                instance->descr().c_str());
      mysqlshdk::mysql::release_lock(*instance, k_lock_ns);

    } catch (const shcore::Error &error) {
      // Ignore any error trying to release locks (e.g., might have not
      // been previously acquired due to lack of permissions).
      log_error("Unable to release '%s' locks for '%s': %s", k_lock_ns,
                instance->descr().c_str(), error.what());
    }
  };

  return mysqlshdk::mysql::Lock_scoped{std::move(release_cb)};
}

}  // namespace mysqlsh::dba
