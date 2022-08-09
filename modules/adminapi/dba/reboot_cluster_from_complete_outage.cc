/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/mod_dba.h"

#include <algorithm>
#include <string_view>

#include "modules/adminapi/cluster/cluster_join.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

namespace {

struct Cluster_set_info {
  bool is_member;
  bool is_primary;
  bool is_primary_invalidated;
  bool removed_from_set;
  Cluster_global_status primary_status;
};

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

/*
 * Retrieves all instances of the cluster.
 */
std::vector<std::shared_ptr<Instance>> retrieve_instances(
    Cluster_impl *cluster, const mysqlshdk::mysql::IInstance &target_instance,
    std::vector<Instance_metadata> *instances_unreachable,
    const shcore::Option_pack_ref<Reboot_cluster_options> &options) {
  // check quorum
  check_instance_type(target_instance,
                      get_member_name("forceQuorumUsingPartitionOf",
                                      shcore::current_naming_style()));

  auto instances = cluster->get_instances();

  std::vector<std::shared_ptr<Instance>> out_instances;
  out_instances.reserve(instances.size());

  for (const auto &i : instances) {
    // skip the target
    if (i.uuid == target_instance.get_uuid()) continue;

    std::shared_ptr<Instance> instance;
    try {
      log_info("Opening a new session to the instance: %s", i.endpoint.c_str());
      instance = cluster->connect_target_instance(i.endpoint, false, false);
    } catch (const shcore::Error &e) {
      log_info("Unable to open a connection to the instance: %s",
               i.endpoint.c_str());
      if (instances_unreachable) instances_unreachable->emplace_back(i);
      continue;
    }

    log_info("Checking state of instance '%s'", i.endpoint.c_str());

    auto instance_type = check_instance_type(*instance, {});

    if (instance_type == TargetType::Standalone) {
      if (!options->get_force()) {
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
std::shared_ptr<Instance> pick_best_instance_gtid(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::vector<std::shared_ptr<Instance>> &instances, bool force,
    std::string_view intended_instance) {
  std::vector<Instance_gtid_info> instance_gtids;
  {
    // list of replication channel names that must be considered when comparing
    // GTID sets. With ClusterSets, the async channel for secondaries must be
    // added here.
    const std::vector<std::string> k_known_channel_names = {
        "group_replication_applier"};

    auto current_session_options = target_instance.get_connection_options();

    {
      Instance_gtid_info info;
      info.server = target_instance.get_canonical_address();
      info.gtid_executed = mysqlshdk::mysql::get_total_gtid_set(
          target_instance, k_known_channel_names);
      instance_gtids.push_back(std::move(info));
    }

    for (const auto &i : instances) {
      Instance_gtid_info info;
      info.server = i->get_canonical_address();
      info.gtid_executed =
          mysqlshdk::mysql::get_total_gtid_set(*i, k_known_channel_names);
      instance_gtids.push_back(std::move(info));
    }
    assert(instance_gtids.size() == (instances.size() + 1));
  }

  std::set<std::string> conflits;
  auto primary_candidates =
      filter_primary_candidates(target_instance, instance_gtids,
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

    assert(it != instance_gtids.end());  // checked earlier

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

/*
 * Reboots the seed instance
 */
void reboot_seed(
    Cluster_impl *cluster_impl,
    const std::shared_ptr<Instance> &target_instance,
    const Cluster_set_info &cs_info,
    const shcore::Option_pack_ref<Reboot_cluster_options> &options) {
  // if the cluster isn't part of a set anymore
  if (cs_info.removed_from_set) {
    // enable mysql_disable_super_read_only_if_primary if needed
    if (bool enabled;
        !mysqlshdk::gr::get_member_action_status(
            *target_instance,
            mysqlshdk::gr::k_gr_disable_super_read_only_if_primary, &enabled) ||
        !enabled) {
      log_info("Enabling automatic super_read_only management on '%s'",
               target_instance->descr().c_str());

      mysqlshdk::gr::enable_member_action(
          *target_instance,
          mysqlshdk::gr::k_gr_disable_super_read_only_if_primary,
          mysqlshdk::gr::k_gr_member_action_after_primary_election);
    }

    // remove replication channel
    if (!cs_info.is_primary) {
      if (mysqlshdk::mysql::Replication_channel channel;
          mysqlshdk::mysql::get_channel_status(
              *target_instance, k_clusterset_async_channel_name, &channel)) {
        auto status = channel.status();
        log_info("State of clusterset replication channel: %d", status);

        if (status == mysqlshdk::mysql::Replication_channel::OFF) {
          try {
            mysqlshdk::mysql::reset_slave(
                target_instance.get(), k_clusterset_async_channel_name, true);
          } catch (const shcore::Error &e) {
            throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
          }
        }
      }
    }
  }

  Group_replication_options gr_options(Group_replication_options::NONE);

  // Set the communicationStack if option used
  if (options->switch_communication_stack)
    gr_options.communication_stack = *(options->switch_communication_stack);

  // Set localAddress if option used
  if (!options->gr_options.local_address.is_null()) {
    gr_options.local_address = options->gr_options.local_address.get_safe();
  } else {
    // Ensure it'll be auto resolved later
    gr_options.local_address = nullptr;
  }

  // Set the ipAllowlist if option used
  if (!options->gr_options.ip_allowlist.is_null()) {
    gr_options.ip_allowlist = options->gr_options.ip_allowlist;
  }

  cluster::Cluster_join joiner(
      cluster_impl, nullptr, target_instance, gr_options, {},
      current_shell_options()->get().wizards,
      static_cast<bool>(options->switch_communication_stack));

  joiner.prepare_reboot();
  joiner.reboot();

  current_console()->print_info(target_instance->descr() + " was restored.");
}

/*
 * Rejoins all valid instances back into the cluster.
 */
void rejoin_instances(
    Cluster_impl *cluster_impl, const Instance &target_instance,
    const std::vector<std::shared_ptr<Instance>> &instances,
    const shcore::Option_pack_ref<Reboot_cluster_options> &options) {
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

      // Set the ipAllowlist if option used
      Group_replication_options gr_options(Group_replication_options::NONE);
      if (options->gr_options.ip_allowlist)
        gr_options.ip_allowlist = options->gr_options.ip_allowlist;

      cluster::Cluster_join joiner(
          cluster_impl, &target_instance, instance, gr_options, {}, false,
          static_cast<bool>(options->switch_communication_stack));

      bool uuid_mistmatch = false;
      if (joiner.prepare_rejoin(&uuid_mistmatch,
                                cluster::Cluster_join::Intent::Reboot))
        joiner.rejoin(true);

      if (uuid_mistmatch) cluster_impl->update_metadata_for_instance(*instance);

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
    if (!instance->get_sysvar_bool("super_read_only").get_safe()) {
      log_info("Enabling super_read_only on instance '%s'...",
               instance->descr().c_str());
      instance->set_sysvar("super_read_only", true);
    }
  }

  // Verification step to ensure the server_id is an attribute on all
  // the instances of the cluster
  cluster_impl->ensure_metadata_has_server_id(target_instance);
}

/*
 * Retrieves info regarding the clusters ClusterSet (if any)
 */
Cluster_set_info retrieve_cs_info(Cluster_impl *cluster) {
  Cluster_set_info cs_info;
  cs_info.is_member = cluster->is_cluster_set_member();
  cs_info.is_primary = cs_info.is_member && cluster->is_primary_cluster();
  cs_info.is_primary_invalidated = false;
  cs_info.removed_from_set = false;
  cs_info.primary_status = Cluster_global_status::UNKNOWN;

  if (!cs_info.is_member) return cs_info;

  auto cs = cluster->get_cluster_set_object();

  {
    if (!cs_info.is_primary) cs->connect_primary();
    auto pc = cs->get_primary_cluster();

    if (cs_info.is_primary)
      cs_info.is_primary_invalidated = pc->get_id() != cluster->get_id();
    else {
      cs_info.primary_status = cs->get_cluster_global_status(pc.get());
    }
  }

  try {
    auto target_cluster = cs->get_cluster(cluster->get_name(), true, true);

    cs_info.removed_from_set = target_cluster->is_invalidated();
    if (!cs_info.removed_from_set) {
      target_cluster->check_and_get_cluster_set_for_cluster();

      cs_info.removed_from_set =
          target_cluster->is_cluster_set_remove_pending();
    }
  } catch (const shcore::Exception &e) {
    if ((e.code() != SHERR_DBA_METADATA_MISSING) &&
        (e.code() != SHERR_DBA_ASYNC_MEMBER_INVALIDATED))
      throw;
    cs_info.removed_from_set = true;
  }

  log_info(
      "ClusterSet info: %smember, %sprimary, %sprimary_invalidated, %sremoved "
      "from set, primary status: %s",
      cs_info.is_member ? "" : "not ", cs_info.is_primary ? "" : "not ",
      cs_info.is_primary_invalidated ? "" : "not ",
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

}  // namespace

std::shared_ptr<Cluster> Dba::reboot_cluster_from_complete_outage(
    const mysqlshdk::null_string &cluster_name,
    const shcore::Option_pack_ref<Reboot_cluster_options> &options) {
  std::shared_ptr<MetadataStorage> metadata;
  std::shared_ptr<Instance> target_instance;
  // The cluster is completely dead, so we can't find the primary anyway...
  // thus this instance will be used as the primary
  connect_to_target_group({}, &metadata, &target_instance, false);

  check_function_preconditions("Dba.rebootClusterFromCompleteOutage", metadata,
                               target_instance);

  // Validate and handle command options
  {
    // Get the communication in stack that was being used by the Cluster
    auto target_instance_version = target_instance->get_version();
    std::string comm_stack = kCommunicationStackXCom;

    if (target_instance_version >=
        k_mysql_communication_stack_initial_version) {
      comm_stack =
          target_instance
              ->get_persisted_value("group_replication_communication_stack")
              .get_safe();
    }

    // Validate the options:
    //   - switchCommunicationStack
    //   - localAddress
    static_cast<Reboot_cluster_options>(*options).check_option_values(
        target_instance_version, target_instance->get_canonical_port(),
        comm_stack);
  }

  Scoped_instance_pool ipool(
      metadata, false,
      Instance_pool::Auth_options{target_instance->get_connection_options()});

  const auto console = current_console();

  if (options->get_dry_run())
    console->print_note(
        "dryRun option was specified. Validations will be executed, but no "
        "changes will be applied.");

  // Getting the cluster from the metadata already complies with:
  //  - ensure that a Metadata Schema exists on the current session instance
  //  - ensure that the provided cluster identifier exists on the Metadata
  // Schema
  std::shared_ptr<mysqlsh::dba::Cluster> cluster;
  try {
    cluster = get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                          metadata, target_instance, true, true);
  } catch (const shcore::Error &e) {
    // If the GR plugin is not installed, we can get this error.
    // In that case, we install the GR plugin and retry.
    if (e.code() != ER_UNKNOWN_SYSTEM_VARIABLE) throw;

    if (!options->get_dry_run()) {
      log_info("%s: installing GR plugin (%s)",
               target_instance->descr().c_str(), e.format().c_str());

      mysqlshdk::gr::install_group_replication_plugin(*target_instance,
                                                      nullptr);

      cluster = get_cluster(!cluster_name ? nullptr : cluster_name->c_str(),
                            metadata, target_instance, true, true);
    }
  }

  if (!cluster) {
    if (!cluster_name)
      throw shcore::Exception::logic_error("No default Cluster is configured.");
    throw shcore::Exception::logic_error(shcore::str_format(
        "The Cluster '%s' is not configured.", cluster_name->c_str()));
  }

  auto cluster_impl = cluster->impl();
  auto cs_info = retrieve_cs_info(cluster_impl.get());

  if (!options->get_dry_run()) {
    console->print_info("Restoring the Cluster '" + cluster_impl->get_name() +
                        "' from complete outage...");
    console->print_info();
  }

  // after all the checks are done, this will store all the instances we need to
  // work with
  std::vector<std::shared_ptr<Instance>> instances;
  {
    // ensure that the current session instance exists on the Metadata Schema
    if (std::string group_md_address = target_instance->get_canonical_address();
        !cluster_impl->contains_instance_with_address(group_md_address))
      throw shcore::Exception::runtime_error(
          "The current session instance does not belong to the Cluster: '" +
          cluster_impl->get_name() + "'.");

    // gather all instances
    std::vector<Instance_metadata> instances_unreachable;
    instances = retrieve_instances(cluster_impl.get(), *target_instance,
                                   &instances_unreachable, options);

    // gather all (including target instance) instances states
    std::unordered_map<std::shared_ptr<Instance>, mysqlshdk::gr::Member_state>
        istates;

    istates.rehash(instances.size() + 1);
    istates[target_instance] =
        mysqlshdk::gr::get_member_state(*target_instance);
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
        istates, options->get_dry_run(), &instances_online);

    assert(!members_all_offline || instances_online.empty());

    // reaching this point, we have all the info needed to decide if the command
    // can proceed.

    if (members_all_offline)
      log_info("All members of the Cluster are either offline or unreachable.");

    // if any instance is online, we can't proceed
    if (!instances_online.empty())
      throw std::runtime_error("The Cluster is ONLINE");

    if (!cs_info.is_primary_invalidated &&
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

      if (!options->get_force())
        throw std::runtime_error(
            "Could not determine if Cluster is completely OFFLINE");
    }
  }

  if (options->switch_communication_stack && cs_info.is_primary_invalidated)
    throw shcore::Exception::runtime_error(
        "Cannot switch the communication stack in an invalidated Cluster "
        "because its instances won't be removed or rejoined.");

  if (!options->gr_options.local_address.is_null()) {
    console->print_warning(
        "The value used for 'localAddress' only applies to the current "
        "session instance (seed). If the values generated automatically for "
        "other rejoining Cluster members are not valid, please use "
        "<Cluster>.<<<rejoinInstance>>>() with the 'localAddress' option.");
    console->print_info();
  }

  // pick the seed instance
  std::shared_ptr<Instance> best_instance_gtid;
  {
    best_instance_gtid =
        pick_best_instance_gtid(*target_instance, instances,
                                options->get_force(), options->get_primary());

    // check if the other instances are compatible in regards to GTID with the
    // (new) seed
    if (!options->get_force()) {
      auto new_seed = best_instance_gtid ? best_instance_gtid : target_instance;

      instances.push_back(target_instance);  // temporary

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
  if (cs_info.is_primary_invalidated && options->get_force())
    throw std::runtime_error(shcore::str_format(
        "The 'force' option cannot be used in a Cluster that belongs to a "
        "ClusterSet and is PRIMARY INVALIDATED."));

  // everything is ready, proceed with the reboot...

  // reboot seed
  if (best_instance_gtid) {
    // if we have a better instance in the cluster (in terms of highest GTID),
    // use it instead of the target
    if (best_instance_gtid) {
      if (mysqlshdk::utils::are_endpoints_equal(
              best_instance_gtid->get_canonical_address(),
              options->get_primary())) {
        console->print_info(shcore::str_format(
            "Switching over to instance '%s' to be used as seed.",
            best_instance_gtid->get_canonical_address().c_str()));
      } else {
        console->print_info(shcore::str_format(
            "Switching over to instance '%s' (which has the highest GTID set), "
            "to be used as seed.",
            best_instance_gtid->get_canonical_address().c_str()));
      }
    }

    if (!options->get_dry_run()) {
      reboot_seed(cluster_impl.get(), best_instance_gtid, cs_info, options);

      instances.push_back(std::exchange(target_instance, best_instance_gtid));
      best_instance_gtid = nullptr;

      // refresh metadata and cluster info
      metadata.reset();

      connect_to_target_group(target_instance, &metadata, nullptr, false);
      ipool->set_metadata(metadata);

      cluster = get_cluster(cluster_impl->get_name().c_str(), metadata,
                            target_instance, true, true);

      cluster_impl = cluster->impl();
    }

  } else if (!options->get_dry_run()) {
    // use the target instance as seed
    reboot_seed(cluster_impl.get(), target_instance, cs_info, options);
  }

  // don't rejoin the instances *if* cluster is in a cluster set and is
  // invalidated (former primary) or is a replica and the primary doesn't have
  // global status OK
  if (cs_info.is_member &&
      (cs_info.is_primary_invalidated ||
       (!cs_info.is_primary &&
        cs_info.primary_status != Cluster_global_status::OK))) {
    auto msg = cs_info.is_primary_invalidated
                   ? "Skipping rejoining remaining instances because the "
                     "Cluster belongs to a ClusterSet and is INVALIDATED. "
                     "Please add or remove the instances after the Cluster is "
                     "rejoined to the ClusterSet."
                   : "Skipping rejoining remaining instances because the "
                     "Cluster belongs to a ClusterSet and its global status is "
                     "not OK. Please add or remove the instances after the "
                     "Cluster is rejoined to the ClusterSet.";
    console->print_info(msg);

  } else if (!options->get_dry_run()) {
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
      if (!options->get_dry_run() && cs) {
        // Acquire primary to ensure the correct primary member will be used
        // from now on when the Cluster belongs to a ClusterSet.
        cluster_impl->acquire_primary();

        // If the communication stack was changed and this is a replica cluster,
        // we must ensure here that 'grLocal' reflects the new value for local
        // address
        if (!cs_info.is_primary &&
            options->switch_communication_stack.has_value()) {
          cluster_impl->update_metadata_for_instance(*target_instance);
        }
      }
    }

    rejoin_instances(cluster_impl.get(), *target_instance, instances, options);
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
    } else if (!cs_info.removed_from_set) {
      // this is a replica cluster, so try and rejoin the cluster set
      console->print_info("Rejoining Cluster into its original ClusterSet...");
      console->print_info();

      if (!options->get_dry_run()) {
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
              metadata->check_cluster_set(nullptr, nullptr, &cs_domain_name);
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

  if (options->get_dry_run()) {
    console->print_info("dryRun finished.");
    console->print_info();

    return nullptr;
  }

  console->print_info("The Cluster was successfully rebooted.");
  console->print_info();

  return cluster;
}  // namespace mysqlsh

}  // namespace dba
}  // namespace mysqlsh
