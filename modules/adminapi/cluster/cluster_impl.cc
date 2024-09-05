/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/cluster_impl.h"

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "adminapi/common/clone_options.h"
#include "adminapi/common/instance_pool.h"
#include "modules/adminapi/cluster/add_instance.h"
#include "modules/adminapi/cluster/add_replica_instance.h"
#include "modules/adminapi/cluster/describe.h"
#include "modules/adminapi/cluster/dissolve.h"
#include "modules/adminapi/cluster/options.h"
#include "modules/adminapi/cluster/rejoin_instance.h"
#include "modules/adminapi/cluster/rejoin_replica_instance.h"
#include "modules/adminapi/cluster/remove_instance.h"
#include "modules/adminapi/cluster/remove_replica_instance.h"
#include "modules/adminapi/cluster/rescan.h"
#include "modules/adminapi/cluster/reset_recovery_accounts_password.h"
#include "modules/adminapi/cluster/set_instance_option.h"
#include "modules/adminapi/cluster/set_option.h"
#include "modules/adminapi/cluster/set_primary_instance.h"
#include "modules/adminapi/cluster/status.h"
#include "modules/adminapi/cluster/switch_to_multi_primary_mode.h"
#include "modules/adminapi/cluster/switch_to_single_primary_mode.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/execute.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/topology_executor.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/dba_utils.h"
#include "mysql/instance.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/undo.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "scripting/types.h"
#include "utils/debug.h"
#include "utils/logger.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {
// Constants with the names used to lock the cluster
constexpr std::string_view k_lock_ns{"AdminAPI_cluster"};
constexpr std::string_view k_lock_name{"AdminAPI_lock"};

template <typename T>
struct Option_info {
  bool found_non_default = false;
  bool found_not_supported = false;
  T non_default_value = T{};
};

void check_gr_empty_local_address_exception(
    const shcore::Exception &exp, const mysqlshdk::mysql::IInstance &instance) {
  if ((exp.code() != SHERR_DBA_EMPTY_LOCAL_ADDRESS)) return;

  mysqlsh::current_console()->print_error(shcore::str_format(
      "Unable to read Group Replication local address setting for instance "
      "'%s', probably due to connectivity issues. Please retry the operation.",
      instance.get_canonical_address().c_str()));
}
}  // namespace

Cluster_impl::Cluster_impl(
    const std::shared_ptr<Cluster_set_impl> &cluster_set,
    const Cluster_metadata &metadata,
    const std::shared_ptr<Instance> &group_server,
    const std::shared_ptr<MetadataStorage> &metadata_storage,
    Cluster_availability availability)
    : Cluster_impl(metadata, group_server, metadata_storage, availability) {
  m_cluster_set = cluster_set;
  if (auto primary = cluster_set->get_primary_master(); primary)
    m_primary_master = primary;
}

Cluster_impl::Cluster_impl(
    const Cluster_metadata &metadata,
    const std::shared_ptr<Instance> &group_server,
    const std::shared_ptr<MetadataStorage> &metadata_storage,
    Cluster_availability availability)
    : Base_cluster_impl(metadata.cluster_name, group_server, metadata_storage),
      m_topology_type(metadata.cluster_topology_type),
      m_availability(availability),
      m_cs_md(),
      m_repl_account_mng{*this} {
  m_id = metadata.cluster_id;
  m_description = metadata.description;
  m_group_name = metadata.group_name;
  m_primary_master = group_server;

  observe_notification(kNotifyClusterSetPrimaryChanged);
}

Cluster_impl::Cluster_impl(
    const std::string &cluster_name, const std::string &group_name,
    const std::shared_ptr<Instance> &group_server,
    const std::shared_ptr<Instance> &primary_master,
    const std::shared_ptr<MetadataStorage> &metadata_storage,
    mysqlshdk::gr::Topology_mode topology_type)
    : Base_cluster_impl(cluster_name, group_server, metadata_storage),
      m_group_name(group_name),
      m_topology_type(topology_type),
      m_repl_account_mng{*this} {
  // cluster is being created

  m_primary_master = primary_master;

  assert(topology_type == mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY ||
         topology_type == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  observe_notification(kNotifyClusterSetPrimaryChanged);
}

Cluster_impl::~Cluster_impl() = default;

void Cluster_impl::sanity_check() const {
  if (m_availability == Cluster_availability::ONLINE)
    verify_topology_type_change();
}

void Cluster_impl::find_real_cluster_set_primary(Cluster_set_impl *cs) const {
  assert(cs);

  for (;;) {
    if (!m_primary_master || !cs->get_primary_cluster()) {
      cs->connect_primary();
    }

    if (!cs->reconnect_target_if_invalidated(false)) {
      break;
    }
  }
}

/*
 * Verify if the topology type changed and issue an error if needed
 */
void Cluster_impl::verify_topology_type_change() const {
  // TODO(alfredo) - this should be replaced by a clusterErrors node in
  // cluster.status(), along with other cluster level diag msgs

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary

  bool single_primary_mode;
  mysqlshdk::gr::get_group_primary_uuid(*get_cluster_server(),
                                        &single_primary_mode);

  // Check if the topology type matches the real settings used by the
  // cluster instance, otherwise an error is issued.
  // NOTE: The GR primary mode is guaranteed (by GR) to be the same for all
  // instance of the same group.
  if (single_primary_mode && get_cluster_topology_type() ==
                                 mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Multi-Primary) does not match the "
        "current Group Replication configuration (Single-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
  } else if (!single_primary_mode &&
             get_cluster_topology_type() ==
                 mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) {
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Single-Primary) does not match the "
        "current Group Replication configuration (Multi-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
  }
}

void Cluster_impl::adopt_from_gr() {
  shcore::Value ret_val;
  auto console = mysqlsh::current_console();

  auto newly_discovered_instances_list(get_newly_discovered_instances(
      *get_cluster_server(), get_metadata_storage(), get_id()));

  // Add all instances to the cluster metadata
  for (NewInstanceInfo &instance : newly_discovered_instances_list) {
    mysqlshdk::db::Connection_options newly_discovered_instance;

    newly_discovered_instance.set_host(instance.host);
    newly_discovered_instance.set_port(instance.port);

    log_info("Adopting member %s:%d from existing group", instance.host.c_str(),
             instance.port);
    console->print_info("Adding Instance '" + instance.host + ":" +
                        std::to_string(instance.port) + "'...");

    auto md_instance = get_metadata_storage()->get_md_server();

    auto session_data = md_instance->get_connection_options();

    newly_discovered_instance.set_login_options_from(session_data);

    add_metadata_for_instance(newly_discovered_instance,
                              Instance_type::GROUP_MEMBER);

    // Store the communicationStack in the Metadata as a Cluster capability
    // TODO(miguel): build and add the list of allowed operations
    get_metadata_storage()->update_cluster_capability(
        get_id(), kCommunicationStack,
        get_communication_stack(*get_cluster_server()),
        std::set<std::string>());
  }
}

/** Iterates through all the cluster members in a given state calling the given
 * function on each of then.
 * @param states Vector of strings with the states of members on which the
 * functor will be called.
 * @param cnx_opt Connection options to be used to connect to the cluster
 * members
 * @param ignore_instances_vector Vector with addresses of instances to be
 * ignored even if their state is specified in the states vector.
 * @param functor Function that is called on each member of the cluster whose
 * state is specified in the states vector.
 * @param condition Optional condition function. If provided, the result will be
 * evaluated and the functor will only be called if the condition returns true.
 * @param ignore_network_conn_errors Optional flag to indicate whether
 * connection errors should be treated as errors and stop execution in remaining
 * instances, by default is set to true.
 */
void Cluster_impl::execute_in_members(
    const std::vector<mysqlshdk::gr::Member_state> &states,
    const mysqlshdk::db::Connection_options &cnx_opt,
    const std::vector<std::string> &ignore_instances_vector,
    const std::function<bool(const std::shared_ptr<Instance> &instance,
                             const mysqlshdk::gr::Member &gr_member)> &functor,
    const std::function<
        bool(const Instance_md_and_gr_member &instance_with_state)> &condition,
    bool ignore_network_conn_errors) const {
  std::shared_ptr<Instance> instance_session;
  // Note (nelson): should we handle the super_read_only behavior here or should
  // it be the responsibility of the functor?
  auto instance_definitions = get_instances_with_state();

  for (auto &instance_def : instance_definitions) {
    // If the condition functor is given and the condition is not met then
    // continues to the next member
    if (condition && !condition(instance_def)) continue;

    std::string instance_address = instance_def.first.endpoint;

    // if instance is on the list of instances to be ignored, skip it
    if (std::find_if(ignore_instances_vector.begin(),
                     ignore_instances_vector.end(),
                     mysqlshdk::utils::Endpoint_predicate{instance_address}) !=
        ignore_instances_vector.end()) {
      continue;
    }
    // if state list is given but it doesn't match, skip too
    if (!states.empty() &&
        std::find(states.begin(), states.end(), instance_def.second.state) ==
            states.end()) {
      continue;
    }

    auto target_coptions = mysqlshdk::db::Connection_options(instance_address);

    target_coptions.set_login_options_from(cnx_opt);
    try {
      log_debug(
          "Opening a new session to instance '%s' while iterating "
          "cluster members",
          instance_address.c_str());
      instance_session = Instance::connect(
          target_coptions, current_shell_options()->get().wizards);
    } catch (const shcore::Error &e) {
      if (ignore_network_conn_errors && e.code() == CR_CONN_HOST_ERROR) {
        mysqlsh::current_console()->print_note(shcore::str_format(
            "Could not open connection to '%s': %s, but ignoring it.",
            instance_address.c_str(), e.what()));
        continue;
      } else {
        log_error("Could not open connection to '%s': %s",
                  instance_address.c_str(), e.what());
        throw;
      }
    } catch (const std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }
    bool continue_loop = functor(instance_session, instance_def.second);
    if (!continue_loop) {
      log_debug("Cluster iteration stopped because functor returned false.");
      break;
    }
  }
}

void Cluster_impl::execute_in_read_replicas(
    const std::vector<mysqlshdk::mysql::Read_replica_status> &states,
    const mysqlshdk::db::Connection_options &cnx_opt,
    const std::vector<std::string> &ignore_instances_vector,
    const std::function<bool(const Instance &instance)> &functor,
    bool ignore_network_conn_errors) const {
  std::shared_ptr<Instance> instance_session;
  auto instance_definitions = get_replica_instances();

  for (auto &instance_def : instance_definitions) {
    const std::string &instance_address = instance_def.endpoint;

    // if instance is on the list of instances to be ignored, skip it
    if (std::find_if(ignore_instances_vector.begin(),
                     ignore_instances_vector.end(),
                     mysqlshdk::utils::Endpoint_predicate{instance_address}) !=
        ignore_instances_vector.end()) {
      continue;
    }

    auto target_coptions = mysqlshdk::db::Connection_options(instance_address);

    target_coptions.set_login_options_from(cnx_opt);
    try {
      log_debug(
          "Opening a new session to instance '%s' while iterating "
          "cluster members",
          instance_address.c_str());
      instance_session = Instance::connect(
          target_coptions, current_shell_options()->get().wizards);

      // if state list is given but it doesn't match, skip too
      if (!states.empty() &&
          std::find(states.begin(), states.end(),
                    mysqlshdk::mysql::get_read_replica_status(
                        *instance_session)) == states.end()) {
        continue;
      }
    } catch (const shcore::Error &e) {
      if (ignore_network_conn_errors && e.code() == CR_CONN_HOST_ERROR) {
        mysqlsh::current_console()->print_note(shcore::str_format(
            "Could not open connection to '%s': %s, but ignoring it.",
            instance_address.c_str(), e.what()));
        continue;
      } else {
        log_error("Could not open connection to '%s': %s",
                  instance_address.c_str(), e.what());
        throw;
      }
    } catch (const std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }
    bool continue_loop = functor(*instance_session);
    if (!continue_loop) {
      log_debug("Cluster iteration stopped because functor returned false.");
      break;
    }
  }
}

void Cluster_impl::execute_in_members(
    const std::function<bool(const std::shared_ptr<Instance> &instance,
                             const Instance_md_and_gr_member &info)>
        &on_connect,
    const std::function<bool(const shcore::Error &error,
                             const Instance_md_and_gr_member &info)>
        &on_connect_error) const {
  auto instance_definitions = get_instances_with_state(true);
  auto ipool = current_ipool();

  for (const auto &i : instance_definitions) {
    Scoped_instance instance_session;

    try {
      instance_session =
          Scoped_instance(ipool->connect_unchecked_endpoint(i.first.endpoint));
    } catch (const shcore::Error &e) {
      if (on_connect_error) {
        if (!on_connect_error(e, i)) break;
        continue;
      } else {
        throw;
      }
    }
    if (!on_connect(instance_session, i)) break;
  }
}

void Cluster_impl::execute_in_read_replicas(
    const std::function<bool(const std::shared_ptr<Instance> &,
                             const Instance_metadata &)> &on_connect,
    const std::function<bool(const shcore::Error &, const Instance_metadata &)>
        &on_connect_error) const {
  assert(on_connect || on_connect_error);

  auto instance_definitions = get_replica_instances();
  auto ipool = current_ipool();

  for (const auto &instance_def : instance_definitions) {
    Scoped_instance instance_session;

    try {
      instance_session = Scoped_instance(
          ipool->connect_unchecked_endpoint(instance_def.endpoint));
    } catch (const shcore::Error &e) {
      if (on_connect_error) {
        if (!on_connect_error(e, instance_def)) break;
        continue;
      } else {
        throw;
      }
    }

    if (on_connect && !on_connect(instance_session, instance_def)) break;
  }
}

mysqlshdk::db::Connection_options Cluster_impl::pick_seed_instance() const {
  bool single_primary;
  auto primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      *get_cluster_server(), &single_primary);

  if (!single_primary) {
    // instance we're connected to should be OK if we're multi-master
    return get_cluster_server()->get_connection_options();
  }

  if (primary_uuid.empty())
    throw shcore::Exception::runtime_error(
        "Unable to determine a suitable peer instance to join the group");

  Instance_metadata info =
      get_metadata_storage()->get_instance_by_uuid(primary_uuid);

  mysqlshdk::db::Connection_options coptions(info.endpoint);
  mysqlshdk::db::Connection_options group_session_target(
      get_cluster_server()->get_connection_options());

  coptions.set_login_options_from(group_session_target);

  return coptions;
}

void Cluster_impl::validate_variable_compatibility(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &var_name) const {
  auto cluster_inst = get_cluster_server();
  auto instance_val = target_instance.get_sysvar_string(var_name).value_or("");
  auto cluster_val = cluster_inst->get_sysvar_string(var_name).value_or("");
  std::string instance_address =
      target_instance.get_connection_options().as_uri(
          mysqlshdk::db::uri::formats::only_transport());
  log_info(
      "Validating if '%s' variable has the same value on target instance '%s' "
      "as it does on the cluster.",
      var_name.c_str(), instance_address.c_str());

  // If values are different between cluster and target instance throw an
  // exception.
  if (instance_val == cluster_val) return;

  auto console = mysqlsh::current_console();
  console->print_error(shcore::str_format(
      "Cannot join instance '%s' to cluster: incompatible '%s' value.",
      instance_address.c_str(), var_name.c_str()));
  throw shcore::Exception::runtime_error(
      shcore::str_format("The '%s' value '%s' of the instance '%s' is "
                         "different from the value of the cluster '%s'.",
                         var_name.c_str(), instance_val.c_str(),
                         instance_address.c_str(), cluster_val.c_str()));
}

/**
 * Returns group seeds for all members of the cluster
 *
 * @return (server_uuid, endpoint) map of all group seeds of the cluster
 */
std::map<std::string, std::string> Cluster_impl::get_cluster_group_seeds()
    const {
  std::map<std::string, std::string> group_seeds;

  auto instances = get_metadata_storage()->get_all_instances(get_id());
  for (const auto &i : instances) {
    assert(!i.grendpoint.empty());
    assert(!i.uuid.empty());
    if (i.grendpoint.empty() || i.uuid.empty())
      log_error("Metadata for instance %s is invalid (grendpoint=%s, uuid=%s)",
                i.endpoint.c_str(), i.grendpoint.c_str(), i.uuid.c_str());

    group_seeds[i.uuid] = i.grendpoint;
  }

  return group_seeds;
}

std::string Cluster_impl::get_cluster_group_seeds_list(
    std::string_view skip_uuid) const {
  // Compute group_seeds using local_address from each active cluster member
  std::string ret;
  for (const auto &i : get_cluster_group_seeds()) {
    if (i.first != skip_uuid) {
      if (!ret.empty()) ret.append(",");
      ret.append(i.second);
    }
  }
  return ret;
}

