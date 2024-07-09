/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#include <algorithm>
#include <tuple>
#include <utility>

#include "modules/adminapi/cluster_set/status.h"
#include "modules/adminapi/common/common_status.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

namespace {

shcore::Array_t cluster_diagnostics(
    Cluster_impl *primary_cluster, Cluster_impl *cluster,
    const mysqlshdk::mysql::Replication_channel &channel,
    shcore::Array_t cluster_errors) {
  using mysqlshdk::mysql::Replication_channel;

  auto append_error = [&cluster_errors](std::string msg) {
    if (!cluster_errors) {
      cluster_errors = shcore::make_array();
    }
    cluster_errors->push_back(shcore::Value(std::move(msg)));
  };

  // GTID_EXECUTED consistency handled elsewhere

  if (cluster->cluster_availability() != Cluster_availability::UNREACHABLE) {
    if (cluster->is_primary_cluster()) {
      if (!channel.host.empty() ||
          channel.status() != Replication_channel::Status::OFF) {
        append_error(shcore::str_format(
            "WARNING: Unexpected replication channel '%s' at Primary Cluster",
            channel.channel_name.c_str()));
      }
    } else {
      if (channel.host.empty()) {
        append_error(
            "WARNING: Replication channel from the Primary Cluster is missing");
      } else {
        auto channel_status = channel.status();
        if ((channel_status != Replication_channel::Status::CONNECTING) &&
            (channel_status != Replication_channel::Status::ON)) {
          append_error(
              "WARNING: Replication from the Primary Cluster not in expected "
              "state");

          if (primary_cluster->cluster_availability() ==
              Cluster_availability::ONLINE) {
            auto primary = primary_cluster->get_primary_master();
            if (channel.source_uuid != primary->get_uuid())
              append_error(shcore::str_format(
                  "WARNING: Replicating from wrong source. Expected %s (%s) "
                  "but is %s:%i (%s)",
                  primary->descr().c_str(), primary->get_uuid().c_str(),
                  channel.host.c_str(), channel.port,
                  channel.source_uuid.c_str()));
          }
        }
      }
    }
  }

  if (cluster->is_invalidated()) {
    append_error(
        "WARNING: Cluster was invalidated and must be either removed from the "
        "ClusterSet or rejoined");
  }

  return cluster_errors;
}

shcore::Value cluster_status(const Cluster_set_member_metadata &mmd,
                             Cluster_set_impl *clusterset,
                             Cluster_impl *cluster,
                             Cluster_global_status cl_status, int extended,
                             mysqlshdk::mysql::Gtid_set *out_gtid_set,
                             mysqlshdk::mysql::Gtid_set *out_received_gtid_set,
                             std::string *out_view_change_uuid) {
  // Get trimmed down cluster
  auto primary = clusterset->get_primary_master();

  shcore::Dictionary_t status =
      cluster->cluster_status((extended > 1 ? 1 : 0)).as_map();
  shcore::Dictionary_t status_map = status->get_map("defaultReplicaSet");

  shcore::Array_t cluster_errors = nullptr;
  if (status_map->has_key("clusterErrors")) {
    cluster_errors = status_map->get_array("clusterErrors");
  }

  shcore::Dictionary_t res = shcore::make_dict(
      "clusterRole",
      shcore::Value(mmd.primary_cluster ? "PRIMARY" : "REPLICA"));

  if (mmd.cluster.tags && !mmd.cluster.tags->empty()) {
    (*res)["tags"] = shcore::Value(mmd.cluster.tags);
  }

  // Handle extended option
  if (extended) {
    auto topology = status_map->get_map("topology");

    for (auto &inst : *topology) {
      auto inst_info = inst.second.as_map();

      if (extended <= 1) {
        inst_info->erase("applierWorkerThreads");
        inst_info->erase("fenceSysVars");
        inst_info->erase("memberId");
      }
    }

    res->set("topology", shcore::Value(topology));
    if (extended > 1) {
      for (const auto key : {"ssl", "groupName", "groupViewChangeUuid",
                             "communicationStack", "paxosSingleLeader"})
        if (status_map->has_key(key)) res->set(key, status_map->at(key));
    }
  }
  if (extended || cl_status != Cluster_global_status::OK) {
    res->set("status", status_map->at("status"));
    res->set("statusText", status_map->at("statusText"));
  }

  res->set("globalStatus", shcore::Value(to_string(cl_status)));

  bool add_repl_info = false;

  Cluster_channel_status ch_status =
      clusterset->get_replication_channel_status(*cluster);

  // It's a primary cluster
  if (mmd.primary_cluster) {
    if (primary && cl_status == Cluster_global_status::OK)
      res->set("primary", shcore::Value(primary->get_canonical_address()));
    else
      res->set("primary", shcore::Value::Null());

    if (ch_status != Cluster_channel_status::MISSING &&
        ch_status != Cluster_channel_status::STOPPED) {
      res->set("clusterSetReplicationStatus",
               shcore::Value(to_string(ch_status)));
      add_repl_info = true;
    }
  } else {  // it's a replica cluster
    res->set("clusterSetReplicationStatus",
             shcore::Value(to_string(ch_status)));

    // if replica cluster is available
    if (cluster->get_cluster_server()) add_repl_info = true;
  }

  if (auto cluster_server = cluster->get_cluster_server()) {
    *out_gtid_set =
        mysqlshdk::mysql::Gtid_set::from_gtid_executed(*cluster_server);
    *out_received_gtid_set =
        mysqlshdk::mysql::Gtid_set::from_received_transaction_set(
            *cluster_server, k_clusterset_async_channel_name);
    *out_view_change_uuid = cluster_server->get_sysvar_string(
        "group_replication_view_change_uuid", "");
  }

  if (add_repl_info && cluster->get_cluster_server()) {
    // Add clusterSetReplication object
    mysqlshdk::mysql::Replication_channel cs_channel;
    bool channel_exists = mysqlshdk::mysql::get_channel_status(
        *cluster->get_cluster_server(), k_clusterset_async_channel_name,
        &cs_channel);

    if (channel_exists && extended) {
      mysqlshdk::mysql::Replication_channel_master_info master_info;
      mysqlshdk::mysql::Replication_channel_relay_log_info relay_info;

      mysqlshdk::mysql::get_channel_info(*cluster->get_cluster_server(),
                                         k_clusterset_async_channel_name,
                                         &master_info, &relay_info);

      shcore::Dictionary_t chstatus =
          channel_status(&cs_channel, &master_info, &relay_info, "",
                         extended - 1, true, false);

      if (extended > 0) {
        chstatus->set("receiver",
                      shcore::Value(cluster->get_cluster_server()->descr()));

        // SSL channel mode
        {
          std::string_view ssl_mode = kClusterSSLModeDisabled;
          if (master_info.enabled_ssl != 0) {
            bool certs_enabled =
                !master_info.ssl_ca.empty() || !master_info.ssl_capath.empty();
            if (master_info.ssl_verify_server_cert) {
              assert(certs_enabled);
              ssl_mode = kClusterSSLModeVerifyIdentity;
            } else {
              ssl_mode = certs_enabled ? kClusterSSLModeVerifyCA
                                       : kClusterSSLModeRequired;
            }
          }

          chstatus->set("replicationSslMode", shcore::Value(ssl_mode));
        }

        // SSL info
        if (primary) {
          auto repl_user =
              clusterset->get_metadata_storage()->get_cluster_repl_account_user(
                  cluster->get_id());

          std::string ssl_cipher, ssl_version;
          mysqlshdk::mysql::iterate_thread_variables(
              *primary, "Binlog Dump GTID", repl_user, "Ssl%",
              [&ssl_cipher, &ssl_version](const std::string &var_name,
                                          std::string var_value) {
                if (var_name == "Ssl_cipher")
                  ssl_cipher = std::move(var_value);
                else if (var_name == "Ssl_version")
                  ssl_version = std::move(var_value);

                return (ssl_cipher.empty() || ssl_version.empty());
              });

          if (!ssl_cipher.empty() || !ssl_version.empty()) {
            assert(chstatus->has_key("replicationSsl"));

            if (ssl_cipher.empty())
              (*chstatus)["replicationSsl"] =
                  shcore::Value(std::move(ssl_version));
            else if (ssl_version.empty())
              (*chstatus)["replicationSsl"] =
                  shcore::Value(std::move(ssl_cipher));
            else
              (*chstatus)["replicationSsl"] = shcore::Value(shcore::str_format(
                  "%s %s", ssl_cipher.c_str(), ssl_version.c_str()));
          }
        } else {
          (*chstatus)["replicationSsl"] = shcore::Value::Null();
        }
      }

      res->set("clusterSetReplication", shcore::Value(std::move(chstatus)));
    } else if (!channel_exists) {
      res->set("clusterSetReplication", shcore::Value(shcore::make_dict()));
    }

    cluster_errors =
        cluster_diagnostics(clusterset->get_primary_cluster().get(), cluster,
                            cs_channel, cluster_errors);
  } else {
    cluster_errors = cluster_diagnostics(
        clusterset->get_primary_cluster().get(), cluster, {}, cluster_errors);
  }
  if (cluster_errors) {
    res->set("clusterErrors", shcore::Value(cluster_errors));
  }

  return shcore::Value(res);
}

void check_cluster_consistency(
    Cluster_impl *cluster, shcore::Dictionary_t status,
    const mysqlshdk::mysql::Gtid_set &cluster_gtid,
    const mysqlshdk::mysql::Gtid_set &cluster_received_gtid,
    const std::vector<std::string> &view_change_uuids,
    const mysqlshdk::mysql::Gtid_set &primary_gtid, int extended) {
  auto cluster_server = cluster->get_cluster_server();

  mysqlshdk::mysql::Gtid_set gtid_missing = primary_gtid;
  gtid_missing.subtract(cluster_gtid, *cluster_server);

  mysqlshdk::mysql::Gtid_set gtid_errant = cluster_gtid;

  // filter out GTIDs received via clusterset AR channel, so that we don't
  // report transactions that were already replicated but not yet exposed to
  // GTID_EXECUTED at the source (can happen if the primary has very high load)
  gtid_errant.subtract(cluster_received_gtid, *cluster_server);

  for (const auto &uuid : view_change_uuids)
    gtid_errant.subtract(gtid_errant.get_gtids_from(uuid), *cluster_server);
  gtid_errant.subtract(primary_gtid, *cluster_server);

  if (extended > 0 || !gtid_errant.empty()) {
    status->set("transactionSetConsistencyStatus",
                shcore::Value(gtid_errant.empty() ? "OK" : "INCONSISTENT"));

    if (!gtid_errant.empty()) {
      status->set("transactionSetConsistencyStatusText",
                  shcore::Value(shcore::str_format(
                      "There are %zi transactions that were executed in this "
                      "instance that did not originate from the PRIMARY.",
                      static_cast<size_t>(gtid_errant.count()))));

      auto cluster_errors = status->get_array("clusterErrors");
      if (!cluster_errors) {
        cluster_errors = shcore::make_array();
        status->set("clusterErrors", shcore::Value(cluster_errors));
      }
      cluster_errors->push_back(
          shcore::Value("ERROR: Errant transactions detected"));
    }
  }
  if (extended > 0) {
    status->set(
        "transactionSetErrantGtidSet",
        shcore::Value(shcore::str_replace(gtid_errant.str(), "\n", "")));
    status->set(
        "transactionSetMissingGtidSet",
        shcore::Value(shcore::str_replace(gtid_missing.str(), "\n", "")));
  }
}
}  // namespace

