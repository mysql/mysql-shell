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

#include "modules/adminapi/cluster/rescan.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "utils/version.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

constexpr const char *k_xplugin_disabled = "<disabled>";

Rescan::Rescan(const Rescan_options &options, Cluster_impl *cluster)
    : m_options(options), m_cluster(cluster) {
  assert(cluster);
  assert((m_options.auto_add && m_options.add_instances_list.empty()) ||
         !m_options.auto_add);
  assert((m_options.auto_remove && m_options.remove_instances_list.empty()) ||
         !m_options.auto_remove);
}

void Rescan::validate_list_duplicates() const {
  std::set<std::string> add_instances_set, remove_instances_set;

  // Lambda function to check for instance duplicates in list, creating a set
  // with the instance address to compare later.
  auto duplicates_check =
      [](const std::string &option_name,
         const std::vector<mysqlshdk::db::Connection_options> &in_list,
         std::set<std::string> *out_set) {
        for (const auto &cnx_opt : in_list) {
          std::string instance_address =
              cnx_opt.as_uri(mysqlshdk::db::uri::formats::only_transport());
          auto res = out_set->insert(instance_address);
          if (!res.second) {
            throw shcore::Exception::argument_error(
                "Duplicated value found for instance '" + instance_address +
                "' in '" + option_name + "' option.");
          }
        }
      };

  // Check for duplicates in the addInstances list.
  duplicates_check("addInstances", m_options.add_instances_list,
                   &add_instances_set);

  // Check for duplicates in the removeInstances list.
  duplicates_check("removeInstances", m_options.remove_instances_list,
                   &remove_instances_set);

  // Find common instance in the two options.
  std::vector<std::string> repeated_instances;
  std::set_intersection(add_instances_set.begin(), add_instances_set.end(),
                        remove_instances_set.begin(),
                        remove_instances_set.end(),
                        std::back_inserter(repeated_instances));

  // The same instance cannot be included in both options.
  if (!repeated_instances.empty()) {
    std::string plural = (repeated_instances.size() > 1) ? "s" : "";
    throw shcore::Exception::argument_error(
        "The same instance" + plural +
        " cannot be used in both 'addInstances' and 'removeInstances' options: "
        "'" +
        shcore::str_join(repeated_instances, ", ") + "'.");
  }
}

void Rescan::ensure_unavailable_instances_not_auto_rejoining(
    std::vector<MissingInstanceInfo> &unavailable_instances) const {
  mysqlshdk::db::Connection_options group_conn_opt =
      m_cluster->get_cluster_server()->get_connection_options();

  auto console = mysqlsh::current_console();
  auto it = unavailable_instances.begin();
  while (it != unavailable_instances.end()) {
    auto instance_conn_opt = mysqlshdk::db::Connection_options(it->endpoint);
    instance_conn_opt.set_login_options_from(group_conn_opt);
    bool is_rejoining = false;
    try {
      auto instance = Instance::connect_raw(instance_conn_opt);
      is_rejoining = mysqlshdk::gr::is_running_gr_auto_rejoin(*instance);
    } catch (const std::exception &e) {
      // if you cant connect to the instance then we assume it really is offline
      // or unreachable and it is not auto-rejoining
    }

    if (is_rejoining) {
      console->print_warning(
          "The instance '" + instance_conn_opt.uri_endpoint() +
          "' is MISSING but currently trying to auto-rejoin.");
      it = unavailable_instances.erase(it);
    } else {
      ++it;
    }
  }
}

std::vector<std::string> Rescan::detect_invalid_members(
    const std::vector<mysqlshdk::db::Connection_options> &instances_list,
    bool is_active) {
  std::vector<std::string> invalid_instances;

  for (const auto &cnx_opt : instances_list) {
    std::string instance_address =
        cnx_opt.as_uri(mysqlshdk::db::uri::formats::only_transport());

    const char *check_type = (is_active) ? "" : "not ";
    log_debug(
        "Checking if the instance '%s' is %san active member of the group.",
        instance_address.c_str(), check_type);

    std::shared_ptr<Instance> cluster_instance =
        m_cluster->get_cluster_server();

    if (mysqlshdk::gr::is_active_member(*cluster_instance, cnx_opt.get_host(),
                                        cnx_opt.get_port()) != is_active) {
      invalid_instances.push_back(instance_address);
    }
  }

  return invalid_instances;
}

