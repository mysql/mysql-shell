/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster_set/dissolve.h"

#include <unordered_map>

#include "modules/adminapi/cluster_set/status.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh::dba::clusterset {

namespace {

struct Cluster_info {
  bool is_primary{false};
  bool is_unreachable{false};
  std::shared_ptr<Cluster_impl> impl;
  std::string name;
  bool supports_member_actions{false};

  std::vector<Cluster_impl::Instance_md_and_gr_member> instances_md;
  std::vector<Instance_metadata> instances_rr_md;
  std::unordered_map<std::string, std::shared_ptr<Instance>> instances;

  bool has_unreachable_instances{false};
  bool has_not_online_instances{false};
};

void deal_with_unreachable_clusters(
    const Cluster_impl &primary,
    const std::list<Cluster_id> &replica_clusters_unreachable, bool force) {
  if (replica_clusters_unreachable.empty()) return;

  auto console = current_console();

  auto get_cluster_name = [&primary](const Cluster_id &cluster_id) {
    Cluster_metadata cluster_md;
    if (!primary.get_metadata_storage()->get_cluster(cluster_id, &cluster_md))
      return cluster_id;
    return std::move(cluster_md.cluster_name);
  };

  // just warn the user, don't throw anything
  if (force) {
    if (replica_clusters_unreachable.size() == 1) {
      console->print_note(shcore::str_format(
          "Unable to connect to Cluster '%s'. For all reachable instances of "
          "the Cluster, replication will be stopped and, if possible, "
          "configuration reset. However, internal replication accounts and "
          "metadata will remain unchanged, thus for those instances to be "
          "re-used the metadata schema must be dropped.",
          get_cluster_name(replica_clusters_unreachable.front()).c_str()));
    } else {
      auto list = shcore::str_join(replica_clusters_unreachable, ", ",
                                   [&get_cluster_name](const std::string &id) {
                                     return shcore::str_format(
                                         "'%s'", get_cluster_name(id).c_str());
                                   });

      console->print_note(shcore::str_format(
          "Unable to connect to Clusters %s. For all reachable instances of "
          "the Clusters, replication will be stopped and, if possible, "
          "configuration reset. However, internal replication accounts and "
          "metadata will remain unchanged, thus for those instances to be "
          "re-used the metadata schema must be dropped.",
          list.c_str()));
    }

    return;
  }

  // reaching this point, we can't proceed

  if (replica_clusters_unreachable.size() == 1) {
    console->print_error(shcore::str_format(
        "Unable to connect to Cluster '%s'. Please make sure the cluster is "
        "reachable. You can also choose to skip this error using the 'force: "
        "true' option, but it might leave the cluster in an inconsistent state "
        "and lead to errors if you want to reuse it.",
        get_cluster_name(replica_clusters_unreachable.front()).c_str()));
  } else {
    auto list = shcore::str_join(replica_clusters_unreachable, ", ",
                                 [&get_cluster_name](const std::string &id) {
                                   return shcore::str_format(
                                       "'%s'", get_cluster_name(id).c_str());
                                 });

    console->print_error(shcore::str_format(
        "Unable to connect to Clusters %s. Please make sure they are "
        "reachable. You can also choose to skip this error using the 'force: "
        "true' option, but it might leave the clusters in an inconsistent "
        "state and lead to errors if you want to reuse them.",
        list.c_str()));
  }

  throw shcore::Exception("One or more REPLICA Clusters are unreachable.",
                          SHERR_DBA_UNREACHABLE_REPLICA_CLUSTERS);
}

Cluster_info gather_cluster_info(const std::shared_ptr<Cluster_impl> &cluster,
                                 bool is_primary, bool force) {
  Cluster_info cluster_info;

  cluster_info.is_primary = is_primary;
  cluster_info.is_unreachable = false;
  cluster_info.name = cluster->get_name();
  cluster_info.impl = cluster;
  cluster_info.supports_member_actions =
      cluster->get_lowest_instance_version() >=
      Precondition_checker::k_min_cs_version;

  cluster_info.instances_md = cluster->get_instances_with_state(true);
  cluster_info.instances_rr_md = cluster->get_replica_instances();

  auto console = current_console();

  auto connect_to_instance =
      [&](const std::string &address) -> std::shared_ptr<Instance> {
    try {
      return cluster->get_session_to_cluster_instance(address);
    } catch (const std::exception &) {
      cluster_info.has_unreachable_instances = true;
      if (!force) {
        console->print_error(
            shcore::str_format("Unable to connect to instance '%s'. "
                               "Please verify connection credentials and "
                               "make sure the instance is available.",
                               address.c_str()));
      } else {
        console->print_note(shcore::str_format(
            "The instance '%s' is not reachable and it will only be "
            "removed from the metadata. Please take any necessary actions "
            "to make sure that the instance will not start/rejoin the "
            "cluster if brought back online.",
            address.c_str()));
        console->print_info();
      }

      return nullptr;
    }
  };

  auto instance_not_online = [&](const std::string &address,
                                 const std::string &state) {
    cluster_info.has_not_online_instances = true;
    if (!force) {
      console->print_error(shcore::str_format(
          "The instance '%s' cannot be removed because it is on a '%s' "
          "state. Please bring the instance back ONLINE and try to "
          "dissolve the ClusterSet again. If the instance is permanently "
          "not reachable, then please use <ClusterSet>.dissolve() with the "
          "'force' option enabled to proceed with the operation and only "
          "remove the instance from the ClusterSet Metadata.",
          address.c_str(), state.c_str()));
      return;
    }

    console->print_note(shcore::str_format(
        "The instance '%s' is '%s' and it will only be removed from the "
        "metadata. Please take any necessary actions to make sure that the "
        "instance will not start/rejoin the cluster if brought back "
        "online.",
        address.c_str(), state.c_str()));
    console->print_info();
  };

  for (const auto &i_md : cluster_info.instances_md) {
    if (i_md.second.state != mysqlshdk::gr::Member_state::ONLINE &&
        i_md.second.state != mysqlshdk::gr::Member_state::RECOVERING) {
      instance_not_online(i_md.first.address,
                          mysqlshdk::gr::to_string(i_md.second.state));
      continue;
    }

    auto instance = connect_to_instance(i_md.first.address);
    if (instance) cluster_info.instances[i_md.first.uuid] = std::move(instance);
  }

  for (const auto &i_md : cluster_info.instances_rr_md) {
    try {
      auto instance = connect_to_instance(i_md.endpoint);
      if (!instance) continue;

      auto status = mysqlshdk::mysql::get_read_replica_status(*instance);
      if (status != mysqlshdk::mysql::Read_replica_status::ONLINE) {
        instance_not_online(i_md.endpoint, to_string(status));
        continue;
      }

      cluster_info.instances[i_md.uuid] = std::move(instance);

    } catch (const mysqlshdk::db::Error &) {
      log_info("Unable to connect to '%s'", i_md.endpoint.c_str());
      instance_not_online(
          i_md.endpoint,
          to_string(mysqlshdk::mysql::Read_replica_status::UNREACHABLE));
    } catch (const shcore::Error &) {
      throw;
    }
  }

  return cluster_info;
}

Cluster_info gather_cluster_info(Cluster_set_impl &cs,
                                 const Cluster_id &cluster_id) {
  Cluster_info cluster_info;

  cluster_info.is_primary = false;
  cluster_info.is_unreachable = true;
  cluster_info.supports_member_actions = false;
  cluster_info.impl = nullptr;

  {
    auto cluster_md = cs.get_cluster_metadata(cluster_id);
    cluster_info.name = std::move(cluster_md.cluster.cluster_name);
  }

  for (const auto &i : cs.get_all_instances()) {
    if (i.cluster_id != cluster_id) continue;

    std::shared_ptr<Instance> instance;
    try {
      instance = cs.connect_target_instance(i.address, false);
    } catch (const std::exception &) {
      // since the cluster is unavailable, this is a last-ditch attempt to
      // connect to one of its instances, and so we can safly ignore any
      // errors
      continue;
    }

    if (i.instance_type == Instance_type::READ_REPLICA)
      cluster_info.instances_rr_md.push_back(i);
    else
      cluster_info.instances_md.emplace_back(i, mysqlshdk::gr::Member{});

    cluster_info.instances[i.uuid] = std::move(instance);
  }

  return cluster_info;
}

std::vector<Cluster_info> gather_clusters_info(
    Cluster_set_impl &cs,
    const std::list<std::shared_ptr<Cluster_impl>> &replica_clusters,
    const std::list<Cluster_id> &replica_clusters_unreachable, bool force) {
  std::vector<Cluster_info> clusters_info;
  clusters_info.reserve(replica_clusters.size() +
                        replica_clusters_unreachable.size() + 1);

  // gather unreachable clusters
  if (force) {
    for (const auto &cluster : replica_clusters_unreachable)
      clusters_info.push_back(gather_cluster_info(cs, cluster));
  }

  // gather reachable clusters (doesn't include the primary)
  for (const auto &cluster : replica_clusters)
    clusters_info.push_back(gather_cluster_info(cluster, false, force));

  // gather primary cluster (make sure it's the last one)
  clusters_info.push_back(
      gather_cluster_info(cs.get_primary_cluster(), true, force));

  if (!force) {
    bool has_not_online = std::any_of(
        clusters_info.begin(), clusters_info.end(),
        [](const auto &info) { return info.has_not_online_instances; });

    if (has_not_online)
      throw shcore::Exception(
          "Failed to dissolve the ClusterSet: Detected instances with "
          "(MISSING) state.",
          SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);

    bool has_unreachable = std::any_of(
        clusters_info.begin(), clusters_info.end(),
        [](const auto &info) { return info.has_unreachable_instances; });

    if (has_unreachable)
      throw shcore::Exception(
          "Failed to dissolve the ClusterSet: Unable to connect to all "
          "instances.",
          SHERR_DBA_UNREACHABLE_INSTANCES);
  }

  return clusters_info;
}

void sync_clusters(Cluster_set_impl &cluster_set,
                   const std::vector<Cluster_info> &clusters_info,
                   const clusterset::Dissolve_options &options,
                   bool throw_on_error) {
  if (options.dry_run) return;

  for (const auto &cluster_info : clusters_info) {
    if (cluster_info.is_unreachable || cluster_info.is_primary) continue;

    try {
      cluster_set.sync_transactions(*cluster_info.impl->get_cluster_server(),
                                    {k_clusterset_async_channel_name},
                                    options.timeout, false, true);
    } catch (const shcore::Exception &e) {
      auto console = current_console();

      if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
        console->print_error(shcore::str_format(
            "The Cluster '%s' failed to synchronize its transaction set with "
            "the PRIMARY Cluster. You may increase the transaction sync "
            "timeout with the option 'timeout' or use the 'force' option to "
            "ignore the timeout.",
            cluster_info.name.c_str()));
      }

      if (!throw_on_error) return;

      if (!options.force) throw;

      console->print_warning(shcore::str_format(
          "Transaction sync failed but ignored because of 'force' option: %s",
          e.format().c_str()));
    }
  }
}