std::vector<Instance_metadata> Cluster_impl::get_instances(
    const std::vector<mysqlshdk::gr::Member_state> &states) const {
  std::vector<Instance_metadata> all_instances =
      get_metadata_storage()->get_all_instances(get_id());

  if (states.empty()) return all_instances;

  std::vector<mysqlshdk::gr::Member> members =
      mysqlshdk::gr::get_members(*get_cluster_server());

  std::vector<Instance_metadata> res;
  res.reserve(all_instances.size());

  for (const auto &i : all_instances) {
    auto m = std::find_if(members.begin(), members.end(),
                          [&i](const mysqlshdk::gr::Member &member) {
                            return member.uuid == i.uuid;
                          });
    if (m != members.end() &&
        std::find(states.begin(), states.end(), m->state) != states.end()) {
      res.push_back(i);
    }
  }
  return res;
}

std::vector<Instance_metadata> Cluster_impl::get_replica_instances() const {
  std::vector<Instance_metadata> all_instances =
      get_metadata_storage()->get_all_instances(get_id(), true);

  std::vector<Instance_metadata> res;
  res.reserve(all_instances.size());

  for (auto &i : all_instances) {
    if (i.instance_type != Instance_type::READ_REPLICA) continue;

    res.push_back(std::move(i));
  }

  return res;
}

std::vector<Instance_metadata> Cluster_impl::get_all_instances() const {
  return get_metadata_storage()->get_all_instances(get_id(), true);
}

std::vector<Instance_metadata> Cluster_impl::get_instances_from_metadata()
    const {
  return get_metadata_storage()->get_all_instances(get_id());
}

void Cluster_impl::ensure_metadata_has_server_id(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  execute_in_members(
      {}, target_instance.get_connection_options(), {},
      [this](const std::shared_ptr<Instance> &instance,
             const mysqlshdk::gr::Member &gr_member) {
        get_metadata_storage()->update_instance_attribute(
            gr_member.uuid, k_instance_attribute_server_id,
            shcore::Value(instance->get_server_id()));

        return true;
      },
      [](const Instance_md_and_gr_member &md) {
        return md.first.server_id == 0 &&
               (md.second.state == mysqlshdk::gr::Member_state::ONLINE ||
                md.second.state == mysqlshdk::gr::Member_state::RECOVERING);
      });
}

std::vector<Instance_metadata> Cluster_impl::get_active_instances_md(
    bool online_only) const {
  if (online_only)
    return get_instances({mysqlshdk::gr::Member_state::ONLINE});
  else
    return get_instances({mysqlshdk::gr::Member_state::ONLINE,
                          mysqlshdk::gr::Member_state::RECOVERING});
}

std::list<std::shared_ptr<mysqlshdk::mysql::IInstance>>
Cluster_impl::get_active_instances(bool online_only) const {
  std::list<std::shared_ptr<mysqlshdk::mysql::IInstance>> active_instances;

  auto active_instances_md = get_active_instances_md(online_only);

  for (const auto &active_instance : active_instances_md) {
    auto potential_source =
        get_session_to_cluster_instance(active_instance.address);
    active_instances.push_back(std::move(potential_source));
  }

  return active_instances;
}

std::shared_ptr<mysqlsh::dba::Instance> Cluster_impl::get_online_instance(
    const std::string &exclude_uuid) const {
  std::vector<Instance_metadata> instances(get_active_instances_md());

  // Get the cluster connection credentials to use to connect to instances.
  mysqlshdk::db::Connection_options cluster_cnx_opts =
      get_cluster_server()->get_connection_options();

  for (const auto &instance : instances) {
    // Skip instance with the provided UUID exception (if specified).
    if (exclude_uuid.empty() || instance.uuid != exclude_uuid) {
      try {
        // Use the cluster connection credentials.
        mysqlshdk::db::Connection_options coptions(instance.endpoint);
        coptions.set_login_options_from(cluster_cnx_opts);

        log_info("Opening session to the member of InnoDB cluster at %s...",
                 coptions.as_uri().c_str());

        // Return the first valid (reachable) instance.
        return Instance::connect(coptions);
      } catch (const std::exception &e) {
        log_debug(
            "Unable to establish a session to the cluster member '%s': %s",
            instance.endpoint.c_str(), e.what());
      }
    }
  }

  // No reachable online instance was found.
  return nullptr;
}

/**
 * Check the instance server UUID and ID of the specified instance.
 *
 * The server UUID and ID must be unique for all instances in a cluster. This
 * function checks if the server_uuid and server_id of the target instance are
 * unique among all members of the cluster.
 *
 * @param instance_session Session to the target instance to check its server
 *                         IDs.
 */
void Cluster_impl::validate_server_ids(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Get the server_uuid of the target instance.
  std::string server_uuid = *target_instance.get_sysvar_string(
      "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get the server_id of the target instance.
  uint32_t server_id = target_instance.get_server_id();

  // Get list of all instances in the metadata (includes instances from all
  // Clusters when in a ClusterSet)
  auto all_instances = get_metadata_storage()->get_all_instances("", true);

  // Get and compare the ids on all instances with the ones of
  // the target instance.
  for (const auto &member : all_instances) {
    if (server_uuid == member.uuid) {
      mysqlsh::current_console()->print_error(
          "The target instance '" + target_instance.get_canonical_address() +
          "' has a 'server_uuid' already being used by instance '" +
          member.endpoint + "'.");

      throw shcore::Exception("Invalid server_uuid.",
                              SHERR_DBA_INVALID_SERVER_UUID);
    }

    if (server_id == member.server_id) {
      mysqlsh::current_console()->print_error(
          "The target instance '" + target_instance.get_canonical_address() +
          "' has a 'server_id' already being used by instance '" +
          member.endpoint + "'.");
      throw shcore::Exception("Invalid server_id.",
                              SHERR_DBA_INVALID_SERVER_ID);
    }
  }
}

std::vector<Cluster_impl::Instance_md_and_gr_member>
Cluster_impl::get_instances_with_state(bool allow_offline) const {
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>> ret;

  std::vector<mysqlshdk::gr::Member> members;

  if (auto server = get_cluster_server(); server) {
    try {
      members = mysqlshdk::gr::get_members(*server);
    } catch (const std::runtime_error &e) {
      if (!allow_offline ||
          !strstr(e.what(), "Group replication does not seem to be"))
        throw;
    }
  }

  std::vector<Instance_metadata> md = get_instances();

  for (const auto &i : md) {
    auto m = std::find_if(members.begin(), members.end(),
                          [&i](const mysqlshdk::gr::Member &member) {
                            return member.uuid == i.uuid;
                          });
    if (m != members.end()) {
      ret.push_back({i, *m});
    } else {
      mysqlshdk::gr::Member mm;
      mm.uuid = i.uuid;
      mm.state = mysqlshdk::gr::Member_state::MISSING;
      ret.push_back({i, mm});
    }
  }

  return ret;
}

void Cluster_impl::iterate_read_replicas(
    const std::function<bool(const Instance_metadata &,
                             const mysqlshdk::mysql::Replication_channel &)>
        &cb) const {
  execute_in_read_replicas(
      [&cb](const std::shared_ptr<Instance> &instance,
            const Instance_metadata &i_md) {
        mysqlshdk::mysql::Replication_channel channel;
        mysqlshdk::mysql::get_channel_status(
            *instance, mysqlsh::dba::k_read_replica_async_channel_name,
            &channel);

        return cb(i_md, channel);
      },
      [&cb](const shcore::Error &, const Instance_metadata &i_md) {
        return cb(i_md, {});
      });
}

std::unique_ptr<mysqlshdk::config::Config> Cluster_impl::create_config_object(
    const std::vector<std::string> &ignored_instances, bool skip_invalid_state,
    bool persist_only, bool best_effort, bool allow_cluster_offline) const {
  auto cfg = std::make_unique<mysqlshdk::config::Config>();

  auto console = mysqlsh::current_console();

  // Get all cluster instances, including state information to update
  // auto-increment values.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instance_defs = get_instances_with_state(allow_cluster_offline);

  for (const auto &instance_def : instance_defs) {
    // If instance is on the list of instances to be ignored, skip it.
    if (std::find_if(ignored_instances.begin(), ignored_instances.end(),
                     mysqlshdk::utils::Endpoint_predicate{
                         instance_def.first.endpoint}) !=
        ignored_instances.end()) {
      continue;
    }

    // Use the GR state hold by instance_def.state (but convert it to a proper
    // mysqlshdk::gr::Member_state to be handled properly).
    mysqlshdk::gr::Member_state state = instance_def.second.state;

    if (best_effort || state == mysqlshdk::gr::Member_state::ONLINE ||
        state == mysqlshdk::gr::Member_state::RECOVERING) {
      // Set login credentials to connect to instance.
      // NOTE: It is assumed that the same login credentials can be used to
      //       connect to all cluster instances.
      Connection_options instance_cnx_opts(instance_def.first.endpoint);
      instance_cnx_opts.set_login_options_from(
          get_cluster_server()->get_connection_options());

      // Try to connect to instance.
      log_debug("Connecting to instance '%s'",
                instance_def.first.endpoint.c_str());
      std::shared_ptr<mysqlsh::dba::Instance> instance;
      try {
        instance = Instance::connect(instance_cnx_opts);
        log_debug("Successfully connected to instance");
      } catch (const shcore::Error &err) {
        if (best_effort) {
          console->print_warning(
              "The settings cannot be updated for instance '" +
              instance_def.first.endpoint + "': " + err.format());
          continue;
        } else {
          log_debug("Failed to connect to instance: %s", err.format().c_str());
          console->print_error(
              "Unable to connect to instance '" + instance_def.first.endpoint +
              "'. Please verify connection credentials and make sure the "
              "instance is available.");

          throw;
        }
      }

      // Determine if SET PERSIST is supported.
      std::optional<bool> support_set_persist =
          instance->is_set_persist_supported();
      mysqlshdk::mysql::Var_qualifier set_type =
          mysqlshdk::mysql::Var_qualifier::GLOBAL;
      bool skip = false;
      if (support_set_persist.value_or(false)) {
        set_type = persist_only ? mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY
                                : mysqlshdk::mysql::Var_qualifier::PERSIST;

      } else {
        // If we want persist_only but it's not supported, we skip it since it
        // can't help us
        if (persist_only) skip = true;
      }

      // Add configuration handler for server.
      if (!skip) {
        cfg->add_handler(
            instance_def.first.endpoint,
            std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                std::make_unique<mysqlshdk::config::Config_server_handler>(
                    instance, set_type)));
      }

      // Print a warning if SET PERSIST is not supported, for users to execute
      // dba.configureInstance().
      if (!support_set_persist.has_value()) {
        console->print_warning(shcore::str_format(
            "Instance '%s' cannot persist configuration since MySQL version %s "
            "does not support the SET PERSIST command (MySQL version >= 8.0.11 "
            "required). Please use the dba.<<<configureInstance>>>() command "
            "locally, using the 'mycnfPath' option, to persist the changes.",
            instance_def.first.endpoint.c_str(),
            instance->get_version().get_base().c_str()));

      } else if (!support_set_persist.value()) {
        console->print_warning(shcore::str_format(
            "Instance '%s' will not load the persisted cluster configuration "
            "upon reboot since 'persisted-globals-load' is set to 'OFF'. "
            "Please use the dba.<<<configureInstance>>>() command locally "
            "(with the config file path) to persist the changes or set "
            "'persisted-globals-load' to 'ON' on the configuration file.",
            instance_def.first.endpoint.c_str()));
      }
    } else {
      // Ignore instance with an invalid state (i.e., do not issue an erro), if
      // skip_invalid_state is true.
      if (skip_invalid_state) continue;

      // Issue an error if the instance is not active.
      console->print_error("The settings cannot be updated for instance '" +
                           instance_def.first.endpoint +
                           "' because it is on a '" +
                           mysqlshdk::gr::to_string(state) + "' state.");

      throw shcore::Exception::runtime_error(
          "The instance '" + instance_def.first.endpoint + "' is '" +
          mysqlshdk::gr::to_string(state) + "'");
    }
  }

  return cfg;
}

void Cluster_impl::query_group_wide_option_values(
    mysqlshdk::mysql::IInstance *target_instance,
    std::optional<std::string> *out_gr_consistency,
    std::optional<int64_t> *out_gr_member_expel_timeout) const {
  auto console = mysqlsh::current_console();

  Option_info<std::string> gr_consistency;
  Option_info<int64_t> gr_member_expel_timeout;

  std::vector<std::string> skip_list;
  skip_list.push_back(target_instance->get_canonical_address());

  // loop though all members to check if there is any member that doesn't:
  // - have support for the group_replication_consistency option (null value)
  // or a member that doesn't have the default value. The default value
  // Eventual has the same behavior as members had before the option was
  // introduced. As such, having that value or having no support for the
  // group_replication_consistency is the same.
  // - have support for the group_replication_member_expel_timeout option
  // (null value) or a member that doesn't have the default value. The default
  // value 0 has the same behavior as members had before the option was
  // introduced. As such, having the 0 value or having no support for the
  // group_replication_member_expel_timeout is the same.
  execute_in_members({mysqlshdk::gr::Member_state::ONLINE,
                      mysqlshdk::gr::Member_state::RECOVERING},
                     get_cluster_server()->get_connection_options(), skip_list,
                     [&gr_consistency, &gr_member_expel_timeout](
                         const std::shared_ptr<Instance> &instance,
                         const mysqlshdk::gr::Member &) {
                       {
                         auto value = instance->get_sysvar_string(
                             "group_replication_consistency",
                             mysqlshdk::mysql::Var_qualifier::GLOBAL);

                         if (!value.has_value()) {
                           gr_consistency.found_not_supported = true;
                         } else if (*value != "EVENTUAL" && *value != "0") {
                           gr_consistency.found_non_default = true;
                           gr_consistency.non_default_value = *value;
                         }
                       }

                       {
                         auto value = instance->get_sysvar_int(
                             "group_replication_member_expel_timeout",
                             mysqlshdk::mysql::Var_qualifier::GLOBAL);

                         if (!value.has_value()) {
                           gr_member_expel_timeout.found_not_supported = true;
                         } else if (*value != 0) {
                           gr_member_expel_timeout.found_non_default = true;
                           gr_member_expel_timeout.non_default_value = *value;
                         }
                       }
                       // if we have found both a instance that doesnt have
                       // support for the option and an instance that doesn't
                       // have the default value, then we don't need to look at
                       // any other instance on the cluster.
                       return !(gr_consistency.found_not_supported &&
                                gr_consistency.found_non_default &&
                                gr_member_expel_timeout.found_not_supported &&
                                gr_member_expel_timeout.found_non_default);
                     });

  if (target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 14)) {
    if (gr_consistency.found_non_default) {
      console->print_warning(
          "The " + gr_consistency.non_default_value +
          " consistency value of the cluster "
          "is not supported by the instance '" +
          target_instance->get_connection_options().uri_endpoint() +
          "' (version >= 8.0.14 is required). In single-primary mode, upon "
          "failover, the member with the lowest version is the one elected as"
          " primary.");
    }
  } else {
    if (out_gr_consistency) *out_gr_consistency = "EVENTUAL";

    if (gr_consistency.found_non_default) {
      // if we found any non default group_replication_consistency value, then
      // we use that value on the instance being added
      if (out_gr_consistency)
        *out_gr_consistency = gr_consistency.non_default_value;

      if (gr_consistency.found_not_supported) {
        console->print_warning(
            "The instance '" +
            target_instance->get_connection_options().uri_endpoint() +
            "' inherited the " + gr_consistency.non_default_value +
            " consistency value from the cluster, however some instances on "
            "the group do not support this feature (version < 8.0.14). In "
            "single-primary mode, upon failover, the member with the lowest "
            "version will be the one elected and it doesn't support this "
            "option.");
      }
    }
  }

  if (target_instance->get_version() < mysqlshdk::utils::Version(8, 0, 13)) {
    if (gr_member_expel_timeout.found_non_default) {
      console->print_warning(
          "The expelTimeout value of the cluster '" +
          std::to_string(gr_member_expel_timeout.non_default_value) +
          "' is not supported by the instance '" +
          target_instance->get_connection_options().uri_endpoint() +
          "' (version >= 8.0.13 is required). A member "
          "that doesn't have support for the expelTimeout option has the "
          "same behavior as a member with expelTimeout=0.");
    }
  } else {
    if (out_gr_member_expel_timeout) *out_gr_member_expel_timeout = 0;

    if (gr_member_expel_timeout.found_non_default) {
      // if we found any non default group_replication_member_expel_timeout
      // value, then we use that value on the instance being added
      if (out_gr_member_expel_timeout)
        *out_gr_member_expel_timeout =
            gr_member_expel_timeout.non_default_value;

      if (gr_member_expel_timeout.found_not_supported) {
        console->print_warning(
            "The instance '" +
            target_instance->get_connection_options().uri_endpoint() +
            "' inherited the '" +
            std::to_string(gr_member_expel_timeout.non_default_value) +
            "' consistency value from the cluster, however some instances on "
            "the group do not support this feature (version < 8.0.13). There "
            "is a possibility that the cluster member (killer node), "
            "responsible for expelling the member suspected of having "
            "failed, does not support the expelTimeout option. In "
            "this case the behavior would be the same as if having "
            "expelTimeout=0.");
      }
    }
  }
}