shcore::Dictionary_t cluster_set_status(Cluster_set_impl *cluster_set,
                                        uint64_t extended) {
  assert(cluster_set);
  shcore::Dictionary_t dict = shcore::make_dict();

  dict->set("domainName", shcore::Value(cluster_set->get_name()));

  auto pc = cluster_set->get_primary_cluster();

  dict->set("primaryCluster", shcore::Value(pc->get_name()));

  if (extended >= 1) {
    auto md_server = cluster_set->get_metadata_storage()->get_md_server();
    (*dict)["metadataServer"] =
        shcore::Value(md_server->get_canonical_address());
  }

  {
    shcore::Dictionary_t cstatus = shcore::make_dict();

    std::vector<Cluster_set_member_metadata> clusters;

    cluster_set->get_metadata_storage()->get_cluster_set(
        cluster_set->get_id(), true, nullptr, &clusters);

    // ensure the primary cluster is the last in the list, so that
    // GTID_EXECUTED we query to compare between clusters is the freshest
    std::sort(clusters.begin(), clusters.end(),
              [](const Cluster_set_member_metadata &a,
                 const Cluster_set_member_metadata &b) {
                return a.primary_cluster < b.primary_cluster;
              });

    Cluster_global_status primary_status = Cluster_global_status::UNKNOWN;
    Cluster_id primary_cluster_id;
    mysqlshdk::mysql::Gtid_set primary_gtid_set;
    int num_cluster_ok = 0;
    int num_cluster_total = 0;

    std::map<Cluster_id,
             std::tuple<std::shared_ptr<Cluster_impl>, shcore::Dictionary_t,
                        mysqlshdk::mysql::Gtid_set, mysqlshdk::mysql::Gtid_set,
                        std::string>>
        gtid_sets;

    std::vector<std::string> view_change_uuids;

    for (const auto &cluster_md : clusters) {
      auto cluster = cluster_set->get_cluster_object(cluster_md, true);
      auto cl_status = cluster_set->get_cluster_global_status(cluster.get());

      mysqlshdk::mysql::Gtid_set gtid_executed;
      mysqlshdk::mysql::Gtid_set gtid_received;
      std::string view_change_uuid;
      auto status = cluster_status(cluster_md, cluster_set, cluster.get(),
                                   cl_status, extended, &gtid_executed,
                                   &gtid_received, &view_change_uuid);

      view_change_uuids.emplace_back(view_change_uuid);

      gtid_sets.emplace(cluster_md.cluster.cluster_id,
                        std::make_tuple(cluster, status.as_map(), gtid_executed,
                                        gtid_received, view_change_uuid));

      cstatus->set(cluster->get_name(), status);

      num_cluster_total++;

      if (cluster->is_primary_cluster()) {
        primary_status = cl_status;
        primary_cluster_id = cluster_md.cluster.cluster_id;
        primary_gtid_set = gtid_executed;
      }

      // we consider any OK status to be enough to mark a cluster as available,
      // even if there are replication issues
      if (cl_status != Cluster_global_status::NOT_OK &&
          cl_status != Cluster_global_status::INVALIDATED &&
          cl_status != Cluster_global_status::UNKNOWN) {
        if (cl_status == Cluster_global_status::OK) num_cluster_ok++;
      }
    }

    for (const auto &cluster_md : clusters) {
      const auto &info = gtid_sets[cluster_md.cluster.cluster_id];
      auto cluster = std::get<0>(info);
      auto status = std::get<1>(info);
      const auto &cluster_gtid = std::get<2>(info);
      const auto &received_gtid = std::get<3>(info);

      bool is_primary = cluster_md.cluster.cluster_id == primary_cluster_id;

      if (extended > 0) {
        if (!cluster->get_cluster_server()) {
          status->set("transactionSet", shcore::Value::Null());
          if (extended > 1 && !is_primary)
            status->set("receivedTransactionSet", shcore::Value::Null());
        } else {
          status->set("transactionSet", shcore::Value(shcore::str_replace(
                                            cluster_gtid.str(), "\n", "")));
          if (extended > 1 && !is_primary)
            status->set("receivedTransactionSet",
                        shcore::Value(received_gtid.str()));
        }
      }
      // check cluster consistency if we could query the primary
      if (!primary_gtid_set.empty() && !is_primary) {
        if (cluster->get_cluster_server())
          check_cluster_consistency(cluster.get(), status, cluster_gtid,
                                    received_gtid, view_change_uuids,
                                    primary_gtid_set, extended);
      }
    }

    dict->set("clusters", shcore::Value(cstatus));

    dict->set("globalPrimaryInstance",
              cstatus->get_map(pc->get_name())->at("primary"));

    {
      Cluster_set_global_status global_availability =
          Cluster_set_global_status::UNAVAILABLE;
      std::string status_text;

      switch (primary_status) {
        case Cluster_global_status::OK_NOT_REPLICATING:  // shouldn't happen
          assert(0);
        case Cluster_global_status::OK:
          if (num_cluster_ok >= num_cluster_total) {
            status_text = "All Clusters available.";
            global_availability = Cluster_set_global_status::HEALTHY;
          } else {
            status_text = "Primary Cluster available, there are issues with ";
            if (num_cluster_total - num_cluster_ok == 1) {
              status_text += "a Replica cluster.";
            } else {
              status_text +=
                  std::to_string(num_cluster_total - num_cluster_ok) +
                  " Replica Clusters.";
            }
            global_availability = Cluster_set_global_status::AVAILABLE;
          }
          break;

        case Cluster_global_status::OK_FENCED_WRITES:
          status_text = "Primary Cluster is fenced from write traffic.";
          global_availability = Cluster_set_global_status::UNAVAILABLE;
          break;

        case Cluster_global_status::INVALIDATED:        // shouldn't happen
        case Cluster_global_status::OK_NOT_CONSISTENT:  // shouldn't happen
          assert(0);
        case Cluster_global_status::OK_MISCONFIGURED:
          status_text = "Primary Cluster available but requires attention.";
          global_availability = Cluster_set_global_status::AVAILABLE;
          break;

        case Cluster_global_status::NOT_OK:
          status_text =
              "Primary Cluster is not available. ClusterSet availability may "
              "be restored by restoring the Primary Cluster or failover.";
          global_availability = Cluster_set_global_status::UNAVAILABLE;
          break;

        case Cluster_global_status::UNKNOWN:
          status_text =
              "Primary Cluster is not reachable from the Shell, assuming it to "
              "be unavailable.";
          global_availability = Cluster_set_global_status::UNAVAILABLE;
          break;
      }

      dict->set("status", shcore::Value(to_string(global_availability)));
      dict->set("statusText", shcore::Value(status_text));
    }
  }

  return dict;
}

