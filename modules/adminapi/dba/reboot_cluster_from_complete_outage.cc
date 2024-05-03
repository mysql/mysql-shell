/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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
#include <string_view>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/cluster_topology_executor.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {

/*
 * Retrieves info regarding the clusters ClusterSet (if any)
 */
Cluster_set_info retrieve_cs_info(Cluster_impl *cluster) {
  Cluster_set_info cs_info;
  cs_info.is_member = cluster->is_cluster_set_member();
  cs_info.is_primary = cs_info.is_member && cluster->is_primary_cluster();
  cs_info.is_invalidated = false;
  cs_info.removed_from_set = false;
  cs_info.primary_status = Cluster_global_status::UNKNOWN;

  if (!cs_info.is_member) return cs_info;

  auto cs = cluster->get_cluster_set_object();

  {
    if (!cs_info.is_primary) cs->connect_primary();
    auto pc = cs->get_primary_cluster();

    if (cs_info.is_primary)
      cs_info.is_invalidated = pc->get_id() != cluster->get_id();
    else {
      cs_info.primary_status = cs->get_cluster_global_status(pc.get());
    }
  }

  try {
    auto target_cluster = cs->get_cluster(cluster->get_name(), true, true);

    cs = target_cluster->check_and_get_cluster_set_for_cluster();

    cs_info.removed_from_set = target_cluster->is_cluster_set_remove_pending();

    auto pc = cs->get_primary_cluster();

    if (cs_info.is_primary) {
      cs_info.is_invalidated = pc->get_id() != target_cluster->get_id();
    } else {
      cs_info.is_invalidated = target_cluster->is_invalidated();
    }

  } catch (const shcore::Exception &e) {
    if ((e.code() != SHERR_DBA_METADATA_MISSING) &&
        (e.code() != SHERR_DBA_ASYNC_MEMBER_INVALIDATED))
      throw;
    cs_info.removed_from_set = true;
  }

  log_info(
      "ClusterSet info: %smember, %sprimary, %sinvalidated, %sremoved "
      "from ClusterSet, primary status: %s",
      cs_info.is_member ? "" : "not ", cs_info.is_primary ? "" : "not ",
      cs_info.is_invalidated ? "" : "not ",
      cs_info.removed_from_set ? "" : "not ",
      to_string(cs_info.primary_status).c_str());

  return cs_info;
}

/*
 * Make sure that all instances are offline (trying to disable GR in those that
 * aren't)
 */
bool ensure_all_members_offline(
    const std::unordered_map<std::shared_ptr<Instance>,
                             mysqlshdk::gr::Member_state> &istates,
    bool dry_run, std::vector<std::shared_ptr<Instance>> *instances_online) {
  // try to stop the ones (either recovering or error) if all others are either
  // offline or error

  auto console = current_console();

  size_t num_offline = 0, num_online = 0;
  for (auto it = istates.begin(); it != istates.end(); ++it) {
    switch (it->second) {
      case mysqlshdk::gr::Member_state::OFFLINE:
      case mysqlshdk::gr::Member_state::MISSING:
        // already offline
        num_offline++;
        continue;
      case mysqlshdk::gr::Member_state::ERROR:
      case mysqlshdk::gr::Member_state::RECOVERING:
        // try and stop GR
        break;
      default:
        // assume instance is online
        num_online++;
        if (instances_online) instances_online->push_back(it->first);
        continue;
    }

    // if there are other online members, there's no point in stopping GR for
    // this member, so there's no point in proceding
    if (num_online > 0) break;

    // check all other instances
    {
      auto itNext = it;
      ++itNext;

      auto all_offline_error =
          (itNext == istates.end()) ||
          std::all_of(itNext, istates.end(), [](const auto &state) {
            return (state.second == mysqlshdk::gr::Member_state::ERROR) ||
                   (state.second == mysqlshdk::gr::Member_state::OFFLINE) ||
                   (state.second == mysqlshdk::gr::Member_state::MISSING);
          });

      // if we can't stop this member, then there's no point in proceding
      // because we'll have a member not offline
      if (!all_offline_error) break;
    }

    console->print_info(shcore::str_format(
        "Stopping Group Replication on '%s'...", it->first->descr().c_str()));

    num_offline++;

    if (!dry_run) {
      try {
        mysqlshdk::gr::stop_group_replication(*(it->first));
      } catch (const shcore::Exception &e) {
        console->print_error(shcore::str_format(
            "Unable to stop Group Replication: %s", e.format().c_str()));
        throw;
      }
    }
  }

  return (num_offline == istates.size());
}

TargetType::Type check_instance_type(
    const mysqlshdk::mysql::IInstance &instance,
    std::string_view quorum_restore_function) {
  auto type = get_gr_instance_type(instance);
  switch (type) {
    case TargetType::GroupReplication:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + instance.descr() +
          "' belongs to a GR group that is not managed as an "
          "InnoDB Cluster. ");

    case TargetType::AsyncReplicaSet:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + instance.descr() +
          "' belongs to an InnoDB ReplicaSet. ");

    case TargetType::AsyncReplication:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + instance.descr() +
          "' belongs to a AR topology that is not managed as an "
          "InnoDB ReplicaSet. ");

    case TargetType::InnoDBClusterSet:
    case TargetType::InnoDBClusterSetOffline:
      throw shcore::Exception::runtime_error(
          "The MySQL instance '" + instance.descr() +
          "' belongs to an InnoDB ClusterSet. ");

    case TargetType::InnoDBCluster:
      if (!quorum_restore_function.empty() &&
          !mysqlshdk::gr::has_quorum(instance, nullptr, nullptr)) {
        auto msg = shcore::str_format(
            "The MySQL instance '%s' belongs to an InnoDB Cluster and is "
            "reachable. Please use <Cluster>.%.*s() to restore from the quorum "
            "loss.",
            instance.descr().c_str(), (int)quorum_restore_function.size(),
            quorum_restore_function.data());
        throw shcore::Exception::runtime_error(msg);
      }
      [[fallthrough]];
    case TargetType::Standalone:
    case TargetType::StandaloneWithMetadata:
    case TargetType::StandaloneInMetadata:
    case TargetType::Unknown:
      break;
  }

  return type;
}