void Cluster_impl::update_group_members_for_removed_member(
    const std::string &server_uuid, bool dry_run) {
  // Get the Cluster Config Object
  auto cfg = create_config_object({}, true);

  // Iterate through all ONLINE and RECOVERING cluster members and update
  // their group_replication_group_seeds value by removing the
  // gr_local_address of the instance that was removed
  log_debug("Updating group_replication_group_seeds of cluster members");

  auto group_seeds = get_cluster_group_seeds();
  if (group_seeds.find(server_uuid) != group_seeds.end()) {
    group_seeds.erase(server_uuid);
  }
  mysqlshdk::gr::update_group_seeds(cfg.get(), group_seeds);

  if (!dry_run) cfg->apply();

  // Update the auto-increment values
  {
    // Auto-increment values must be updated according to:
    //
    // Set auto-increment for single-primary topology:
    // - auto_increment_increment = 1
    // - auto_increment_offset = 2
    //
    // Set auto-increment for multi-primary topology:
    // - auto_increment_increment = n;
    // - auto_increment_offset = 1 + server_id % n;
    // where n is the size of the GR group if > 7, otherwise n = 7.
    //
    // We must update the auto-increment values in Remove_instance for 2
    // scenarios
    //   - Multi-primary Cluster
    //   - Cluster that had more 7 or more members before the
    //   Remove_instance
    //     operation
    //
    // NOTE: in the other scenarios, the Add_instance operation is in charge
    // of updating auto-increment accordingly

    mysqlshdk::gr::Topology_mode topology_mode =
        get_metadata_storage()->get_cluster_topology_mode(get_id());

    // Get the current number of members of the Cluster
    uint64_t cluster_member_count =
        get_metadata_storage()->get_cluster_size(get_id());

    // we really only need to update auto_increment if we were above 7
    // (otherwise is always 7), but since the member was already removed, the
    // condition must be >= 7 (to catch were be moved from 8 to 7)
    if ((topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) &&
        (cluster_member_count >= 7) && !dry_run) {
      // Call update_auto_increment to do the job in all instances
      mysqlshdk::gr::update_auto_increment(
          cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

      cfg->apply();
    }
  }
}

void Cluster_impl::add_instance(
    const mysqlshdk::db::Connection_options &instance_def,
    const cluster::Add_instance_options &options) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("addInstance")
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .cluster_global_status(Cluster_global_status::OK)
            .build();

    check_preconditions(conds);
  }

  Scoped_instance target(connect_target_instance(instance_def, true, true));

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // put an exclusive lock on the target instance
  auto i_lock = target->get_lock_exclusive();

  Topology_executor<cluster::Add_instance>{this, target, options}.run();

  // Verification step to ensure the server_id is an attribute on all the
  // instances of the cluster
  // TODO(miguel): there should be some tag in the metadata to know whether
  // server_id is already stored or not, to avoid connecting to all members
  // each time to do so. Also, this should probably be done at the beginning.
  ensure_metadata_has_server_id(target);
}

void Cluster_impl::rejoin_instance(
    const Connection_options &instance_def,
    const cluster::Rejoin_instance_options &options) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("rejoinInstance")
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .cluster_global_status(Cluster_global_status::OK)
            .build();

    check_preconditions(conds);
  }

  Scoped_instance target(connect_target_instance(instance_def, true));

  // put a shared lock on the cluster
  auto c_lock = get_lock_shared();

  // put an exclusive lock on the target instance
  auto i_lock = target->get_lock_exclusive();

  if (!is_read_replica(target)) {
    Topology_executor<cluster::Rejoin_instance>{this, target, options}.run();
  } else {
    Topology_executor<cluster::Rejoin_replica_instance>{this, target, options}
        .run();
  }
}

void Cluster_impl::remove_instance(
    const Connection_options &instance_def,
    const cluster::Remove_instance_options &options) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("removeInstance")
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .cluster_global_status(Cluster_global_status::OK)
            .build();

    check_preconditions(conds);
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  bool read_replica = false;
  Scoped_instance target_instance;

  // If the instance is reachable, we can check right away if it is a
  // read-replica to execute its Executor class (Remove_replica_instance).
  // Otherwise, let Remove_instance handle it.
  try {
    target_instance = Scoped_instance(connect_target_instance(instance_def));

    if (is_read_replica(target_instance)) read_replica = true;
  } catch (const std::exception &e) {
    mysqlsh::current_console()->print_warning(
        shcore::str_format("Failed to connect to '%s': %s",
                           instance_def.as_uri().c_str(), e.what()));
  }

  if (read_replica) {
    return Topology_executor<cluster::Remove_replica_instance>{
        this, target_instance, options}
        .run();
  }

  if (target_instance) {
    Topology_executor<cluster::Remove_instance>{this, target_instance, options}
        .run();
  } else {
    Topology_executor<cluster::Remove_instance>{this, instance_def, options}
        .run();
  }
}

std::shared_ptr<Instance> connect_to_instance_for_metadata(
    const mysqlshdk::db::Connection_options &instance_definition) {
  std::string instance_address =
      instance_definition.as_uri(mysqlshdk::db::uri::formats::only_transport());

  log_debug("Connecting to '%s' to query for metadata information...",
            instance_address.c_str());

  std::shared_ptr<Instance> classic;
  try {
    classic = Instance::connect(instance_definition,
                                current_shell_options()->get().wizards);
  } catch (const shcore::Error &e) {
    std::stringstream ss;
    ss << "Error opening session to '" << instance_address << "': " << e.what();
    log_warning("%s", ss.str().c_str());

    // TODO(alfredo) - this should be validated before adding the instance
    // Check if we're adopting a GR cluster, if so, it could happen that
    // we can't connect to it because root@localhost exists but root@hostname
    // doesn't (GR keeps the hostname in the members table)
    if (e.code() == ER_ACCESS_DENIED_ERROR) {
      std::stringstream se;
      se << "Access denied connecting to instance " << instance_address << ".\n"
         << "Please ensure all instances in the same group/cluster have"
            " the same password for account '"
         << instance_definition.get_user()
         << "' and that it is accessible from the host mysqlsh is running "
            "from.";
      throw shcore::Exception::runtime_error(se.str());
    }
    throw shcore::Exception::runtime_error(ss.str());
  }

  return classic;
}

void Cluster_impl::add_metadata_for_instance(
    const mysqlshdk::db::Connection_options &instance_definition,
    Instance_type instance_type, const std::string &label,
    Transaction_undo *undo, bool dry_run) const {
  log_debug("Adding instance to metadata");

  auto instance = connect_to_instance_for_metadata(instance_definition);

  add_metadata_for_instance(*instance, instance_type, label, undo, dry_run);
}

void Cluster_impl::add_metadata_for_instance(
    const mysqlshdk::mysql::IInstance &instance, Instance_type instance_type,
    const std::string &label, Transaction_undo *undo, bool dry_run) const {
  try {
    Instance_metadata instance_md(
        query_instance_info(instance, true, instance_type));

    if (!label.empty()) instance_md.label = label;

    instance_md.cluster_id = get_id();

    // Add the instance to the metadata.
    if (!dry_run) {
      get_metadata_storage()->insert_instance(instance_md, undo);
    }
  } catch (const shcore::Exception &exp) {
    log_error("Failed to add metadata for instance '%s'",
              instance.descr().c_str());

    if (instance_type == Instance_type::GROUP_MEMBER) {
      check_gr_empty_local_address_exception(exp, instance);
    }
    throw;
  }
}

void Cluster_impl::update_metadata_for_instance(
    const mysqlshdk::db::Connection_options &instance_definition,
    Instance_id instance_id, const std::string &label) const {
  auto instance = connect_to_instance_for_metadata(instance_definition);

  update_metadata_for_instance(*instance, instance_id, label);
}

void Cluster_impl::update_metadata_for_instance(
    const mysqlshdk::mysql::IInstance &instance, Instance_id instance_id,
    const std::string &label) const {
  log_debug("Updating instance metadata");

  auto console = mysqlsh::current_console();
  console->print_info("Updating instance metadata...");

  try {
    Instance_metadata instance_md(query_instance_info(instance, true));

    instance_md.cluster_id = get_id();
    instance_md.id = instance_id;
    // note: label is only updated in MD if <> ''
    instance_md.label = label;

    // Updates the instance metadata.
    get_metadata_storage()->update_instance(instance_md);

    console->print_info("The instance metadata for '" +
                        instance.get_canonical_address() +
                        "' was successfully updated.");
    console->print_info();
  } catch (const shcore::Exception &exp) {
    check_gr_empty_local_address_exception(exp, instance);
    throw;
  }
}

void Cluster_impl::remove_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_def) {
  log_debug("Removing instance from metadata");

  get_metadata_storage()->remove_instance(mysqlshdk::utils::make_host_and_port(
      instance_def.get_host(), static_cast<uint16_t>(instance_def.get_port())));
}

void Cluster_impl::configure_cluster_set_member(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance) const {
  try {
    target_instance->set_sysvar("skip_replica_start", true,
                                mysqlshdk::mysql::Var_qualifier::PERSIST_ONLY);
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }

  if (is_primary_cluster()) return;

  auto console = mysqlsh::current_console();
  console->print_info(
      "* Waiting for the Cluster to synchronize with the PRIMARY Cluster...");

  sync_transactions(*get_cluster_server(), k_clusterset_async_channel_name, 0);

  auto cs = get_cluster_set_object();

  auto ar_options = cs->get_clusterset_replication_options(get_id(), nullptr);
  {
    auto account = m_repl_account_mng.refresh_replication_user(
        *get_primary_master(), get_id(), false);
    ar_options.repl_credentials = std::move(account.auth);
  }

  auto repl_source = cs->get_primary_master();

  // Update the credentials on all cluster members
  execute_in_members({}, get_primary_master()->get_connection_options(), {},
                     [&](const std::shared_ptr<Instance> &target,
                         const mysqlshdk::gr::Member &) {
                       async_update_replica_credentials(
                           target.get(), k_clusterset_async_channel_name,
                           ar_options, false);
                       return true;
                     });

  // Setup the replication channel at the target instance but do not start
  // it since that's handled by Group Replication
  console->print_info(
      "* Configuring ClusterSet managed replication channel...");

  async_add_replica(repl_source.get(), target_instance.get(),
                    k_clusterset_async_channel_name, ar_options, true, false,
                    false);

  console->print_info();
}

void Cluster_impl::change_recovery_credentials_all_members(
    const mysqlshdk::mysql::Auth_options &repl_account) const {
  execute_in_members(
      [&repl_account](const std::shared_ptr<Instance> &instance,
                      const Instance_md_and_gr_member &info) {
        if (info.second.state != mysqlshdk::gr::Member_state::ONLINE)
          return true;

        try {
          mysqlshdk::mysql::Replication_credentials_options options;
          options.password = repl_account.password.value_or("");

          mysqlshdk::mysql::change_replication_credentials(
              *instance, mysqlshdk::gr::k_gr_recovery_channel,
              repl_account.user, options);
        } catch (const std::exception &e) {
          current_console()->print_warning(shcore::str_format(
              "Error updating recovery credentials for %s: %s",
              instance->descr().c_str(), e.what()));
        }

        return true;
      },
      [](const shcore::Error &, const Instance_md_and_gr_member &) {
        return true;
      });
}

void Cluster_impl::update_group_peers(
    const mysqlshdk::mysql::IInstance &target_instance,
    const Group_replication_options &gr_options, int cluster_member_count,
    const std::string &self_address, bool group_seeds_only) {
  assert(cluster_member_count > 0);

  // Get the gr_address of the instance being added
  std::string added_instance_gr_address = *gr_options.local_address;

  // Create a configuration object for the cluster, ignoring the added
  // instance, to update the remaining cluster members.
  // NOTE: only members that already belonged to the cluster and are either
  //       ONLINE or RECOVERING will be considered.
  std::vector<std::string> ignore_instances_vec = {self_address};
  std::unique_ptr<mysqlshdk::config::Config> cluster_cfg =
      create_config_object(ignore_instances_vec, true);

  // Update the group_replication_group_seeds of the cluster members
  // by adding the gr_local_address of the instance that was just added.
  log_debug("Updating Group Replication seeds on all active members...");
  {
    auto group_seeds = get_cluster_group_seeds();
    assert(!added_instance_gr_address.empty());
    group_seeds[target_instance.get_uuid()] = added_instance_gr_address;
    mysqlshdk::gr::update_group_seeds(cluster_cfg.get(), group_seeds);
  }

  cluster_cfg->apply();

  if (!group_seeds_only) {
    // Increase the cluster_member_count counter
    cluster_member_count++;

    // Auto-increment values must be updated according to:
    //
    // Set auto-increment for single-primary topology:
    // - auto_increment_increment = 1
    // - auto_increment_offset = 2
    //
    // Set auto-increment for multi-primary topology:
    // - auto_increment_increment = n;
    // - auto_increment_offset = 1 + server_id % n;
    // where n is the size of the GR group if > 7, otherwise n = 7.
    //
    // We must update the auto-increment values in Cluster_join for 2
    // scenarios
    //   - Multi-primary Cluster
    //   - Cluster that has 7 or more members after the Cluster_join
    //     operation
    //
    // NOTE: in the other scenarios, the Cluster_join operation is in
    // charge of updating auto-increment accordingly

    // Get the topology mode of the replicaSet
    mysqlshdk::gr::Topology_mode topology_mode =
        get_metadata_storage()->get_cluster_topology_mode(get_id());

    if (topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY &&
        cluster_member_count > 7) {
      log_debug("Updating auto-increment settings on all active members...");
      mysqlshdk::gr::update_auto_increment(
          cluster_cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY,
          cluster_member_count);
      cluster_cfg->apply();
    }
  }
}

void Cluster_impl::check_instance_configuration(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    bool using_clone_recovery, checks::Check_type checks_type,
    bool already_member) const {
  // Check instance version compatibility with cluster.
  if (checks_type != checks::Check_type::READ_REPLICA) {
    cluster_topology_executor_ops::
        ensure_instance_check_installed_schema_version(
            target_instance, get_lowest_instance_version());
  }

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  ensure_gr_instance_configuration_valid(target_instance.get(), true,
                                         using_clone_recovery);

  // Validate the lower_case_table_names and default_table_encryption
  // variables. Their values must be the same on the target instance as they
  // are on the cluster.

  // The lower_case_table_names can only be set the first time the server
  // boots, as such there is no need to validate it other than the first time
  // the instance is added to the cluster.
  validate_variable_compatibility(*target_instance, "lower_case_table_names");

  // The default_table_encryption is a dynamic variable, so we validate it on
  // the Cluster_join and on the rejoin operation. The reboot operation does a
  // rejoin in the background, so running the check on the rejoin will cover
  // both operations.
  if (get_lowest_instance_version() >= mysqlshdk::utils::Version(8, 0, 16) &&
      target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 16)) {
    validate_variable_compatibility(*target_instance,
                                    "default_table_encryption");
  }

  // Verify if the instance is running asynchronous
  // replication
  // NOTE: Verify for all operations: addInstance(), rejoinInstance() and
  // rebootClusterFromCompleteOutage()
  if (checks_type == checks::Check_type::READ_REPLICA) {
    checks::validate_async_channels(
        *target_instance, {k_read_replica_async_channel_name}, checks_type);
  } else {
    checks::validate_async_channels(
        *target_instance,
        is_cluster_set_member()
            ? std::unordered_set<std::string>{k_clusterset_async_channel_name}
            : std::unordered_set<std::string>{},
        checks_type);
  }

  if ((checks_type == checks::Check_type::JOIN ||
       checks_type == checks::Check_type::READ_REPLICA) &&
      !already_member) {
    // Check instance server UUID and ID (must be unique among the cluster
    // members).
    validate_server_ids(*target_instance);
  }
}

shcore::Value Cluster_impl::describe() {
  {
    auto conds = Command_conditions::Builder::gen_cluster("describe")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .primary_not_required()
                     .allowed_on_fence()
                     .build();

    check_preconditions(conds);
  }

  return cluster_describe();
}

shcore::Value Cluster_impl::cluster_describe() {
  // Create the Cluster_describe command and execute it.
  cluster::Describe op_describe(*this);
  // Always execute finish when leaving "try catch".
  auto finally =
      shcore::on_leave_scope([&op_describe]() { op_describe.finish(); });
  // Prepare the Cluster_describe command execution (validations).
  op_describe.prepare();
  // Execute Cluster_describe operations.
  return op_describe.execute();
}

