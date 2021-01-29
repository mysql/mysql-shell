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

#include <algorithm>
#include <list>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster/create_cluster_set.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/cluster_set/create_replica_cluster.h"
#include "modules/adminapi/cluster_set/status.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/async_utils.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/dba/create_cluster.h"
#include "modules/adminapi/replica_set/replica_set_status.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "scripting/types.h"

namespace mysqlsh {
namespace dba {

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

void Cluster_set_impl::_set_instance_option(
    const std::string & /*instance_def*/, const std::string & /*option*/,
    const shcore::Value & /*value*/) {
  throw std::logic_error("Not implemented yet.");
}

void Cluster_set_impl::_set_option(const std::string & /*option*/,
                                   const shcore::Value & /*value*/) {
  throw std::logic_error("Not implemented yet.");
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

mysqlshdk::mysql::Auth_options
Cluster_set_impl::create_cluster_replication_user(Instance *cluster_primary,
                                                  bool dry_run) {
  // we use server_id as an arbitrary unique number within the clusterset. Once
  // the user is created, no relationship between the server_id and the number
  // should be assumed. The only way to obtain the replication user name for a
  // cluster is through the metadata.
  std::string repl_user = shcore::str_format(
      "%s%x", k_cluster_set_async_user_name, cluster_primary->get_server_id());

  log_info("Creating async replication account '%s'@'%%' for new cluster at %s",
           repl_user.c_str(), cluster_primary->descr().c_str());

  // auto primary = get_primary_master();
  auto primary = get_metadata_storage()->get_md_server();

  mysqlshdk::mysql::Auth_options repl_account;

  if (!dry_run) {
    // If the user already exists, it may have a different password causing an
    // authentication error. Also, it may have the wrong or incomplete set of
    // required grants. For those reasons, we must simply drop the account if
    // already exists to ensure a new one is created.
    mysqlshdk::mysql::drop_all_accounts_for_user(*primary, repl_user);
    repl_account = mysqlshdk::gr::create_recovery_user(repl_user, *primary,
                                                       {"%"}, {}, true, true);
  } else {
    repl_account.user = repl_user;
  }
  return repl_account;
}

void Cluster_set_impl::record_cluster_replication_user(
    Cluster_impl *cluster, const mysqlshdk::mysql::Auth_options &repl_user) {
  get_metadata_storage()->update_cluster_repl_account(cluster->get_id(),
                                                      repl_user.user, "%");
}

void Cluster_set_impl::drop_cluster_replication_user(Cluster_impl *cluster) {
  auto console = current_console();

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
  if (get_metadata_storage()->is_recovery_account_unique(repl_user, true)) {
    log_info("Dropping replication account '%s'@'%%' for cluster '%s'",
             repl_user.c_str(), cluster->get_name().c_str());

    try {
      primary->drop_user(repl_user, "%", true);
    } catch (const std::exception &e) {
      console->print_warning(shcore::str_format(
          "Error dropping replication account '%s'@'%%' for cluster '%s'",
          repl_user.c_str(), cluster->get_name().c_str()));
    }

    // Also update metadata
    try {
      get_metadata_storage()->update_cluster_repl_account(cluster->get_id(), "",
                                                          "");
    } catch (const std::exception &e) {
      log_warning("Could not update replication account metadata for '%s': %s",
                  cluster->get_name().c_str(), e.what());
    }
  }
}

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_primary_cluster() const {
  return m_primary_cluster;
}

Cluster_set_member_metadata Cluster_set_impl::get_primary_cluster_metadata()
    const {
  std::vector<Cluster_set_member_metadata> cs_members;

  if (!m_metadata_storage->get_cluster_set(m_id, nullptr, &cs_members)) {
    throw std::runtime_error("Metadata not found");
  }

  for (const auto &m : cs_members) {
    if (m.primary_cluster) {
      return m;
    }
  }

  throw std::runtime_error("No primary cluster found");
}

/**
 * Connect to PRIMARY of PRIMARY Cluster
 */
mysqlsh::dba::Instance *Cluster_set_impl::connect_primary() {
  Cluster_set_member_metadata primary_cluster(get_primary_cluster_metadata());

  Scoped_instance_pool ipool(
      m_metadata_storage, current_shell_options()->get().wizards,
      Instance_pool::Auth_options(
          get_cluster_server()->get_connection_options()));

  m_primary_cluster = get_cluster_object(primary_cluster, true);

  m_primary_master = m_primary_cluster->get_primary_master();
  return m_primary_master.get();
}

namespace {
void check_cluster_online(Cluster_impl *cluster) {
  std::string role =
      cluster->is_primary_cluster() ? "Primary Cluster" : "Cluster";

  switch (cluster->cluster_availability()) {
    case Cluster_availability::ONLINE:
      break;
    case Cluster_availability::ONLINE_NO_PRIMARY:
      throw shcore::Exception("Could not connect to PRIMARY of " + role + " '" +
                                  cluster->get_name() + "'",
                              SHERR_DBA_CLUSTER_PRIMARY_UNAVAILABLE);

    case Cluster_availability::OFFLINE:
      throw shcore::Exception("All members of " + role + " '" +
                                  cluster->get_name() + "' are OFFLINE",
                              SHERR_DBA_GROUP_OFFLINE);

    case Cluster_availability::NO_QUORUM:
      throw shcore::Exception("Could not connect to an ONLINE member of " +
                                  role + " '" + cluster->get_name() +
                                  "' within quorum",
                              SHERR_DBA_GROUP_HAS_NO_QUORUM);

    case Cluster_availability::UNREACHABLE:
      throw shcore::Exception("Could not connect to any ONLINE member of " +
                                  role + " '" + cluster->get_name() + "'",
                              SHERR_DBA_GROUP_UNREACHABLE);
  }
}
}  // namespace

/**
 * Connect to PRIMARY of PRIMARY Cluster, throw exception if not possible
 */
mysqlsh::dba::Instance *Cluster_set_impl::acquire_primary(
    mysqlshdk::mysql::Lock_mode /*mode*/,
    const std::string & /*skip_lock_uuid*/) {
  auto primary = connect_primary();
  {
    auto primary_cluster = get_primary_cluster();
    assert(primary_cluster);

    check_cluster_online(primary_cluster.get());
  }
  return primary;
}

void Cluster_set_impl::release_primary(mysqlsh::dba::Instance *) {
  m_primary_master.reset();
}

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_cluster(
    const std::string &name, bool allow_unavailable) {
  if (m_primary_cluster->get_name() == name) return m_primary_cluster;

  Cluster_set_member_metadata cluster_md;

  if (!m_metadata_storage->get_cluster_set_member_for_cluster_name(
          name, &cluster_md)) {
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

  if (cluster && !allow_unavailable) check_cluster_online(cluster.get());

  return cluster;
}

std::shared_ptr<Cluster_impl> Cluster_set_impl::get_cluster_object(
    const Cluster_set_member_metadata &cluster_md, bool allow_unavailable) {
  if (m_primary_cluster &&
      cluster_md.cluster.group_name == m_primary_cluster->get_group_name()) {
    return m_primary_cluster;
  }

  auto ipool = current_ipool();
  try {
    if (allow_unavailable) {
      Cluster_availability availability;

      auto group_server = ipool->try_connect_cluster_primary_with_fallback(
          cluster_md.cluster.cluster_id, &availability);

      if (group_server) {
        group_server->steal();

        if (cluster_md.primary_cluster && !cluster_md.invalidated)
          m_metadata_storage = std::make_shared<MetadataStorage>(group_server);
      }

      return std::make_shared<Cluster_impl>(shared_from_this(),
                                            cluster_md.cluster, group_server,
                                            m_metadata_storage, availability);
    } else {
      auto group_server =
          ipool->connect_group_member(cluster_md.cluster.group_name);

      group_server->steal();
      if (cluster_md.primary_cluster && !cluster_md.invalidated)
        m_metadata_storage = std::make_shared<MetadataStorage>(group_server);

      return std::make_shared<Cluster_impl>(
          shared_from_this(), cluster_md.cluster, group_server,
          m_metadata_storage, Cluster_availability::ONLINE);
    }
  } catch (const shcore::Exception &e) {
    if (allow_unavailable) {
      current_console()->print_warning(
          "Could not connect to any member of cluster '" +
          cluster_md.cluster.cluster_name + "': " + e.format());

      return std::make_shared<Cluster_impl>(
          shared_from_this(), cluster_md.cluster, nullptr,
          get_metadata_storage(), Cluster_availability::UNREACHABLE);
    }
    throw;
  }
}

Cluster_channel_status Cluster_set_impl::get_replication_channel_status(
    const Cluster_impl &cluster) const {
  // Get the ClusterSet member metadata
  auto cluster_primary = cluster.get_cluster_server();

  if (cluster_primary) {
    mysqlshdk::mysql::Replication_channel channel;

    if (mysqlshdk::mysql::get_channel_status(
            *cluster_primary, k_clusterset_async_channel_name, &channel)) {
      if (channel.status() != mysqlshdk::mysql::Replication_channel::ON) {
        log_info("Channel '%s' at %s not ON: %s",
                 k_clusterset_async_channel_name,
                 cluster_primary->descr().c_str(),
                 mysqlshdk::mysql::format_status(channel, true).c_str());
      } else {
        if (cluster.is_primary_cluster()) {
          log_info("Unexpected channel '%s' at %s: %s",
                   k_clusterset_async_channel_name,
                   cluster_primary->descr().c_str(),
                   mysqlshdk::mysql::format_status(channel, true).c_str());
        }
      }

      switch (channel.status()) {
        case mysqlshdk::mysql::Replication_channel::CONNECTING:
        case mysqlshdk::mysql::Replication_channel::ON: {
          auto primary = get_primary_master();
          auto primary_cluster = get_primary_cluster();

          if (primary && primary_cluster &&
              primary_cluster->cluster_availability() ==
                  Cluster_availability::ONLINE) {
            if (channel.source_uuid != primary->get_uuid())
              return Cluster_channel_status::MISCONFIGURED;
          }
          return Cluster_channel_status::OK;
          break;
        }

        case mysqlshdk::mysql::Replication_channel::OFF:
        case mysqlshdk::mysql::Replication_channel::RECEIVER_OFF:
        case mysqlshdk::mysql::Replication_channel::APPLIER_OFF:
          return Cluster_channel_status::STOPPED;
        case mysqlshdk::mysql::Replication_channel::CONNECTION_ERROR:
        case mysqlshdk::mysql::Replication_channel::APPLIER_ERROR:
          return Cluster_channel_status::ERROR;
      }
    } else {
      return Cluster_channel_status::MISSING;
    }
  }
  return Cluster_channel_status::UNKNOWN;
}

namespace {
void handle_user_warnings(const Cluster_global_status &global_status,
                          const std::string &replica_cluster_name) {
  auto console = mysqlsh::current_console();

  if (global_status == Cluster_global_status::OK_MISCONFIGURED ||
      global_status == Cluster_global_status::OK_NOT_REPLICATING) {
    console->print_warning(
        "The Cluster's Replication Channel is misconfigured or stopped, "
        "topology changes will not be allowed on the InnoDB Cluster "
        "'" +
        replica_cluster_name + "'");
    console->print_note(
        "To restore the Cluster and ClusterSet operations, the Replication "
        "Channel must be fixed using <<<rejoinCluster>>>()");
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
  Cluster_global_status ret = Cluster_global_status::UNKNOWN;
  assert(cluster->is_cluster_set_member());

  if (cluster->is_invalidated()) {
    ret = Cluster_global_status::INVALIDATED;
  } else if (cluster->cluster_availability() ==
             Cluster_availability::UNREACHABLE) {
    ret = Cluster_global_status::UNKNOWN;
  } else if (cluster->cluster_availability() ==
                 Cluster_availability::NO_QUORUM ||
             cluster->cluster_availability() == Cluster_availability::OFFLINE) {
    ret = Cluster_global_status::NOT_OK;
  } else {
    try {
      // Acquire the primary first
      cluster->acquire_primary();
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
        ret = Cluster_global_status::NOT_OK;
      } else {
        throw;
      }
    }

    if (cluster->get_cluster_server()) {
      Cluster_status cl_status = cluster->cluster_status();

      if (cluster->is_primary_cluster()) {
        if (cl_status == Cluster_status::NO_QUORUM ||
            cl_status == Cluster_status::OFFLINE ||
            cl_status == Cluster_status::UNKNOWN) {
          ret = Cluster_global_status::NOT_OK;
        } else {
          Cluster_channel_status ch_status =
              get_replication_channel_status(*cluster);

          switch (ch_status) {
            case Cluster_channel_status::OK:
            case Cluster_channel_status::MISCONFIGURED:
            case Cluster_channel_status::ERROR:
              // unexpected at primary cluster
              ret = Cluster_global_status::OK_MISCONFIGURED;
              break;

            case Cluster_channel_status::UNKNOWN:
            case Cluster_channel_status::STOPPED:
            case Cluster_channel_status::MISSING:
              ret = Cluster_global_status::OK;
              break;
          }
        }
      } else {  // it's a replica cluster
        // check if the primary cluster is unavailable
        auto primary_status =
            get_cluster_global_status(get_primary_cluster().get());

        if (primary_status == Cluster_global_status::NOT_OK ||
            primary_status == Cluster_global_status::UNKNOWN ||
            primary_status == Cluster_global_status::INVALIDATED)
          return Cluster_global_status::NOT_OK;

        if (cl_status == Cluster_status::NO_QUORUM ||
            cl_status == Cluster_status::OFFLINE ||
            cl_status == Cluster_status::UNKNOWN) {
          ret = Cluster_global_status::NOT_OK;
        } else {
          Cluster_channel_status ch_status =
              get_replication_channel_status(*cluster);

          switch (ch_status) {
            case Cluster_channel_status::OK:
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
        }

        handle_user_warnings(ret, cluster->get_name());
      }
    }
  }

  // if the cluster is OK, check consistency
  if (ret == Cluster_global_status::OK && !cluster->is_primary_cluster()) {
    if (!check_gtid_consistency(cluster)) {
      ret = Cluster_global_status::OK_NOT_CONSISTENT;
    }
  }

  return ret;
}

bool Cluster_set_impl::check_gtid_consistency(Cluster_impl *cluster) const {
  assert(get_primary_master());

  auto my_gtid_set = mysqlshdk::mysql::Gtid_set::from_gtid_executed(
      *cluster->get_cluster_server());
  auto my_view_change_uuid = *cluster->get_cluster_server()->get_sysvar_string(
      "group_replication_view_change_uuid");

  my_gtid_set.subtract(my_gtid_set.get_gtids_from(my_view_change_uuid),
                       *cluster->get_cluster_server());

  auto source_gtid_set =
      mysqlshdk::mysql::Gtid_set::from_gtid_executed(*get_primary_master());

  auto errants = my_gtid_set;
  errants.subtract(source_gtid_set, *cluster->get_cluster_server());

  if (!errants.empty()) {
    log_warning(
        "Cluster %s has errant transactions: source=%s source_gtids=%s "
        "replica=%s replica_gtids=%s errants=%s",
        cluster->get_name().c_str(), get_primary_master()->descr().c_str(),
        source_gtid_set.str().c_str(),
        cluster->get_cluster_server()->descr().c_str(),
        my_gtid_set.str().c_str(), errants.str().c_str());
  }

  return errants.empty();
}

Async_replication_options Cluster_set_impl::get_clusterset_replication_options()
    const {
  Async_replication_options repl_options;

  // Get the ClusterSet configured SSL mode
  shcore::Value ssl_mode;
  get_metadata_storage()->query_clusterset_attribute(
      get_id(), k_clusterset_attribute_ssl_mode, &ssl_mode);

  std::string ssl_mode_str = ssl_mode.as_string();
  repl_options.ssl_mode = to_cluster_ssl_mode(ssl_mode_str);

  // Set CONNECTION_RETRY_INTERVAL and CONNECTION_RETRY_COUNT
  repl_options.master_connect_retry = k_cluster_set_master_connect_retry;
  repl_options.master_retry_count = k_cluster_set_master_retry_count;

  return repl_options;
}

Member_recovery_method Cluster_set_impl::validate_instance_recovery(
    Member_op_action op_action, mysqlshdk::mysql::IInstance *target_instance,
    Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
    bool interactive) {
  auto donor_instance = get_primary_master();
  auto check_recoverable =
      [donor_instance](mysqlshdk::mysql::IInstance *tgt_instance) {
        // Get the gtid state in regards to the donor
        mysqlshdk::mysql::Replica_gtid_state state =
            mysqlshdk::mysql::check_replica_gtid_state(
                *donor_instance, *tgt_instance, nullptr, nullptr);

        if (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
          return true;
        } else {
          return false;
        }
      };

  Member_recovery_method recovery_method =
      mysqlsh::dba::validate_instance_recovery(
          Cluster_type::REPLICATED_CLUSTER, op_action, donor_instance.get(),
          target_instance, check_recoverable, opt_recovery_method,
          gtid_set_is_complete, interactive);

  return recovery_method;
}

std::vector<Cluster_metadata> Cluster_set_impl::get_clusters() const {
  std::vector<Cluster_metadata> all_clusters =
      get_metadata_storage()->get_all_clusters();

  std::vector<Cluster_metadata> res;

  for (const auto &i : all_clusters) {
    if (i.cluster_set_id == get_id()) {
      res.push_back(i);
    }
  }

  return res;
}

shcore::Value Cluster_set_impl::create_replica_cluster(
    const std::string &instance_def, const std::string &cluster_name,
    Recovery_progress_style progress_style,
    const clusterset::Create_replica_cluster_options &options) {
  check_preconditions("createReplicaCluster");

  // Acquire the primary from the clusterSet
  auto primary = acquire_primary();

  // Release the primary when the command terminates
  auto finally_primary =
      shcore::on_leave_scope([this]() { release_primary(); });

  Scoped_instance target(connect_target_instance(instance_def, true, true));

  // Create the replica cluster
  {
    // Create the Create_replica_cluster command and execute it.
    clusterset::Create_replica_cluster op_create_replica_cluster(
        this, primary, target, cluster_name, progress_style, options);

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

  // Acquire the primary from the clusterSet
  acquire_primary();

  // Release the primary when the command terminates
  auto finally_primary =
      shcore::on_leave_scope([this]() { release_primary(); });

  std::shared_ptr<Cluster_impl> target_cluster;
  bool skip_channel_check = false;
  auto console = mysqlsh::current_console();

  // Validations and initializations of variables
  {
    // Validate the Cluster name
    mysqlsh::dba::validate_cluster_name(cluster_name,
                                        Cluster_type::GROUP_REPLICATION);

    // Get the Cluster object
    // NOTE: This will throw an exception if the Cluster does not exist in the
    // metadata
    try {
      target_cluster = get_cluster(cluster_name);
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_METADATA_MISSING) {
        console->print_error(shcore::str_format(
            "The Cluster '%s' does not exist or does not belong to the "
            "ClusterSet.",
            cluster_name.c_str()));
        throw;
      } else {
        if (options.force) {
          target_cluster = get_cluster(cluster_name, true);

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

  // Execute the operation
  {
    shcore::Scoped_callback_list undo_list;
    bool skip_replication_user_undo_step = false;

    try {
      console->print_info("The Cluster '" + cluster_name +
                          "' will be removed from the InnoDB ClusterSet.");
      console->print_info();
      console->print_note(
          "The Cluster's Group Replication configurations will be kept and the "
          "Cluster will become an independent entity.");
      console->print_info();

      // Check if the channel exists and its OK (not ERROR) first
      mysqlshdk::mysql::Replication_channel channel;
      if (!skip_channel_check) {
        if (!get_channel_status(*target_cluster->get_cluster_server(),
                                {k_clusterset_async_channel_name}, &channel)) {
          if (!options.force) {
            console->print_error(
                "The ClusterSet Replication channel could not be found at the "
                "Cluster '" +
                cluster_name +
                "'. Use the 'force' option to ignore this check.");

            throw shcore::Exception("Replication channel does not exist",
                                    SHERR_DBA_REPLICATION_OFF);
          } else {
            console->print_warning(
                "Ignoring non-existing ClusterSet Replication channel because "
                "of 'force' option");
          }
        } else {
          if (channel.status() !=
              mysqlshdk::mysql::Replication_channel::Status::ON) {
            if (!options.force) {
              console->print_error(
                  "The ClusterSet Replication channel has an invalid state '" +
                  to_string(channel.status()) +
                  "'. Use the 'force' option to ignore this check.");

              throw shcore::Exception(
                  "ClusterSet Replication Channel not in expected state",
                  SHERR_DBA_REPLICATION_INVALID);
            } else {
              console->print_warning(
                  "ClusterSet Replication channel has invalid state '" +
                  to_string(channel.status()) +
                  "'. Ignoring because of 'force' option");
            }
          }
        }
      }

      // Sync transactions before making any changes
      console->print_info(
          "* Waiting for the Cluster to synchronize with the PRIMARY "
          "Cluster...");

      if (!target_cluster->get_primary_master()) {
        console->print_note(
            "Transaction sync was skipped because cluster is unavailable");
      } else {
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
          } else if (options.force) {
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

      // Update Metadata
      console->print_info("* Updating topology");

      log_debug("Removing Cluster from the Metadata.");

      auto metadata = get_metadata_storage();

      if (!options.dry_run) {
        MetadataStorage::Transaction trx(metadata);

        metadata->record_cluster_set_member_removed(get_id(),
                                                    target_cluster->get_id());

        // Push the whole transaction and changes to the Undo list
        undo_list.push_front([=]() {
          log_info("Revert: Recording back ClusterSet member removed");
          Cluster_set_member_metadata cluster_md;
          cluster_md.cluster.cluster_id = target_cluster->get_id();
          cluster_md.cluster_set_id = get_id();
          cluster_md.master_cluster_id = get_primary_cluster()->get_id();
          cluster_md.primary_cluster = false;

          metadata->record_cluster_set_member_added(cluster_md);
        });

        // Only commit transactions once everything is done
        trx.commit();

        // Set debug trap to test reversion of Metadata topology updates to
        // remove the member
        DBUG_EXECUTE_IF("dba_remove_cluster_fail_post_cs_member_removed",
                        { throw std::logic_error("debug"); });

        // Drop replication user
        drop_cluster_replication_user(target_cluster.get());

        // Revert the replication account handling
        undo_list.push_front([=, &skip_replication_user_undo_step]() {
          auto primary = get_metadata_storage()->get_md_server();

          // NOTE: skip if the primary cluster is offline or the flag to skip
          // it is enabled
          if (primary || !skip_replication_user_undo_step) {
            log_info("Revert: Creating replication account at '%s'",
                     primary->descr().c_str());
            auto repl_credentials =
                create_cluster_replication_user(primary.get(), options.dry_run);

            // Update the credentials on the primary and restart the channel
            // otherwise the channel will be broken
            // NOTE: Only if the channel exists, otherwise skip it
            if (get_channel_status(*target_cluster->get_cluster_server(),
                                   {k_clusterset_async_channel_name},
                                   nullptr)) {
              stop_channel(target_cluster->get_cluster_server().get(),
                           k_clusterset_async_channel_name, false, false);

              change_replication_credentials(
                  *target_cluster->get_cluster_server(), repl_credentials.user,
                  repl_credentials.password.get_safe(),
                  k_clusterset_async_channel_name);

              start_channel(target_cluster->get_cluster_server().get(),
                            k_clusterset_async_channel_name, false);
            }

            record_cluster_replication_user(target_cluster.get(),
                                            repl_credentials);
          }
        });

        // Set debug trap to test reversion of replication user creation
        DBUG_EXECUTE_IF(
            "dba_remover_cluster_fail_post_replication_user_removal",
            { throw std::logic_error("debug"); });

        // Sync again to catch-up the drop user and metadata update
        if (target_cluster->cluster_availability() ==
                Cluster_availability::ONLINE &&
            options.timeout >= 0) {
          try {
            sync_transactions(*target_cluster->get_cluster_server(),
                              {k_clusterset_async_channel_name},
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

      // Stop replication
      // NOTE: This is done last so all other changes are propagated first to
      // the Cluster being removed
      console->print_info(
          "* Stopping and deleting ClusterSet managed replication channel...");

      // Store the replication options before stopping and deleting the
      // channel to use in the revert process in case of a failure
      auto ar_options = get_clusterset_replication_options();

      if (target_cluster->get_primary_master() &&
          target_cluster->cluster_availability() ==
              Cluster_availability::ONLINE) {
        // Call the primitive to remove the replica, ensuring:
        //  - super_read_only management is enabled
        //  - the ClusterSet replication channel is stopped and reset
        //  - The managed connection failover configurations are deleted
        remove_replica(target_cluster->get_cluster_server().get(),
                       options.dry_run);

        // Revert in case of failure
        // NOTE: Mark the user handling undo step to be skipped.
        // This has to be done because that handling apart from
        // creating a new user, it updates the channel to use that new user.
        // That must be done otherwise the channel would get broken, however,
        // we're updating the channel here too so to avoid repeating the whole
        // logic and having to share resourced (user credentials) we simply
        // remove that handling from the undo_list and do it in here
        undo_list.push_front([=, &skip_replication_user_undo_step]() {
          skip_replication_user_undo_step = true;
          log_info("Revert: Re-adding Cluster as Replica");
          auto repl_credentials = create_cluster_replication_user(
              target_cluster->get_primary_master().get(), options.dry_run);

          record_cluster_replication_user(target_cluster.get(),
                                          repl_credentials);

          auto ar_channel_options = ar_options;

          ar_channel_options.repl_credentials = repl_credentials;

          update_replica(target_cluster->get_cluster_server().get(),
                         get_primary_master().get(), ar_channel_options, true,
                         options.dry_run);
        });
      } else {
        // If the target cluster is OFFLINE, check if there are reachable
        // members in order to reset the settings in those instances
        mysqlshdk::db::Connection_options opts_cluster =
            target_cluster->get_metadata_storage()
                ->get_md_server()
                ->get_connection_options();

        for (const auto &instance_def :
             target_cluster->get_metadata_storage()->get_all_instances(
                 target_cluster->get_id())) {
          mysqlshdk::db::Connection_options opts(instance_def.endpoint);

          try {
            opts.set_login_options_from(opts_cluster);
            auto reachable_member = Instance::connect(opts);

            // Check if super_read_only is enabled. If so it must be disabled to
            // reset the ClusterSet settings
            if (reachable_member->get_sysvar_bool("super_read_only")
                    .get_safe()) {
              reachable_member->set_sysvar("super_read_only", false);
            }

            // Reset the ClusterSet settings and replication channel
            remove_replica(reachable_member.get(), options.dry_run);

            // Disable skip_replica_start
            reachable_member->set_sysvar(
                "skip_replica_start", false,
                mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

            // Revert in case of failure
            // NOTE: Mark the user handling undo step to be skipped. On this
            // case the replication user wasn't dropped because the primary
            // cluster is offline we can simply call update_replica without any
            // credentials because it will keep them
            undo_list.push_front([=, &skip_replication_user_undo_step]() {
              skip_replication_user_undo_step = true;
              log_info("Revert: Re-adding Cluster as Replica");
              update_replica(reachable_member.get(), get_primary_master().get(),
                             ar_options, false, options.dry_run);

              log_info("Revert: Enabling skip_replica_start");
              reachable_member->set_sysvar(
                  "skip_replica_start", true,
                  mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
            });
          } catch (const shcore::Error &e) {
            // Unreachable member, ignore
            mysqlsh::current_console()->print_note("Can't connect to '" +
                                                   opts.as_uri() + "'");
          }
        }
      }

      // Set debug trap to test reversion of replica removal
      DBUG_EXECUTE_IF("dba_remover_cluster_fail_post_replica_removal",
                      { throw std::logic_error("debug"); });

      // Disable skip_replica_start
      if (!options.dry_run && target_cluster->get_cluster_server() &&
          target_cluster->cluster_availability() ==
              Cluster_availability::ONLINE) {
        log_info("Persisting skip_replica_start=0 across the cluster...");

        auto config =
            target_cluster->create_config_object({}, true, true, true);

        config->set("skip_replica_start",
                    mysqlshdk::utils::nullable<bool>(false));
        config->apply();

        undo_list.push_front([=]() {
          log_info("Revert: Enabling skip_replica_start");
          auto config_undo =
              target_cluster->create_config_object({}, true, true, true);

          config_undo->set("skip_replica_start",
                           mysqlshdk::utils::nullable<bool>(true));
          config_undo->apply();
        });

        // Set debug trap to test reversion of the ClusterSet setting set-up
        DBUG_EXECUTE_IF("dba_remove_cluster_fail_disable_skip_replica_start",
                        { throw std::logic_error("debug"); });
      }

      // Remove the Cluster's Metadata and Cluster members' info from the
      // Metadata of the ClusterSet
      if (!options.dry_run) {
        metadata->drop_cluster(target_cluster->get_name());
      }

      undo_list.cancel();

      log_debug("removeCluster() metadata updates done");

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

      undo_list.call();

      console->print_info();
      console->print_info("Changes successfully reverted.");

      throw;
    }
  }
}

void Cluster_set_impl::update_replica_settings(Instance *instance,
                                               Instance *new_primary,
                                               bool dry_run) {
  try {
    bool enabled;

    log_info(
        "Checking Group Action mysql_disable_super_read_obly_if_primary %s",
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

  log_info("Configuring the managed connection failover configurations");
  if (!dry_run) {
    try {
      // Delete any previous managed connection failover configurations
      delete_managed_connection_failover(
          *instance, k_clusterset_async_channel_name, dry_run);

      // Setup the new managed connection failover
      add_managed_connection_failover(*instance, *new_primary,
                                      k_clusterset_async_channel_name, "",
                                      dry_run);
    } catch (...) {
      log_error("Error setting up connection failover at %s: %s",
                instance->descr().c_str(), format_active_exception().c_str());
      throw;
    }
  }
}  // namespace dba

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
    log_error("Error enabling automatic super_read_only management at %s: %s",
              instance->descr().c_str(), format_active_exception().c_str());
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
    log_error("Error disabling automatic failover at %s: %s",
              instance->descr().c_str(), format_active_exception().c_str());
    throw;
  }
}

void Cluster_set_impl::ensure_replica_settings(Cluster_impl *replica,
                                               bool dry_run) {
  // Ensure SRO is enabled on all members of the Cluster
  replica->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE},
      replica->get_cluster_server()->get_connection_options(), {},
      [=](const std::shared_ptr<Instance> &instance,
          const mysqlshdk::gr::Member &) {
        if (!instance->get_sysvar_bool("super_read_only").get_safe()) {
          log_info("Enabling super_read_only at '%s'",
                   instance->descr().c_str());
          if (!dry_run) {
            instance->set_sysvar("super_read_only", true);
          }
        }

        return true;
      });
}

void Cluster_set_impl::promote_to_primary(Instance *instance, bool dry_run) {
  // Reset ClusterSet related configurations first:
  //  - Enable super_read_only management
  //  - Stop and reset the ClusterSet replication channel
  //  - Delete the managed connection failover configuration
  remove_replica_settings(instance, dry_run);

  // Ensure GR action 'mysql_start_failover_channels_if_primary' is enabled
  // to ensure the automatic connection failover for receiver failure.
  // NOTE: This is not needed on PRIMARY Clusters and won't have any effect,
  // but we should set it during the creation of the PRIMARY cluster to
  // avoid having to deal with it during switchover/failovers.
  try {
    bool enabled;

    if (!mysqlshdk::gr::get_member_action_status(
            *instance, mysqlshdk::gr::k_gr_start_failover_channels_if_primary,
            &enabled) ||
        !enabled) {
      log_info(
          "Enabling Group Action mysql_start_failover_channels_if_primary");
      if (!dry_run) {
        mysqlshdk::gr::enable_member_action(
            *instance, mysqlshdk::gr::k_gr_start_failover_channels_if_primary,
            mysqlshdk::gr::k_gr_member_action_after_primary_election);
      }
    } else {
      log_info("Already enabled");
    }
  } catch (...) {
    log_error(
        "Error enabling automatic connection failover for receiver failure "
        "handling at %s: %s",
        instance->descr().c_str(), format_active_exception().c_str());
    throw;
  }
}

void Cluster_set_impl::demote_from_primary(Instance *instance,
                                           Instance *new_primary,
                                           bool dry_run) {
  update_replica_settings(instance, new_primary, dry_run);
}

void Cluster_set_impl::swap_primary(Instance *demoted, Instance *promoted,
                                    const Async_replication_options &ar_options,
                                    bool dry_run) {
  auto console = current_console();

  try {
    // First, promote the new primary by stopping slave, unfencing it and
    // making the old primary a slave of the new one.
    // Topology changes are reverted on exception.
    async_swap_primary(demoted, promoted, k_clusterset_async_channel_name,
                       ar_options, nullptr, dry_run);
  } catch (...) {
    console->print_error("Error changing replication source: " +
                         format_active_exception());

    console->print_note("Reverting replication changes");

    throw shcore::Exception("Error during switchover",
                            SHERR_DBA_SWITCHOVER_ERROR);
  }

  // Clear replication configs from the promoted instance. Do it after
  // everything is done, to make reverting easier.
  try {
    reset_channel(promoted, k_clusterset_async_channel_name, true, dry_run);
  } catch (...) {
    // Failure to reset slave is not fatal
    console->print_warning("Error resetting replication configurations at " +
                           promoted->descr() + ": " +
                           format_active_exception());
  }
}

void Cluster_set_impl::update_replica(
    Cluster_impl *replica, Instance *master,
    const Async_replication_options &ar_options, bool dry_run) {
  auto primary_uuid = replica->get_primary_master()->get_uuid();

  replica->execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE}, master->get_connection_options(),
      {},
      [=](const std::shared_ptr<Instance> &instance,
          const mysqlshdk::gr::Member &gr_member) {
        update_replica(instance.get(), master, ar_options,
                       (gr_member.role == mysqlshdk::gr::Member_role::PRIMARY),
                       dry_run);
        return true;
      });
}

void Cluster_set_impl::update_replica(
    Instance *replica, Instance *master,
    const Async_replication_options &ar_options, bool primary_instance,
    bool dry_run) {
  auto console = current_console();
  auto repl_options = ar_options;

  try {
    // Enable SOURCE_CONNECTION_AUTO_FAILOVER if the instance is the primary
    if (primary_instance) {
      repl_options.auto_failover = true;
    }

    // This will re-point the slave to the new master without changing any
    // other replication parameter if ar_options has no credentials.
    async_change_primary(replica, master, k_clusterset_async_channel_name,
                         repl_options, primary_instance ? true : false,
                         dry_run);

    log_info("PRIMARY changed for instance %s", replica->descr().c_str());
  } catch (...) {
    console->print_error("Error changing replication source: " +
                         format_active_exception());

    throw shcore::Exception("Error during switchover",
                            SHERR_DBA_SWITCHOVER_ERROR);
  }

  if (primary_instance) update_replica_settings(replica, master, dry_run);
}

void Cluster_set_impl::remove_replica(Instance *instance, bool dry_run) {
  // Stop and reset the replication channel configuration
  remove_channel(instance, k_clusterset_async_channel_name, dry_run);

  remove_replica_settings(instance, dry_run);

  if (!dry_run) fence_instance(instance);
}

void Cluster_set_impl::record_in_metadata(
    const Cluster_id &seed_cluster_id,
    const clusterset::Create_cluster_set_options &m_options) {
  auto metadata = get_metadata_storage();

  // delete unrelated records if there are any
  metadata->cleanup_for_cluster(seed_cluster_id);

  Cluster_set_id csid = metadata->create_cluster_set_record(
      this, seed_cluster_id, shcore::make_dict());

  // Record the ClusterSet SSL mode in the Metadata
  metadata->update_clusterset_attribute(
      csid, k_clusterset_attribute_ssl_mode,
      shcore::Value(to_string(m_options.ssl_mode)));

  // Migrate Router registered in the Cluster to the ClusterSet
  log_info(
      "Migrating Metadata records of the Cluster Router instances to "
      "the ClusterSet");
  get_metadata_storage()->migrate_routers_to_clusterset(seed_cluster_id,
                                                        get_id());
}

shcore::Value Cluster_set_impl::status(int extended) {
  check_preconditions("status");

  return shcore::Value(clusterset::cluster_set_status(this, extended));
}

shcore::Value Cluster_set_impl::describe() {
  check_preconditions("describe");

  return shcore::Value(clusterset::cluster_set_describe(this));
}

}  // namespace dba
}  // namespace mysqlsh