void rejoin_instances(Cluster_impl *cluster_impl,
                      const Instance &target_instance,
                      const std::vector<std::shared_ptr<Instance>> &instances,
                      const Reboot_cluster_options &options, bool enable_sro) {
  const auto console = current_console();

  auto removed_from_set = cluster_impl->is_cluster_set_remove_pending();

  for (const auto &instance : instances) {
    // ignore seed
    if (instance->get_uuid() == target_instance.get_uuid()) continue;

    // if GTIDs aren't compatible, skip this instance
    if (auto gtid_state = check_replica_gtid_state(target_instance, *instance);
        (gtid_state != mysqlshdk::mysql::Replica_gtid_state::IDENTICAL) &&
        (gtid_state != mysqlshdk::mysql::Replica_gtid_state::RECOVERABLE)) {
      console->print_note(shcore::str_format(
          "Not rejoining instance '%s' because its GTID set isn't compatible "
          "with '%s'.",
          instance->descr().c_str(), target_instance.descr().c_str()));
      continue;
    }

    // Reset the clusterset replication channel if the instance belongs to
    // replica that was removed from the cluster set
    if (removed_from_set) {
      if (mysqlshdk::mysql::Replication_channel channel;
          mysqlshdk::mysql::get_channel_status(
              *instance, k_clusterset_async_channel_name, &channel)) {
        auto status = channel.status();
        log_info("State of clusterset replication channel: %d", status);

        if (status == mysqlshdk::mysql::Replication_channel::OFF)
          mysqlsh::dba::remove_channel(instance.get(),
                                       k_clusterset_async_channel_name, false);
      }
    }

    try {
      auto conn_options = instance->get_connection_options();

      cluster::Rejoin_instance_options rejoin_options;

      // Set the ipAllowlist if option used
      if (options.gr_options.ip_allowlist &&
          options.switch_communication_stack.value_or("") ==
              kCommunicationStackXCom) {
        rejoin_options.gr_options.ip_allowlist =
            options.gr_options.ip_allowlist;
      } else {
        // Let it be resolved later
        rejoin_options.gr_options.ip_allowlist = std::nullopt;
      }

      // Do not handle the ClusterSet-related steps (configuration of the
      // managed channel and transaction sync with the primary cluster) when:
      //   - The Cluster was a Replica Cluster that was removed from the
      //     ClusterSet, or
      //   - It's not a ClusterSet member, or
      //   - It's an INVALIDATED Cluster
      bool ignore_cluster_set = removed_from_set ||
                                !cluster_impl->is_cluster_set_member() ||
                                cluster_impl->is_invalidated();

      Cluster_topology_executor<cluster::Rejoin_instance>{
          cluster_impl,       instance,
          rejoin_options,     options.switch_communication_stack.has_value(),
          ignore_cluster_set, true}
          .run();

    } catch (const shcore::Error &e) {
      console->print_warning(instance->descr() + ": " + e.format());
      // TODO(miguel) Once WL#13535 is implemented and rejoin supports
      // clone, simplify the following note by telling the user to use
      // rejoinInstance. E.g: “%s’ could not be automatically rejoined.
      // Please use cluster.rejoinInstance() to manually re-add it.”
      console->print_note(shcore::str_format(
          "Unable to rejoin instance '%s' to the Cluster but the "
          "dba.<<<rebootClusterFromCompleteOutage>>>() operation will "
          "continue.",
          instance->descr().c_str()));
      console->print_info();
    }

    // re-enable SRO if needed
    if (enable_sro && !instance->get_sysvar_bool("super_read_only", false)) {
      log_info("Enabling super_read_only on instance '%s'...",
               instance->descr().c_str());
      instance->set_sysvar("super_read_only", true);
    }
  }

  // Verification step to ensure the server_id is an attribute on all
  // the instances of the cluster
  cluster_impl->ensure_metadata_has_server_id(target_instance);
}

}  // namespace

std::vector<std::shared_ptr<Instance>>
Reboot_cluster_from_complete_outage::retrieve_instances(
    std::vector<Instance_metadata> *instances_unreachable) {
  // check quorum
  check_instance_type(*m_target_instance,
                      get_member_name("forceQuorumUsingPartitionOf",
                                      shcore::current_naming_style()));

  auto instances = m_cluster->impl()->get_instances();

  std::vector<std::shared_ptr<Instance>> out_instances;
  out_instances.reserve(instances.size());

  for (const auto &i : instances) {
    // skip the target
    if (i.uuid == m_target_instance->get_uuid()) continue;

    std::shared_ptr<Instance> instance;
    try {
      log_info("Opening a new session to the instance: %s", i.endpoint.c_str());
      instance =
          m_cluster->impl()->connect_target_instance(i.endpoint, false, false);
    } catch (const shcore::Error &e) {
      log_info("Unable to open a connection to the instance: %s",
               i.endpoint.c_str());
      if (instances_unreachable) instances_unreachable->emplace_back(i);
      continue;
    }

    log_info("Checking state of instance '%s'", i.endpoint.c_str());

    auto instance_type = check_instance_type(*instance, {});

    if (instance_type == TargetType::Standalone) {
      if (!m_options.get_force()) {
        throw shcore::Exception::logic_error(shcore::str_format(
            "The instance '%s' doesn't belong to the Cluster. Use option "
            "'force' to ignore this check.",
            instance->descr().c_str()));
      } else {
        current_console()->print_warning(
            shcore::str_format("The instance '%s' doesn't belong to the "
                               "Cluster and will be ignored.",
                               instance->descr().c_str()));
        continue;
      }
    }

    out_instances.emplace_back(std::move(instance));
  }

  return out_instances;
}

/*
 * It verifies which of the online instances of the cluster has the
 * GTID superset. If also checks the command options such as picking an explicit
 * instance and the force option.
 */