Cluster_status Cluster_impl::cluster_status(int *out_num_failures_tolerated,
                                            int *out_num_failures) const {
  int total_members =
      static_cast<int>(m_metadata_storage->get_cluster_size(get_id()));

  if (!m_primary_master) {
    if (out_num_failures_tolerated) *out_num_failures_tolerated = 0;
    if (out_num_failures) *out_num_failures = total_members;

    switch (cluster_availability()) {
      case Cluster_availability::ONLINE:
      case Cluster_availability::ONLINE_NO_PRIMARY:
        // this can happen if we're in a clusterset and didn't acquire the
        // global primary
        break;
      case Cluster_availability::OFFLINE:
        return Cluster_status::OFFLINE;
      case Cluster_availability::NO_QUORUM:
        return Cluster_status::NO_QUORUM;
      case Cluster_availability::SOME_UNREACHABLE:
      case Cluster_availability::UNREACHABLE:
        return Cluster_status::UNKNOWN;
    }
  }

  auto target = get_cluster_server();

  bool single_primary = false;
  bool has_quorum = false;

  // If the primary went OFFLINE for some reason, we must use the target
  // instance to obtain the group membership info.
  //
  // A possible scenario on which that occur is when the primary is
  // auto-rejoining. other members may not have noticed yet so they still
  // consider it as the primary from their point of view. However, the instance
  // itself is still OFFLINE so the group membership information cannot be
  // queried.
  if (!mysqlshdk::gr::is_active_member(*target) ||
      mysqlshdk::gr::is_running_gr_auto_rejoin(*target) ||
      mysqlshdk::gr::is_group_replication_delayed_starting(*target)) {
    target = get_cluster_server();
  }

  std::vector<mysqlshdk::gr::Member> members;

  try {
    members = mysqlshdk::gr::get_members(*target, &single_primary, &has_quorum,
                                         nullptr);
  } catch (const std::exception &e) {
    if (shcore::str_beginswith(
            e.what(), "Group replication does not seem to be active")) {
      return Cluster_status::OFFLINE;
    } else {
      throw;
    }
  }

  int num_online = 0;
  for (const auto &m : members) {
    if (m.state == mysqlshdk::gr::Member_state::ONLINE) {
      num_online++;
    }
  }

  int number_of_failures_tolerated = num_online > 0 ? (num_online - 1) / 2 : 0;

  if (out_num_failures_tolerated) {
    *out_num_failures_tolerated = number_of_failures_tolerated;
  }
  if (out_num_failures) *out_num_failures = total_members - num_online;

  if (!has_quorum) {
    return Cluster_status::NO_QUORUM;
  } else {
    if (num_online > 0 && is_fenced_from_writes()) {
      return Cluster_status::FENCED_WRITES;
    } else if (total_members > num_online) {
      // partial, some members are not online
      if (number_of_failures_tolerated == 0) {
        return Cluster_status::OK_NO_TOLERANCE_PARTIAL;
      } else {
        return Cluster_status::OK_PARTIAL;
      }
    } else {
      if (number_of_failures_tolerated == 0) {
        return Cluster_status::OK_NO_TOLERANCE;
      } else {
        return Cluster_status::OK;
      }
    }
  }
}

shcore::Value Cluster_impl::cluster_status(int64_t extended) {
  cluster::Status op_status(shared_from_this(), extended);
  shcore::on_leave_scope finally([&op_status]() { op_status.finish(); });
  op_status.prepare();
  return op_status.execute();
}

shcore::Value Cluster_impl::status(int64_t extended) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("status")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .primary_not_required()
                     .allowed_on_fence()
                     .build();

    check_preconditions(conds);
  }

  return cluster_status(extended);
}

shcore::Value Cluster_impl::list_routers(bool only_upgrade_required) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("listRouters")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .primary_not_required()
                     .allowed_on_fence()
                     .build();

    check_preconditions(conds);
  }

  if (is_cluster_set_member()) {
    current_console()->print_error(
        "Cluster '" + get_name() + "' is a member of ClusterSet '" +
        get_cluster_set_object()->get_name() +
        "', use <ClusterSet>.<<<listRouters>>>() to list "
        "the ClusterSet Router instances");
    throw shcore::Exception::runtime_error(
        "Function not available for ClusterSet members");
  }

  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(get_name());
  (*dict)["routers"] = router_list(get_metadata_storage().get(), get_id(),
                                   only_upgrade_required);

  return shcore::Value(dict);
}

void Cluster_impl::reset_recovery_password(std::optional<bool> force,
                                           const bool interactive) {
  {
    auto conds = Command_conditions::Builder::gen_cluster(
                     "resetRecoveryAccountsPassword")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .quorum_state(ReplicationQuorum::States::Normal)
                     .primary_required()
                     .cluster_global_status_any_ok()
                     .build();

    check_preconditions(conds);
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the reset_recovery_command.
  cluster::Reset_recovery_accounts_password op_reset(interactive, force, *this);

  // Always execute finish
  shcore::on_leave_scope finally([&op_reset]() { op_reset.finish(); });

  // prepare and execute
  op_reset.prepare();
  op_reset.execute();
}

void Cluster_impl::enable_super_read_only_globally() const {
  auto console = mysqlsh::current_console();
  // Enable super_read_only on the primary member
  auto primary = get_cluster_server();

  if (!primary->get_sysvar_bool("super_read_only", false)) {
    console->print_info("* Enabling super_read_only on '" + primary->descr() +
                        "'...");
    primary->set_sysvar("super_read_only", true);
  }

  // Enable super_read_only on the remaining members
  using mysqlshdk::gr::Member_state;
  execute_in_members({Member_state::ONLINE, Member_state::RECOVERING},
                     get_cluster_server()->get_connection_options(),
                     {primary->descr()},
                     [=](const std::shared_ptr<Instance> &instance,
                         const mysqlshdk::gr::Member &) {
                       console->print_info("* Enabling super_read_only on '" +
                                           instance->descr() + "'...");
                       instance->set_sysvar("super_read_only", true);
                       return true;
                     });
}

void Cluster_impl::fence_all_traffic() {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("fenceAllTraffic")
            .min_mysql_version(Precondition_checker::k_min_cs_version)
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .allowed_on_fence()
            .build();

    check_preconditions(conds);
  }

  auto console = mysqlsh::current_console();
  console->print_info("The Cluster '" + get_name() +
                      "' will be fenced from all traffic");
  console->print_info();

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // To fence all traffic, the command must:
  //  1. enable super_read_only and offline on the primary member
  //  2. enable offline_mode and stop Group Replication on all secondaries
  //  3. stop Group Replication on the primary

  // Enable super_read_only and off_line on the primary member
  auto primary = get_cluster_server();
  console->print_info("* Enabling super_read_only on the primary '" +
                      primary->descr() + "'...");
  primary->set_sysvar("super_read_only", true);

  console->print_info("* Enabling offline_mode on the primary '" +
                      primary->descr() + "'...");
  primary->set_sysvar("offline_mode", true);

  // Enable super_read_only, offline_mode and stop GR on all secondaries
  using mysqlshdk::gr::Member_state;
  execute_in_members(
      {Member_state::ONLINE, Member_state::RECOVERING},
      get_cluster_server()->get_connection_options(),
      {get_cluster_server()->descr()},
      [&console](const std::shared_ptr<Instance> &instance,
                 const mysqlshdk::gr::Member &) {
        // Every secondary should be already super_read_only
        if (!instance->get_sysvar_bool("super_read_only", false)) {
          console->print_info("* Enabling super_read_only on '" +
                              instance->descr() + "'...");
          instance->set_sysvar("super_read_only", true);
        }

        console->print_info("* Enabling offline_mode on '" + instance->descr() +
                            "'...");
        instance->set_sysvar("offline_mode", true);

        console->print_info("* Stopping Group Replication on '" +
                            instance->descr() + "'...");
        mysqlshdk::gr::stop_group_replication(*instance);

        return true;
      });

  execute_in_read_replicas(
      {mysqlshdk::mysql::Read_replica_status::CONNECTING,
       mysqlshdk::mysql::Read_replica_status::ONLINE,
       mysqlshdk::mysql::Read_replica_status::OFFLINE,
       mysqlshdk::mysql::Read_replica_status::ERROR},
      get_cluster_server()->get_connection_options(), {},
      [&console](const Instance &instance) {
        console->print_info("* Enabling offline_mode on '" + instance.descr() +
                            "'...");
        instance.set_sysvar("offline_mode", true);

        console->print_info("* Stopping replication on '" + instance.descr() +
                            "'...");
        stop_channel(instance, k_read_replica_async_channel_name, true, false);
        return true;
      });

  // Stop Group Replication on the primary member
  console->print_info("* Stopping Group Replication on the primary '" +
                      primary->descr() + "'...");
  mysqlshdk::gr::stop_group_replication(*primary);

  console->print_info();
  console->print_info("Cluster successfully fenced from all traffic");
}

void Cluster_impl::fence_writes() {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("fenceWrites")
            .min_mysql_version(Precondition_checker::k_min_cs_version)
            .target_instance(TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .build();

    check_preconditions(conds);
  }

  auto console = mysqlsh::current_console();

  console->print_info("The Cluster '" + get_name() +
                      "' will be fenced from write traffic");
  console->print_info();

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Disable the GR member action mysql_disable_super_read_only_if_primary
  auto primary = get_cluster_server();

  console->print_info(
      "* Disabling automatic super_read_only management on the Cluster...");
  mysqlshdk::gr::disable_member_action(
      *primary, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
      mysqlshdk::gr::k_gr_member_action_after_primary_election);

  // Enable super_read_only on the primary member first and after all remaining
  // members
  enable_super_read_only_globally();

  console->print_info();

  if (is_cluster_set_member() && is_primary_cluster()) {
    console->print_note(
        "Applications will now be blocked from performing writes on Cluster '" +
        get_name() +
        "'. Use <Cluster>.<<<unfenceWrites>>>() to resume writes if you're "
        "certain a split-brain is not in effect.");
  }

  console->print_info();
  console->print_info("Cluster successfully fenced from write traffic");
}

void Cluster_impl::unfence_writes() {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("unfenceWrites")
            .min_mysql_version(Precondition_checker::k_min_cs_version)
            .target_instance(TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .allowed_on_fence()
            .build();

    check_preconditions(conds);
  }

  auto console = mysqlsh::current_console();

  console->print_info("The Cluster '" + get_name() +
                      "' will be unfenced from write traffic");
  console->print_info();

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  auto primary = get_cluster_server();

  // Check if the Cluster belongs to a ClusterSet and is a REPLICA Cluster, it
  // must be forbidden to unfence from write traffic REPLICA Clusters.
  if (is_cluster_set_member() && !is_primary_cluster()) {
    console->print_error(
        "Unable to unfence Cluster from write traffic: operation not permitted "
        "on REPLICA Clusters");

    throw shcore::Exception(
        shcore::str_format(
            "The Cluster '%s' is a REPLICA Cluster of the ClusterSet '%s'",
            get_name().c_str(), get_cluster_set_object()->get_name().c_str()),
        SHERR_DBA_UNSUPPORTED_CLUSTER_TYPE);
  }

  // Check if the Cluster is fenced from Write traffic
  if (!is_fenced_from_writes()) {
    throw shcore::Exception("Cluster not fenced to write traffic",
                            SHERR_DBA_CLUSTER_NOT_FENCED);
  }

  // Check if offline_mode is enabled and disable it on all members
  using mysqlshdk::gr::Member_state;
  execute_in_members({Member_state::ONLINE, Member_state::RECOVERING},
                     get_cluster_server()->get_connection_options(), {},
                     [&console](const std::shared_ptr<Instance> &instance,
                                const mysqlshdk::gr::Member &) {
                       if (instance->get_sysvar_bool("offline_mode", false)) {
                         console->print_info("* Disabling 'offline_mode' on '" +
                                             instance->descr() + "'...");
                         instance->set_sysvar("offline_mode", false);
                       }
                       return true;
                     });

  // Enable the GR member action mysql_disable_super_read_only_if_primary
  console->print_info(
      "* Enabling automatic super_read_only management on the Cluster...");
  mysqlshdk::gr::enable_member_action(
      *primary, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
      mysqlshdk::gr::k_gr_member_action_after_primary_election);

  // Disable super_read_only on the primary member
  if (primary->get_sysvar_bool("super_read_only", false)) {
    console->print_info("* Disabling super_read_only on the primary '" +
                        primary->descr() + "'...");
    primary->set_sysvar("super_read_only", false);
  }

  console->print_info();
  console->print_info("Cluster successfully unfenced from write traffic");
}

bool Cluster_impl::is_fenced_from_writes() const {
  if (!m_cluster_server) {
    return false;
  }

  auto primary = m_cluster_server;

  // Fencing is only supported for versions >= 8.0.27 with the introduction of
  // ClusterSet
  if (primary->get_version() < Precondition_checker::k_min_cs_version) {
    return false;
  }

  // If Cluster is a standalone Cluster or a PRIMARY Cluster it cannot be fenced
  if (!is_cluster_set_member() ||
      (is_cluster_set_member() && !is_primary_cluster())) {
    return false;
  }

  if (!mysqlshdk::gr::is_active_member(*primary)) {
    return false;
  }

  if (!mysqlshdk::gr::is_primary(*primary)) {
    // Try to get the primary member, if we cannot return false right away since
    // we cannot determine whether the cluster is fenced or not
    try {
      primary = get_primary_member_from_group(primary);
    } catch (...) {
      log_info(
          "Failed to get primary member from cluster '%s' using the session "
          "from '%s': %s",
          get_name().c_str(), primary->descr().c_str(),
          format_active_exception().c_str());
      return false;
    }
  }

  // Check if GR group action 'mysql_disable_super_read_only_if_primary' is
  // enabled
  bool disable_sro_if_primary_enabled = false;
  try {
    mysqlshdk::gr::get_member_action_status(
        *primary, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
        &disable_sro_if_primary_enabled);
  } catch (...) {
    log_debug(
        "Failed to get status of GR member action "
        "'mysql_disable_super_read_only_if_primary' from '%s': %s",
        primary->descr().c_str(), format_active_exception().c_str());
    return false;
  }

  if (disable_sro_if_primary_enabled) {
    return false;
  }

  // Check if SRO is enabled on the primary member
  if (!primary->get_sysvar_bool("super_read_only", false)) {
    return false;
  }

  return true;
}

shcore::Value Cluster_impl::create_cluster_set(
    const std::string &domain_name,
    const clusterset::Create_cluster_set_options &options) {
  try {
    // TODO(anyone): All of the preconditions quorum related checks are done
    // using ReplicationQuorum::State which should eventually be replaced by
    // Cluster_status, i.e. to satisfy WL12805-FR2.1
    auto conds =
        Command_conditions::Builder::gen_cluster("createClusterSet")
            .min_mysql_version(Precondition_checker::k_min_cs_version)
            .target_instance(TargetType::InnoDBCluster)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0)
            .primary_required()
            .build();

    check_preconditions(conds);

  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
      // NOTE: This overrides the original quorum error, it was implemented this
      // way in Create_cluster_set operation
      current_console()->print_error(
          "Target cluster status is 'NO_QUORUM' which is not valid for "
          "InnoDB ClusterSet.");
      throw shcore::Exception("Invalid Cluster status: NO_QUORUM.",
                              SHERR_DBA_CLUSTER_STATUS_INVALID);
    } else {
      throw;
    }
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  return Cluster_set_impl::create_cluster_set(this, domain_name, options);
}

std::shared_ptr<ClusterSet> Cluster_impl::get_cluster_set() {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("getClusterSet")
            .min_mysql_version(Precondition_checker::k_min_cs_version)
            .target_instance(TargetType::InnoDBClusterSet)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_1_0)
            .primary_not_required()
            .allowed_on_fence()
            .build();

    check_preconditions(conds);
  }

  auto cs = get_cluster_set_object();

  // Check if the target Cluster is invalidated
  if (is_invalidated()) {
    mysqlsh::current_console()->print_warning(
        "Cluster '" + get_name() +
        "' was INVALIDATED and must be removed from the ClusterSet.");
  }

  // Check if the target Cluster still belongs to the ClusterSet
  Cluster_set_member_metadata cluster_member_md;
  if (!cs->get_metadata_storage()->get_cluster_set_member(get_id(),
                                                          &cluster_member_md)) {
    current_console()->print_error("The Cluster '" + get_name() +
                                   "' appears to have been removed from "
                                   "the ClusterSet '" +
                                   cs->get_name() +
                                   "', however its own metadata copy wasn't "
                                   "properly updated during the removal");

    throw shcore::Exception("The cluster '" + get_name() +
                                "' is not a part of the ClusterSet '" +
                                cs->get_name() + "'",
                            SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET);
  }

  return std::make_shared<ClusterSet>(cs);
}

void Cluster_impl::check_cluster_online() {
  std::string role = is_primary_cluster() ? "Primary Cluster" : "Cluster";

  switch (cluster_availability()) {
    case Cluster_availability::ONLINE:
      break;
    case Cluster_availability::ONLINE_NO_PRIMARY:
      throw shcore::Exception(
          "Could not connect to PRIMARY of " + role + " '" + get_name() + "'",
          SHERR_DBA_CLUSTER_PRIMARY_UNAVAILABLE);

    case Cluster_availability::OFFLINE:
      throw shcore::Exception(
          "All members of " + role + " '" + get_name() + "' are OFFLINE",
          SHERR_DBA_GROUP_OFFLINE);

    case Cluster_availability::SOME_UNREACHABLE:
      throw shcore::Exception(
          "All reachable members of " + role + " '" + get_name() +
              "' are OFFLINE, but there are some unreachable members that "
              "could be ONLINE",
          SHERR_DBA_GROUP_OFFLINE);

    case Cluster_availability::NO_QUORUM:
      throw shcore::Exception("Could not connect to an ONLINE member of " +
                                  role + " '" + get_name() + "' within quorum",
                              SHERR_DBA_GROUP_HAS_NO_QUORUM);

    case Cluster_availability::UNREACHABLE:
      throw shcore::Exception("Could not connect to any ONLINE member of " +
                                  role + " '" + get_name() + "'",
                              SHERR_DBA_GROUP_UNREACHABLE);
  }
}

