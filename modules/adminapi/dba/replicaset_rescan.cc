/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba/replicaset_rescan.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>

#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Rescan::Rescan(
    bool interactive, mysqlshdk::utils::nullable<bool> update_topology_mode,
    bool auto_add_instances, bool auto_remove_instances,
    const std::vector<mysqlshdk::db::Connection_options> &add_instances_list,
    const std::vector<mysqlshdk::db::Connection_options> &remove_instances_list,
    std::shared_ptr<Cluster> cluster, std::shared_ptr<ReplicaSet> replicaset,
    const shcore::NamingStyle &naming_style)
    : m_interactive(interactive),
      m_update_topology_mode(update_topology_mode),
      m_auto_add_instances(auto_add_instances),
      m_auto_remove_instances(auto_remove_instances),
      m_add_instances_list(std::move(add_instances_list)),
      m_remove_instances_list(std::move(remove_instances_list)),
      m_cluster(cluster),
      m_replicaset(replicaset),
      m_naming_style(naming_style) {
  assert(cluster);
  assert(replicaset);
  assert((auto_add_instances && add_instances_list.empty()) ||
         !auto_add_instances);
  assert((auto_remove_instances && remove_instances_list.empty()) ||
         !auto_remove_instances);
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
  duplicates_check("addInstances", m_add_instances_list, &add_instances_set);

  // Check for duplicates in the removeInstances list.
  duplicates_check("removeInstances", m_remove_instances_list,
                   &remove_instances_set);

  // Find common instance in the two options.
  std::vector<std::string> reapeated_instances;
  std::set_intersection(add_instances_set.begin(), add_instances_set.end(),
                        remove_instances_set.begin(),
                        remove_instances_set.end(),
                        std::back_inserter(reapeated_instances));

  // The same instance cannot be included in both options.
  if (!reapeated_instances.empty()) {
    std::string plural = (reapeated_instances.size() > 1) ? "s" : "";
    throw shcore::Exception::argument_error(
        "The same instance" + plural +
        " cannot be used in both 'addInstances' and 'removeInstances' options: "
        "'" +
        shcore::str_join(reapeated_instances, ", ") + "'.");
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

    mysqlshdk::mysql::Instance cluster_instance(m_cluster->get_group_session());

    if (mysqlshdk::gr::is_active_member(cluster_instance, cnx_opt.get_host(),
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
  std::vector<std::string> invalid_add_members =
      detect_invalid_members(m_add_instances_list, true);

  if (!invalid_add_members.empty()) {
    console->print_error(
        "The following instances cannot be added because they are not active "
        "members of the cluster: '" +
        shcore::str_join(invalid_add_members, ", ") +
        "'. Please verify if the specified addresses are correct, or if the "
        "instances are currently inactive.");
    console->println();

    throw shcore::Exception::runtime_error(
        "The following instances are not active members of the cluster: '" +
        shcore::str_join(invalid_add_members, ", ") + "'.");
  }

  // All instance to remove cannot be an active member of the GR group.
  std::vector<std::string> invalid_remove_members =
      detect_invalid_members(m_remove_instances_list, false);

  if (!invalid_remove_members.empty()) {
    console->print_error(
        "The following instances cannot be removed because they are active "
        "members of the cluster: '" +
        shcore::str_join(invalid_remove_members, ", ") +
        "'. Please verify if the specified addresses are correct.");
    console->println();

    throw shcore::Exception::runtime_error(
        "The following instances are active members of the cluster: '" +
        shcore::str_join(invalid_remove_members, ", ") + "'.");
  }
}

shcore::Value::Map_type_ref Rescan::get_rescan_report() const {
  auto replicaset_map = std::make_shared<shcore::Value::Map_type>();

  // Set the ReplicaSet name on the result map
  (*replicaset_map)["name"] = shcore::Value(m_replicaset->get_name());

  std::vector<NewInstanceInfo> newly_discovered_instances_list =
      get_newly_discovered_instances(m_cluster->get_metadata_storage(),
                                     m_replicaset->get_id());

  // Creates the newlyDiscoveredInstances map
  auto newly_discovered_instances =
      std::make_shared<shcore::Value::Array_type>();

  for (auto &instance : newly_discovered_instances_list) {
    auto newly_discovered_instance =
        std::make_shared<shcore::Value::Map_type>();
    (*newly_discovered_instance)["member_id"] =
        shcore::Value(instance.member_id);
    (*newly_discovered_instance)["name"] = shcore::Value::Null();

    std::string instance_address =
        instance.host + ":" + std::to_string(instance.port);

    (*newly_discovered_instance)["host"] = shcore::Value(instance_address);
    newly_discovered_instances->push_back(
        shcore::Value(newly_discovered_instance));
  }
  // Add the newly_discovered_instances list to the result Map
  (*replicaset_map)["newlyDiscoveredInstances"] =
      shcore::Value(newly_discovered_instances);

  shcore::Value unavailable_instances_result;

  std::vector<MissingInstanceInfo> unavailable_instances_list =
      get_unavailable_instances(m_cluster->get_metadata_storage(),
                                m_replicaset->get_id());

  // Creates the unavailableInstances array
  auto unavailable_instances = std::make_shared<shcore::Value::Array_type>();

  for (auto &instance : unavailable_instances_list) {
    auto unavailable_instance = std::make_shared<shcore::Value::Map_type>();
    (*unavailable_instance)["member_id"] = shcore::Value(instance.id);
    (*unavailable_instance)["label"] = shcore::Value(instance.label);
    (*unavailable_instance)["host"] = shcore::Value(instance.host);

    unavailable_instances->push_back(shcore::Value(unavailable_instance));
  }
  // Add the missing_instances list to the result Map
  (*replicaset_map)["unavailableInstances"] =
      shcore::Value(unavailable_instances);

  // Get the topology mode from the metadata.
  mysqlshdk::gr::Topology_mode metadata_topology_mode =
      m_cluster->get_metadata_storage()->get_replicaset_topology_mode(
          m_replicaset->get_id());

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      m_cluster->get_group_session(), nullptr);

  // Check if the topology mode match and report needed change in the metadata.
  if (gr_primary_uuid.empty() &&
      metadata_topology_mode == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) {
    // Topology mode need to be changed to multi-primary on the metadata.
    (*replicaset_map)["newTopologyMode"] = shcore::Value(
        mysqlshdk::gr::to_string(mysqlshdk::gr::Topology_mode::MULTI_PRIMARY));
  } else if (!gr_primary_uuid.empty() &&
             metadata_topology_mode ==
                 mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    // Topology mode need to be changed to single-primary on the metadata.
    (*replicaset_map)["newTopologyMode"] = shcore::Value(
        mysqlshdk::gr::to_string(mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY));
  } else {
    // Topology mode is the same (no change needed).
    (*replicaset_map)["newTopologyMode"] = shcore::Value::Null();
  }

  return replicaset_map;
}

void Rescan::update_auto_increment_setting(
    mysqlshdk::gr::Topology_mode topology_mode) {
  auto console = mysqlsh::current_console();
  mysqlshdk::config::Config cfg;
  std::vector<std::unique_ptr<mysqlshdk::mysql::Instance>> available_instances;

  // Get all cluster instances, including state information to update
  // auto-increment values.
  std::vector<Instance_definition> instance_defs =
      m_cluster->get_metadata_storage()->get_replicaset_instances(
          m_replicaset->get_id(), true);

  for (const auto &instance_def : instance_defs) {
    // Use the GR state hold by instance_def.state (but convert it to a proper
    // mysqlshdk::gr::Member_state to be handled properly).
    mysqlshdk::gr::Member_state state =
        mysqlshdk::gr::to_member_state(instance_def.state);

    if (state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Set login credentials to connect to instance.
      // NOTE: It is assumed that the same login credentials can be used to
      //       connect to all cluster instances.
      Connection_options instance_cnx_opts =
          shcore::get_connection_options(instance_def.endpoint, false);
      instance_cnx_opts.set_login_options_from(
          m_cluster->get_group_session()->get_connection_options());

      // Try to connect to instance.
      log_debug("Connecting to instance '%s'", instance_def.endpoint.c_str());
      std::shared_ptr<mysqlshdk::db::ISession> session;
      try {
        session = mysqlshdk::db::mysql::Session::create();
        session->connect(instance_cnx_opts);
        log_debug("Successfully connected to instance");
      } catch (const std::exception &err) {
        log_debug("Failed to connect to instance: %s", err.what());
        console->print_error(
            "Unable to connect to instance '" + instance_def.endpoint +
            "'. Please, verify connection credentials and make sure the "
            "instance is available.");

        throw shcore::Exception::runtime_error(err.what());
      }

      available_instances.emplace_back(new mysqlshdk::mysql::Instance(session));

      // Determine if SET PERSIST is supported.
      mysqlshdk::utils::nullable<bool> support_set_persist =
          available_instances.back()->is_set_persist_supported();
      mysqlshdk::mysql::Var_qualifier set_type =
          mysqlshdk::mysql::Var_qualifier::GLOBAL;
      if (!support_set_persist.is_null() && *support_set_persist) {
        set_type = mysqlshdk::mysql::Var_qualifier::PERSIST;
      }

      // Add configuration handler for server.
      cfg.add_handler(instance_def.endpoint,
                      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                          new mysqlshdk::config::Config_server_handler(
                              available_instances.back().get(), set_type)));

      // Print a warning if SET PERSIST is not supported, for users to execute
      // dba.configureLocalInstance().
      if (support_set_persist.is_null()) {
        console->print_warning(
            "Auto-increment settings cannot be persisted remotely on instance "
            "'" +
            instance_def.endpoint + "' because MySQL version " +
            available_instances.back()->get_version().get_base() +
            " does not support the SET PERSIST command "
            "(MySQL version >= 8.0.11 required). Please execute the <Dba>." +
            get_member_name("configureLocalInstance", m_naming_style) +
            " command locally to persist these changes.");
      } else if (!*support_set_persist) {
        console->print_warning(
            "Auto-increment settings cannot be persisted remotely on instance "
            "'" +
            instance_def.endpoint +
            "' because 'persisted-globals-load' is set "
            "to 'OFF' and persisted configurations will not be loaded upon "
            "reboot. Please execute the <Dba>." +
            get_member_name("configureLocalInstance", m_naming_style) +
            " command locally to persist these changes.");
      }
    } else {
      // Issue an error if the instance is not active.
      console->print_error(
          "Auto-increment settings cannot be updated for instance '" +
          instance_def.endpoint + "' because it is on a '" +
          mysqlshdk::gr::to_string(state) +
          "' state. Please bring the instance back ONLINE and try to rescan "
          "the cluster again.");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance_def.endpoint + "' is '" +
          mysqlshdk::gr::to_string(state) + "'");
    }
  }

  // Update auto-increment settings on all servers.
  mysqlshdk::gr::update_auto_increment(&cfg, topology_mode);
  cfg.apply();

  // Close the session for all used instances.
  for (auto &instance : available_instances) {
    instance->close_session();
  }
}