std::shared_ptr<Instance>
Reboot_cluster_from_complete_outage::pick_best_instance_gtid(
    const std::vector<std::shared_ptr<Instance>> &instances,
    bool is_cluster_set_member, bool force,
    std::string_view intended_instance) {
  std::vector<Instance_gtid_info> instance_gtids;
  {
    // list of replication channel names that must be considered when comparing
    // GTID sets. With ClusterSets, the async channel for secondaries must be
    // added here.
    auto current_session_options = m_target_instance->get_connection_options();

    std::vector<std::string> channels;
    channels.push_back(mysqlshdk::gr::k_gr_applier_channel);
    if (is_cluster_set_member)
      channels.push_back(k_clusterset_async_channel_name);

    {
      Instance_gtid_info info;
      info.server = m_target_instance->get_canonical_address();
      info.gtid_executed =
          mysqlshdk::mysql::get_total_gtid_set(*m_target_instance, channels);
      instance_gtids.push_back(std::move(info));
    }

    for (const auto &i : instances) {
      Instance_gtid_info info;
      info.server = i->get_canonical_address();
      info.gtid_executed = mysqlshdk::mysql::get_total_gtid_set(*i, channels);
      instance_gtids.push_back(std::move(info));
    }

    assert(instance_gtids.size() == (instances.size() + 1));
  }

  std::set<std::string> conflits;
  auto primary_candidates =
      filter_primary_candidates(*m_target_instance, instance_gtids,
                                [&conflits](const Instance_gtid_info &instA,
                                            const Instance_gtid_info &instB) {
                                  conflits.insert(instA.server);
                                  conflits.insert(instB.server);
                                  return true;
                                });

  assert(!conflits.empty() || !primary_candidates.empty());

  // helper function to return the instance ptr of a target Instance_gtid_info
  auto get_inst_ptr = [&instance_gtids,
                       &instances](const Instance_gtid_info &target) {
    auto it = std::find_if(
        instance_gtids.begin(), instance_gtids.end(), [&target](const auto &i) {
          return mysqlshdk::utils::are_endpoints_equal(i.server, target.server);
        });

    assert(it != instance_gtids.end());
    assert(it != instance_gtids.begin());  // the target shouldn't be requested
    return instances[std::distance(instance_gtids.begin(), it) - 1];
  };

  // check if the intended instance is valid
  if (!intended_instance.empty()) {
    auto it = std::find_if(instance_gtids.begin(), instance_gtids.end(),
                           [&intended_instance](const auto &i) {
                             return mysqlshdk::utils::are_endpoints_equal(
                                 i.server, intended_instance);
                           });

    if (it == instance_gtids.end())
      throw shcore::Exception::runtime_error(
          shcore::str_format("The requested instance '%.*s' isn't part of the "
                             "members of the Cluster.",
                             static_cast<int>(intended_instance.size()),
                             intended_instance.data()));
  }

  // lets take care of stuff when we don't have any conflits
  if (conflits.empty()) {
    if (intended_instance.empty()) {
      // if the target instance is already in the candidates, use it (don't
      // return a best candidate)
      {
        auto it =
            std::find_if(primary_candidates.begin(), primary_candidates.end(),
                         [&target = instance_gtids[0]](const auto &p) {
                           return mysqlshdk::utils::are_endpoints_equal(
                               p.server, target.server);
                         });

        if (it != primary_candidates.end()) return nullptr;
      }

      // return a better candidate (we simply pick the first of the candidates)
      return get_inst_ptr(primary_candidates.front());
    }

    // reaching this point, we don't have conflits, but the user explicitly
    // specified the new primary
    {
      auto it =
          std::find_if(primary_candidates.begin(), primary_candidates.end(),
                       [&intended_instance](const auto &p) {
                         return mysqlshdk::utils::are_endpoints_equal(
                             p.server, intended_instance);
                       });

      // if we can use the instance the user specified
      if (it != primary_candidates.end()) {
        if (it->server == instance_gtids[0].server)
          return nullptr;  // use the target
        return get_inst_ptr(*it);
      }
    }

    if (!force) {
      std::string endpoint_list;
      for (const auto &c : primary_candidates)
        endpoint_list.append("'").append(c.server).append("', ");
      endpoint_list.erase(endpoint_list.size() - 2);

      throw shcore::Exception::runtime_error(shcore::str_format(
          "The requested instance '%.*s' can't be used as the new seed because "
          "it has a lower GTID when compared with the other members: %s. Use "
          "option 'force' if this is indeed the desired behaviour.",
          static_cast<int>(intended_instance.size()), intended_instance.data(),
          endpoint_list.c_str()));
    }

    auto it = std::find_if(instance_gtids.begin(), instance_gtids.end(),
                           [&intended_instance](const auto &i) {
                             return mysqlshdk::utils::are_endpoints_equal(
                                 i.server, intended_instance);
                           });

    // checked earlier
    assert(it != instance_gtids.end());

    if (it == instance_gtids.begin()) return nullptr;  // use the target
    return instances[std::distance(instance_gtids.begin(), it) - 1];
  }

  // we have to manage GTIDs conflits

  {
    std::string endpoint_list;
    for (const auto &c : conflits)
      endpoint_list.append("'").append(c).append("', ");
    endpoint_list.erase(endpoint_list.size() - 2);

    const auto console = current_console();
    console->print_warning(shcore::str_format(
        "Detected GTID conflits between instances: %s", endpoint_list.c_str()));
  }

  if (!force || intended_instance.empty())
    throw shcore::Exception::runtime_error(
        "To reboot a Cluster with GTID conflits, both the 'force' and "
        "'primary' options must be used to proceed with the command and "
        "to explicitly pick a new seed instance.");

  // if the intended instance is already the target one...
  if (mysqlshdk::utils::are_endpoints_equal(instance_gtids[0].server,
                                            intended_instance))
    return nullptr;

  auto it = std::find_if(instance_gtids.begin(), instance_gtids.end(),
                         [&intended_instance](const auto &i) {
                           return mysqlshdk::utils::are_endpoints_equal(
                               i.server, intended_instance);
                         });

  assert((it != instance_gtids.end()) &&
         (it != instance_gtids.begin()));  // checked earlier

  return instances[std::distance(instance_gtids.begin(), it) - 1];
}

void Reboot_cluster_from_complete_outage::
    validate_local_address_ip_compatibility(
        const std::string &local_address) const {
  // local_address must have some value
  assert(!local_address.empty());

  // Validate that the group_replication_local_address is valid for the version
  // of the target instance.
  if (!mysqlshdk::gr::is_endpoint_supported_by_gr(
          local_address, m_target_instance->get_version())) {
    auto console = mysqlsh::current_console();
    console->print_error("Cannot join instance '" + m_target_instance->descr() +
                         "' to cluster: unsupported localAddress value.");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Cannot use value '%s' for option localAddress because it has "
        "an IPv6 address which is only supported by Group Replication "
        "from MySQL version >= 8.0.14 and the target instance version is %s.",
        local_address.c_str(),
        m_target_instance->get_version().get_base().c_str()));
  }
}