std::shared_ptr<Cluster_set_impl> Cluster_impl::get_cluster_set_object(
    bool print_warnings, bool check_status) const {
  if (m_cs_md_remove_pending) {
    throw shcore::Exception("Cluster is not part of a ClusterSet",
                            SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET);
  }

  if (auto ptr = m_cluster_set.lock(); ptr) {
    if (check_status) ptr->get_primary_cluster()->check_cluster_online();
    return ptr;
  }

  // NOTE: The operation must work even if the PRIMARY cluster is not reachable,
  // as long as the target instance is a reachable member of an InnoDB Cluster
  // that is part of a ClusterSet.
  Cluster_metadata cluster_md = get_metadata();

  Cluster_set_metadata cset_md;
  auto metadata = get_metadata_storage();

  // Get the ClusterSet metadata handle
  if (!metadata->get_cluster_set(cluster_md.cluster_set_id, false, &cset_md,
                                 nullptr)) {
    throw shcore::Exception("No metadata found for the ClusterSet that " +
                                get_cluster_server()->descr() + " belongs to.",
                            SHERR_DBA_MISSING_FROM_METADATA);
  }

  auto cs = std::make_shared<Cluster_set_impl>(cset_md, get_cluster_server(),
                                               metadata);

  // Acquire primary
  cs->acquire_primary(true, check_status);

  try {
    // Verify the Primary Cluster's availability
    cs->get_primary_cluster()->check_cluster_online();
  } catch (const shcore::Exception &e) {
    if (check_status) throw;

    if (e.code() == SHERR_DBA_GROUP_HAS_NO_PRIMARY ||
        e.code() == SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING) {
      log_warning("Could not connect to any member of the PRIMARY Cluster: %s",
                  e.what());
      if (print_warnings) {
        current_console()->print_warning(
            "Could not connect to any member of the PRIMARY Cluster, topology "
            "changes will not be allowed");
      }
    } else if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
      log_warning("PRIMARY Cluster doesn't have quorum: %s", e.what());
      if (print_warnings) {
        auto console = current_console();
        console->print_warning(
            "The PRIMARY Cluster lost the quorum, topology changes will "
            "not be allowed");
        console->print_note(
            "To restore the Cluster and ClusterSet operations, restore the "
            "quorum on the PRIMARY Cluster using "
            "<<<forceQuorumUsingPartitionOf>>>()");
      }
    }
  }

  // Return a handle (Cluster_set_impl) to the ClusterSet the target Cluster
  // belongs to
  return cs;
}

std::optional<std::string> Cluster_impl::get_comm_stack() const {
  if (shcore::Value value; get_metadata_storage()->query_cluster_capability(
                               get_id(), kCommunicationStack, &value) &&
                           (value.get_type() == shcore::Map)) {
    return value.as_map()->get_string("value");
  }

  return {};
}

void Cluster_impl::refresh_connections() {
  get_metadata_storage()->get_md_server()->reconnect_if_needed("Metadata");

  if (m_primary_master) {
    m_primary_master->reconnect_if_needed("Primary");
  }

  if (m_cluster_server) {
    m_cluster_server->reconnect_if_needed("Cluster");
  }
}

void Cluster_impl::ensure_compatible_clone_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient) const {
  {
    auto donor_address = donor.get_canonical_address();

    // check if the donor belongs to the cluster (MD)
    try {
      get_metadata_storage()->get_instance_by_address(donor_address, get_id());
    } catch (const shcore::Exception &e) {
      if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

      throw shcore::Exception(
          shcore::str_format("Instance '%s' does not belong to the Cluster",
                             donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }

    // check donor state
    if (mysqlshdk::gr::get_member_state(donor) !=
        mysqlshdk::gr::Member_state::ONLINE) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' is not an ONLINE member of the Cluster.",
              donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
    }
  }

  // further checks (related to the instances and unrelated to the cluster)
  Base_cluster_impl::check_compatible_clone_donor(donor, recipient);
}

void Cluster_impl::setup_admin_account(const std::string &username,
                                       const std::string &host,
                                       const Setup_account_options &options) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("setupAdminAccount")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .quorum_state(ReplicationQuorum::States::Normal)
                     .primary_required()
                     .cluster_global_status(Cluster_global_status::OK)
                     .build();

    check_preconditions(conds);
  }

  // put a shared lock on the cluster
  auto c_lock = get_lock_shared();

  Base_cluster_impl::setup_admin_account(username, host, options);
}

void Cluster_impl::setup_router_account(const std::string &username,
                                        const std::string &host,
                                        const Setup_account_options &options) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("setupRouterAccount")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .quorum_state(ReplicationQuorum::States::Normal)
                     .primary_required()
                     .cluster_global_status(Cluster_global_status::OK)
                     .build();

    check_preconditions(conds);
  }

  // put a shared lock on the cluster
  auto c_lock = get_lock_shared();

  Base_cluster_impl::setup_router_account(username, host, options);
}

shcore::Value Cluster_impl::options(const bool all) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("options")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .primary_not_required()
                     .allowed_on_fence()
                     .build();

    check_preconditions(conds);
  }

  // Create the Cluster_options command and execute it.
  cluster::Options op_option(*this, all);
  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally([&op_option]() { op_option.finish(); });
  // Prepare the Cluster_options command execution (validations).
  op_option.prepare();

  // Execute Cluster_options operations.
  return op_option.execute();
}

void Cluster_impl::dissolve(std::optional<bool> force, const bool interactive) {
  // We need to check if the group has quorum and if not we must abort the
  // operation otherwise GR blocks the writes to preserve the consistency
  // of the group and we end up with a hang.
  // This check is done at check_preconditions()
  try {
    auto conds =
        Command_conditions::Builder::gen_cluster("dissolve")
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .allowed_on_fence()
            .build();

    check_preconditions(conds);
  } catch (const shcore::Exception &e) {
    // special case for dissolving a cluster that was removed from a clusterset
    // but its local metadata couldn't be updated
    if (e.code() == SHERR_DBA_CLUSTER_ALREADY_IN_CLUSTERSET &&
        m_cs_md_remove_pending) {
      log_info(
          "Dissolving cluster '%s' which was removed from a clusterset without "
          "a local metadata update.",
          get_name().c_str());
    } else {
      throw;
    }
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the Dissolve command and execute it.
  cluster::Dissolve op_dissolve(interactive, force, this);

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally([&op_dissolve]() { op_dissolve.finish(); });

  // Prepare the dissolve command execution (validations).
  op_dissolve.prepare();
  // Execute dissolve operations.
  op_dissolve.execute();
}

void Cluster_impl::force_quorum_using_partition_of(
    const Connection_options &instance_def, const bool interactive) {
  Scoped_instance target_instance{
      connect_target_instance(instance_def, true, true)};

  {
    auto conds =
        Command_conditions::Builder::gen_cluster("forceQuorumUsingPartitionOf")
            .target_instance(TargetType::GroupReplication,
                             TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_not_required()
            .build();

    check_preconditions(conds);
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  std::vector<Instance_metadata> read_replicas = get_replica_instances();

  {
    std::vector<Instance_metadata> online_instances =
        get_active_instances_md(false);

    if (online_instances.empty()) {
      throw shcore::Exception::logic_error(
          "No online instances are visible from the given one.");
    }

    if (interactive) {
      const auto console = current_console();

      auto group_peers_print =
          shcore::str_join(online_instances.begin(), online_instances.end(),
                           ",", [](const auto &i) { return i.endpoint; });

      // Remove the trailing comma of group_peers_print
      if (group_peers_print.back() == ',') group_peers_print.pop_back();

      console->print_info(
          shcore::str_format("Restoring cluster '%s' from loss of quorum, by "
                             "using the partition composed of [%s]\n",
                             get_name().c_str(), group_peers_print.c_str()));

      console->print_info("Restoring the InnoDB cluster ...");
      console->print_info();
    }
  }

  // TODO(miguel): test if there's already quorum and add a 'force' option to
  // be used if so

  // TODO(miguel): test if the instance if part of the current cluster, for
  // the scenario of restoring a cluster quorum from another

  std::string instance_address =
      instance_def.as_uri(mysqlshdk::db::uri::formats::only_transport());

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  {
    // Get instance address in metadata.
    std::string md_address = target_instance->get_canonical_address();

    // Check if the instance belongs to the Cluster on the Metadata
    if (!contains_instance_with_address(md_address)) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "The instance '%s' does not belong to the cluster: '%s'.",
          instance_address.c_str(), get_name().c_str()));
    }

    if (!validate_cluster_group_name(*target_instance, get_group_name())) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "The instance '%s' cannot be used to restore the cluster as it may "
          "belong to a different cluster as the one registered in the Metadata "
          "since the value of 'group_replication_group_name' does not match "
          "the one registered in the cluster's Metadata: possible split-brain "
          "scenario.",
          instance_address.c_str()));
    }
  }

  // Get the instance state
  Cluster_check_info state;

  auto instance_type = get_gr_instance_type(*target_instance);

  if (instance_type != TargetType::Standalone &&
      instance_type != TargetType::StandaloneWithMetadata &&
      instance_type != TargetType::StandaloneInMetadata) {
    state = get_replication_group_state(*target_instance, instance_type);

    if (state.source_state != ManagedInstance::OnlineRW &&
        state.source_state != ManagedInstance::OnlineRO) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "The instance '%s' cannot be used to restore the cluster as it is on "
          "a %s state, and should be ONLINE",
          instance_address.c_str(),
          ManagedInstance::describe(
              static_cast<ManagedInstance::State>(state.source_state))
              .c_str()));
    }
  } else {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "The instance '%s' cannot be used to restore the cluster as it is not "
        "an active member of replication group.",
        instance_address.c_str()));
  }

  // Check if there is quorum to issue an error.
  if (mysqlshdk::gr::has_quorum(*target_instance, nullptr, nullptr)) {
    mysqlsh::current_console()->print_error(
        "Cannot perform operation on an healthy cluster because it can only "
        "be used to restore a cluster from quorum loss.");

    throw shcore::Exception::runtime_error(
        shcore::str_format("The cluster has quorum according to instance '%s'",
                           instance_address.c_str()));
  }

  // Get the all instances from MD and members visible by the target instance.
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
      instances_info = get_instances_with_state();

  std::string group_peers;
  int online_count = 0;

  for (auto &instance : instances_info) {
    std::string instance_host = instance.first.endpoint;
    auto target_coptions = mysqlshdk::db::Connection_options(instance_host);
    // We assume the login credentials are the same on all instances
    target_coptions.set_login_options_from(
        target_instance->get_connection_options());

    std::shared_ptr<Instance> instance_session;
    try {
      log_info(
          "Opening a new session to a group_peer instance to obtain "
          "group_replication_local_address: %s",
          instance_host.c_str());
      instance_session = Instance::connect(
          target_coptions, current_shell_options()->get().wizards);

      if (instance.second.state == mysqlshdk::gr::Member_state::ONLINE ||
          instance.second.state == mysqlshdk::gr::Member_state::RECOVERING) {
        // Add GCS address of active instance to the force quorum membership.
        std::string group_peer_instance_xcom_address =
            instance_session
                ->get_sysvar_string("group_replication_local_address")
                .value_or("");

        group_peers.append(group_peer_instance_xcom_address);
        group_peers.append(",");

        online_count++;
      } else {
        // Stop GR on not active instances.
        mysqlshdk::gr::stop_group_replication(*instance_session);
      }
    } catch (const std::exception &e) {
      log_error("Could not open connection to %s: %s", instance_address.c_str(),
                e.what());

      // Only throw errors if the instance is active, otherwise ignore it.
      if (instance.second.state == mysqlshdk::gr::Member_state::ONLINE ||
          instance.second.state == mysqlshdk::gr::Member_state::RECOVERING) {
        throw;
      }
    }
  }

  if (online_count == 0) {
    throw shcore::Exception::logic_error(
        "No online instances are visible from the given one.");
  }

  // Stop Replication on all ONLINE Read-Replicas
  std::vector<
      std::pair<std::shared_ptr<mysqlsh::dba::Instance>, Instance_metadata>>
      online_read_replicas;

  for (const auto &rr : read_replicas) {
    std::shared_ptr<Instance> rr_instance;
    try {
      rr_instance = get_session_to_cluster_instance(rr.endpoint);
    } catch (const std::exception &) {
      log_info("Unable to connect to '%s'", rr.endpoint.c_str());
      continue;
    }

    if (!rr_instance) continue;

    // Stop the channel
    // NOTE: attempt to stop all replicas regardless of their state, i.e.
    // ONLINE, CONNECTING, ERROR OR OFFLINE.
    try {
      stop_channel(*rr_instance, k_read_replica_async_channel_name, true,
                   false);
      log_info(
          "Successfully stopped replication channel of Read-Replica at "
          "'%s'",
          rr.endpoint.c_str());
    } catch (std::exception &e) {
      mysqlsh::current_console()->print_warning(
          shcore::str_format("Failed to stop replication channel on '%s': '%s'",
                             rr.endpoint.c_str(), e.what()));
    }

    // Add to the list of online read_replicas
    online_read_replicas.push_back(std::make_pair(std::move(rr_instance), rr));
  }

  // Force the reconfiguration of the GR group
  {
    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',') group_peers.pop_back();

    log_info("Setting group_replication_force_members at instance %s",
             instance_address.c_str());

    // Setting the group_replication_force_members will force a new group
    // membership, triggering the necessary actions from GR upon being set to
    // force the quorum. Therefore, the variable can be cleared immediately
    // after it is set.
    target_instance->set_sysvar("group_replication_force_members", group_peers,
                                mysqlshdk::mysql::Var_qualifier::GLOBAL);

    // Clear group_replication_force_members at the end to allow GR to be
    // restarted later on the instance (without error).
    target_instance->set_sysvar("group_replication_force_members",
                                std::string(),
                                mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }

  // If this is a REPLICA Cluster of a ClusterSet, ensure SRO is enabled on all
  // members
  if (is_cluster_set_member() && !is_primary_cluster()) {
    auto cs = get_cluster_set_object();
    cs->ensure_replica_settings(this, false);
  }

  // Restart the replication channel of the Read-Replicas
  for (const auto &rr : online_read_replicas) {
    cluster::Replication_sources replication_sources =
        get_read_replica_replication_sources(rr.second.uuid);

    using mysqlshdk::gr::Member_state;
    execute_in_members(
        {Member_state::ONLINE, Member_state::RECOVERING, Member_state::OFFLINE},
        get_cluster_server()->get_connection_options(), {},
        [&rr, &replication_sources](const std::shared_ptr<Instance> &instance,
                                    const mysqlshdk::gr::Member &) {
          // If not a potential source of the read-replica skip it (if primary
          // or secondary, we must check all members since all of them are
          // potential sources)
          if (replication_sources.source_type == cluster::Source_type::CUSTOM) {
            Managed_async_channel_source managed_src_address(
                instance->get_canonical_address());
            if (std::find(replication_sources.replication_sources.begin(),
                          replication_sources.replication_sources.end(),
                          managed_src_address) ==
                replication_sources.replication_sources.end()) {
              return true;
            }
          }

          check_compatible_replication_sources(*instance, *rr.first);
          return true;
        });

    try {
      start_channel(*rr.first, k_read_replica_async_channel_name, false);
      log_info(
          "Successfully restarted replication channel of Read-Replica at '%s'",
          rr.second.endpoint.c_str());
    } catch (std::exception &e) {
      mysqlsh::current_console()->print_warning(shcore::str_format(
          "Failed to restart replication channel on '%s': '%s'",
          rr.second.endpoint.c_str(), e.what()));
    }
  }

  if (interactive) {
    const auto console = current_console();

    console->print_info(shcore::str_format(
        "The InnoDB cluster was successfully restored using the partition from "
        "the instance '%s'.",
        instance_def.as_uri(mysqlshdk::db::uri::formats::user_transport())
            .c_str()));

    console->print_info();
    console->print_info(
        "WARNING: To avoid a split-brain scenario, ensure that all other "
        "members of the cluster are removed or joined back to the group that "
        "was restored.");
    console->print_info();
  }
}

void Cluster_impl::rescan(const cluster::Rescan_options &options) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("rescan")
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .cluster_global_status(Cluster_global_status::OK)
            .build();

    check_preconditions(conds);
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the rescan command and execute it.
  cluster::Rescan op_rescan(options, this);

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally([&op_rescan]() { op_rescan.finish(); });

  // Prepare the rescan command execution (validations).
  op_rescan.prepare();

  // Execute rescan operation.
  op_rescan.execute();
}

bool Cluster_impl::get_disable_clone_option() const {
  shcore::Value value;
  if (m_metadata_storage->query_cluster_attribute(
          get_id(), k_cluster_attribute_disable_clone, &value)) {
    return value.as_bool();
  }
  return false;
}

void Cluster_impl::set_disable_clone_option(const bool disable_clone) {
  m_metadata_storage->update_cluster_attribute(
      get_id(), k_cluster_attribute_disable_clone,
      disable_clone ? shcore::Value::True() : shcore::Value::False());
}

