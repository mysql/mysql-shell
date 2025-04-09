/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/router.h"

#include <map>
#include <utility>
#include <vector>

#include "adminapi/common/base_cluster_impl.h"
#include "adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/router_options.h"
#include "scripting/types.h"
#include "utils/utils_string.h"
#include "utils/version.h"

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
      shcore::Value(k_default_router_option_read_only_targets)},
     {k_router_option_routing_guideline, shcore::Value::Null()}};

const std::map<std::string, shcore::Value> k_default_cluster_router_options = {
    {k_router_option_tags, shcore::Value(shcore::make_dict())},
    {k_router_option_read_only_targets,
     shcore::Value(k_default_router_option_read_only_targets)},
    {k_router_option_stats_updates_frequency, shcore::Value::Null()},
    {k_router_option_unreachable_quorum_allowed_traffic, shcore::Value::Null()},
    {k_router_option_routing_guideline, shcore::Value::Null()}};

const std::map<std::string, shcore::Value> k_default_replicaset_router_options =
    {{k_router_option_tags, shcore::Value(shcore::make_dict())},
     {k_router_option_stats_updates_frequency, shcore::Value::Null()},
     {k_router_option_routing_guideline, shcore::Value::Null()}};

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

  if (!router_md.rw_split_port.has_value())
    (*router)["rwSplitPort"] = shcore::Value::Null();
  else
    (*router)["rwSplitPort"] = shcore::Value(*router_md.rw_split_port);

  if (!router_md.local_cluster.has_value())
    (*router)["localCluster"] = shcore::Value::Null();
  else
    (*router)["localCluster"] = shcore::Value(*router_md.local_cluster);

  if (!router_md.current_routing_guideline.has_value())
    (*router)["currentRoutingGuideline"] = shcore::Value::Null();
  else
    (*router)["currentRoutingGuideline"] =
        shcore::Value(*router_md.current_routing_guideline);

  if (!router_md.supported_routing_guidelines_version.has_value())
    (*router)["supportedRoutingGuidelinesVersion"] = shcore::Value::Null();
  else
    (*router)["supportedRoutingGuidelinesVersion"] =
        shcore::Value(*router_md.supported_routing_guidelines_version);

  return router;
}

namespace {
void check_router_errors(shcore::Array_t issues,
                         const Router_metadata &router_md, MetadataStorage *md,
                         const Cluster_id &cluster_id,
                         Cluster_type cluster_type) {
  // Check if this Router's version is >= 8.4.0. If so, confirm that the
  // global dynamic options exist in the Metadata. If not, it means that
  // either the Router needs to be re-bootstrapped (because was upgraded) or
  // the Router accounts need to be upgraded.
  if (router_md.version.has_value() &&
      mysqlshdk::utils::Version(router_md.version.value()) >=
          mysqlshdk::utils::Version(8, 4, 0)) {
    auto highest_router_configuration_schema_version =
        md->get_highest_router_configuration_schema_version(cluster_type,
                                                            cluster_id);

    auto highest_router_version =
        md->get_highest_bootstrapped_router_version(cluster_type, cluster_id);

    if (highest_router_configuration_schema_version < highest_router_version) {
      issues->push_back(shcore::Value(
          "WARNING: Unable to fetch all configuration options: Please "
          "ensure the Router account has the right privileges using "
          "<Cluster>.setupRouterAccount() and re-bootstrap Router."));
    }
  }
}
}  // namespace

