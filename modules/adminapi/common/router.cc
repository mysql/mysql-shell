/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include <unordered_set>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

Router_options_metadata k_default_router_options = {
    {},
    {{k_router_option_invalidated_cluster_routing_policy,
      shcore::Value(
          k_router_option_invalidated_cluster_routing_policy_drop_all)},
     {k_router_option_target_cluster,
      shcore::Value(k_router_option_target_cluster_primary)}}};

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
  if (router_md.version.is_null()) {
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

  if (router_md.last_checkin.is_null())
    (*router)["lastCheckIn"] = shcore::Value::Null();
  else
    (*router)["lastCheckIn"] = shcore::Value(*router_md.last_checkin);

  if (router_md.ro_port.is_null())
    (*router)["roPort"] = shcore::Value::Null();
  else
    (*router)["roPort"] = shcore::Value(*router_md.ro_port);

  if (router_md.rw_port.is_null())
    (*router)["rwPort"] = shcore::Value::Null();
  else
    (*router)["rwPort"] = shcore::Value(*router_md.rw_port);

  if (router_md.ro_x_port.is_null())
    (*router)["roXPort"] = shcore::Value::Null();
  else
    (*router)["roXPort"] = shcore::Value(*router_md.ro_x_port);

  if (router_md.rw_x_port.is_null())
    (*router)["rwXPort"] = shcore::Value::Null();
  else
    (*router)["rwXPort"] = shcore::Value(*router_md.rw_x_port);

  return router;
}

shcore::Value router_list(MetadataStorage *md, const Cluster_id &cluster_id,
                          bool only_upgrade_required) {
  auto router_list = shcore::make_dict();
  auto routers_md = md->get_routers(cluster_id);

  for (const auto &router_md : routers_md) {
    auto router = get_router_dict(router_md, only_upgrade_required);
    if (router->empty()) continue;
    std::string label = router_md.hostname + "::" + router_md.name;
    router_list->set(label, shcore::Value(router));
  }

  return shcore::Value(router_list);
}

shcore::Value clusterset_list_routers(MetadataStorage *md,
                                      const Cluster_set_id &clusterset_id,
                                      const std::string &router) {
  auto router_list = shcore::make_dict();
  auto routers_md = md->get_clusterset_routers(clusterset_id);

  for (const auto &router_md : routers_md) {
    std::string label = router_md.hostname + "::" + router_md.name;
    if (!router.empty() && router != label) continue;
    auto r = get_router_dict(router_md, false);
    if (r->empty()) continue;

    if (router_md.target_cluster.is_null()) {
      (*r)["targetCluster"] = shcore::Value::Null();
    } else if (router_md.target_cluster.get_safe() !=
               k_router_option_target_cluster_primary) {
      // Translate the Cluster's UUID (group_replication_group_name) to the
      // Cluster's name
      std::string cluster_name =
          md->get_cluster_name(router_md.target_cluster.get_safe());
      (*r)["targetCluster"] = shcore::Value(cluster_name);
    } else {
      (*r)["targetCluster"] = shcore::Value(*router_md.target_cluster);
    }

    router_list->set(label, shcore::Value(r));
  }

  if (!router.empty() && router_list->empty())
    throw shcore::Exception::argument_error(
        "Router '" + router + "' is not registered in the ClusterSet");

  return shcore::Value(router_list);
}

shcore::Dictionary_t router_options(MetadataStorage *md,
                                    const std::string &clusterset_id,
                                    const std::string &router_id) {
  auto router_options = shcore::make_dict();
  auto romds = md->get_routing_options(clusterset_id);

  Router_options_metadata global_defaults;
  for (const auto &entry : romds) {
    if (entry.router_id.is_null()) {
      global_defaults = entry;
      break;
    }
  }

  const auto get_options_dict =
      [global_defaults](const Router_options_metadata &entry) {
        auto ret = shcore::make_dict();
        for (const auto &option : entry.defined_options)
          (*ret)[option.first] = option.second;

        for (const auto &option : global_defaults.defined_options) {
          if (!(*ret).has_key(option.first))
            (*ret)[option.first] = option.second;
        }

        for (const auto &option : k_default_router_options.defined_options) {
          if (!(*ret).has_key(option.first))
            (*ret)[option.first] = option.second;
        }
        return shcore::Value(ret);
      };

  if (!router_id.empty()) {
    for (const auto &entry : romds) {
      if (entry.router_id == router_id) {
        (*router_options)[*entry.router_id] = get_options_dict(entry);
      }
    }

    if (router_options->empty()) {
      throw shcore::Exception::argument_error(
          "Router '" + router_id + "' is not registered in the ClusterSet");
    }
  } else {
    auto routers = shcore::make_dict();
    for (const auto &entry : romds) {
      if (entry.router_id.is_null()) {
        (*router_options)["global"] = get_options_dict(entry);
      } else {
        (*routers)[*entry.router_id] = get_options_dict(entry);
      }
    }
    (*router_options)["routers"] = shcore::Value(routers);
  }

  return router_options;
}

void validate_router_option(const Cluster_set_impl &cluster_set,
                            std::string name, const shcore::Value &value) {
  if (std::find(k_router_options.begin(), k_router_options.end(), name) ==
      k_router_options.end())
    throw shcore::Exception::argument_error(
        "Unsupported routing option, '" + name +
        "' supported options: " + shcore::str_join(k_router_options, ", "));

  if (value.get_type() != shcore::Value_type::Null) {
    if (name == k_router_option_invalidated_cluster_routing_policy) {
      if (value.get_type() != shcore::Value_type::String ||
          (value.get_string() !=
               k_router_option_invalidated_cluster_routing_policy_accept_ro &&
           value.get_string() !=
               k_router_option_invalidated_cluster_routing_policy_drop_all))
        throw shcore::Exception::argument_error(
            std::string("Invalid value for routing option '") +
            k_router_option_invalidated_cluster_routing_policy +
            "', accepted values: '" +
            k_router_option_invalidated_cluster_routing_policy_accept_ro +
            "', '" +
            k_router_option_invalidated_cluster_routing_policy_drop_all + "'");
    } else if (name == k_router_option_target_cluster) {
      const auto is_cluster_name = [&cluster_set](const std::string &cname) {
        auto clusters = cluster_set.get_clusters();
        return std::find_if(clusters.begin(), clusters.end(),
                            [&](const Cluster_metadata &c) {
                              return c.cluster_name == cname;
                            }) != clusters.end();
      };

      if (value.get_type() != shcore::Value_type::String ||
          (value.get_string() != k_router_option_target_cluster_primary &&
           !is_cluster_name(value.get_string())))
        throw shcore::Exception::argument_error(
            std::string("Invalid value for routing option '") +
            k_router_option_target_cluster +
            "', accepted values 'primary' or valid cluster name");
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