bool Cluster_impl::get_manual_start_on_boot_option() const {
  shcore::Value flag;
  if (m_metadata_storage->query_cluster_attribute(
          get_id(), k_cluster_attribute_manual_start_on_boot, &flag))
    return flag.as_bool();
  // default false
  return false;
}

const std::string Cluster_impl::get_view_change_uuid() const {
  shcore::Value view_change_uuid;

  if (m_metadata_storage->query_cluster_attribute(
          get_id(), "group_replication_view_change_uuid", &view_change_uuid)) {
    return view_change_uuid.as_string();
  }

  // default empty
  return "";
}

void Cluster_impl::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("switchToSinglePrimaryMode")
            .target_instance(TargetType::InnoDBCluster)
            .quorum_state(ReplicationQuorum::States::All_online)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .build();

    check_preconditions(conds);
  }

  // Switch to single-primary mode

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the Switch_to_single_primary_mode object and execute it.
  cluster::Switch_to_single_primary_mode op_switch_to_single_primary_mode(
      instance_def, this);

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally([&op_switch_to_single_primary_mode]() {
    op_switch_to_single_primary_mode.finish();
  });

  // Prepare the Switch_to_single_primary_mode command execution
  // (validations).
  op_switch_to_single_primary_mode.prepare();

  // Execute Switch_to_single_primary_mode operation.
  op_switch_to_single_primary_mode.execute();
}

void Cluster_impl::switch_to_multi_primary_mode() {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("switchToMultiPrimaryMode")
            .target_instance(TargetType::InnoDBCluster)
            .quorum_state(ReplicationQuorum::States::All_online)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_0_0)
            .primary_required()
            .build();

    check_preconditions(conds);
  }

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the Switch_to_multi_primary_mode object and execute it.
  cluster::Switch_to_multi_primary_mode op_switch_to_multi_primary_mode(this);

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally([&op_switch_to_multi_primary_mode]() {
    op_switch_to_multi_primary_mode.finish();
  });

  // Prepare the Switch_to_multi_primary_mode command execution (validations).
  op_switch_to_multi_primary_mode.prepare();

  // Execute Switch_to_multi_primary_mode operation.
  op_switch_to_multi_primary_mode.execute();
}

void Cluster_impl::set_primary_instance(
    const Connection_options &instance_def,
    const cluster::Set_primary_instance_options &options) {
  {
    // In case of a Cluster, because we have to have all members online, the
    // primary is automatically also available. But in case of a ClusterSet,
    // this refers to the set's primary, which isn't required

    auto conds = Command_conditions::Builder::gen_cluster("setPrimaryInstance")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .quorum_state(ReplicationQuorum::States::All_online)
                     .primary_not_required()
                     .build();

    check_preconditions(conds);
  }

  // Set primary instance

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // Create the Set_primary_instance object and execute it.
  cluster::Set_primary_instance op_set_primary_instance(instance_def, this,
                                                        options);

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally(
      [&op_set_primary_instance]() { op_set_primary_instance.finish(); });

  // Prepare the Set_primary_instance command execution (validations).
  op_set_primary_instance.prepare();

  // Execute Set_primary_instance operation.
  op_set_primary_instance.execute();
}

shcore::Value Cluster_impl::execute(
    const std::string &cmd, const shcore::Value &instances,
    const shcore::Option_pack_ref<Execute_options> &options) {
  {
    auto conds = Command_conditions::Builder::gen_cluster("execute")
                     .target_instance(TargetType::InnoDBCluster,
                                      TargetType::InnoDBClusterSet)
                     .primary_not_required()
                     .allowed_on_fence()
                     .build();

    check_preconditions(conds);
  }

  auto c_lock = get_lock_shared();

  return Topology_executor<Execute>{*this, options->dry_run}.run(
      cmd, Execute::convert_to_instances_def(instances, false),
      options->exclude, std::chrono::seconds{options->timeout});
}

mysqlshdk::utils::Version Cluster_impl::get_lowest_instance_version(
    std::vector<std::string> *out_instances_addresses) const {
  std::string current_instance_address;
  // Get the server version of the current cluster instance and initialize
  // the lowest cluster session.
  mysqlshdk::utils::Version lowest_version =
      get_cluster_server()->get_version();

  // Get address of the cluster instance to skip it in the next step.
  std::string cluster_instance_address =
      get_cluster_server()->get_canonical_address();

  // Get the lowest server version from the available cluster instances.
  execute_in_members({mysqlshdk::gr::Member_state::ONLINE,
                      mysqlshdk::gr::Member_state::RECOVERING},
                     get_cluster_server()->get_connection_options(),
                     {cluster_instance_address},
                     [&lowest_version, &out_instances_addresses](
                         const std::shared_ptr<Instance> &instance,
                         const mysqlshdk::gr::Member &) {
                       mysqlshdk::utils::Version version =
                           instance->get_version();

                       if (version < lowest_version) {
                         lowest_version = version;

                         if (out_instances_addresses) {
                           out_instances_addresses->push_back(
                               instance->get_canonical_address());
                         }
                       }
                       return true;
                     });

  return lowest_version;
}

std::pair<Async_replication_options, std::string>
Cluster_impl::create_read_replica_replication_user(
    mysqlshdk::mysql::IInstance *target, std::string_view auth_cert_subject,
    int sync_timeout, bool dry_run) const {
  assert(target);

  Async_replication_options ar_options;

  // Set CONNECTION_RETRY_INTERVAL and CONNECTION_RETRY_COUNT
  ar_options.connect_retry = k_read_replica_master_connect_retry;
  ar_options.retry_count = k_read_replica_master_retry_count;

  // Enable SOURCE_CONNECTION_AUTO_FAILOVER
  ar_options.auto_failover = true;

  // Get the Cluster's SSL mode and auth type
  ar_options.ssl_mode =
      to_cluster_ssl_mode(get_cluster_server()->get_sysvar_string(
          "group_replication_ssl_mode", ""));
  ar_options.auth_type = query_cluster_auth_type();

  // Create replication account
  std::string repl_account_host;
  {
    auto repl_account = m_repl_account_mng.create_replication_user(
        *target, auth_cert_subject,
        Replication_account::Account_type::READ_REPLICA, false, {}, true,
        dry_run);

    ar_options.repl_credentials = std::move(repl_account.auth);
    repl_account_host = std::move(repl_account.host);
  }

  // If the Cluster belongs to a ClusterSet sync with the primary Cluster
  // to ensure the replication account that was just re-created was already
  // replicated to the targer Cluster, otherwise, the read-replica channel
  // can fail to start with an authentication error
  if (is_cluster_set_member() && !is_primary_cluster() && !dry_run) {
    auto console = mysqlsh::current_console();
    console->print_info();
    console->print_info(
        "* Waiting for the Cluster to synchronize with the PRIMARY "
        "Cluster...");

    try {
      sync_transactions(*get_cluster_server(), k_clusterset_async_channel_name,
                        sync_timeout);
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_GTID_SYNC_TIMEOUT) {
        console->print_warning(
            "The Cluster failed to synchronize its transaction set "
            "with the PRIMARY Cluster. You may increase or disable the "
            "transaction sync timeout with the option 'timeout'");
      }
    }
  }

  return {ar_options, repl_account_host};
}

void Cluster_impl::setup_read_replica_connection_failover(
    const Instance &replica, const Instance &source,
    cluster::Replication_sources replication_sources, bool rejoin,
    bool dry_run) {
  try {
    // Delete any previous managed connection failover configurations
    delete_managed_connection_failover(
        replica, k_read_replica_async_channel_name, dry_run);

    // Setup the new managed connection failover if no replicationSources was
    // given or it was set to "primary"
    if (replication_sources.source_type == cluster::Source_type::PRIMARY) {
      add_managed_connection_failover(
          replica, source, k_read_replica_async_channel_name, "",
          k_read_replica_higher_weight, k_read_replica_lower_weight, dry_run);
    } else if (replication_sources.source_type ==
               cluster::Source_type::SECONDARY) {
      add_managed_connection_failover(
          replica, source, k_read_replica_async_channel_name, "",
          k_read_replica_lower_weight, k_read_replica_higher_weight, dry_run);
    } else if (replication_sources.source_type ==
               cluster::Source_type::CUSTOM) {
      // Get the current configuration
      Managed_async_channel channel_config;
      if (rejoin &&
          get_managed_connection_failover_configuration(replica,
                                                        &channel_config) &&
          !channel_config.sources.empty() &&
          channel_config.managed_name.empty()) {
        // Get all sources that do not belong to the configured ones in the
        // Metadata
        std::set<Managed_async_channel_source,
                 std::greater<Managed_async_channel_source>>
            sources_not_in_md;

        std::set_difference(
            channel_config.sources.begin(), channel_config.sources.end(),
            replication_sources.replication_sources.begin(),
            replication_sources.replication_sources.end(),
            std::inserter(sources_not_in_md, sources_not_in_md.end()),
            std::greater<Managed_async_channel_source>{});

        // Clean-up the sources that don't belong to the MD. Those are the
        // instances in channel_config.sources that don't belong to
        // replication_sources.replication_sources
        for (const auto &src : sources_not_in_md) {
          log_info("Deleting replication source '%s' from '%s'",
                   src.to_string().c_str(), replica.descr().c_str());
          delete_source_connection_failover(replica, src.host, src.port,
                                            k_read_replica_async_channel_name,
                                            "", dry_run);
        }

        // Add unconfigured sources
        for (const auto &src : replication_sources.replication_sources) {
          // Check if the source is already configured, if so leave it.
          if (std::find_if(
                  channel_config.sources.begin(), channel_config.sources.end(),
                  Managed_async_channel_source::Predicate_weight(src)) ==
              channel_config.sources.end()) {
            log_info("Adding replication source '%s' to '%s'",
                     src.to_string().c_str(), replica.descr().c_str());
            add_source_connection_failover(replica, src.host, src.port,
                                           k_read_replica_async_channel_name,
                                           "", src.weight, dry_run);
          }
        }
      } else {
        // Delete any previous managed connection failover configurations
        delete_managed_connection_failover(
            replica, k_read_replica_async_channel_name, dry_run);

        // Add all sources
        for (const auto &src : replication_sources.replication_sources) {
          log_info("Adding replication source '%s' to '%s'",
                   src.to_string().c_str(), replica.descr().c_str());
          add_source_connection_failover(replica, src.host, src.port,
                                         k_read_replica_async_channel_name, "",
                                         src.weight, dry_run);
        }
      }
    }
  } catch (...) {
    log_error("Error setting up connection failover at %s: %s",
              replica.descr().c_str(), format_active_exception().c_str());
    throw;
  }
}

void Cluster_impl::setup_read_replica(
    Instance *replica, const Async_replication_options &ar_options,
    cluster::Replication_sources replication_sources, bool rejoin,
    bool dry_run) {
  log_info("Configuring '%s' as a Read-Replica of Cluster '%s'",
           replica->descr().c_str(), get_name().c_str());

  auto ipool = current_ipool();
  Scoped_instance source;

  if (rejoin &&
      replication_sources.source_type != cluster::Source_type::CUSTOM) {
    stop_channel(*replica, k_read_replica_async_channel_name, true, dry_run);
    reset_managed_connection_failover(*replica, dry_run);
  }

  try {
    if (replication_sources.source_type == cluster::Source_type::PRIMARY) {
      // The source must be the primary
      source = Scoped_instance(get_cluster_server());
      source->retain();
    } else if (replication_sources.source_type ==
               cluster::Source_type::SECONDARY) {
      // Get an online member to use as source, preferably a secondary
      auto instances_md = get_active_instances_md();

      for (const auto &instance : instances_md) {
        // Skip the primary only if there are more than 1 ONLINE members,
        // otherwise, the primary must be use
        if (instances_md.size() > 1 &&
            instance.uuid == get_cluster_server()->get_uuid()) {
          continue;
        }

        try {
          log_info(
              "Establishing a session to the InnoDB Cluster member '%s'...",
              instance.endpoint.c_str());

          // Return the first valid (reachable) instance.
          source = Scoped_instance(
              ipool->connect_unchecked_endpoint(instance.endpoint));

          break;
        } catch (const std::exception &e) {
          log_debug(
              "Unable to establish a session to the Cluster member '%s': %s",
              instance.endpoint.c_str(), e.what());
        }
      }

      // Check if source is valid. It might not be if for some reaason we
      // weren't able to connect to any of the online members
      if (!source) {
        mysqlsh::current_console()->print_error(
            "Unable to connect to any Cluster member to be used as source.");
        throw shcore::Exception(
            "No reachable members available",
            SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_NOT_ONLINE);
      }
    } else if (replication_sources.source_type ==
                   cluster::Source_type::CUSTOM &&
               !replication_sources.replication_sources.empty()) {
      // The source must be the first member of the 'replicationSources'
      auto first_element = *(replication_sources.replication_sources.begin());

      std::string source_str = first_element.to_string();

      // Try to connect to instance.
      log_debug("Connecting to instance '%s'", source_str.c_str());

      try {
        source = Scoped_instance(ipool->connect_unchecked_endpoint(source_str));
        log_debug("Successfully connected to instance");
      } catch (const shcore::Error &err) {
        mysqlsh::current_console()->print_error(
            "Unable to use '" + source_str +
            "' as a source, instance is unreachable: " + err.format());

        throw shcore::Exception(
            "Source is not reachable",
            SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_UNREACHABLE);
      }
    }

    async_change_primary(replica, source.get(),
                         k_read_replica_async_channel_name, ar_options, true,
                         dry_run);

    log_info("Replication source of '%s' changed to '%s'",
             replica->descr().c_str(), source->descr().c_str());
  } catch (...) {
    mysqlsh::current_console()->print_error(
        "Error updating replication source for " + replica->descr() + ": " +
        format_active_exception());

    throw shcore::Exception(
        "Could not update replication source of " + replica->descr(),
        SHERR_DBA_READ_REPLICA_SETUP_ERROR);
  }

  log_info("Configuring the managed connection failover configurations for %s",
           replica->descr().c_str());

  setup_read_replica_connection_failover(*replica, *source, replication_sources,
                                         rejoin, dry_run);
}

void Cluster_impl::validate_replication_sources(
    const std::set<Managed_async_channel_source,
                   std::greater<Managed_async_channel_source>> &sources,
    const mysqlshdk::mysql::IInstance &target_instance, bool is_rejoin,
    std::vector<std::string> *out_sources_canonical_address) const {
  auto target_instance_address = target_instance.get_canonical_address();
  auto target_instance_uuid = target_instance.get_uuid();

  // Check if the replicationSources are reachable and ONLINE cluster members
  for (const auto &source : sources) {
    std::shared_ptr<Instance> source_instance;
    std::string source_string = source.to_string();

    try {
      source_instance = get_session_to_cluster_instance(source_string);
    } catch (const shcore::Exception &) {
      // We can't connect to the instance so it's not a valid source
      auto basic_error = shcore::str_format(
          "Unable to use '%s' as source, instance is unreachable.",
          source_string.c_str());

      if (!is_rejoin)
        mysqlsh::current_console()->print_error(basic_error);
      else
        mysqlsh::current_console()->print_error(shcore::str_format(
            "Unable to rejoin Read-Replica '%s' to the Cluster: %s Use "
            "Cluster.setInstanceOption() with the option 'replicationSources' "
            "to update the instance's sources.",
            target_instance_address.c_str(), basic_error.c_str()));

      throw shcore::Exception(
          "Source is not reachable",
          SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_UNREACHABLE);
    }

    // can't be a replication source of itself
    if (target_instance_uuid == source_instance->get_uuid()) {
      auto basic_error = shcore::str_format(
          "Unable to use '%s' as source for itself.", source_string.c_str());

      if (!is_rejoin)
        mysqlsh::current_console()->print_error(basic_error);
      else
        mysqlsh::current_console()->print_error(shcore::str_format(
            "Unable to rejoin Read-Replica '%s' to the Cluster: %s Use "
            "Cluster.setInstanceOption() with the option 'replicationSources' "
            "to update the instance's sources.",
            target_instance_address.c_str(), basic_error.c_str()));

      throw shcore::Exception(
          "Source is invalid",
          SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_ITSELF);
    }

    Instance_metadata instance_md;

    try {
      // Check if the address is in the Metadata

      // Use the canonical address, since that's the one stored in the MD
      auto source_canononical_address =
          source_instance->get_canonical_address();

      instance_md = get_metadata_storage()->get_instance_by_address(
          source_canononical_address, get_id());

      if (out_sources_canonical_address) {
        out_sources_canonical_address->push_back(source_canononical_address);
      }
    } catch (const shcore::Exception &) {
      auto basic_error = shcore::str_format(
          "Unable to use '%s' as source, instance does not belong to the "
          "Cluster.",
          source_string.c_str());

      if (!is_rejoin)
        mysqlsh::current_console()->print_error(basic_error);
      else
        mysqlsh::current_console()->print_error(shcore::str_format(
            "Unable to rejoin Read-Replica '%s' to the Cluster: %s Use "
            "Cluster.setInstanceOption() with the option 'replicationSources' "
            "to update the instance's sources.",
            target_instance_address.c_str(), basic_error.c_str()));

      throw shcore::Exception(
          "Source does not belong to the Cluster",
          SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_NOT_IN_MD);
    }

    // Check if the instance is a Read-Replica
    if (instance_md.instance_type == Instance_type::READ_REPLICA) {
      auto basic_error = shcore::str_format(
          "Unable to use '%s', which is a Read-Replica, as source.",
          source_string.c_str());

      if (!is_rejoin)
        mysqlsh::current_console()->print_error(basic_error);
      else
        mysqlsh::current_console()->print_error(shcore::str_format(
            "Unable to rejoin Read-Replica '%s' to the Cluster: %s Use "
            "Cluster.setInstanceOption() with the option 'replicationSources' "
            "to update the instance's sources.",
            target_instance_address.c_str(), basic_error.c_str()));

      throw shcore::Exception(
          "Source cannot be a Read-Replica",
          SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_NOT_CLUSTER_MEMBER);
    }

    auto state = mysqlshdk::gr::get_member_state(*source_instance);

    if (state != mysqlshdk::gr::Member_state::ONLINE) {
      auto basic_error = shcore::str_format(
          "Unable to use '%s' as source, instance's state is '%s'.",
          source_string.c_str(), to_string(state).c_str());

      if (!is_rejoin)
        mysqlsh::current_console()->print_error(basic_error);
      else
        mysqlsh::current_console()->print_error(shcore::str_format(
            "Unable to rejoin Read-Replica '%s' to the Cluster: %s Use "
            "Cluster.setInstanceOption() with the option 'replicationSources' "
            "to update the instance's sources.",
            target_instance_address.c_str(), basic_error.c_str()));

      throw shcore::Exception(
          "Source is not ONLINE",
          SHERR_DBA_READ_REPLICA_INVALID_SOURCE_LIST_NOT_ONLINE);
    }

    // Verify the versions compatibility
    check_compatible_replication_sources(*source_instance, target_instance);
  }
}