shcore::Value router_list(MetadataStorage *md, const Cluster_id &cluster_id,
                          bool only_upgrade_required) {
  auto router_list = shcore::make_dict();
  auto routers_md = md->get_routers(cluster_id);

  for (const auto &router_md : routers_md) {
    shcore::Array_t router_errors = shcore::make_array();

    auto router = get_router_dict(router_md, only_upgrade_required);
    if (router->empty()) continue;

    // Check for router errors
    check_router_errors(router_errors, router_md, md, cluster_id,
                        Cluster_type::GROUP_REPLICATION);

    if (router_errors && !router_errors->empty()) {
      router->emplace("routerErrors", shcore::Value(std::move(router_errors)));
    }

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

    // Check for router errors
    check_router_errors(router_errors, router_md, md, clusterset_id,
                        Cluster_type::REPLICATED_CLUSTER);

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
                                    const std::string &name,
                                    const Router_options_options &options) {
  auto router_options = shcore::make_dict();

  switch (type) {
    case Cluster_type::GROUP_REPLICATION:
      (*router_options)["clusterName"] = shcore::Value(name);
      break;
    case Cluster_type::ASYNC_REPLICATION:
      (*router_options)["name"] = shcore::Value(name);
      break;
    case Cluster_type::REPLICATED_CLUSTER:
      (*router_options)["domainName"] = shcore::Value(name);
      break;
    case Cluster_type::NONE:
      throw std::logic_error("internal error");
  }

  auto highest_router_version =
      md->get_highest_bootstrapped_router_version(type, id);

  // Get and parse the Configuration changes schema
  auto json_configuration_schema_parsed = Router_configuration_changes_schema(
      md->get_router_configuration_changes_schema(type, id,
                                                  highest_router_version));

  // Get and parse the default global options
  auto default_global_options =
      get_default_router_options(md, type, id, json_configuration_schema_parsed,
                                 options.extended, highest_router_version);

  auto default_global_options_parsed =
      Router_configuration_schema(default_global_options);

  // Get all routers options
  auto all_router_options =
      get_router_options(md, type, id, json_configuration_schema_parsed,
                         default_global_options_parsed, options.extended,
                         options.router.value_or(""));

  // Set the Default router configuration
  (*router_options)["configuration"] = default_global_options;

  // If the 'router' option was used return right away, nothing else is included
  // in the output
  if (options.router.has_value()) {
    (*router_options)["routers"] = all_router_options;

    return router_options;
  }

  // Get all routers options
  (*router_options)["routers"] = all_router_options;

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
          if (e.code() != SHERR_DBA_MISSING_FROM_METADATA) throw;
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
  } else if (name == k_router_option_unreachable_quorum_allowed_traffic) {
    if (value.get_type() != shcore::Value_type::String) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for routing option '%s', value is "
          "expected to be either 'read', 'all' or 'none'.",
          k_router_option_unreachable_quorum_allowed_traffic));
    }

    const auto &value_str = value.get_string();
    if (!shcore::str_caseeq(value_str, "read") &&
        !shcore::str_caseeq(value_str, "all") &&
        !shcore::str_caseeq(value_str, "none")) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for routing option '%s', value is "
          "expected to be either 'read', 'all' or 'none'.",
          k_router_option_unreachable_quorum_allowed_traffic));
    }

    if (!shcore::str_caseeq(value_str, "none")) {
      mysqlsh::current_console()->print_warning(shcore::str_format(
          "Setting the '%s' option to '%s' may have unwanted consequences: the "
          "consistency guarantees provided by InnoDB Cluster are broken since "
          "the data read can be stale; different Routers may be accessing "
          "different partitions, thus return different data; and different "
          "Routers may also have different behavior (i.e. some provide only "
          "read traffic while others read and write traffic). Note that "
          "writes on a partition with no quorum will block until quorum is "
          "restored.",
          k_router_option_unreachable_quorum_allowed_traffic,
          value_str.c_str()));

      mysqlsh::current_console()->print_warning(
          "This option has no practical effect if the server variable "
          "group_replication_unreachable_majority_timeout is set to a positive "
          "number and group_replication_exit_state_action is set to either "
          "OFFLINE_MODE or ABORT_SERVER.");
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
  } else if (name == k_router_option_routing_guideline) {
    if (value.get_type() == shcore::Value_type::String) {
      // Translate the name into the routing guideline ID. If not found, it
      // errors out
      try {
        Routing_guideline_metadata rg =
            cluster.get_metadata_storage()->get_routing_guideline(
                cluster.get_type(), cluster.get_id(), value.get_string());
        fixed_value = shcore::Value(rg.id);
      } catch (const shcore::Exception &) {
        throw;
      }
    } else {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid value for routing option '%s'. Accepted "
                             "value must be valid Routing Guideline name.",
                             k_router_option_routing_guideline));
    }
  }
  return fixed_value;
}

shcore::Value get_default_router_options(
    MetadataStorage *md, Cluster_type type, const std::string &id,
    const Router_configuration_changes_schema &changes_schema,
    uint64_t extended, const mysqlshdk::utils::Version &version) {
  auto default_global_options =
      md->get_default_router_options(type, id, version);

  // If the changes schema is empty, because it's not yet in the Metadata
  // (Router needs a re-bootstrap), then return right away the default options
  // and don't do any filtering.
  if (!changes_schema) {
    return shcore::Value(default_global_options);
  }

  // By default (extended=0), filter out everything that does not belong to the
  // configuration changes schema
  if (extended == 0) {
    auto default_global_options_parsed =
        Router_configuration_schema(default_global_options);

    auto filtered_options =
        default_global_options_parsed.filter_schema_by_changes(changes_schema);

    return shcore::Value(filtered_options.to_value());
  } else if (extended == 1) {
    // With extended:1 we must filter out everything that is the same as in the
    // "common" section
    auto default_global_options_parsed =
        Router_configuration_schema(default_global_options);

    auto filtered_options =
        default_global_options_parsed.filter_common_router_options();

    return shcore::Value(filtered_options.to_value());
  }

  // extended > 1
  return shcore::Value(default_global_options);
}

shcore::Value get_router_options(
    MetadataStorage *md, Cluster_type type, const std::string &id,
    const Router_configuration_changes_schema &changes_schema,
    const Router_configuration_schema &global_dynamic_options,
    uint64_t extended, const std::string &router_name) {
  auto router_options_md = md->get_router_options(type, id, router_name);

  // If the 'router' option was used, check if the router exists
  if (!router_name.empty() && !router_options_md) {
    throw shcore::Exception::argument_error(
        "Router '" + router_name + "' is not registered in the " +
        to_display_string(type, Display_form::THING));
  }

  auto ret = shcore::make_dict();

  if (!router_options_md) return shcore::Value(ret);

  auto configuration_map = router_options_md.as_map();

  auto highest_router_version =
      md->get_highest_bootstrapped_router_version(type, id);

  auto highest_router_configuration_schema_version =
      md->get_highest_router_configuration_schema_version(type, id);

  const auto warning_checker = [&](const std::string &router_label) {
    shcore::Array_t warnings = shcore::make_array();

    // Check if this Router's version is >= 8.4.0. If so, confirm that the
    // global dynamic options exist in the Metadata. If not, it means that
    // either the Router needs to be re-bootstrapped (because was upgraded) or
    // the Router accounts need to be upgraded.
    if (md->get_router_version(type, id, router_label) >=
        mysqlshdk::utils::Version(8, 4, 0)) {
      if (highest_router_configuration_schema_version <
          highest_router_version) {
        warnings->push_back(shcore::Value(
            "WARNING: Unable to fetch all configuration options: Please ensure "
            "the Router account has the right privileges using "
            "<Cluster>.setupRouterAccount() and re-bootstrap Router."));
      }
    }

    return warnings;
  };

  if (extended > 1) {
    for (const auto &[name, options] : *configuration_map) {
      auto new_configuration_map = shcore::make_dict();

      new_configuration_map->emplace("configuration", options);

      if (auto router_errors = warning_checker(name);
          router_errors && !router_errors->empty()) {
        new_configuration_map->emplace("routerErrors",
                                       shcore::Value(std::move(router_errors)));
      }

      ret->emplace(name, std::move(new_configuration_map));
    }

    return shcore::Value(ret);
  }

  for (const auto &[name, options] : *configuration_map) {
    auto filtered_schema = shcore::make_dict();

    if (auto router_errors = warning_checker(name);
        router_errors && !router_errors->empty()) {
      filtered_schema->emplace("routerErrors",
                               shcore::Value(std::move(router_errors)));
    }

    auto configuration_map_parsed = Router_configuration_schema(options);

    if (extended == 0) {
      auto filtered_options =
          configuration_map_parsed.filter_schema_by_changes(changes_schema);

      auto diff_schema =
          filtered_options.diff_configuration_schema(global_dynamic_options);

      auto schema = diff_schema.to_value();

      filtered_schema->emplace("configuration", schema);

      ret->emplace(name, std::move(filtered_schema));
    } else {
      // In extended:1 we must first filter out the common router options from
      // the defaults
      auto global_router_options_filtered_schema =
          configuration_map_parsed.filter_common_router_options();

      // Then get filter out the differences with the global dynamic options
      auto diff_schema =
          global_router_options_filtered_schema.diff_configuration_schema(
              global_dynamic_options);

      auto schema = diff_schema.to_value();

      filtered_schema->emplace("configuration", schema);

      ret->emplace(name, std::move(filtered_schema));
    }
  }

  return shcore::Value(ret);
}

}  // namespace dba
}  // namespace mysqlsh