void Rescan::update_topology_mode(
    const mysqlshdk::gr::Topology_mode &topology_mode) {
  auto console = mysqlsh::current_console();
  console->println("Updating topology mode in the cluster metadata...");

  // Update auto-increment settings.
  update_auto_increment_setting(topology_mode);

  // Update topology mode in metadata.
  m_cluster->get_metadata_storage()->update_replicaset_topology_mode(
      m_replicaset->get_id(), topology_mode);

  // Update the topology mode on the ReplicaSet object.
  std::string topo_mode =
      (topology_mode == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY)
          ? mysqlsh::dba::ReplicaSet::kTopologySinglePrimary
          : mysqlsh::dba::ReplicaSet::kTopologyMultiPrimary;
  m_replicaset->set_topology_type(topo_mode);

  console->println("Topology mode was successfully updated to '" +
                   mysqlshdk::gr::to_string(topology_mode) +
                   "' in the cluster metadata.");
  console->println();
}

void Rescan::add_instance_to_metadata(
    const mysqlshdk::db::Connection_options &instance_cnx_opts) {
  auto console = mysqlsh::current_console();
  console->println("Adding instance to the cluster metadata...");

  // Create a copy of the connection options to set the cluster user
  // credentials if needed (should not change the value passed as parameter).
  mysqlshdk::db::Connection_options cnx_opts = instance_cnx_opts;
  if (!cnx_opts.has_user()) {
    // It is assumed that the same login options of the cluster can be
    // used to connect to the instance, if no login (user) was provided.
    cnx_opts.set_login_options_from(
        m_cluster->get_group_session()->get_connection_options());
  }

  m_replicaset->add_instance_metadata(cnx_opts);

  console->println(
      "The instance '" +
      instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "' was successfully added to the cluster metadata.");
  console->println();
}