void Rescan::prepare() {
  // Validate duplicates in addInstances and removeInstances options.
  validate_list_duplicates();

  auto console = mysqlsh::current_console();

  // All instance to add must be an active member of the GR group.
  {
    std::vector<std::string> invalid_add_members =
        detect_invalid_members(m_options.add_instances_list, true);

    if (!invalid_add_members.empty()) {
      console->print_error(
          "The following instances cannot be added because they are not active "
          "members of the cluster: '" +
          shcore::str_join(invalid_add_members, ", ") +
          "'. Please verify if the specified addresses are correct, or if the "
          "instances are currently inactive.");
      console->print_info();

      throw shcore::Exception::runtime_error(
          "The following instances are not active members of the cluster: '" +
          shcore::str_join(invalid_add_members, ", ") + "'.");
    }
  }

  // can't add instances if recovery auth isn't password
  if (((!m_options.add_instances_list.empty() || m_options.auto_add) &&
       (m_cluster->query_cluster_auth_type() !=
        Replication_auth_type::PASSWORD))) {
    console->print_error(
        "Unrecognized members were detected in the group, but the cluster is "
        "configured to require SSL certificate authentication. Please stop GR "
        "on those members and then add them to the cluster using "
        "cluster.<<<addInstance>>>() with the appropriate authentication "
        "options.");
    console->print_info();

    throw shcore::Exception::runtime_error(shcore::str_format(
        "Can't automatically add unrecognized members to the cluster when "
        "memberAuthType is '%s'.",
        to_string(m_cluster->query_cluster_auth_type()).c_str()));
  }

  // All instance to remove cannot be an active member of the GR group.
  std::vector<std::string> invalid_remove_members =
      detect_invalid_members(m_options.remove_instances_list, false);

  if (!invalid_remove_members.empty()) {
    console->print_error(
        "The following instances cannot be removed because they are active "
        "members of the cluster: '" +
        shcore::str_join(invalid_remove_members, ", ") +
        "'. Please verify if the specified addresses are correct.");
    console->print_info();

    throw shcore::Exception::runtime_error(
        "The following instances are active members of the cluster: '" +
        shcore::str_join(invalid_remove_members, ", ") + "'.");
  }

  // Check if group_replication_view_change_uuid is supported
  m_is_view_change_uuid_supported = m_cluster->get_lowest_instance_version() >=
                                    Precondition_checker::k_min_cs_version;

  // Validate the usage of the option 'updateViewChangeUuid'
  if (m_options.update_view_change_uuid.value_or(false) &&
      !m_is_view_change_uuid_supported) {
    throw shcore::Exception::argument_error(
        "The Cluster cannot be configured to use "
        "group_replication_view_change_uuid: unsupported version.");
  }
}

void Rescan::check_mismatched_hostnames_addresses(
    shcore::Array_t instances) const {
  m_cluster->execute_in_members(
      [&instances](const std::shared_ptr<Instance> &instance,
                   const Cluster_impl::Instance_md_and_gr_member &info) {
        // skip members that are already in the update list
        for (const auto &i : *instances) {
          if (i.as_map()->get_string("member_id") == instance->get_uuid())
            return true;
        }

        bool grendpoint_valid = true;
        try {
          mysqlshdk::utils::split_host_and_port(info.first.grendpoint);
        } catch (const std::invalid_argument &e) {
          log_debug("Unable to parse endpoint: %s",
                    info.first.grendpoint.c_str());
          grendpoint_valid = false;
        }

        auto address = instance->get_canonical_address();

        std::optional<int> xport = instance->get_xport();

        auto res = instance->query(
            "select @@hostname, @@report_host, @@port, @@report_port");
        auto row = res->fetch_one();

        log_debug(
            "Hostname metadata check for %s: address=%s @@hostname=%s "
            "@@report_host=%s @@port=%s @@report_port=%s @@mysqlx_port=%s "
            " md.instance_id=%u md.endpoint=%s md.address=%s md.label=%s "
            "md.xendpoint=%s md.grendpoint=%s member_id=%s member_host=%s "
            "member_port=%i",
            instance->get_uuid().c_str(), address.c_str(),
            row->get_as_string(0).c_str(), row->get_as_string(1).c_str(),
            row->get_as_string(2).c_str(), row->get_as_string(3).c_str(),
            xport.has_value() ? std::to_string(*xport).c_str() : "null",
            info.first.id, info.first.endpoint.c_str(),
            info.first.address.c_str(), info.first.label.c_str(),
            info.first.xendpoint.c_str(), info.first.grendpoint.c_str(),
            info.second.uuid.c_str(), info.second.host.c_str(),
            info.second.port);

        bool xport_equal = false;
        bool address_equal = false;
        std::string x_address;

        auto canonical_hostname = instance->get_canonical_hostname();

        if (xport.has_value()) {
          x_address =
              mysqlshdk::utils::make_host_and_port(canonical_hostname, *xport);
        }

        if (mysqlshdk::utils::are_endpoints_equal(info.first.xendpoint,
                                                  x_address)) {
          xport_equal = true;
        }

        if (grendpoint_valid &&
            mysqlshdk::utils::are_endpoints_equal(info.first.endpoint,
                                                  address) &&
            mysqlshdk::utils::are_endpoints_equal(info.first.address,
                                                  address)) {
          address_equal = true;
        }

        if (xport_equal && address_equal) return true;

        shcore::Dictionary_t dict = shcore::make_dict();

        dict->set("member_id", shcore::Value(instance->get_uuid()));
        dict->set("id", shcore::Value(info.first.id));
        dict->set("label", shcore::Value(info.first.label));
        dict->set("host", shcore::Value(instance->get_canonical_address()));
        dict->set(
            "xport_endpoint",
            shcore::Value(x_address.empty() ? k_xplugin_disabled : x_address));

        instances->emplace_back(shcore::Value(dict));

        if (!address_equal) {
          dict->set("old_host", shcore::Value(info.first.endpoint));
        }

        if (!xport_equal) {
          dict->set("old_xport_endpoint", shcore::Value(info.first.xendpoint));
        }

        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });
}

shcore::Value::Map_type_ref Rescan::get_rescan_report() const {
  auto cluster_map = std::make_shared<shcore::Value::Map_type>();

  // Set the Cluster name on the result map
  (*cluster_map)["name"] = shcore::Value(m_cluster->get_name());

  std::vector<NewInstanceInfo> newly_discovered_instances_list =
      get_newly_discovered_instances(*m_cluster->get_cluster_server(),
                                     m_cluster->get_metadata_storage(),
                                     m_cluster->get_id());

  // Creates the newlyDiscoveredInstances map
  auto newly_discovered_instances =
      std::make_shared<shcore::Value::Array_type>();

  for (auto &instance : newly_discovered_instances_list) {
    auto newly_discovered_instance = shcore::make_dict();
    (*newly_discovered_instance)["member_id"] =
        shcore::Value(instance.member_id);
    (*newly_discovered_instance)["name"] = shcore::Value::Null();

    std::string instance_address =
        instance.host + ":" + std::to_string(instance.port);

    (*newly_discovered_instance)["host"] = shcore::Value(instance_address);

    if (!instance.version.empty()) {
      (*newly_discovered_instance)["version"] = shcore::Value(instance.version);
    }

    newly_discovered_instances->push_back(
        shcore::Value(newly_discovered_instance));
  }

  std::sort(newly_discovered_instances->begin(),
            newly_discovered_instances->end(),
            [](const shcore::Value &a, const shcore::Value &b) {
              return a.as_map()->get_string("member_id") <
                     b.as_map()->get_string("member_id");
            });

  // Add the newly_discovered_instances list to the result Map
  (*cluster_map)["newlyDiscoveredInstances"] =
      shcore::Value(newly_discovered_instances);

  shcore::Value unavailable_instances_result;

  std::vector<MissingInstanceInfo> unavailable_instances_list =
      get_unavailable_instances(*m_cluster->get_cluster_server(),
                                m_cluster->get_metadata_storage(),
                                m_cluster->get_id());

  // Registers instances that are both new/unavailable but have the same
  // host:port as updated instances.
  auto updated_instances = std::make_shared<shcore::Value::Array_type>();

  // Creates the unavailableInstances array
  auto unavailable_instances = std::make_shared<shcore::Value::Array_type>();
  ensure_unavailable_instances_not_auto_rejoining(unavailable_instances_list);
  for (auto &instance : unavailable_instances_list) {
    auto unavailable_instance = std::make_shared<shcore::Value::Map_type>();
    auto endpoint = instance.endpoint;
    (*unavailable_instance)["member_id"] = shcore::Value(instance.id);
    (*unavailable_instance)["label"] = shcore::Value(instance.label);
    (*unavailable_instance)["host"] = shcore::Value(endpoint);

    // Instance is both new and unavailable, means its UUID changed
    auto new_instance = std::find_if(
        newly_discovered_instances->begin(), newly_discovered_instances->end(),
        [endpoint](const shcore::Value &existing) {
          return endpoint == existing.as_map()->at("host").as_string();
        });

    if (new_instance != newly_discovered_instances->end()) {
      (*unavailable_instance)["old_member_id"] =
          (*unavailable_instance)["member_id"];
      (*unavailable_instance)["member_id"] =
          new_instance->as_map()->at("member_id");
      updated_instances->push_back(shcore::Value(unavailable_instance));
      newly_discovered_instances->erase(new_instance);
    } else {
      unavailable_instances->push_back(shcore::Value(unavailable_instance));
    }
  }

  check_mismatched_hostnames_addresses(updated_instances);

  // Add the missing_instances list to the result Map
  (*cluster_map)["unavailableInstances"] = shcore::Value(unavailable_instances);

  // Add the missing_instances list to the result Map
  (*cluster_map)["updatedInstances"] = shcore::Value(updated_instances);

  // Get the topology mode from the metadata.
  mysqlshdk::gr::Topology_mode metadata_topology_mode =
      m_cluster->get_metadata_storage()->get_cluster_topology_mode(
          m_cluster->get_id());

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  bool single_primary;
  mysqlshdk::gr::get_group_primary_uuid(*m_cluster->get_cluster_server(),
                                        &single_primary);

  // Check if the topology mode match and report needed change in the metadata.
  if (!single_primary &&
      metadata_topology_mode == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) {
    // Topology mode need to be changed to multi-primary on the metadata.
    (*cluster_map)["newTopologyMode"] = shcore::Value(
        mysqlshdk::gr::to_string(mysqlshdk::gr::Topology_mode::MULTI_PRIMARY));
  } else if (single_primary &&
             metadata_topology_mode ==
                 mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    // Topology mode need to be changed to single-primary on the metadata.
    (*cluster_map)["newTopologyMode"] = shcore::Value(
        mysqlshdk::gr::to_string(mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY));
  } else {
    // Topology mode is the same (no change needed).
    (*cluster_map)["newTopologyMode"] = shcore::Value::Null();
  }

  return cluster_map;
}

void Rescan::update_topology_mode(
    const mysqlshdk::gr::Topology_mode &topology_mode) {
  auto console = mysqlsh::current_console();
  console->print_info("Updating topology mode in the cluster metadata...");

  // Update auto-increment settings.

  // Get the Cluster Config Object
  auto cfg = m_cluster->create_config_object();

  // Call update_auto_increment to do the job in all instances
  mysqlshdk::gr::update_auto_increment(cfg.get(), topology_mode);

  cfg->apply();

  // Update topology mode in metadata.
  m_cluster->get_metadata_storage()->update_cluster_topology_mode(
      m_cluster->get_id(), topology_mode);

  // Update the topology mode on the Cluster object.
  m_cluster->set_topology_type(topology_mode);

  console->print_info("Topology mode was successfully updated to '" +
                      mysqlshdk::gr::to_string(topology_mode) +
                      "' in the cluster metadata.");
  console->print_info();
}

void Rescan::add_instance_to_metadata(
    const mysqlshdk::db::Connection_options &instance_cnx_opts) {
  auto console = mysqlsh::current_console();
  console->print_info("Adding instance to the cluster metadata...");

  // Create a copy of the connection options to set the cluster user
  // credentials if needed (should not change the value passed as parameter).
  mysqlshdk::db::Connection_options cnx_opts = instance_cnx_opts;
  if (!cnx_opts.has_user()) {
    // It is assumed that the same login options of the cluster can be
    // used to connect to the instance, if no login (user) was provided.
    cnx_opts.set_login_options_from(
        m_cluster->get_cluster_server()->get_connection_options());
  }

  m_cluster->add_metadata_for_instance(cnx_opts, Instance_type::GROUP_MEMBER);

  console->print_info(
      "The instance '" +
      instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "' was successfully added to the cluster metadata.");
  console->print_info();
}

void Rescan::update_metadata_for_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    Instance_id instance_id, const std::string &label) {
  // Create a copy of the connection options to set the cluster user
  // credentials if needed (should not change the value passed as parameter).
  mysqlshdk::db::Connection_options cnx_opts = instance_cnx_opts;
  if (!cnx_opts.has_user()) {
    // It is assumed that the same login options of the cluster can be
    // used to connect to the instance, if no login (user) was provided.
    cnx_opts.set_login_options_from(
        m_cluster->get_cluster_server()->get_connection_options());
  }

  m_cluster->update_metadata_for_instance(cnx_opts, instance_id, label);
}

void Rescan::remove_instance_from_metadata(
    const std::string &instance_address) {
  mysqlshdk::db::Connection_options instance_cnx_opts =
      shcore::get_connection_options(instance_address, false);

  auto console = mysqlsh::current_console();
  console->print_info("Removing instance from the cluster metadata...");

  m_cluster->remove_instance_metadata(instance_cnx_opts);

  console->print_info(
      "The instance '" +
      instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "' was successfully removed from the cluster metadata.");
  console->print_info();
}

namespace {
using Instance_list = std::vector<mysqlshdk::db::Connection_options>;
Instance_list::const_iterator find_in_instance_list(
    const Instance_list &list, std::string_view instance_address) {
  if (list.empty()) return list.end();

  // Lambda function (predicate) to check if the instance address
  // matches.
  auto in_list =
      [&instance_address](const mysqlshdk::db::Connection_options &cnx_opt) {
        std::string address =
            cnx_opt.as_uri(mysqlshdk::db::uri::formats::only_transport());
        return mysqlshdk::utils::are_endpoints_equal(address, instance_address);
      };

  // Return iterator with a matching address (if in the list).
  return std::find_if(list.begin(), list.end(), in_list);
}
}  // namespace

void Rescan::add_metadata_for_instances(
    const std::shared_ptr<shcore::Value::Array_type> &instance_list) {
  if (instance_list->empty()) return;

  auto console = mysqlsh::current_console();

  if (m_cluster->query_cluster_auth_type() != Replication_auth_type::PASSWORD) {
    console->print_info(
        "New instances were discovered in the cluster but ignore because the "
        "cluster requires SSL certificate authentication.\nPlease stop GR on "
        "those members and then add them to the cluster using "
        "cluster.addInstance() with the appropriate authentication options.");
    return;
  }

  for (const auto &instance : *instance_list) {
    auto instance_map = instance.as_map();

    std::string instance_address = instance_map->get_string("host");
    // Report that a new instance was discovered (not in the metadata).
    console->print_info("A new instance '" + instance_address +
                        "' was discovered in the cluster.");

    // Check if the new instance belongs to the addInstances list.
    auto list_iter =
        find_in_instance_list(m_options.add_instances_list, instance_address);

    // Decide to add/remove instance based on given operation options.
    mysqlshdk::db::Connection_options instance_cnx_opts;
    if (list_iter != m_options.add_instances_list.end()) {
      instance_cnx_opts = *list_iter;
      m_options.add_instances_list.erase(list_iter);
    } else if (m_options.auto_add) {
      instance_cnx_opts =
          shcore::get_connection_options(instance_address, false);
    } else if (m_options.interactive()) {
      if (console->confirm("Would you like to add it to the cluster metadata?",
                           Prompt_answer::YES) == Prompt_answer::YES) {
        instance_cnx_opts =
            shcore::get_connection_options(instance_address, false);
      }
    }

    if (instance_cnx_opts.has_data()) {
      add_instance_to_metadata(instance_cnx_opts);
    }
  }
}

void Rescan::remove_metadata_for_instances(
    const std::shared_ptr<shcore::Value::Array_type> &instance_list) {
  auto console = mysqlsh::current_console();

  for (const auto &instance : *instance_list) {
    auto instance_map = instance.as_map();

    std::string instance_address = instance_map->get_string("host");
    // Report that an obsolete instance was found (in the metadata).
    console->print_info("The instance '" + instance_address +
                        "' is no longer part of the cluster.");

    // Check if the new instance belongs to the addInstances list.
    // Decide to add/remove instance based on given operation options.
    auto list_iter = find_in_instance_list(m_options.remove_instances_list,
                                           instance_address);
    if (list_iter != m_options.remove_instances_list.end()) {
      remove_instance_from_metadata(instance_address);

      m_options.remove_instances_list.erase(list_iter);
    } else if (m_options.auto_remove) {
      // Remove instance automatically if "auto" was used.
      remove_instance_from_metadata(instance_address);
    } else if (m_options.interactive()) {
      console->print_info(
          "The instance is either offline or left the HA group. You can try "
          "to add it to the cluster again with the cluster.rejoinInstance('" +
          instance_map->get_string("host") +
          "') command or you can remove it from the cluster configuration.");

      // Ask to remove instance if prompts are enabled.
      if (console->confirm(
              "Would you like to remove it from the cluster metadata?",
              Prompt_answer::YES) == Prompt_answer::YES) {
        remove_instance_from_metadata(instance_address);
      }
    }
  }
}

void Rescan::update_metadata_for_instances(
    const std::shared_ptr<shcore::Value::Array_type> &instance_list) {
  auto console = mysqlsh::current_console();

  for (const auto &instance : *instance_list) {
    auto instance_map = instance.as_map();
    std::string label;
    std::string xport_endpoint_str;

    std::string instance_address = instance_map->get_string("host");
    // Report that an obsolete instance was found (in the metadata).
    if (instance_map->has_key("old_member_id")) {
      // member UUID changed
      console->print_info(shcore::str_format(
          "The instance '%s' is part of the cluster but its UUID has changed. "
          "Old UUID: %s. Current UUID: %s.",
          instance_address.c_str(),
          instance_map->get_string("old_member_id").c_str(),
          instance_map->get_string("member_id").c_str()));
    } else {
      // member address changed
      if (instance_map->has_key("old_host")) {
        console->print_info(shcore::str_format(
            "The instance '%s' is part of the cluster but its "
            "reported address has changed. "
            "Old address: %s. Current address: %s.",
            instance_address.c_str(),
            instance_map->get_string("old_host").c_str(),
            instance_map->get_string("host").c_str()));
      }

      // update instance label if it's the default value
      if (instance_map->get_string("label") ==
          instance_map->get_string("old_host")) {
        label = instance_map->get_string("host");
      }

      // xport has changed
      if (instance_map->has_key("old_xport_endpoint")) {
        xport_endpoint_str = instance_map->get_string("xport_endpoint");

        console->print_info(shcore::str_format(
            "The instance '%s' is part of the Cluster but the reported X "
            "plugin port has changed. "
            "Old xport: %s. Current xport: %s.",
            instance_address.c_str(),
            instance_map->get_string("old_xport_endpoint").c_str(),
            xport_endpoint_str.c_str()));
      }
    }

    // Decide to add/remove instance based on given operation options.
    auto instance_cnx_opts =
        shcore::get_connection_options(instance_address, false);

    update_metadata_for_instance(instance_cnx_opts,
                                 instance_map->get_uint("id"), label);

    // The xplugin was disabled, remove the endpoint from the metadata
    if (xport_endpoint_str == k_xplugin_disabled) {
      m_cluster->get_metadata_storage()->update_instance_addresses(
          instance_map->get_string("member_id"), "mysqlX",
          shcore::Value::Null());
    }
  }
}

void Rescan::upgrade_comm_protocol() {
  auto console = mysqlsh::current_console();

  std::shared_ptr<Instance> cluster_instance = m_cluster->get_cluster_server();

  mysqlshdk::utils::Version gr_protocol_version_to_upgrade;

  // After removing instance, the remove command must set
  // the GR protocol of the group to the lowest MySQL version on the group.
  if (m_options.upgrade_comm_protocol) {
    if (mysqlshdk::gr::is_protocol_upgrade_possible(
            *cluster_instance, "", &gr_protocol_version_to_upgrade)) {
      console->print_note(
          "Upgrading Group Replication communication protocol to version " +
          gr_protocol_version_to_upgrade.get_full());

      mysqlshdk::gr::set_group_protocol_version(*cluster_instance,
                                                gr_protocol_version_to_upgrade);
    }
  } else {
    check_protocol_upgrade_possible(*cluster_instance, "");
  }
}

void Rescan::ensure_view_change_uuid_set() {
  // Only generate and set the value when all Cluster members are running a
  // version >= k_min_cs_version
  // NOTE: group_replication_view_change_uuid must be the same on all Cluster
  // members, otherwise the members won't join the cluster. Members that do not
  // know about the sysvar (<= 8.0.26) will be considered as having the sysvar
  // set to AUTOMATIC so we must ensure the sysvar is only set when all members
  // support it
  if (m_cluster->get_lowest_instance_version() <
      Precondition_checker::k_min_cs_version)
    return;

  // Check if group_replication_view_change_uuid is already set on the cluster
  std::shared_ptr<Instance> cluster_instance = m_cluster->get_cluster_server();
  auto view_change_uuid = cluster_instance->get_sysvar_string(
      "group_replication_view_change_uuid", "");

  bool skip_md_mismatch_check = false;

  // If not set (AUTOMATIC), generate a new value and set it on all members
  if (view_change_uuid == "AUTOMATIC") {
    // Check if the variable was persisted so it needs a restart
    std::string view_change_uuid_persisted =
        cluster_instance
            ->get_persisted_value("group_replication_view_change_uuid")
            .value_or("");

    auto console = mysqlsh::current_console();

    if (!view_change_uuid_persisted.empty() &&
        view_change_uuid_persisted != view_change_uuid) {
      console->print_warning(
          "The current Cluster group_replication_view_change_uuid setting does "
          "not allow ClusterSet to be implemented, because it's set but not "
          "yet effective.");

      skip_md_mismatch_check = true;
    } else {
      console->print_note(
          "The Cluster's group_replication_view_change_uuid is not set.");
      console->print_info(
          "Generating and setting a value for "
          "group_replication_view_change_uuid...");

      // Generate a value for group_replication_view_change_uuid
      view_change_uuid = cluster_instance->generate_uuid();

      // Get the Cluster Config Object
      // This ensures the config is set on ONLINE || RECOVERING members
      auto cfg = m_cluster->create_config_object({}, false, true);

      cfg->set("group_replication_view_change_uuid", view_change_uuid);
      cfg->apply();
    }

    console->print_note(
        "The Cluster must be completely taken OFFLINE and restarted "
        "(<<<dba.rebootClusterFromCompleteOutage>>>()) for the settings to "
        "be effective");
  }

  ensure_view_change_uuid_set_stored_metadata(view_change_uuid,
                                              skip_md_mismatch_check);
}

void Rescan::ensure_view_change_uuid_set_stored_metadata(
    const std::string &view_change_uuid, bool skip_md_mismatch_check) {
  auto console = mysqlsh::current_console();

  // Check if view_change_uuid is stored in the Metadata
  if (m_cluster->get_view_change_uuid().empty()) {
    console->print_info(
        "Updating group_replication_view_change_uuid in the Cluster's "
        "metadata...");

    if (view_change_uuid.empty()) {
      m_cluster->get_metadata_storage()->update_cluster_attribute(
          m_cluster->get_id(), "group_replication_view_change_uuid",
          shcore::Value(m_cluster->get_cluster_server()->get_sysvar_string(
              "group_replication_view_change_uuid", "")));
    } else {
      m_cluster->get_metadata_storage()->update_cluster_attribute(
          m_cluster->get_id(), "group_replication_view_change_uuid",
          shcore::Value(view_change_uuid));
    }
  } else if (!skip_md_mismatch_check) {
    // Ensure that the value of group_replication_view_change_uuid set on the
    // cluster matches the value stored in the Metadata. Do not check when the
    // Cluster was just updated with a new value for view_change_uuid and needs
    // to be restarted
    auto view_change_uuid_cluster =
        m_cluster->get_cluster_server()->get_sysvar_string(
            "group_replication_view_change_uuid", "");

    if (view_change_uuid_cluster != m_cluster->get_view_change_uuid()) {
      console->print_info(
          "Updating group_replication_view_change_uuid in the Cluster's "
          "metadata...");

      m_cluster->get_metadata_storage()->update_cluster_attribute(
          m_cluster->get_id(), "group_replication_view_change_uuid",
          shcore::Value(view_change_uuid_cluster));
    }
  }
}

void Rescan::ensure_transaction_size_limit_stored_metadata() {
  auto console = mysqlsh::current_console();
  shcore::Value value;

  // Check if transaction_size_limit is stored in the Metadata
  if (!m_cluster->get_metadata_storage()->query_cluster_attribute(
          m_cluster->get_id(), k_cluster_attribute_transaction_size_limit,
          &value)) {
    console->print_info(
        "Updating group_replication_transaction_size_limit in the Cluster's "
        "metadata...");

    m_cluster->get_metadata_storage()->update_cluster_attribute(
        m_cluster->get_id(), k_cluster_attribute_transaction_size_limit,
        shcore::Value(m_cluster->get_cluster_server()
                          ->get_sysvar_int(kGrTransactionSizeLimit)
                          .value_or(0)));
  }
}

void Rescan::ensure_transaction_size_limit_consistency() {
  // Get the Cluster's transaction size_limit stored in the Metadata if the
  // Cluster is standlone or a Primary. Otherwise, it's a Replica so it should
  // be zero
  int64_t cluster_transaction_size_limit = 0;

  if (!m_cluster->is_cluster_set_member() ||
      (m_cluster->is_cluster_set_member() && m_cluster->is_primary_cluster())) {
    shcore::Value value;
    m_cluster->get_metadata_storage()->query_cluster_attribute(
        m_cluster->get_id(), k_cluster_attribute_transaction_size_limit,
        &value);

    cluster_transaction_size_limit = value.as_int();
  }

  m_cluster->execute_in_members(
      [cluster_transaction_size_limit](
          const std::shared_ptr<Instance> &instance,
          const Cluster_impl::Instance_md_and_gr_member &info) {
        if (info.second.state != mysqlshdk::gr::Member_state::RECOVERING &&
            info.second.state != mysqlshdk::gr::Member_state::UNREACHABLE) {
          // Get the instance's value for transaction_size_limit
          int64_t instance_transaction_size_limit =
              instance->get_sysvar_int(kGrTransactionSizeLimit).value_or(0);

          if (instance_transaction_size_limit !=
              cluster_transaction_size_limit) {
            auto console = mysqlsh::current_console();

            console->print_note(
                "'group_replication_transaction_size_limit' value at '" +
                instance->descr() + "' does not match the Cluster's value: '" +
                std::to_string(cluster_transaction_size_limit) + "'");

            console->print_info("Updating the member's value...");

            log_info(
                "Updating group_replication_transaction_size_limit on "
                "instance '%s' to the Cluster's value '%" PRId64 "'",
                instance->descr().c_str(), cluster_transaction_size_limit);

            auto cfg = mysqlsh::dba::create_server_config(
                instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

            cfg->set(kGrTransactionSizeLimit,
                     std::optional<int64_t>(cluster_transaction_size_limit));

            cfg->apply();
          }
        }
        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });
}

void Rescan::ensure_recovery_accounts_match() {
  m_cluster->execute_in_members(
      [this](const std::shared_ptr<Instance> &instance,
             const Cluster_impl::Instance_md_and_gr_member &info) {
        if (info.second.state != mysqlshdk::gr::Member_state::RECOVERING &&
            info.second.state != mysqlshdk::gr::Member_state::UNREACHABLE) {
          // Get the instance's recovery account
          std::string recovery_user;
          std::vector<std::string> recovery_user_hosts;
          std::tie(recovery_user, recovery_user_hosts, std::ignore) =
              m_cluster->get_replication_user(*instance);

          // Check if the account is using the old prefix
          bool old_prefix = false;
          if (shcore::str_beginswith(
                  recovery_user,
                  mysqlshdk::gr::k_group_recovery_old_user_prefix)) {
            old_prefix = true;
          }

          // Generate the recovery account string for this instance
          std::string recovery_user_generated =
              Cluster_impl::make_replication_user_name(
                  instance->get_server_id(),
                  old_prefix ? mysqlshdk::gr::k_group_recovery_old_user_prefix
                             : mysqlshdk::gr::k_group_recovery_user_prefix);

          // If the recovery user being used is different than the generated
          // one it means the instance was added using clone as the recovery
          // method and waitRecovery set to zero so the AdminAPI couldn't
          // handle the fact that clone copies everything including
          // mysql.slave_master_info where the replication credentials for the
          // recovery channel are stored
          if (recovery_user != recovery_user_generated) {
            // Ensure the user exists first
            if (!m_cluster->get_primary_master()->user_exists(
                    recovery_user_generated, recovery_user_hosts.front())) {
              log_debug("Recovery account '%s' does not exist in the cluster",
                        recovery_user_generated.c_str());
              return true;
            }

            mysqlsh::current_console()->print_info(shcore::str_format(
                "Fixing incorrect recovery account '%s' in instance '%s'",
                recovery_user.c_str(), instance->descr().c_str()));

            // Generate and set a new password and update the replication
            // credentials
            m_cluster->reset_recovery_password(
                instance, recovery_user_generated, recovery_user_hosts);

            // Update the recovery account to the right one
            log_info("Updating recovery account in metadata");
            m_cluster->get_metadata_storage()->update_instance_repl_account(
                instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
                Replica_type::GROUP_MEMBER, recovery_user_generated,
                recovery_user_hosts.front());
          }
        }
        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });
}

shcore::Value Rescan::execute() {
  auto console = mysqlsh::current_console();
  console->print_info("Rescanning the cluster...");
  console->print_info();

  shcore::Value::Map_type_ref result = get_rescan_report();

  console->print_info("Result of the rescanning operation for the '" +
                      result->get_string("name") + "' cluster:");
  console->print_value(shcore::Value(result), "");
  console->print_info();

  // Check if there are unknown instances
  if (result->has_key("newlyDiscoveredInstances")) {
    add_metadata_for_instances(result->get_array("newlyDiscoveredInstances"));
  }

  // Print warning about not used instances in addInstances.
  {
    std::vector<std::string> not_used_add_instances;
    not_used_add_instances.reserve(m_options.add_instances_list.size());

    for (const auto &cnx_opts : m_options.add_instances_list) {
      not_used_add_instances.push_back(
          cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()));
    }

    if (!not_used_add_instances.empty()) {
      console->print_warning(
          "The following instances were not added to the metadata because they "
          "are already part of the cluster: '" +
          shcore::str_join(not_used_add_instances, ", ") +
          "'. Please verify if the specified value for 'addInstances' option "
          "is correct.");
    }
  }

  // Check if there are missing instances
  std::shared_ptr<shcore::Value::Array_type> missing_instances;

  if (result->has_key("unavailableInstances")) {
    missing_instances = result->get_array("unavailableInstances");

    remove_metadata_for_instances(missing_instances);
  }

  if (result->has_key("updatedInstances")) {
    update_metadata_for_instances(result->get_array("updatedInstances"));
  }

  // Fix all missing recovery accounts from metadata.
  m_cluster->ensure_metadata_has_recovery_accounts();

  // This calls ensures all of the instances have server_id as instance
  // attribute, including the case were rescan is executed and no new/updated
  // instances were found
  m_cluster->ensure_metadata_has_server_id(*m_cluster->get_cluster_server());

  // Check if group_replication_view_change_uuid is set on the Cluster and all
  // of its members when running MySQL >= 8.0.27. Also verify the Metadata
  // consistency regarding group_replication_view_change_uuid when already in
  // use.
  // Skip if running MySQL >= 8.3.0, since the option is no longer needed.
  if (m_is_view_change_uuid_supported) {
    auto view_change_uuid = m_cluster->get_primary_master()->get_sysvar_string(
        "group_replication_view_change_uuid", "");

    if (view_change_uuid == "AUTOMATIC") {
      if (!m_options.update_view_change_uuid.has_value() &&
          m_cluster->get_lowest_instance_version() <
              k_view_change_uuid_deprecated) {
        console->print_note(
            "The Cluster is not configured to use "
            "'group_replication_view_change_uuid', which is required "
            "for InnoDB ClusterSet. Configuring it requires a full Cluster "
            "reboot.");
        if (m_options.interactive()) {
          // Prompt the user to update the View Change UUID
          if (console->confirm(
                  "Would you like 'group_replication_view_change_uuid' to be "
                  "configured automatically?",
                  Prompt_answer::YES) == Prompt_answer::YES) {
            ensure_view_change_uuid_set();
          }
        } else {
          console->print_info(
              "Use the 'updateViewChangeUuid' option to generate and "
              "configure a value for the Cluster.");
        }
      } else if (m_options.update_view_change_uuid.value_or(false)) {
        ensure_view_change_uuid_set();
      }
    } else {
      // Ensures the value is stored in the metadata schema
      ensure_view_change_uuid_set_stored_metadata();
    }
  }

  // Ensures the value of group_replication_transaction_size_limit is stored
  // in the metadata
  ensure_transaction_size_limit_stored_metadata();

  // Ensures group_replication_transaction_size_limit is set to the same value
  // in all cluster members
  ensure_transaction_size_limit_consistency();

  // Print warning about not used instances in removeInstances.
  {
    std::vector<std::string> not_used_remove_instances;
    not_used_remove_instances.reserve(m_options.remove_instances_list.size());

    for (const auto &cnx_opts : m_options.remove_instances_list) {
      not_used_remove_instances.push_back(
          cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()));
    }
    if (!not_used_remove_instances.empty()) {
      console->print_warning(
          "The following instances were not removed from the metadata because "
          "they are already not part of the cluster or are running auto-rejoin"
          ": '" +
          shcore::str_join(not_used_remove_instances, ", ") +
          "'. Please verify if the specified value for 'removeInstances' "
          "option is correct.");
    }
  }

  // Check if the topology mode changed.
  // NOTE: This must be performed after updating the instances in the metadata,
  //       in order for the list of instance to be retrieved properly to update
  //       the auto-increment settings on all of them.
  if (result->has_key("newTopologyMode") &&
      !result->is_null("newTopologyMode")) {
    std::string new_topology_mode = result->get_string("newTopologyMode");

    if (!new_topology_mode.empty()) {
      console->print_note("The topology mode of the cluster changed to '" +
                          new_topology_mode + "'.");

      // Update the topology mode in the Metadata
      update_topology_mode(mysqlshdk::gr::to_topology_mode(new_topology_mode));
    }
  }

  if (m_options.upgrade_comm_protocol) {
    upgrade_comm_protocol();
  }

  // Check for instances using a different account than its own for the
  // recovery channel and do a reset+update of the credentials to ensure the
  // right account is used. Finally, update the MD schema to store the right
  // account in use.
  //
  // A member rejoining needs to be able to connect to itself and to all
  // cluster members, as well as all cluster members need to be able to
  // connect to the joining member.
  ensure_recovery_accounts_match();

  // removes unused recovery accounts
  for (const auto &[user, host] : m_cluster->get_unused_recovery_accounts(
           m_cluster->get_mismatched_recovery_accounts())) {
    try {
      console->print_info(
          shcore::str_format("Dropping unused recovery account: '%s'@'%s'",
                             user.c_str(), host.c_str()));
      m_cluster->get_primary_master()->drop_user(user, host, true);
    } catch (...) {
      console->print_warning(shcore::str_format(
          "Error dropping recovery account '%s'@'%s': %s", user.c_str(),
          host.c_str(), format_active_exception().c_str()));
    }
  }

  return shcore::Value();
}

void Rescan::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Rescan::finish() {
  // Do nothing.
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