cluster::Replication_sources Cluster_impl::get_read_replica_replication_sources(
    const std::string &replica_uuid) const {
  cluster::Replication_sources replication_sources_opts;
  replication_sources_opts.replication_sources =
      std::set<Managed_async_channel_source,
               std::greater<Managed_async_channel_source>>{};

  if (shcore::Value source_attribute;
      get_metadata_storage()->query_instance_attribute(
          replica_uuid, k_instance_attribute_read_replica_replication_sources,
          &source_attribute)) {
    if (source_attribute.get_type() == shcore::String) {
      auto string = source_attribute.as_string();

      if (shcore::str_caseeq(string, kReplicationSourcesAutoPrimary)) {
        replication_sources_opts.source_type = cluster::Source_type::PRIMARY;
      } else if (shcore::str_caseeq(string, kReplicationSourcesAutoSecondary)) {
        replication_sources_opts.source_type = cluster::Source_type::SECONDARY;
      } else {
        // Should never happen, unless the Metadata is edited manually
        throw shcore::Exception("Invalid metadata for Read-Replica found",
                                SHERR_DBA_METADATA_INCONSISTENT);
      }
    } else if (source_attribute.get_type() == shcore::Array) {
      replication_sources_opts.source_type = cluster::Source_type::CUSTOM;

      uint64_t src_weight = k_read_replica_max_weight;

      for (const auto &v : *source_attribute.as_array()) {
        std::string src_string = v.as_string();

        replication_sources_opts.replication_sources.emplace(
            Managed_async_channel_source(src_string, src_weight));

        // When 100 read-replicas were added, the following ones will have the
        // weight of 1. Group Replication's range of weights goes from 100 to 1
        // but we should not limit the number of read-replicas
        if (src_weight != 1) src_weight--;
      }
    }
  }

  return replication_sources_opts;
}

void Cluster_impl::ensure_compatible_donor(
    const mysqlshdk::mysql::IInstance &donor,
    const mysqlshdk::mysql::IInstance &recipient,
    Member_recovery_method recovery_method) {
  if (recovery_method == Member_recovery_method::CLONE) {
    ensure_compatible_clone_donor(donor, recipient);
  } else {
    // Check if the source is valid
    auto donor_address = donor.get_canonical_address();

    // check if the source belongs to the cluster (MD)
    try {
      get_metadata_storage()->get_instance_by_address(donor_address, get_id());
    } catch (const shcore::Exception &e) {
      if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

      throw shcore::Exception(
          shcore::str_format("Instance '%s' does not belong to the Cluster",
                             donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER);
    }

    // check source state
    if (mysqlshdk::gr::get_member_state(donor) !=
        mysqlshdk::gr::Member_state::ONLINE) {
      throw shcore::Exception(
          shcore::str_format(
              "Instance '%s' is not an ONLINE member of the Cluster.",
              donor_address.c_str()),
          SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE);
    }
  }
}

void Cluster_impl::reset_recovery_password(
    const std::shared_ptr<Instance> &target,
    const std::string &recovery_account,
    const std::vector<std::string> &hosts) const {
  bool needs_password{false};
  switch (auto auth_type = query_cluster_auth_type(); auth_type) {
    case Replication_auth_type::PASSWORD:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      needs_password = true;
      break;
    default:
      break;
  }

  // Get the recovery account for the target instance
  Replication_account::User_hosts user_hosts;
  user_hosts.user = recovery_account;
  user_hosts.hosts = hosts;

  if (user_hosts.user.empty() && user_hosts.hosts.empty()) {
    user_hosts = m_repl_account_mng.get_replication_user(*target, false);
  }

  log_info("Resetting recovery account credentials for '%s'%s.",
           target->descr().c_str(),
           needs_password ? " (with password)" : " (without password)");

  // Generate and set a new password and update the replication credentials
  mysqlshdk::mysql::Replication_credentials_options options;
  if (needs_password) {
    std::string password;
    mysqlshdk::mysql::set_random_password(
        *get_primary_master(), user_hosts.user, user_hosts.hosts, &password);

    options.password = std::move(password);
  }

  mysqlshdk::mysql::change_replication_credentials(
      *target, mysqlshdk::gr::k_gr_recovery_channel, user_hosts.user, options);
}

bool Cluster_impl::contains_instance_with_address(
    const std::string &host_port) const {
  return get_metadata_storage()->is_instance_on_cluster(get_id(), host_port);
}

/*
 * Ensures the cluster object (and metadata) can perform update operations and
 * returns a session to the PRIMARY.
 *
 * For a cluster to be updatable, it's necessary that:
 * - the MD object is connected to the PRIMARY of the
 * primary cluster, so that the MD can be updated (and is also not lagged)
 * - the primary of the cluster is reachable, so that cluster accounts
 * can be created there.
 *
 * An exception is thrown if not possible to connect to the primary.
 *
 * An Instance object connected to the primary is returned. The session
 * is owned by the cluster object.
 */
mysqlsh::dba::Instance *Cluster_impl::acquire_primary(
    bool primary_required, bool check_primary_status) {
  if (!m_cluster_server) {
    return nullptr;
  }

  // clear cached clusterset related metadata
  m_cs_md.cluster_set_id.clear();

  // Get the Metadata state and return right away if the Metadata is
  // nonexisting, upgrading, or the setup failed
  //
  // TODO(anyone) In such scenarios, there will be a double-check for the
  // Metadata state. This should be refactored.
  switch (m_metadata_storage->state()) {
    case metadata::State::FAILED_SETUP:
    case metadata::State::FAILED_UPGRADE:
    case metadata::State::NONEXISTING:
    case metadata::State::UPGRADING:
      return m_cluster_server.get();
    default:
      break;
  }

  // Ensure m_cluster_server points to the PRIMARY of the group
  std::string primary_url;
  try {
    primary_url = find_primary_member_uri(m_cluster_server, false, nullptr);

    if (primary_url.empty()) {
      // Might happen and it should be possible to check the Cluster's status
      // in such situation
      if (primary_required)
        current_console()->print_info("No PRIMARY member found for cluster '" +
                                      get_name() + "'");
    } else if (!mysqlshdk::utils::are_endpoints_equal(
                   primary_url,
                   m_cluster_server->get_connection_options().uri_endpoint())) {
      mysqlshdk::db::Connection_options copts(primary_url);
      log_info("Connecting to %s...", primary_url.c_str());
      copts.set_login_options_from(m_cluster_server->get_connection_options());

      auto new_primary = Instance::connect(copts);

      // Check if GR is active or not. If not active or in error state, throw
      // an exception and leave the attributes unchanged. This may happen when
      // a failover is happening at the moment
      auto instance_state = mysqlshdk::gr::get_member_state(*new_primary);

      if (instance_state == mysqlshdk::gr::Member_state::OFFLINE ||
          instance_state == mysqlshdk::gr::Member_state::ERROR) {
        primary_url.clear();
        throw shcore::Exception("PRIMARY instance is not ONLINE",
                                SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
      } else {
        m_cluster_server = new_primary;
        m_primary_master = m_cluster_server;
      }
    } else {
      if (!m_primary_master) {
        m_primary_master = m_cluster_server;
      }
    }
  } catch (...) {
    if (!primary_url.empty() && primary_required) {
      current_console()->print_error(
          "A connection to the PRIMARY instance at " + primary_url +
          " could not be established to perform this action.");
    }

    throw;
  }

  // Check if the Cluster belongs to a ClusterSet
  if (is_cluster_set_member()) {
    invalidate_cluster_set_metadata_cache();

    auto cs = get_cluster_set_object(true, check_primary_status);

    if (auto cs_primary_master = cs->get_primary_master(); cs_primary_master) {
      // Check if the ClusterSet Primary is different than the Cluster's one
      auto cs_primary_master_url =
          cs_primary_master->get_connection_options().uri_endpoint();

      // The ClusterSet Primary is different, establish a connection to it to
      // set it as the Cluster's primary
      if (!m_primary_master ||
          m_primary_master->get_connection_options().uri_endpoint() !=
              cs_primary_master_url) {
        mysqlshdk::db::Connection_options copts(cs_primary_master_url);
        log_info("Connecting to %s...", cs_primary_master_url.c_str());
        copts.set_login_options_from(
            cs_primary_master->get_connection_options());

        m_primary_master = Instance::connect(copts);
        m_metadata_storage =
            std::make_shared<MetadataStorage>(Instance::connect(copts));
      }

      // if we think we're the primary cluster, try connecting to replica
      // clusters and look for a newer view_id generation, in case we got
      // failed over from and invalidated
      if (is_primary_cluster()) {
        find_real_cluster_set_primary(cs.get());

        auto pc = cs->get_primary_cluster();
        // If the real PC is different, it means this Cluster is INVALIDATED

        if (pc->get_id() != get_id()) {
          // Set m_primary_master to the actual Primary of the Primary Cluster
          m_primary_master = pc->get_primary_master();
        }
      }
    } else {
      // If there's no global primary master, the PRIMARY Cluster is down
      m_primary_master = nullptr;
    }
  } else {
    log_debug("Cluster does not belong to a ClusterSet");
  }

  if (m_primary_master && m_metadata_storage->get_md_server()->get_uuid() !=
                              m_primary_master->get_uuid()) {
    m_metadata_storage = std::make_shared<MetadataStorage>(
        Instance::connect(m_primary_master->get_connection_options()));

    if (is_cluster_set_member()) {
      // Update the Cluster's Cluster_set_member_metadata data according to
      // the Correct Metadata Note: This also marks the cluster as
      // invalidated. That info might be required later on
      m_metadata_storage->get_cluster_set_member(get_id(), &m_cs_md);
    }
  }

  return m_primary_master.get();
}

Cluster_metadata Cluster_impl::get_metadata() const {
  Cluster_metadata cmd;
  if (!get_metadata_storage()->get_cluster(get_id(), &cmd)) {
    throw shcore::Exception(
        "Cluster metadata could not be loaded for " + get_name(),
        SHERR_DBA_MISSING_FROM_METADATA);
  }
  return cmd;
}

std::shared_ptr<Cluster_set_impl>
Cluster_impl::check_and_get_cluster_set_for_cluster() {
  auto cs = get_cluster_set_object(true);

  current_ipool()->set_metadata(cs->get_metadata_storage());

  if (is_invalidated()) {
    current_console()->print_warning(
        "Cluster '" + get_name() +
        "' was INVALIDATED and must be removed from the ClusterSet.");
    return cs;
  }

  bool pc_changed = false;
  // if we think we're the primary cluster, try connecting to replica
  // clusters and look for a newer view_id generation, in case we got
  // failed over from and invalidated
  if (is_primary_cluster()) {
    find_real_cluster_set_primary(cs.get());

    auto pc = cs->get_primary_cluster();

    // If the real PC is different, it means we're actually invalidated
    pc_changed = pc->get_id() != get_id();
  }
  if (pc_changed || cs->get_metadata_storage()->get_md_server()->descr() !=
                        get_metadata_storage()->get_md_server()->descr()) {
    // Check if the cluster was removed from the cs according to the MD in
    // the correct primary cluster
    if (!cs->get_metadata_storage()->check_cluster_set(
            get_cluster_server().get())) {
      std::string msg =
          "The Cluster '" + get_name() +
          "' appears to have been removed from the ClusterSet '" +
          cs->get_name() +
          "', however its own metadata copy wasn't properly updated "
          "during the removal";
      current_console()->print_warning(msg);

      // Mark that the cluster was already removed from the clusterset but
      // doesn't know about it yet
      set_cluster_set_remove_pending(true);

      return nullptr;
    } else if (is_invalidated()) {
      current_console()->print_warning(
          "Cluster '" + get_name() +
          "' was INVALIDATED and must be removed from the ClusterSet.");
    }
  }

  return cs;
}

std::shared_ptr<Instance> Cluster_impl::get_session_to_cluster_instance(
    const std::string &instance_address, bool raw_session) const {
  // If the Cluster's unavailable (offline, unreachable, primary unreachable)
  // error out immediately
  auto availability = cluster_availability();

  switch (availability) {
    case Cluster_availability::ONLINE_NO_PRIMARY:
    case Cluster_availability::OFFLINE:
    case Cluster_availability::UNREACHABLE: {
      throw shcore::Exception::runtime_error(
          shcore::str_format("Unable to get a session to a Cluster instance. "
                             "Cluster's status is %s",
                             to_string(availability).c_str()));
    }
    default:
      break;
  }

  // Set login credentials to connect to instance.
  // use the host and port from the instance address
  Connection_options instance_cnx_opts(instance_address);
  // Use the password from the cluster session.
  Connection_options cluster_cnx_opt =
      get_cluster_server()->get_connection_options();
  instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

  log_debug("Connecting to instance '%s'", instance_address.c_str());
  try {
    // Try to connect to instance
    auto instance = raw_session ? Instance::connect_raw(instance_cnx_opts)
                                : Instance::connect(instance_cnx_opts);
    log_debug("Successfully connected to instance");
    return instance;
  } catch (const std::exception &err) {
    log_debug("Failed to connect to instance: %s", err.what());
    throw;
  }
}

size_t Cluster_impl::setup_clone_plugin(bool enable_clone) const {
  // Get all cluster instances
  std::vector<Instance_metadata> instance_defs = get_instances();

  // Counter for instances that failed to be updated
  size_t count = 0;

  auto ipool = current_ipool();

  for (const Instance_metadata &instance_def : instance_defs) {
    const auto &instance_address = instance_def.endpoint;

    // Establish a session to the cluster instance
    try {
      Scoped_instance instance(
          ipool->connect_unchecked_endpoint(instance_address));

      // Handle the plugin setup only if the target instance supports it
      if (supports_mysql_clone(instance->get_version())) {
        if (!enable_clone) {
          // Uninstall the clone plugin
          log_info("Uninstalling the clone plugin on instance '%s'.",
                   instance_address.c_str());
          mysqlshdk::mysql::uninstall_clone_plugin(*instance, nullptr);
        } else {
          // Install the clone plugin
          log_info("Installing the clone plugin on instance '%s'.",
                   instance_address.c_str());
          mysqlshdk::mysql::install_clone_plugin(*instance, nullptr);

          // Get the recovery account in use by the instance so the grant
          // BACKUP_ADMIN is granted to its recovery account
          {
            Replication_account::User_hosts user_hosts;

            try {
              user_hosts =
                  m_repl_account_mng.get_replication_user(*instance, false);
            } catch (const shcore::Exception &re) {
              if (re.is_runtime()) {
                mysqlsh::current_console()->print_error(shcore::str_format(
                    "Unsupported recovery account has been found for instance "
                    "%s. Operations such as "
                    "<Cluster>.<<<resetRecoveryAccountsPassword>>>() and "
                    "<Cluster>.<<<addInstance>>>() may fail. Please remove and "
                    "add the instance back to the Cluster to ensure a "
                    "supported recovery account is used.",
                    instance->descr().c_str()));
              }
              throw;
            }

            auto primary = get_primary_master();

            // Add the BACKUP_ADMIN grant to the instance's recovery account
            // since it may not be there if the instance was added to a
            // cluster non-supporting clone
            for (const auto &host : user_hosts.hosts) {
              auto grant = "GRANT BACKUP_ADMIN ON *.* TO ?@?"_sql
                           << user_hosts.user << host;
              grant.done();
              primary->execute(grant);
            }
          }
        }
      }
    } catch (const shcore::Error &err) {
      auto console = mysqlsh::current_console();

      auto err_msg =
          shcore::str_format("Unable to %s clone on the instance '%s': %s",
                             enable_clone ? "enable" : "disable",
                             instance_address.c_str(), err.format().c_str());

      // If a cluster member is unreachable, just print a warning. Otherwise
      // print error
      if (err.code() == CR_CONNECTION_ERROR ||
          err.code() == CR_CONN_HOST_ERROR) {
        console->print_warning(err_msg);
      } else {
        console->print_error(err_msg);
      }
      console->print_info();

      // It failed to update this instance, so increment the counter
      count++;
    }
  }

  return count;
}

namespace {
constexpr std::chrono::seconds k_recovery_start_timeout{30};
}

void Cluster_impl::wait_instance_recovery(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &join_begin_time,
    Recovery_progress_style progress_style) const {
  try {
    auto post_clone_auth = target_instance.get_connection_options();
    post_clone_auth.set_login_options_from(
        get_cluster_server()->get_connection_options());

    monitor_gr_recovery_status(
        target_instance.get_connection_options(), post_clone_auth,
        join_begin_time, progress_style, k_recovery_start_timeout.count(),
        current_shell_options()->get().dba_restart_wait_timeout);
  } catch (const shcore::Exception &e) {
    auto console = current_console();
    // If the recovery itself failed we abort, but monitoring errors can be
    // ignored.
    if (e.code() == SHERR_DBA_CLONE_RECOVERY_FAILED ||
        e.code() == SHERR_DBA_DISTRIBUTED_RECOVERY_FAILED) {
      console->print_error("Recovery error in added instance: " + e.format());
      throw;
    } else {
      console->print_warning(
          "Error while waiting for recovery of the added instance: " +
          e.format());
    }
  } catch (const restart_timeout &) {
    auto console = current_console();
    console->print_warning(
        "Clone process appears to have finished and tried to restart the "
        "MySQL server, but it has not yet started back up.");

    console->print_info();
    console->print_info(
        "Please make sure the MySQL server at '" + target_instance.descr() +
        "' is restarted and call <Cluster>.rescan() to complete the process. "
        "To increase the timeout, change "
        "shell.options[\"dba.restartWaitTimeout\"].");

    throw shcore::Exception("Timeout waiting for server to restart",
                            SHERR_DBA_SERVER_RESTART_TIMEOUT);
  }
}

int64_t Cluster_impl::prepare_clone_recovery(
    const mysqlshdk::mysql::IInstance &target_instance,
    bool force_clone) const {
  bool clone_disabled = get_disable_clone_option();

  // Clone must be uninstalled only when disableClone is enabled on the
  // Cluster
  int64_t restore_clone_threshold = 0;

  // Force clone if requested
  if (force_clone) {
    restore_clone_threshold = mysqlshdk::mysql::force_clone(target_instance);

    // RESET MASTER to clear GTID_EXECUTED in case it's diverged, otherwise
    // clone is not executed and GR rejects the instance
    target_instance.query(shcore::str_format(
        "RESET %s", mysqlshdk::mysql::get_binary_logs_keyword(
                        target_instance.get_version())));
  }

  // Make sure the Clone plugin is installed or uninstalled depending on the
  // value of disableClone
  //
  // NOTE: If the clone usage is not disabled on the cluster (disableClone),
  // we must ensure the clone plugin is installed on all members. Otherwise,
  // if the cluster was creating an Older shell (<8.0.17) or if the cluster
  // was set up using the Incremental recovery only and the primary is
  // removed (or a failover happened) the instances won't have the clone
  // plugin installed and GR's recovery using clone will fail.
  //
  // See BUG#29954085 and BUG#29960838

  // First, handle the target instance
  if (!clone_disabled) {
    log_info("Installing the clone plugin on instance '%s'.",
             target_instance.get_canonical_address().c_str());
    mysqlshdk::mysql::install_clone_plugin(target_instance, nullptr);
  } else {
    log_info("Uninstalling the clone plugin on instance '%s'.",
             target_instance.get_canonical_address().c_str());
    mysqlshdk::mysql::uninstall_clone_plugin(target_instance, nullptr);
  }

  // Then, handle the cluster members
  setup_clone_plugin(!clone_disabled);

  return restore_clone_threshold;
}

Member_recovery_method Cluster_impl::check_recovery_method(
    const mysqlshdk::mysql::IInstance &target_instance,
    Member_op_action op_action, Member_recovery_method selected_recovery_method,
    bool interactive) const {
  // Check GTID consistency and whether clone is needed
  auto check_recoverable = [this](const mysqlshdk::mysql::IInstance &target) {
    // Check if the GTIDs were purged from the whole cluster
    bool recoverable = false;

    execute_in_members(
        {mysqlshdk::gr::Member_state::ONLINE}, target.get_connection_options(),
        {},
        [&recoverable, &target](const std::shared_ptr<Instance> &instance,
                                const mysqlshdk::gr::Member &) {
          // Get the gtid state in regards to the cluster_session
          mysqlshdk::mysql::Replica_gtid_state state =
              mysqlshdk::mysql::check_replica_gtid_state(*instance, target,
                                                         nullptr, nullptr);

          if (state != mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
            recoverable = true;
            // stop searching as soon as we find a possible donor
            return false;
          }
          return true;
        });

    return recoverable;
  };

  std::shared_ptr<Instance> donor_instance = get_cluster_server();

  Member_recovery_method recovery_method;
  {
    // to avoid trace order mismatch
    auto gtid_set_is_complete = get_gtid_set_is_complete();
    auto disable_clone_option = get_disable_clone_option();

    recovery_method = mysqlsh::dba::validate_instance_recovery(
        Cluster_type::GROUP_REPLICATION, op_action, *donor_instance,
        target_instance, check_recoverable, selected_recovery_method,
        gtid_set_is_complete, interactive, disable_clone_option);
  }

  // If recovery method was selected as clone, check that there is at least one
  // ONLINE cluster instance not using IPV6, otherwise throw an error
  if (recovery_method == Member_recovery_method::CLONE) {
    bool found_ipv4 = false;
    std::string full_msg;
    // get online members
    const auto online_instances =
        get_instances({mysqlshdk::gr::Member_state::ONLINE});
    // check if at least one of them is not IPv6, otherwise throw an error
    for (const auto &instance : online_instances) {
      if (!mysqlshdk::utils::Net::is_ipv6(
              mysqlshdk::utils::split_host_and_port(instance.endpoint).first)) {
        found_ipv4 = true;
        break;
      } else {
        std::string msg = "Instance '" + instance.endpoint +
                          "' is not a suitable clone donor: Instance "
                          "hostname/report_host is an IPv6 address, which is "
                          "not supported for cloning.";
        log_info("%s", msg.c_str());
        full_msg += msg + "\n";
      }
    }
    if (!found_ipv4) {
      auto console = current_console();
      console->print_error(
          "None of the ONLINE members in the cluster are compatible to be used "
          "as clone donors for " +
          target_instance.descr());
      console->print_info(full_msg);
      throw shcore::Exception("The Cluster has no compatible clone donors.",
                              SHERR_DBA_CLONE_NO_DONORS);
    }
  }

  return recovery_method;
}

void Cluster_impl::_set_instance_option(const std::string &instance_def,
                                        const std::string &option,
                                        const shcore::Value &value) {
  auto instance_conn_opt = Connection_options(instance_def);

  // Set Cluster configuration option

  // Create the Cluster Set_instance_option object and execute it.
  std::unique_ptr<cluster::Set_instance_option> op_set_instance_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (option == kReplicationSources) {
    op_set_instance_option = std::make_unique<cluster::Set_instance_option>(
        *this, instance_conn_opt, option, value);
  } else {
    if (value.get_type() == shcore::String) {
      std::string value_str = value.as_string();
      op_set_instance_option = std::make_unique<cluster::Set_instance_option>(
          *this, instance_conn_opt, option, value_str);
    } else if (value.get_type() == shcore::Integer ||
               value.get_type() == shcore::UInteger) {
      int64_t value_int = value.as_int();
      op_set_instance_option = std::make_unique<cluster::Set_instance_option>(
          *this, instance_conn_opt, option, value_int);
    } else {
      throw shcore::Exception::type_error(
          "Argument #3 is expected to be a string or an integer");
    }
  }

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally(
      [&op_set_instance_option]() { op_set_instance_option->finish(); });

  // Prepare the Cluster Set_instance_option command execution (validations).
  op_set_instance_option->prepare();

  // put an exclusive lock on the target instance
  mysqlshdk::mysql::Lock_scoped i_lock;
  if (option != kLabel) {
    if (auto instance = op_set_instance_option->get_target_instance(); instance)
      i_lock = instance->get_lock_exclusive();
  }

  // Execute Cluster Set_instance_option operations.
  op_set_instance_option->execute();
}

void Cluster_impl::_set_option(const std::string &option,
                               const shcore::Value &value) {
  // put an exclusive lock on the cluster
  mysqlshdk::mysql::Lock_scoped c_lock;
  if (option != kClusterName) c_lock = get_lock_exclusive();

  // Set Cluster configuration option
  // Create the Set_option object and execute it.
  std::unique_ptr<cluster::Set_option> op_cluster_set_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int/bool/... since the type int is convertible
  // to string, thus overloading becomes ambiguous. As soon as that limitation
  // is gone, this type checking shall go away too.
  switch (value.get_type()) {
    case shcore::String:
      op_cluster_set_option = std::make_unique<cluster::Set_option>(
          this, option, value.as_string());
      break;
    case shcore::Integer:
    case shcore::UInteger:
      op_cluster_set_option =
          std::make_unique<cluster::Set_option>(this, option, value.as_int());
      break;
    case shcore::Bool:
      op_cluster_set_option =
          std::make_unique<cluster::Set_option>(this, option, value.as_bool());
      break;
    case shcore::Float:
      op_cluster_set_option = std::make_unique<cluster::Set_option>(
          this, option, value.as_double());
      break;
    case shcore::Null:
      op_cluster_set_option =
          std::make_unique<cluster::Set_option>(this, option, std::monostate{});
      break;
    default:
      throw shcore::Exception::type_error(
          "Argument #2 is expected to be a string, an integer, a double or a "
          "boolean.");
  }

  // Always execute finish when leaving "try catch".
  shcore::on_leave_scope finally(
      [&op_cluster_set_option]() { op_cluster_set_option->finish(); });

  // Prepare the Set_option command execution (validations).
  op_cluster_set_option->prepare();

  // Execute Set_option operations.
  op_cluster_set_option->execute();
}

mysqlshdk::mysql::Lock_scoped Cluster_impl::get_lock_shared(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::SHARED, timeout);
}

mysqlshdk::mysql::Lock_scoped Cluster_impl::get_lock_exclusive(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout);
}

const Cluster_set_member_metadata &Cluster_impl::get_clusterset_metadata()
    const {
  if (m_cs_md.cluster_set_id.empty()) {
    m_metadata_storage->get_cluster_set_member(get_id(), &m_cs_md);
  }
  return m_cs_md;
}

bool Cluster_impl::is_cluster_set_member(const std::string &cs_id) const {
  if (m_cs_md_remove_pending) return false;

  bool is_member = !get_clusterset_metadata().cluster_set_id.empty();

  if (is_member && !cs_id.empty()) {
    is_member = get_clusterset_metadata().cluster_set_id == cs_id;
  }

  return is_member;
}

bool Cluster_impl::is_read_replica(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  try {
    if (get_metadata_storage()
            ->get_instance_by_uuid(target_instance.get_uuid())
            .instance_type == Instance_type::READ_REPLICA) {
      return true;
    }
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) {
      throw;
    } else {
      log_debug("Metadata for instance '%s' not found",
                target_instance.descr().c_str());
    }
  }

  return false;
}

bool Cluster_impl::is_instance_cluster_member(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  try {
    get_metadata_storage()->get_instance_by_uuid(target_instance.get_uuid(),
                                                 get_id());
    return true;
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) {
      throw;
    } else {
      log_debug("Metadata for instance '%s' not found in the Cluster '%s'",
                target_instance.descr().c_str(), get_name().c_str());
    }
  }

  return false;
}

bool Cluster_impl::is_instance_cluster_set_member(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  try {
    get_metadata_storage()->get_instance_by_uuid(target_instance.get_uuid());
    return true;
  } catch (const shcore::Exception &e) {
    if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) {
      throw;
    } else {
      log_debug("Metadata for instance '%s' not found in the Cluster '%s'",
                target_instance.descr().c_str(), get_name().c_str());
    }
  }

  return false;
}

void Cluster_impl::invalidate_cluster_set_metadata_cache() {
  m_cs_md.cluster_set_id.clear();
}

bool Cluster_impl::is_invalidated() const {
  if (!is_cluster_set_member()) throw std::logic_error("internal error");

  return get_clusterset_metadata().invalidated;
}

bool Cluster_impl::is_primary_cluster() const {
  if (!is_cluster_set_member()) throw std::logic_error("internal error");

  return get_clusterset_metadata().primary_cluster;
}

void Cluster_impl::handle_notification(const std::string &name,
                                       const shcore::Object_bridge_ref &,
                                       shcore::Value::Map_type_ref) {
  if (name == kNotifyClusterSetPrimaryChanged) {
    invalidate_cluster_set_metadata_cache();
  }
}

mysqlshdk::mysql::Lock_scoped Cluster_impl::get_lock(
    mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout) {
  if (!m_cluster_server->is_lock_service_installed()) {
    bool lock_service_installed = false;
    // if SRO is disabled, we have a chance to install the lock service
    if (bool super_read_only =
            m_cluster_server->get_sysvar_bool("super_read_only", false);
        !super_read_only) {
      // we must disable log_bin to prevent the installation from being
      // replicated
      mysqlshdk::mysql::Suppress_binary_log nobinlog(*m_cluster_server);

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
          "Cluster lock through primary member '%s'. Another operation "
          "requiring access to the member is still in progress, please wait "
          "for it to finish and try again.",
          m_cluster_server->descr().c_str()));
      throw shcore::Exception(
          shcore::str_format(
              "Failed to acquire Cluster lock through primary member '%s'",
              m_cluster_server->descr().c_str()),
          SHERR_DBA_LOCK_GET_FAILED);
    } else {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "cluster lock through primary member '%s': %s",
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

shcore::Dictionary_t Cluster_impl::routing_options(const std::string &router) {
  auto dict = Base_cluster_impl::routing_options(router);
  if (router.empty()) (*dict)["clusterName"] = shcore::Value(get_name());

  return dict;
}

shcore::Dictionary_t Cluster_impl::router_options(
    const shcore::Option_pack_ref<Router_options_options> &options) {
  if (is_cluster_set_member()) {
    current_console()->print_error(shcore::str_format(
        "Cluster '%s' is a member of ClusterSet '%s', use "
        "<ClusterSet>.<<<routerOptions>>>() instead",
        get_name().c_str(), get_cluster_set_object()->get_name().c_str()));

    throw shcore::Exception::runtime_error(
        "Function not available for ClusterSet members");
  }

  return Base_cluster_impl::router_options(options);
}

void Cluster_impl::set_routing_option(const std::string &option,
                                      const shcore::Value &value) {
  set_routing_option("", option, value);
}

void Cluster_impl::set_routing_option(const std::string &router,
                                      const std::string &option,
                                      const shcore::Value &value) {
  if (((option == k_router_option_read_only_targets) ||
       (option == k_router_option_stats_updates_frequency)) &&
      is_cluster_set_member()) {
    current_console()->print_error(shcore::str_format(
        "Cluster '%s' is a member of ClusterSet '%s', use "
        "<ClusterSet>.<<<setRoutingOption>>>() to change the option '%s'",
        get_name().c_str(), get_cluster_set_object()->get_name().c_str(),
        option.c_str()));

    throw shcore::Exception::runtime_error(
        "Option not available for ClusterSet members");
  }

  Base_cluster_impl::set_routing_option(router, option, value);
}

// Read-Replicas

void Cluster_impl::add_replica_instance(
    const Connection_options &instance_def,
    const cluster::Add_replica_instance_options &options) {
  {
    auto conds =
        Command_conditions::Builder::gen_cluster("addReplicaInstance")
            .min_mysql_version(Precondition_checker::k_min_rr_version)
            .target_instance(TargetType::InnoDBCluster,
                             TargetType::InnoDBClusterSet)
            .quorum_state(ReplicationQuorum::States::Normal)
            .compatibility_check(
                metadata::Compatibility::INCOMPATIBLE_MIN_VERSION_2_2_0)
            .primary_required()
            .cluster_global_status(Cluster_global_status::OK)
            .build();

    check_preconditions(conds);
  }

  Scoped_instance target(connect_target_instance(instance_def, true, true));

  // put an exclusive lock on the cluster
  auto c_lock = get_lock_exclusive();

  // put an exclusive lock on the target instance
  auto i_lock = target->get_lock_exclusive();

  Topology_executor<cluster::Add_replica_instance>{this, target, options}.run();
}

}  // namespace dba
}  // namespace mysqlsh