void Rescan::remove_instance_from_metadata(
    const std::string &instance_address) {
  mysqlshdk::db::Connection_options instance_cnx_opts =
      shcore::get_connection_options(instance_address, false);

  auto console = mysqlsh::current_console();
  console->println("Removing instance from the cluster metadata...");

  m_replicaset->remove_instance_metadata(instance_cnx_opts);

  console->println(
      "The instance '" +
      instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "' was successfully removed from the cluster metadata.");
  console->println();
}

void Rescan::update_instances_list(
    std::shared_ptr<shcore::Value::Array_type> found_instances, bool auto_opt,
    std::vector<mysqlshdk::db::Connection_options> *instances_list_opt,
    bool to_add) {
  assert(instances_list_opt);
  auto console = mysqlsh::current_console();

  for (const auto &instance : *found_instances) {
    auto instance_map = instance.as_map();

    std::string instance_address = instance_map->get_string("host");
    if (to_add) {
      // Report that a new instance was discovered (not in the metadata).
      console->println("A new instance '" + instance_address +
                       "' was discovered in the ReplicaSet.");
    } else {
      // Report that an obsolete instance was found (in the metadata).
      console->println("The instance '" + instance_address +
                       "' is no longer part of the ReplicaSet.");
    }

    // Check if the new instance belongs to the addInstances list.
    auto list_iter = instances_list_opt->end();
    if (!instances_list_opt->empty()) {
      // Lambda function (predicate) to check if the instance address
      // matches.
      auto in_list = [&instance_address](
                         const mysqlshdk::db::Connection_options &cnx_opt) {
        std::string address =
            cnx_opt.as_uri(mysqlshdk::db::uri::formats::only_transport());
        return address == instance_address;
      };

      // Return iterator with a matching address (if in the list).
      list_iter = std::find_if(instances_list_opt->begin(),
                               instances_list_opt->end(), in_list);
    }

    // Decide to add/remove instance based on given operation options.
    if (list_iter != instances_list_opt->end()) {
      if (to_add) {
        // Add instance if in addInstances list.
        mysqlshdk::db::Connection_options instance_cnx_opts = *list_iter;
        add_instance_to_metadata(instance_cnx_opts);
      } else {
        // Remove instance if in removeInstances list.
        remove_instance_from_metadata(instance_address);
      }

      instances_list_opt->erase(list_iter);
    } else if (auto_opt) {
      if (to_add) {
        // Add instance automatically if "auto" was used.
        mysqlshdk::db::Connection_options instance_cnx_opts =
            shcore::get_connection_options(instance_address, false);
        add_instance_to_metadata(instance_cnx_opts);
      } else {
        // Remove instance automatically if "auto" was used.
        remove_instance_from_metadata(instance_address);
      }
    } else if (m_interactive) {
      if (to_add) {
        // Ask to add instance if prompts are enabled.
        if (console->confirm(
                "Would you like to add it to the cluster metadata?",
                Prompt_answer::YES) == Prompt_answer::YES) {
          mysqlshdk::db::Connection_options instance_cnx_opts =
              shcore::get_connection_options(instance_address, false);

          add_instance_to_metadata(instance_cnx_opts);
        }
      } else {
        console->println(
            "The instance is either offline or left the HA group. You can "
            "try "
            "to add it to the cluster again with the "
            "cluster.rejoinInstance('" +
            instance_map->get_string("host") +
            "') command or you can remove it from the cluster "
            "configuration.");

        // Ask to remove instance if prompts are enabled.
        if (console->confirm(
                "Would you like to remove it from the cluster metadata?",
                Prompt_answer::YES) == Prompt_answer::YES) {
          remove_instance_from_metadata(instance_address);
        }
      }
    }
  }
}