shcore::Dictionary_t cluster_set_describe(Cluster_set_impl *cluster_set) {
  assert(cluster_set);

  auto cluster_description = [](const Cluster_set_member_metadata &mmd,
                                Cluster_impl *cluster) {
    shcore::Dictionary_t description = cluster->cluster_describe().as_map();

    shcore::Dictionary_t description_map =
        description->get_map("defaultReplicaSet");

    auto descr = shcore::make_dict(
        "clusterRole",
        shcore::Value(mmd.primary_cluster ? "PRIMARY" : "REPLICA"), "topology",
        description_map->at("topology"));

    if (mmd.invalidated) descr->set("invalidated", shcore::Value::True());

    return shcore::Value(descr);
  };

  shcore::Dictionary_t dict = shcore::make_dict();

  // Set basic info first
  dict->set("domainName", shcore::Value(cluster_set->get_name()));
  dict->set("primaryCluster",
            shcore::Value(cluster_set->get_primary_cluster()->get_name()));

  // Create the dictionary to be used for the topology of the ClusterSet
  shcore::Dictionary_t clusters_description = shcore::make_dict();

  // Get all Cluster belonging to the ClusterSet
  Cluster_set_metadata cset;
  std::vector<Cluster_set_member_metadata> clusters;

  cluster_set->get_primary_cluster()->get_metadata_storage()->get_cluster_set(
      cluster_set->get_id(), true, &cset, &clusters);

  // Populate each Cluster with its description
  for (const auto &cluster_md : clusters) {
    auto cluster = cluster_set->get_cluster_object(cluster_md, true);

    clusters_description->set(cluster->get_name(),
                              cluster_description(cluster_md, cluster.get()));
  }

  dict->set("clusters", shcore::Value(clusters_description));

  return dict;
}

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh
