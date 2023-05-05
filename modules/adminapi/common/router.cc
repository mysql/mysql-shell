/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/router.h"

#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

const std::map<std::string, shcore::Value> k_default_clusterset_router_options =
    {{k_router_option_invalidated_cluster_routing_policy,
      shcore::Value(
          k_router_option_invalidated_cluster_routing_policy_drop_all)},
     {k_router_option_target_cluster,
      shcore::Value(k_router_option_target_cluster_primary)},
     {k_router_option_stats_updates_frequency, shcore::Value::Null()},
     {k_router_option_use_replica_primary_as_rw, shcore::Value::False()},
     {k_router_option_tags, shcore::Value(shcore::make_dict())},
     {k_router_option_read_only_targets,
      shcore::Value(k_default_router_option_read_only_targets)}};

const std::map<std::string, shcore::Value> k_default_cluster_router_options = {
    {k_router_option_tags, shcore::Value(shcore::make_dict())},
    {k_router_option_read_only_targets,
     shcore::Value(k_default_router_option_read_only_targets)},
    {k_router_option_stats_updates_frequency, shcore::Value::Null()}};

const std::map<std::string, shcore::Value> k_default_replicaset_router_options =
    {{k_router_option_tags, shcore::Value(shcore::make_dict())},
     {k_router_option_stats_updates_frequency, shcore::Value::Null()}};

inline bool is_router_upgrade_required(
    const mysqlshdk::utils::Version &version) {
  // Router 1.0.9 shouldn't matter as they're unlikely to exist in the wild,
  // but we mark it to require an upgrade, to make testing possible.
  if (version <= mysqlshdk::utils::Version("1.0.9")) {
    return true;
  }
  // MD 2.0.0 was introduced in Shell + Router 8.0.19
  if (version <= mysqlshdk::utils::Version("8.0.18")) {
    return true;
  }
  return false;
}

shcore::Dictionary_t get_router_dict(const Router_metadata &router_md,
                                     bool only_upgrade_required) {
  auto router = shcore::make_dict();

  mysqlshdk::utils::Version router_version;
  if (!router_md.version.has_value()) {
    router_version = mysqlshdk::utils::Version("8.0.18");
    (*router)["version"] = shcore::Value("<= 8.0.18");
  } else {
    router_version = mysqlshdk::utils::Version(*router_md.version);
    (*router)["version"] = shcore::Value(*router_md.version);
  }

  bool upgrade_required = is_router_upgrade_required(router_version);
  if (upgrade_required)
    (*router)["upgradeRequired"] = shcore::Value(upgrade_required);

  if (only_upgrade_required && !upgrade_required) {
    router->clear();
    return router;
  }

  (*router)["hostname"] = shcore::Value(router_md.hostname);

  if (!router_md.last_checkin.has_value())
    (*router)["lastCheckIn"] = shcore::Value::Null();
  else
    (*router)["lastCheckIn"] = shcore::Value(*router_md.last_checkin);

  if (!router_md.ro_port.has_value())
    (*router)["roPort"] = shcore::Value::Null();
  else
    (*router)["roPort"] = shcore::Value(*router_md.ro_port);

  if (!router_md.rw_port.has_value())
    (*router)["rwPort"] = shcore::Value::Null();
  else
    (*router)["rwPort"] = shcore::Value(*router_md.rw_port);

  if (!router_md.ro_x_port.has_value())
    (*router)["roXPort"] = shcore::Value::Null();
  else
    (*router)["roXPort"] = shcore::Value(*router_md.ro_x_port);

  if (!router_md.rw_x_port.has_value())
    (*router)["rwXPort"] = shcore::Value::Null();
  else
    (*router)["rwXPort"] = shcore::Value(*router_md.rw_x_port);

  if (router_md.rw_split_port.has_value())
    (*router)["rwSplitPort"] = shcore::Value(*router_md.rw_split_port);

  return router;
}