shcore::Value Rescan::execute() {
  auto console = mysqlsh::current_console();
  console->print_info("Rescanning the cluster...");
  console->println();

  shcore::Value::Map_type_ref result = get_rescan_report();

  console->print_info("Result of the rescanning operation for the '" +
                      result->get_string("name") + "' ReplicaSet:");
  console->print_value(shcore::Value(result), "");
  console->println();

  // Check if there are unknown instances
  if (result->has_key("newlyDiscoveredInstances")) {
    auto unknown_instances = result->get_array("newlyDiscoveredInstances");

    update_instances_list(unknown_instances, m_auto_add_instances,
                          &m_add_instances_list, true);
  }

  // Print warning about not used instances in addInstances.
  std::vector<std::string> not_used_add_instances;
  for (const auto &cnx_opts : m_add_instances_list) {
    not_used_add_instances.push_back(
        cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()));
  }
  if (!not_used_add_instances.empty()) {
    console->print_warning(
        "The following instances were not added to the medatada because they "
        "are already part of the replicaset: '" +
        shcore::str_join(not_used_add_instances, ", ") +
        "'. Please verify if the specified value for 'addInstances' option is "
        "correct.");
  }

  // Check if there are missing instances
  if (result->has_key("unavailableInstances")) {
    auto missing_instances = result->get_array("unavailableInstances");

    update_instances_list(missing_instances, m_auto_remove_instances,
                          &m_remove_instances_list, false);
  }

  // Print warning about not used instances in removeInstances.
  std::vector<std::string> not_used_remove_instances;
  for (const auto &cnx_opts : m_remove_instances_list) {
    not_used_remove_instances.push_back(
        cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport()));
  }
  if (!not_used_remove_instances.empty()) {
    console->print_warning(
        "The following instances were not removed from the medatada because "
        "they are already not part of the replicaset: '" +
        shcore::str_join(not_used_remove_instances, ", ") +
        "'. Please verify if the specified value for 'removeInstances' option "
        "is correct.");
  }

  // Check if the topology mode changed.
  // NOTE: This must be performed after updating the instances in the metadata,
  //       in order for the list of instance to be retrieved properly to update
  //       the auto-increment settings on all of them.
  if (result->has_key("newTopologyMode") &&
      !result->is_null("newTopologyMode")) {
    std::string new_topology_mode = result->get_string("newTopologyMode");

    if (!new_topology_mode.empty()) {
      console->println("The topology mode of the ReplicaSet changed to '" +
                       new_topology_mode + "'.");

      // Determine if the topology mode will be updated in the metadata.
      if (!m_update_topology_mode.is_null() && *m_update_topology_mode) {
        // Update topology mode in the metadata.
        update_topology_mode(
            mysqlshdk::gr::to_topology_mode(new_topology_mode));
      } else if (m_update_topology_mode.is_null() && m_interactive) {
        // Ask to update the topology mode if prompts are enabled.
        if (console->confirm(
                "Would you like to update it in the cluster metadata?",
                Prompt_answer::YES) == Prompt_answer::YES) {
          update_topology_mode(
              mysqlshdk::gr::to_topology_mode(new_topology_mode));
        }
      }
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

}  // namespace dba
}  // namespace mysqlsh