void sync_instance(const Cluster_impl &cluster, const Instance &instance,
                   bool is_read_replica, int timeout) {
  try {
    cluster.sync_transactions(instance,
                              is_read_replica ? Instance_type::READ_REPLICA
                                              : Instance_type::GROUP_MEMBER,
                              timeout, false, true);

  } catch (const shcore::Exception &e) {
    current_console()->print_error(
        shcore::str_format("The instance '%s' failed to synchronize its "
                           "transaction set with its Cluster: %s",
                           instance.descr().c_str(), e.what()));
  }
}

}  // namespace

void Dissolve::do_run(const clusterset::Dissolve_options &options) {
  auto console = current_console();

  if (current_shell_options()->get().wizards && !options.force) {
    auto desc = clusterset::cluster_set_describe(&m_cs);
    console->print_info(
        "The ClusterSet has the following registered Clusters and instances:");

    bool pretty_print =
        (current_shell_options()->get().wrap_json.compare("json/raw") != 0);
    console->print_info(shcore::Value{desc}.descr(pretty_print));

    console->print_warning(
        "You are about to dissolve the whole ClusterSet, this operation cannot "
        "be reverted. All Clusters will be removed from the ClusterSet, all "
        "members removed from their corresponding Clusters, replication will "
        "be stopped, internal recovery user accounts and the ClusterSet / "
        "Cluster metadata will be dropped. User data will be kept intact in "
        "all instances.");
    console->print_info();

    if (console->confirm("Are you sure you want to dissolve the ClusterSet?",
                         Prompt_answer::NO) == Prompt_answer::NO) {
      throw shcore::Exception::runtime_error("Operation canceled by user.");
    }

    console->print_info();
  }

  auto primary_cluster = m_cs.get_primary_cluster();

  // put an exclusive lock on the clusterset and on all of its clusters and take
  // the change to also retrieve all the clusters
  mysqlshdk::mysql::Lock_scoped_list api_locks;
  std::list<Cluster_id> replica_clusters_unreachable;
  std::list<std::shared_ptr<Cluster_impl>> replica_clusters;
  {
    api_locks.push_back(m_cs.get_lock_exclusive());

    replica_clusters = m_cs.connect_all_clusters(
        true, &replica_clusters_unreachable, false, options.force);

    deal_with_unreachable_clusters(*primary_cluster,
                                   replica_clusters_unreachable, options.force);

    api_locks.push_back(primary_cluster->get_lock_exclusive());

    for (const auto &cluster : replica_clusters)
      api_locks.push_back(cluster->get_lock_exclusive());
  }

  // gather info on all the clusters
  auto clusters_info = gather_clusters_info(
      m_cs, replica_clusters, replica_clusters_unreachable, options.force);

  auto all_clusters_unreachable = std::all_of(
      clusters_info.begin(), clusters_info.end(), [](const auto &cluster) {
        return cluster.is_primary || cluster.is_unreachable;
      });

  // first sync before dissolving
  if (!all_clusters_unreachable) {
    console->print_info(
        "* Waiting for all reachable REPLICA Clusters to apply received "
        "transactions...");
    sync_clusters(m_cs, clusters_info, options, true);
  }

  console->print_info();
  console->print_info("* Dissolving the ClusterSet...");
  console->print_info();

  if (!options.dry_run) {
    // disable SRO on the primary

    if (auto sro = primary_cluster->get_cluster_server()->get_sysvar_bool(
            "super_read_only", false);
        sro) {
      log_info(
          "Disabling super_read_only mode on instance '%s' to run dissolve().",
          primary_cluster->get_cluster_server()->descr().c_str());
      primary_cluster->get_cluster_server()->set_sysvar("super_read_only",
                                                        false);
    }

    // drop users and metadata
    {
      Replication_account{*primary_cluster}.drop_all_replication_users();
      metadata::uninstall(primary_cluster->get_cluster_server());
    }
  }

  if (!all_clusters_unreachable) {
    console->print_info(
        "* Waiting for all reachable REPLICA Clusters to synchronize with the "
        "PRIMARY Cluster...");
    sync_clusters(m_cs, clusters_info, options, false);
  }

  // finally sync and dissolve each individual member on all clusters
  for (const auto &cluster_info : clusters_info) {
    if (cluster_info.is_unreachable)
      console->print_info(shcore::str_format(
          "* Stopping replication on reachable members of '%s'...",
          cluster_info.name.c_str()));
    else
      console->print_info(shcore::str_format(
          "* Synchronizing all members and dissolving cluster '%s'...",
          cluster_info.name.c_str()));

    if (options.dry_run) continue;

    std::shared_ptr<Instance> cluster_primary;
    if (cluster_info.impl) cluster_info.impl->get_cluster_server();

    // take care of read-replicas
    for (const auto &instance_md : cluster_info.instances_rr_md) {
      auto it = cluster_info.instances.find(instance_md.uuid);
      if (it == cluster_info.instances.end()) continue;

      auto &instance = it->second;

      // 1st: sync instance
      if (cluster_info.impl)
        sync_instance(*cluster_info.impl, *instance, true, options.timeout);

      // 2nd: remove read-replica async channel
      {
        try {
          DBUG_EXECUTE_IF("dba_dissolve_replication_error", {
            throw shcore::Exception(
                "Error while removing read-replica async replication channel",
                SHERR_DBA_REPLICATION_ERROR);
          });

          remove_channel(*instance, k_read_replica_async_channel_name, false);
        } catch (const shcore::Exception &e) {
          console->print_warning(
              shcore::str_format("Unable to remove read-replica replication "
                                 "channel for instance '%s': %s",
                                 instance->descr().c_str(), e.what()));
        }

        try {
          log_info("Disabling automatic failover management at %s",
                   instance->descr().c_str());

          reset_managed_connection_failover(*instance, false);
        } catch (const shcore::Exception &) {
          log_error("Error disabling automatic failover at %s: %s",
                    instance->descr().c_str(),
                    format_active_exception().c_str());
          console->print_warning(shcore::str_format(
              "An error occurred when trying to disable "
              "automatic failover on Read-Replica at '%s': %s",
              instance->descr().c_str(), format_active_exception().c_str()));
        }
      }
    }

    // take care of non read-replicas
    for (const auto &instance_md : cluster_info.instances_md) {
      auto it = cluster_info.instances.find(instance_md.first.uuid);
      if (it == cluster_info.instances.end()) continue;

      auto &instance = it->second;

      // 1st: sync instance
      if (cluster_info.impl)
        sync_instance(*cluster_info.impl, *instance, false, options.timeout);

      // ignore remaining steps if it's the primary of the cluster
      if (cluster_primary &&
          (cluster_primary->get_uuid() == instance_md.first.uuid))
        continue;

      // 2nd: stop GR
      if ((instance_md.first.instance_type == Instance_type::NONE) ||
          (instance_md.first.instance_type == Instance_type::GROUP_MEMBER)) {
        try {
          DBUG_EXECUTE_IF("dba_dissolve_replication_error", {
            throw shcore::Exception("Error while trying to leave cluster",
                                    SHERR_DBA_REPLICATION_ERROR);
          });

          mysqlsh::dba::leave_cluster(
              *instance, cluster_info.supports_member_actions, true, true);

        } catch (const std::exception &err) {
          log_error("Instance '%s' failed to leave the cluster: %s",
                    instance->descr().c_str(), err.what());
          console->print_warning(shcore::str_format(
              "An error occurred when trying to remove instance '%s' from "
              "cluster '%s'. The instance might have been left active, "
              "requiring manual action to fully dissolve the cluster.",
              instance->descr().c_str(), cluster_info.name.c_str()));
          console->print_info();
        }
      }

      // reset skip_replica_start
      instance->set_sysvar("skip_replica_start", false,
                           mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

      // 3rd: remove clusterset async channel (if any)
      try {
        remove_channel(*instance, k_clusterset_async_channel_name, false);
      } catch (const shcore::Exception &e) {
        console->print_error(
            shcore::str_format("Unable to remove ClusterSet replication "
                               "channel for instance '%s': %s",
                               instance->descr().c_str(), e.what()));
      }
    }

    // Reconcile the view change GTIDs generated when the Replica Cluster was
    // created and when members were added to it.
    if (cluster_primary && !cluster_info.is_primary)
      m_cs.reconcile_view_change_gtids(cluster_primary.get());

    // finally, take care of the primary
    if (cluster_primary) {
      try {
        DBUG_EXECUTE_IF("dba_dissolve_replication_error", {
          throw shcore::Exception("Error while trying to leave cluster",
                                  SHERR_DBA_REPLICATION_ERROR);
        });

        mysqlsh::dba::leave_cluster(
            *cluster_primary, cluster_info.supports_member_actions, true, true);

        cluster_primary->set_sysvar(
            "skip_replica_start", false,
            mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);

        reset_managed_connection_failover(*cluster_primary, false);

      } catch (const std::exception &err) {
        log_error("Instance '%s' failed to leave the cluster: %s",
                  cluster_primary->descr().c_str(), err.what());
        console->print_warning(shcore::str_format(
            "An error occurred when trying to remove instance '%s' from "
            "cluster '%s'. The instance might have been left active, requiring "
            "manual action to fully dissolve the cluster.",
            cluster_primary->descr().c_str(), cluster_info.name.c_str()));
        console->print_info();
      }

      try {
        remove_channel(*cluster_primary, k_clusterset_async_channel_name,
                       false);
      } catch (const shcore::Exception &e) {
        console->print_error(
            shcore::str_format("Unable to remove ClusterSet replication "
                               "channel for instance '%s': %s",
                               cluster_primary->descr().c_str(), e.what()));
      }
    }
  }

  // disconnect all internal sessions
  if (!options.dry_run) m_cs.disconnect();

  console->print_info();
  console->print_info(
      "The ClusterSet has been dissolved, user data was left intact.");
  console->print_info();

  if (options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
}

}  // namespace mysqlsh::dba::clusterset