shcore::Value router_list(MetadataStorage *md, const Cluster_id &cluster_id,
                          bool only_upgrade_required) {
  auto router_list = shcore::make_dict();
  auto routers_md = md->get_routers(cluster_id);

  for (const auto &router_md : routers_md) {
    auto router = get_router_dict(router_md, only_upgrade_required);
    if (router->empty()) continue;

    auto label = shcore::str_format("%s::%s", router_md.hostname.c_str(),
                                    router_md.name.c_str());
    router_list->set(std::move(label), shcore::Value(std::move(router)));
  }

  return shcore::Value(std::move(router_list));
}

shcore::Value clusterset_list_routers(MetadataStorage *md,
                                      const Cluster_set_id &clusterset_id,
                                      const std::string &router) {
  auto router_list = shcore::make_dict();
  auto routers_md = md->get_clusterset_routers(clusterset_id);
  std::vector<std::string> routers_needing_rebootstrap;

  for (const auto &router_md : routers_md) {
    auto label = shcore::str_format("%s::%s", router_md.hostname.c_str(),
                                    router_md.name.c_str());
    if (!router.empty() && router != label) continue;
    auto r = get_router_dict(router_md, false);
    if (r->empty()) continue;

    if (!router_md.target_cluster.has_value()) {
      (*r)["targetCluster"] = shcore::Value::Null();
    } else if (router_md.target_cluster.value() !=
               k_router_option_target_cluster_primary) {
      // Translate the Cluster's UUID (group_replication_group_name) to the
      // Cluster's name
      (*r)["targetCluster"] =
          shcore::Value(md->get_cluster_name(router_md.target_cluster.value()));
    } else {
      (*r)["targetCluster"] = shcore::Value(*router_md.target_cluster);
    }

    router_list->set(label, shcore::Value(r));

    // Check if the Router needs a re-bootstrap
    shcore::Array_t router_errors = shcore::make_array();
    if (router_md.bootstrap_target_type.value_or("") != "clusterset") {
      routers_needing_rebootstrap.push_back(label);

      router_errors->push_back(
          shcore::Value("WARNING: Router must be bootstrapped again for the "
                        "ClusterSet to be recognized."));
    }

    if (router_errors && !router_errors->empty()) {
      (*r)["routerErrors"] = shcore::Value(std::move(router_errors));
    }
  }

  if (!router.empty() && router_list->empty())
    throw shcore::Exception::argument_error(
        "Router '" + router + "' is not registered in the ClusterSet");

  // Print a warning if there are Routers needing a re-bootstrap
  if (!routers_needing_rebootstrap.empty()) {
    mysqlsh::current_console()->print_warning(
        "The following Routers were bootstrapped before the ClusterSet was "
        "created: [" +
        shcore::str_join(routers_needing_rebootstrap, ", ") +
        "]. Please re-bootstrap the Routers to ensure the ClusterSet is "
        "recognized and the configurations are updated. Otherwise, Routers "
        "will operate as if the Clusters were standalone.");
  }

  return shcore::Value(std::move(router_list));
}

shcore::Dictionary_t router_options(MetadataStorage *md, Cluster_type type,
                                    const std::string &id,
                                    const std::string &router_label) {
  auto router_options = shcore::make_dict();
  auto romd = md->get_routing_options(type, id);

  const auto get_options_dict =
      [](const std::map<std::string, shcore::Value> &entry) {
        auto ret = shcore::make_dict();
        for (const auto &option : entry) (*ret)[option.first] = option.second;

        return shcore::Value(std::move(ret));
      };

  if (!router_label.empty()) {
    if (romd.routers.find(router_label) != romd.routers.end()) {
      const auto &entry = romd.routers[router_label];

      (*router_options)[router_label] = get_options_dict(entry);
    } else {
      throw shcore::Exception::argument_error(
          "Router '" + router_label + "' is not registered in the " +
          to_display_string(type, Display_form::THING));
    }
  } else {
    auto routers = shcore::make_dict();

    (*router_options)["global"] = get_options_dict(romd.global);
    for (const auto &entry : romd.routers) {
      (*routers)[entry.first] = get_options_dict(entry.second);
    }
    (*router_options)["routers"] = shcore::Value(routers);
  }

  return router_options;
}

shcore::Value validate_router_option(const Base_cluster_impl &cluster,
                                     const std::string &name,
                                     const shcore::Value &value) {
  {
    auto check_option = [&name](const auto &col) {
      if (std::find(col.begin(), col.end(), name) != col.end()) return;

      throw shcore::Exception::argument_error(shcore::str_format(
          "Unsupported routing option, '%s' supported options: %s",
          name.c_str(),
          shcore::str_join(col.begin(), col.end(), ", ").c_str()));
    };

    switch (cluster.get_type()) {
      case Cluster_type::REPLICATED_CLUSTER:
        check_option(k_clusterset_router_options);
        break;
      case Cluster_type::GROUP_REPLICATION:
        check_option(k_cluster_router_options);
        break;
      case Cluster_type::ASYNC_REPLICATION:
        check_option(k_replicaset_router_options);
        break;
      case Cluster_type::NONE:
        throw std::logic_error("internal error");
    }
  }

  if (value.get_type() == shcore::Value_type::Null) return value;

  shcore::Value fixed_value = value;
  if (name == k_router_option_invalidated_cluster_routing_policy) {
    if (value.get_type() != shcore::Value_type::String ||
        (value.get_string() !=
             k_router_option_invalidated_cluster_routing_policy_accept_ro &&
         value.get_string() !=
             k_router_option_invalidated_cluster_routing_policy_drop_all)) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for routing option '%s', accepted values: '%s', '%s'",
          k_router_option_invalidated_cluster_routing_policy,
          k_router_option_invalidated_cluster_routing_policy_accept_ro,
          k_router_option_invalidated_cluster_routing_policy_drop_all));
    }
  } else if (name == k_router_option_target_cluster) {
    bool ok = false;
    if (value.get_type() == shcore::Value_type::String) {
      if (value.get_string() == k_router_option_target_cluster_primary) {
        ok = true;
      } else {
        // Translate the clusterName into the Cluster's UUID
        // (group_replication_group_name)
        try {
          fixed_value = shcore::Value(
              cluster.get_metadata_storage()->get_cluster_group_name(
                  value.get_string()));
          ok = true;
        } catch (const shcore::Exception &e) {
          if (e.code() != SHERR_DBA_METADATA_MISSING) throw;
        }
      }
    }

    if (!ok)
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s', accepted "
                             "values 'primary' or valid cluster name",
                             k_router_option_target_cluster));
  } else if (name == k_router_option_stats_updates_frequency) {
    if (value.get_type() != shcore::Value_type::Integer &&
        value.get_type() != shcore::Value_type::UInteger) {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s', value is "
                             "expected to be an integer.",
                             k_router_option_stats_updates_frequency));
    }

    if (value.as_int() < 0) {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s', value is "
                             "expected to be a positive integer.",
                             k_router_option_stats_updates_frequency));
    }
  } else if (name == k_router_option_use_replica_primary_as_rw) {
    if (value.get_type() != shcore::Value_type::Bool) {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s', value is "
                             "expected to be a boolean.",
                             k_router_option_use_replica_primary_as_rw));
    }
  } else if (name == k_router_option_read_only_targets) {
    if (value.get_type() != shcore::Value_type::String ||
        (value.get_string() != k_router_option_read_only_targets_all &&
         value.get_string() !=
             k_router_option_read_only_targets_read_replicas &&
         value.get_string() != k_router_option_read_only_targets_secondaries)) {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s', accepted "
                             "values: '%s', '%s', '%s'",
                             k_router_option_read_only_targets,
                             k_router_option_read_only_targets_all,
                             k_router_option_read_only_targets_read_replicas,
                             k_router_option_read_only_targets_secondaries));
    }
  }

  return fixed_value;
}

}  // namespace dba
}  // namespace mysqlsh