void Reboot_cluster_from_complete_outage::resolve_local_address(
    Group_replication_options *gr_options) {
  std::string communication_stack,
      hostname = m_target_instance->get_canonical_hostname();

  // During a reboot from complete outage the command does not change the
  // communication stack unless set via switchCommunicationStack
  if (gr_options->communication_stack.has_value()) {
    communication_stack = gr_options->communication_stack.value_or("");
  } else {
    // The default value for communicationStack must be 'mysql' if the target
    // instance is running 8.0.27+
    if (supports_mysql_communication_stack(m_target_instance->get_version())) {
      communication_stack = kCommunicationStackMySQL;
    } else {
      communication_stack = kCommunicationStackXCom;
    }
  }

  gr_options->local_address = mysqlsh::dba::resolve_gr_local_address(
      gr_options->local_address, communication_stack, hostname,
      *m_target_instance->get_sysvar_int("port"), false, true);

  // Validate that the local_address value we want to use as well as the
  // local address values in use on the cluster are compatible with the
  // version of the instance being added to the cluster.
  validate_local_address_ip_compatibility(
      gr_options->local_address.value_or(""));
}

void Reboot_cluster_from_complete_outage::check_instance_configuration() {
  // The current 'group_replication_group_name' must be kept otherwise
  // if instances are rejoined later the operation may fail because
  // a new group_name started being used.
  m_options.gr_options.group_name = m_cluster->impl()->get_group_name();

  // group_replication_view_change_uuid might have been persisted by
  // Cluster.rescan() and the target instance wasn't restarted so the value is
  // not effective yet. Attempt to read it and if not empty, set it right away
  // in the GR options to be used
  if (m_target_instance->get_version() >=
      Precondition_checker::k_min_cs_version) {
    std::string view_change_uuid_persisted =
        m_target_instance
            ->get_persisted_value("group_replication_view_change_uuid")
            .value_or("");

    if (!view_change_uuid_persisted.empty()) {
      m_options.gr_options.view_change_uuid = view_change_uuid_persisted;
    }
  }

  // Read actual GR configurations to preserve them when rejoining the
  // instance.
  m_options.gr_options.read_option_values(
      *m_target_instance, m_options.switch_communication_stack.has_value());

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  ensure_gr_instance_configuration_valid(m_target_instance.get(), true, false);

  // Verify if the instance is running asynchronous
  // replication
  // NOTE: Verify for all operations: addInstance(), rejoinInstance() and
  // rebootClusterFromCompleteOutage()
  validate_async_channels(
      *m_target_instance,
      m_cluster->impl()->is_cluster_set_member()
          ? std::unordered_set<std::string>{k_clusterset_async_channel_name}
          : std::unordered_set<std::string>{},
      checks::Check_type::BOOTSTRAP);

  // no peers if we're bootstrapping
  m_options.gr_options.group_seeds = "";

  // Resolve and validate GR local address.
  // NOTE: Must be done only after getting the report_host used by GR and for
  //       the metadata;
  resolve_local_address(&m_options.gr_options);
}

/*
 * Reboots the seed instance
 */
void Reboot_cluster_from_complete_outage::reboot_seed(
    const Cluster_set_info &cs_info) {
  // If GR auto-started stop it otherwise changing GR member actions will fail
  if (cluster_topology_executor_ops::is_member_auto_rejoining(
          m_target_instance)) {
    cluster_topology_executor_ops::ensure_not_auto_rejoining(m_target_instance);
  }

  const auto is_gr_sro_if_primary_disabled =
      [](const mysqlshdk::mysql::IInstance &instance) {
        bool enabled = false;
        bool action_exists = mysqlshdk::gr::get_member_action_status(
            instance, mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
            &enabled);

        return !action_exists || !enabled;
      };

  bool remove_cs_replication_channel = false;

  if (cs_info.removed_from_set) {
    // enable mysql_disable_super_read_only_if_primary if needed
    if (is_gr_sro_if_primary_disabled(*m_target_instance)) {
      log_info("Enabling automatic super_read_only management on '%s'",
               m_target_instance->descr().c_str());

      mysqlshdk::gr::enable_member_action(
          *m_target_instance,
          mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
          mysqlshdk::gr::k_gr_member_action_after_primary_election);
    }

    // If this is a Replica Cluster we must remove the replication channel too
    if (!cs_info.is_primary) {
      remove_cs_replication_channel = true;
    }
  } else if (cs_info.is_invalidated) {  // The Cluster does not know if it was
                                        // removed from the ClusterSet
    // disable mysql_disable_super_read_only_if_primary if needed
    if (!is_gr_sro_if_primary_disabled(*m_target_instance)) {
      log_info("Disabling automatic super_read_only management on '%s'",
               m_target_instance->descr().c_str());

      mysqlshdk::gr::disable_member_action(
          *m_target_instance,
          mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
          mysqlshdk::gr::k_gr_member_action_after_primary_election);
    }

    // If this is a Replica Cluster we must remove the replication channel too
    if (!cs_info.is_primary) {
      remove_cs_replication_channel = true;
    }
  }

  // remove replication channel
  if (remove_cs_replication_channel) {
    if (mysqlshdk::mysql::Replication_channel channel;
        mysqlshdk::mysql::get_channel_status(
            *m_target_instance, k_clusterset_async_channel_name, &channel)) {
      auto status = channel.status();
      log_info("State of clusterset replication channel: %d", status);

      if (status == mysqlshdk::mysql::Replication_channel::OFF) {
        try {
          mysqlshdk::mysql::reset_slave(*m_target_instance,
                                        k_clusterset_async_channel_name, true);
        } catch (const shcore::Error &e) {
          throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
        }
      }
    }
  }

  // Validations and variables initialization
  {
    // Set the communicationStack if option used
    if (m_options.switch_communication_stack.has_value()) {
      m_options.gr_options.communication_stack =
          *(m_options.switch_communication_stack);
    }

    // Validate the GR options.
    // Note: If the user provides no group_seeds value, it is automatically
    // assigned a value with the local_address values of the existing cluster
    // members and those local_address values are already validated on the
    // validate_local_address_ip_compatibility method, so we only need to
    // validate the group_seeds value provided by the user.
    m_options.gr_options.check_option_values(
        m_target_instance->get_version(),
        m_target_instance->get_canonical_port());

    m_options.gr_options.manual_start_on_boot =
        m_cluster->impl()->get_manual_start_on_boot_option();

    // Make sure the target instance does not already belong to a different
    // cluster.
    try {
      mysqlsh::dba::checks::ensure_instance_not_belong_to_cluster(
          m_target_instance, m_cluster->impl()->get_cluster_server(),
          m_cluster->impl()->get_id());
    } catch (const shcore::Exception &exp) {
      m_already_member =
          (exp.code() == SHERR_DBA_ASYNC_MEMBER_INCONSISTENT) ||
          (exp.code() == SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER);
      if (!m_already_member) throw;
    }

    check_instance_configuration();

    if (get_executed_gtid_set(*m_target_instance).empty()) {
      current_console()->print_note(
          "The target instance '" + m_target_instance->descr() +
          "' has not been pre-provisioned (GTID set is empty). The "
          "Shell is unable to determine whether the instance has "
          "pre-existing data that would be overwritten.");

      throw shcore::Exception("The instance '" + m_target_instance->descr() +
                                  "' has an empty GTID set.",
                              SHERR_DBA_GTID_SYNC_ERROR);
    }
  }

  // Re-bootstrap
  {
    // Set the internal configuration object: read/write configs from the
    // server.
    auto cfg = mysqlsh::dba::create_server_config(
        m_target_instance.get(), mysqlshdk::config::k_dft_cfg_server_handler);

    // Common informative logging
    cluster_topology_executor_ops::log_used_gr_options(m_options.gr_options);

    // If the Cluster is using the 'MySQL' communication stack, we cannot
    // guarantee that:
    //
    //   - The recovery account exists and is configured at every Cluster
    //   member
    //   - The recovery credentials didn't change (for example after a
    //   .resetRecoveryAccountsPassword())
    //   - The recovery credentials have the required Grants
    //
    // For those reasons, we must simply re-create the recovery account
    if (m_options.gr_options.communication_stack.value_or("") ==
        kCommunicationStackMySQL) {
      // If it's a Replica cluster, we must disable the binary logging and
      // ensure the are created later
      if (m_cluster->impl()->is_cluster_set_member() &&
          !m_cluster->impl()->is_primary_cluster()) {
        m_target_instance->execute("SET session sql_log_bin = 0");
      }

      // Disable SRO if enabled
      if (m_target_instance->get_sysvar_bool("super_read_only", false)) {
        m_target_instance->set_sysvar("super_read_only", false);
      }

      // Get the recovery account stored in the Metadata
      std::string recovery_user;
      std::vector<std::string> recovery_user_hosts;
      try {
        std::tie(recovery_user, recovery_user_hosts, std::ignore) =
            m_cluster->impl()->get_replication_user(*m_target_instance);
      } catch (const shcore::Exception &re) {
        if (re.is_runtime()) {
          mysqlsh::current_console()->print_error(
              "Unsupported recovery account has been found for "
              "instance " +
              m_target_instance->descr() +
              ". Operations such as "
              "<Cluster>.<<<resetRecoveryAccountsPassword>>>() and "
              "<Cluster>.<<<addInstance>>>() may fail. Please remove and "
              "add the instance back to the Cluster to ensure a "
              "supported recovery account is used.");
        }
        throw;
      }

      mysqlshdk::mysql::Auth_options repl_account;
      repl_account.user = recovery_user;

      // Check if the replication user already exists to delete it
      // before creating it again
      for (const auto &hostname : recovery_user_hosts) {
        if (!m_target_instance->user_exists(repl_account.user, hostname))
          continue;

        current_console()->print_note(shcore::str_format(
            "User '%s'@'%s' already existed at instance '%s'. It will be "
            "deleted and created again with a new password.",
            repl_account.user.c_str(), hostname.c_str(),
            m_target_instance->descr().c_str()));

        m_target_instance->drop_user(repl_account.user, hostname);
      }

      // Get the replicationAllowedHost value set for the Cluster
      std::string repl_account_host = "%";

      if (shcore::Value allowed_host;
          m_cluster->impl()
              ->get_metadata_storage()
              ->query_cluster_set_attribute(
                  m_cluster->impl()->get_id(),
                  k_cluster_attribute_replication_allowed_host,
                  &allowed_host) &&
          allowed_host.type == shcore::String &&
          !allowed_host.as_string().empty()) {
        repl_account_host = allowed_host.as_string();
      }

      // Create a new recovery account
      {
        std::vector<std::string> hosts;
        hosts.push_back(repl_account_host);

        mysqlshdk::gr::Create_recovery_user_options options;
        options.clone_supported = true;
        options.auto_failover = false;
        options.mysql_comm_stack_supported = true;

        repl_account = mysqlshdk::gr::create_recovery_user(
            repl_account.user, *m_target_instance, hosts, options);
      }

      // Change GR's recovery replication credentials in all possible
      // donors so whenever GR picks a suitable donor it will be able to
      // connect and authenticate at the target
      // NOTE: Instances in RECOVERING must be skipped since won't be used
      // as donor and the change source command would fail anyway
      mysqlshdk::mysql::Replication_credentials_options options;
      options.password = repl_account.password.value_or("");

      mysqlshdk::mysql::change_replication_credentials(
          *m_target_instance, mysqlshdk::gr::k_gr_recovery_channel,
          repl_account.user, options);

      if (m_cluster->impl()->is_cluster_set_member() &&
          !m_cluster->impl()->is_primary_cluster()) {
        m_target_instance->execute("SET session sql_log_bin = 1");
      }

      // Insert the recovery account on the Metadata Schema.
      m_cluster->impl()->get_metadata_storage()->update_instance_repl_account(
          m_target_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
          repl_account.user, repl_account_host);

      // Set the allowlist to 'AUTOMATIC' to ensure no older values are used
      // since reboot will re-use the values persisted in the instance.
      // NOTE: AUTOMATIC because there's no other allowed value when using the
      // 'MySQL' communication stack
      m_options.gr_options.ip_allowlist = "AUTOMATIC";
    }

    // Make sure the GR plugin is installed (only installed if needed).
    // NOTE: An error is issued if it fails to be installed (e.g., DISABLED).
    //       Disable read-only temporarily to install the plugin if needed.
    mysqlshdk::gr::install_group_replication_plugin(*m_target_instance,
                                                    nullptr);

    if (m_options.gr_options.group_name.has_value() &&
        !m_options.gr_options.group_name->empty()) {
      log_info("Using Group Replication group name: %s",
               m_options.gr_options.group_name->c_str());
    }

    // Get the value for transaction size limit stored in the Metadata if it
    // wasn't set by the caller
    if (!m_options.gr_options.transaction_size_limit.has_value()) {
      int64_t transaction_size_limit;

      if (shcore::Value value;
          m_cluster->impl()->get_metadata_storage()->query_cluster_attribute(
              m_cluster->impl()->get_id(),
              k_cluster_attribute_transaction_size_limit, &value)) {
        transaction_size_limit = value.as_int();
      } else {
        // Use what's set in the instance
        transaction_size_limit = get_transaction_size_limit(
            *m_cluster->impl()->get_cluster_server());
      }

      log_info("Using Group Replication transaction size limit: %" PRId64,
               transaction_size_limit);

      m_options.gr_options.transaction_size_limit = transaction_size_limit;
    }

    // Get the persisted value of paxosSingleLeader to use it
    if (!m_options.gr_options.paxos_single_leader.has_value() &&
        m_target_instance->is_set_persist_supported()) {
      std::string paxos_single_leader =
          m_target_instance
              ->get_persisted_value("group_replication_paxos_single_leader")
              .value_or("");

      if (!paxos_single_leader.empty()) {
        m_options.gr_options.paxos_single_leader =
            shcore::str_caseeq(paxos_single_leader, "on") ? true : false;
      }
    }

    log_info("Starting cluster with '%s' using account %s",
             m_target_instance->descr().c_str(),
             m_target_instance->get_connection_options().get_user().c_str());

    // Determine the topology mode to use.
    auto multi_primary = m_cluster->impl()->get_cluster_topology_type() ==
                         mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;

    bool requires_certificates{false};
    switch (m_cluster->impl()->query_cluster_auth_type()) {
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT:
      case mysqlsh::dba::Replication_auth_type::CERT_ISSUER_PASSWORD:
      case mysqlsh::dba::Replication_auth_type::CERT_SUBJECT_PASSWORD:
        requires_certificates = true;
      default:
        break;
    }

    // Start the cluster to bootstrap Group Replication.
    mysqlsh::dba::start_cluster(*m_target_instance, m_options.gr_options,
                                requires_certificates, multi_primary,
                                cfg.get());

    // Wait for the seed instance to become ONLINE in the Group.
    // Especially relevant on Replica Clusters to ensure the seed instance is
    // already ONLINE when other members try to rejoin the Cluster, otherwise,
    // the rejoining members will fail to rejoin because the managed
    // replication channel may be started with auto-failover while the members
    // haven't obtained those channel configurations yet.
    uint32_t timeout = 5 * 60 * 1000;  // 5 minutes

    mysqlsh::current_console()->print_info(
        "* Waiting for seed instance to become ONLINE...");

    mysqlshdk::gr::wait_member_online(*m_target_instance, timeout);

    // Update the instances Metadata to ensure 'grLocal' reflects the new
    // value for local_address Do it only when communicationStack is used and
    // it's not a replica cluster to not introduce errant transactions
    if (m_options.gr_options.communication_stack.has_value() &&
        (!m_cluster->impl()->is_cluster_set_member() ||
         m_cluster->impl()->is_primary_cluster())) {
      m_cluster->impl()->update_metadata_for_instance(*m_target_instance);
    }

    log_debug("Instance add finished");

    current_console()->print_info(m_target_instance->descr() +
                                  " was restored.");
  }
}

std::shared_ptr<Cluster> Reboot_cluster_from_complete_outage::do_run() {
  const auto console = current_console();
  auto cluster_impl = m_cluster->impl();
  auto cluster_ret = m_cluster;

  auto cs_info = retrieve_cs_info(cluster_impl.get());
  bool cluster_is_multi_primary = (cluster_impl->get_cluster_topology_type() ==
                                   mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  if (!m_options.get_dry_run()) {
    console->print_info("Restoring the Cluster '" + cluster_impl->get_name() +
                        "' from complete outage...");
    console->print_info();
  }

  // after all the checks are done, this will store all the instances we need
  // to work with
  std::vector<std::shared_ptr<Instance>> instances;
  {
    // ensure that the current session instance exists on the Metadata Schema
    if (std::string group_md_address =
            m_target_instance->get_canonical_address();
        !cluster_impl->contains_instance_with_address(group_md_address)) {
      throw shcore::Exception::runtime_error(
          "The current session instance does not belong to the Cluster: '" +
          cluster_impl->get_name() + "'.");
    }

    // gather all instances
    std::vector<Instance_metadata> instances_unreachable;
    instances = retrieve_instances(&instances_unreachable);

    // gather all (including target instance) instances states
    std::unordered_map<std::shared_ptr<Instance>, mysqlshdk::gr::Member_state>
        istates;

    istates.rehash(instances.size() + 1);
    istates[m_target_instance] =
        mysqlshdk::gr::get_member_state(*m_target_instance);
    for (const auto &i : instances)
      istates[i] = mysqlshdk::gr::get_member_state(*i);

    assert(instances.size() == (istates.size() - 1));

    {
      std::vector<std::string> addresses;
      addresses.reserve(istates.size() + instances_unreachable.size());

      std::transform(istates.cbegin(), istates.cend(),
                     std::back_inserter(addresses), [](const auto &istate) {
                       return shcore::str_format(
                           "'%s' (%s)",
                           istate.first->get_canonical_address().c_str(),
                           to_string(istate.second).c_str());
                     });
      std::transform(
          instances_unreachable.cbegin(), instances_unreachable.cend(),
          std::back_inserter(addresses), [](const auto &i) {
            return shcore::str_format("'%s' (UNREACHABLE)", i.address.c_str());
          });

      // so that the user msg is consistent
      std::sort(addresses.begin(), addresses.end());

      console->print_info(shcore::str_format(
          "Cluster instances: %s", shcore::str_join(addresses, ", ").c_str()));
    }

    // check the state of the instances
    std::vector<std::shared_ptr<Instance>> instances_online;
    auto members_all_offline = ensure_all_members_offline(
        istates, m_options.get_dry_run(), &instances_online);

    assert(!members_all_offline || instances_online.empty());

    // reaching this point, we have all the info needed to decide if the
    // command can proceed.

    if (members_all_offline)
      log_info("All members of the Cluster are either offline or unreachable.");

    // if any instance is online, we can't proceed
    if (!instances_online.empty())
      throw std::runtime_error("The Cluster is ONLINE");

    if (!cs_info.is_invalidated &&
        (!instances_unreachable.empty() || !members_all_offline)) {
      std::vector<std::string> addresses;
      addresses.reserve(instances_online.size() + instances_unreachable.size());

      std::transform(instances_online.cbegin(), instances_online.cend(),
                     std::back_inserter(addresses),
                     [](const auto &i) { return i->get_canonical_address(); });
      std::transform(instances_unreachable.cbegin(),
                     instances_unreachable.cend(),
                     std::back_inserter(addresses),
                     [](const auto &i) { return i.endpoint; });

      // so that the user msg is consistent
      std::sort(addresses.begin(), addresses.end());

      console->print_warning(shcore::str_format(
          "One or more instances of the Cluster could not be reached "
          "and cannot be rejoined nor ensured to be OFFLINE: %s. Cluster may "
          "diverge and become inconsistent unless all instances are either "
          "reachable or certain to be OFFLINE and not accepting new "
          "transactions. You may use the 'force' option to bypass this check "
          "and proceed anyway.",
          shcore::str_join(addresses, ", ", [](const auto &address) {
            return shcore::str_format("'%s'", address.c_str());
          }).c_str()));

      if (!m_options.get_force())
        throw std::runtime_error(
            "Could not determine if Cluster is completely OFFLINE");
    }
  }

  if (m_options.switch_communication_stack && cs_info.is_invalidated)
    throw shcore::Exception::runtime_error(
        "Cannot switch the communication stack in an invalidated Cluster "
        "because its instances won't be removed or rejoined.");

  if (m_options.gr_options.local_address.has_value()) {
    console->print_warning(
        "The value used for 'localAddress' only applies to the current "
        "session instance (seed). If the values generated automatically for "
        "other rejoining Cluster members are not valid, please use "
        "<Cluster>.<<<rejoinInstance>>>() with the 'localAddress' option.");
    console->print_info();
  }

  // before picking the best seed instance, make sure that all pending
  // transactions are applied
  {
    console->print_info(
        "Waiting for instances to apply pending received transactions...");

    if (!m_options.get_dry_run()) {
      auto check_cb = [](const mysqlshdk::mysql::IInstance &instance,
                         std::chrono::seconds timeout) {
        mysqlshdk::mysql::Replication_channel channel;
        if (!get_channel_status(instance, mysqlshdk::gr::k_gr_applier_channel,
                                &channel))
          return;

        if (auto status = channel.status();
            (status == mysqlshdk::mysql::Replication_channel::OFF) ||
            (status == mysqlshdk::mysql::Replication_channel::APPLIER_OFF) ||
            (status == mysqlshdk::mysql::Replication_channel::APPLIER_ERROR))
          return;

        wait_for_apply_retrieved_trx(
            instance, mysqlshdk::gr::k_gr_applier_channel, timeout, false);
      };

      check_cb(*m_target_instance, m_options.get_timeout());

      for (const auto &i : instances) check_cb(*i, m_options.get_timeout());
    }
  }

  // pick the seed instance
  std::shared_ptr<Instance> best_instance_gtid;
  {
    best_instance_gtid =
        pick_best_instance_gtid(instances, cs_info.is_member,
                                m_options.get_force(), m_options.get_primary());

    // check if the other instances are compatible in regards to GTID with the
    // (new) seed
    if (!m_options.get_force()) {
      auto new_seed =
          best_instance_gtid ? best_instance_gtid : m_target_instance;

      instances.push_back(m_target_instance);  // temporary

      for (const auto &i : instances) {
        if (i->get_uuid() == new_seed->get_uuid()) continue;

        std::string_view reason;
        switch (auto gtid_state = check_replica_gtid_state(*new_seed, *i);
                gtid_state) {
          case mysqlshdk::mysql::Replica_gtid_state::IDENTICAL:
          case mysqlshdk::mysql::Replica_gtid_state::RECOVERABLE:
            continue;
          case mysqlshdk::mysql::Replica_gtid_state::DIVERGED:
            reason = "GTIDs diverged";
            break;
          case mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE:
            reason = "former has missing transactions";
            break;
          case mysqlshdk::mysql::Replica_gtid_state::NEW:
            reason = "former has an empty GTID set";
            break;
        }

        throw shcore::Exception::runtime_error(shcore::str_format(
            "The instance '%s' has an incompatible GTID set with the seed "
            "instance '%s' (%.*s). If you wish to proceed, the 'force' option "
            "must be explicitly set.",
            i->descr().c_str(), new_seed->descr().c_str(), (int)reason.size(),
            reason.data()));
      }

      instances.pop_back();
    }
  }

  // The 'force' option is a no-op in this scenario (instances aren't rejoined
  // if the Cluster belongs to a ClusterSet and is INVALIDATED), however, it's
  // more correct to forbid it instead of ignoring it.
  if (cs_info.is_invalidated && m_options.get_force())
    throw std::runtime_error(shcore::str_format(
        "The 'force' option cannot be used in a Cluster that belongs to a "
        "ClusterSet and is INVALIDATED."));

  // everything is ready, proceed with the reboot...

  // will have to lock the target instance and all reachable members
  mysqlshdk::mysql::Lock_scoped_list i_locks;
  {
    auto new_seed = best_instance_gtid ? best_instance_gtid : m_target_instance;
    i_locks.push_back(new_seed->get_lock_exclusive());

    instances.push_back(m_target_instance);  // temporary

    for (const auto &instance : instances) {
      if (instance->get_uuid() != new_seed->get_uuid())
        i_locks.push_back(instance->get_lock_exclusive());
    }

    instances.pop_back();
  }

  // reboot seed
  if (best_instance_gtid) {
    // if we have a better instance in the cluster (in terms of highest GTID),
    // use it instead of the target
    if (mysqlshdk::utils::are_endpoints_equal(
            best_instance_gtid->get_canonical_address(),
            m_options.get_primary())) {
      console->print_info(shcore::str_format(
          "Switching over to instance '%s' to be used as seed.",
          best_instance_gtid->get_canonical_address().c_str()));
    } else {
      console->print_info(shcore::str_format(
          "Switching over to instance '%s' (which has the highest GTID set), "
          "to be used as seed.",
          best_instance_gtid->get_canonical_address().c_str()));
    }

    if (!m_options.get_dry_run()) {
      // use the new target instance as seed
      instances.push_back(std::exchange(m_target_instance, best_instance_gtid));
      best_instance_gtid = nullptr;

      reboot_seed(cs_info);

      // refresh metadata and cluster info
      std::shared_ptr<MetadataStorage> metadata;
      std::shared_ptr<mysqlsh::dba::Cluster> cluster;

      m_dba->connect_to_target_group(m_target_instance, &metadata, nullptr,
                                     false);
      current_ipool()->set_metadata(metadata);

      cluster = m_dba->get_cluster(cluster_impl->get_name().c_str(), metadata,
                                   m_target_instance, true, true);

      cluster_ret = cluster;
      cluster_impl = cluster->impl();
    }

  } else if (!m_options.get_dry_run()) {
    // use the target instance as seed
    reboot_seed(cs_info);
  }

  bool rejoin_remaning_instances = true;

  // don't rejoin the instances *if* cluster is in a cluster set and is
  // invalidated (former primary) or is a replica and the primary doesn't have
  // global status OK
  if (cs_info.is_member &&
      (cs_info.is_invalidated ||
       (!cs_info.is_primary &&
        cs_info.primary_status != Cluster_global_status::OK))) {
    auto msg = cs_info.is_invalidated
                   ? "Skipping rejoining remaining instances because the "
                     "Cluster belongs to a ClusterSet and is INVALIDATED. "
                     "Please add or remove the instances after the Cluster is "
                     "rejoined to the ClusterSet."
                   : "Skipping rejoining remaining instances because the "
                     "Cluster belongs to a ClusterSet and its global status is "
                     "not OK. Please add or remove the instances after the "
                     "Cluster is rejoined to the ClusterSet.";
    console->print_info(msg);

    rejoin_remaning_instances = false;
  } else if (!m_options.get_dry_run()) {
    // it's either a non ClusterSet instance or it is but it's not the
    // primary, so we just need to acquire the primary before rejoining the
    // instances
    if (cs_info.is_member && !cluster_impl->is_cluster_set_remove_pending()) {
      std::shared_ptr<Cluster_set_impl> cs;
      try {
        cs = cluster_impl->check_and_get_cluster_set_for_cluster();
      } catch (const shcore::Exception &e) {
        if (e.code() != SHERR_DBA_METADATA_MISSING) throw;
      }

      // If the ClusterSet couldn't be obtained, it means the Cluster has been
      // forcefully removed from it
      if (!m_options.get_dry_run() && cs) {
        // Acquire primary to ensure the correct primary member will be used
        // from now on when the Cluster belongs to a ClusterSet.
        cluster_impl->acquire_primary();

        // If the communication stack was changed and this is a replica
        // cluster, we must ensure here that 'grLocal' reflects the new value
        // for local address
        if (!cs_info.is_primary &&
            m_options.switch_communication_stack.has_value()) {
          cluster_impl->update_metadata_for_instance(*m_target_instance);
        }
      }
    }
  }

  // if the cluster is part of a set
  if (cs_info.is_member) {
    // if this is a primary, we don't want to rejoin, but we need to check if
    // the cluster was removed from the set, to mark it as pending removed,
    // otherwise, calling dissolve on the cluster object returned will fail
    if (cs_info.is_primary) {
      try {
        // Get the clusterset object (which also checks for various clusterset
        // MD consistency scenarios)
        cluster_impl->check_and_get_cluster_set_for_cluster();

      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_METADATA_MISSING)
          cluster_impl->set_cluster_set_remove_pending(true);
      }
    } else if (!cs_info.removed_from_set && !cs_info.is_invalidated) {
      // this is a replica cluster, so try and rejoin the cluster set
      console->print_info("Rejoining Cluster into its original ClusterSet...");
      console->print_info();

      if (!m_options.get_dry_run()) {
        try {
          auto cluster_set_impl = cluster_impl->get_cluster_set_object();

          cluster_set_impl->get_primary_cluster()->check_cluster_online();

          cluster_set_impl->rejoin_cluster(cluster_impl->get_name(), {}, false);

          // also ensure SRO is enabled on all members
          cluster_set_impl->ensure_replica_settings(cluster_impl.get(), false);
        } catch (const shcore::Exception &e) {
          switch (e.code()) {
            case SHERR_DBA_DATA_ERRANT_TRANSACTIONS:
              console->print_warning(
                  "Unable to rejoin Cluster to the ClusterSet because this "
                  "Cluster has errant transactions that did not originate from "
                  "the primary Cluster of the ClusterSet.");
              break;
            case SHERR_DBA_GROUP_OFFLINE:
            case SHERR_DBA_GROUP_UNREACHABLE:
            case SHERR_DBA_GROUP_HAS_NO_QUORUM:
            case SHERR_DBA_CLUSTER_PRIMARY_UNAVAILABLE:
              console->print_warning(
                  "Unable to rejoin Cluster to the ClusterSet (primary Cluster "
                  "is unreachable). Please call ClusterSet.rejoinCluster() to "
                  "manually rejoin this Cluster back into the ClusterSet.");
              break;
            case SHERR_DBA_METADATA_MISSING: {
              std::string cs_domain_name;
              cluster_impl->get_metadata_storage()->check_cluster_set(
                  nullptr, nullptr, &cs_domain_name);
              console->print_warning(shcore::str_format(
                  "The Cluster '%s' appears to have been removed from the "
                  "ClusterSet '%s', however its own metadata copy wasn't "
                  "properly updated during the removal.",
                  cluster_impl->get_name().c_str(), cs_domain_name.c_str()));

              cluster_impl->set_cluster_set_remove_pending(true);
            } break;
            default:
              throw;
          }
        }
      }
    }
  }

  if (rejoin_remaning_instances && !m_options.get_dry_run()) {
    // and finally, rejoin all instances
    rejoin_instances(cluster_impl.get(), *m_target_instance, instances,
                     m_options, !cluster_is_multi_primary);
  }

  if (m_options.get_dry_run()) {
    console->print_info("dryRun finished.");
    console->print_info();

    return nullptr;
  }

  console->print_info("The Cluster was successfully rebooted.");
  console->print_info();

  return cluster_ret;
}

}  // namespace dba
}  // namespace mysqlsh
