/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/replica_set/dissolve.h"

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/topology_executor.h"
#include "modules/adminapi/replica_set/describe.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh::dba::replicaset {

void Dissolve::do_run(bool force, std::chrono::seconds timeout) {
  auto console = current_console();

  auto interactive = current_shell_options()->get().wizards;

  if (interactive) {
    {
      console->print_info(
          "The ReplicaSet still has the following registered instances:");

      // Pretty print description only if wrap_json is not json/raw.
      auto desc = Topology_executor<replicaset::Describe>{m_rset}.run();
      console->print_info(
          desc.descr(current_shell_options()->get().wrap_json != "json/raw"));
    }

    console->print_warning(
        "You are about to dissolve the whole ReplicaSet. This operation cannot "
        "be reverted. All members will be removed from the ReplicaSet and "
        "replication will be stopped, internal recovery user accounts and the "
        "ReplicaSet metadata will be dropped. User data will be maintained "
        "intact in all instances.");

    if (!force) {
      console->print_info();
      if (console->confirm("Are you sure you want to dissolve the ReplicaSet?",
                           Prompt_answer::NO) == Prompt_answer::NO) {
        throw shcore::Exception::runtime_error("Operation canceled by user.");
      }
    }

    console->print_info();
  }

  // gather instances
  std::list<std::shared_ptr<Instance>> instances_available;
  std::vector<std::string> instances_skipped;
  std::vector<std::string> instances_sync_error;
  {
    auto topology_mng = m_rset.get_topology_manager();

    std::list<Instance_metadata> unreachable;
    instances_available =
        m_rset.connect_all_members(timeout.count(), true, &unreachable);

    for (const auto &node : topology_mng->topology()->nodes()) {
      if (node->primary_master) continue;

      auto node_status = node->status();
      auto node_endpoint = node->get_primary_member()->endpoint;

      // if the status is OK, check if the instance is reachable
      if ((node_status == mysqlsh::dba::topology::Node_status::ONLINE) ||
          (node_status == mysqlsh::dba::topology::Node_status::CONNECTING)) {
        auto it = std::find_if(unreachable.begin(), unreachable.end(),
                               [primary_member = node->get_primary_member()](
                                   const auto &instance) {
                                 return (instance.uuid == primary_member->uuid);
                               });

        if (it == unreachable.end())
          continue;  // instance is OK and reachable, move to the next one

        // instance isn't reachable
        if (!force) {
          console->print_error(shcore::str_format(
              "Unable to connect to instance '%s'. Please verify connection "
              "credentials and make sure the instance is available.",
              node_endpoint.c_str()));
          if (!interactive)
            throw shcore::Exception::runtime_error(shcore::str_format(
                "The instance '%s' is '%s'", node_endpoint.c_str(),
                to_string(node_status).c_str()));

          auto res = console->confirm(
              "Do you want to continue anyway (only the instance metadata will "
              "be removed)?",
              Prompt_answer::NO);
          if (res != Prompt_answer::YES)
            throw shcore::Exception::runtime_error(shcore::str_format(
                "The instance '%s' is '%s'", node_endpoint.c_str(),
                to_string(node_status).c_str()));

        } else {
          console->print_note(shcore::str_format(
              "The instance '%s' is not reachable and it will only be "
              "removed from the metadata. Please take any necessary actions "
              "to make sure that the instance will not start/rejoin the "
              "cluster if brought back online.",
              node_endpoint.c_str()));
        }

        instances_skipped.push_back(std::move(node_endpoint));
        continue;
      }

      // reaching this point: status isn't OK

      if (force) {
        console->print_note(shcore::str_format(
            "The instance '%s' is '%s' and it will only be removed from the "
            "metadata. Please take any necessary actions to make sure that the "
            "instance will not start/join the ReplicaSet if brought back "
            "online.",
            node_endpoint.c_str(), to_string(node_status).c_str()));
        console->print_info();

        instances_skipped.push_back(std::move(node_endpoint));
        continue;
      }

      auto message = shcore::str_format(
          "The instance '%s' cannot be removed because it is on a '%s' state. "
          "Please bring the instance back ONLINE and try to dissolve the "
          "ReplicaSet again. If the instance is permanently not reachable, ",
          node_endpoint.c_str(), to_string(node_status).c_str());
      if (interactive) {
        message +=
            "then you can choose to proceed with the operation and only remove "
            "the instance from the ReplicaSet Metadata.";

      } else {
        message +=
            "then please use <ReplicaSet>.dissolve() with the force option set "
            "to true to proceed with the operation and only remove the "
            "instance from the ReplicaSet Metadata.";
      }
      console->print_error(message);

      if (!interactive)
        throw shcore::Exception::runtime_error(shcore::str_format(
            "The instance '%s' is '%s'", node_endpoint.c_str(),
            to_string(node_status).c_str()));

      {
        auto res = console->confirm(
            "Do you want to continue anyway (only the instance metadata will "
            "be removed)?",
            Prompt_answer::NO);
        console->print_info();

        if (res != Prompt_answer::YES)
          throw shcore::Exception::runtime_error(shcore::str_format(
              "The instance '%s' is '%s'", node_endpoint.c_str(),
              to_string(node_status).c_str()));
      }

      instances_skipped.push_back(std::move(node_endpoint));
    }
  }

  // sync transactions
  for (const auto &instance : instances_available) {
    if (std::find(instances_skipped.begin(), instances_skipped.end(),
                  instance->descr()) != instances_skipped.end())
      continue;

    try {
      console->print_info(shcore::str_format(
          "* Waiting for instance '%s' to apply received transactions...",
          instance->descr().c_str()));

      m_rset.sync_transactions(*instance, {k_replicaset_channel_name},
                               timeout.count());
    } catch (const std::exception &err) {
      if (force) {
        log_error(
            "Instance '%s' was unable to catch up with cluster transactions "
            "during dissolve preparation: %s",
            instance->descr().c_str(), err.what());

        instances_sync_error.push_back(instance->descr());
        continue;
      }

      mysqlsh::current_console()->print_error(shcore::str_format(
          "The instance '%s' was unable to catch up with cluster transactions. "
          "There might be too many transactions to apply or some replication "
          "error. In the former case, you can retry the operation (using a "
          "higher timeout value). In the later case, analyze and fix any "
          "replication error. You can also choose to skip this error using the "
          "'force: true' option, but it might leave the instance in an "
          "inconsistent state and lead to errors if you want to reuse it.",
          instance->descr().c_str()));

      // In interactive mode and 'force' option not used, ask user to continue
      // with the command
      if (!force) {
        if (!interactive) throw;

        auto res = console->confirm(
            "Do you want to continue anyway (only the instance metadata will "
            "be removed)?",
            Prompt_answer::NO);
        console->print_info();

        if (res != Prompt_answer::YES) throw;
      }

      instances_sync_error.push_back(instance->descr());
    }
  }

  console->print_info("* Dissolving the ReplicaSet...");

  // disable SRO
  if (auto sro = m_rset.get_cluster_server()->get_sysvar_bool("super_read_only",
                                                              false);
      sro) {
    log_info(
        "Disabling super_read_only mode on instance '%s' to run dissolve().",
        m_rset.get_cluster_server()->descr().c_str());
    m_rset.get_cluster_server()->set_sysvar("super_read_only", false);
  }

  // drop user and metadata
  {
    m_rset.drop_all_replication_users();
    metadata::uninstall(m_rset.get_cluster_server());
  }

  // for each reachable instance, sync transactions and remove channel
  for (const auto &instance : instances_available) {
    // 1st: sync transactions (i.e.: drop user and metadata)
    if (std::find(instances_skipped.begin(), instances_skipped.end(),
                  instance->descr()) == instances_skipped.end()) {
      try {
        console->print_info(shcore::str_format(
            "* Waiting for instance '%s' to apply received transactions...",
            instance->descr().c_str()));

        m_rset.sync_transactions(*instance, {k_replicaset_channel_name},
                                 timeout.count());

        // if we were able to sync, then there's no error
        instances_sync_error.erase(
            std::remove_if(
                instances_sync_error.begin(), instances_sync_error.end(),
                mysqlshdk::utils::Endpoint_predicate{instance->descr()}),
            instances_sync_error.end());

      } catch (const std::exception &err) {
        // Skip error if force is true otherwise issue an error.
        if (!force) {
          console->print_error(shcore::str_format(
              "The instance '%s' was unable to catch up with the ReplicaSet "
              "transactions. There might be too many transactions to apply or "
              "some replication error.",
              instance->descr().c_str()));
          throw;
        }

        if (std::find(instances_sync_error.begin(), instances_sync_error.end(),
                      instance->descr()) == instances_sync_error.end()) {
          instances_sync_error.push_back(instance->descr());
        }

        log_error(
            "Instance '%s' was unable to catch up with cluster transactions: "
            "%s",
            instance->descr().c_str(), err.what());
        console->print_warning(shcore::str_format(
            "An error occurred when trying to catch up with the ReplicaSet "
            "transactions and instance '%s' might have been left in an "
            "inconsistent state that will lead to errors if it is reused.",
            instance->descr().c_str()));
        console->print_info();
      }
    }

    // 2nd: remove replication channel
    try {
      async_remove_replica(instance.get(), k_replicaset_channel_name, false);
    } catch (const shcore::Exception &err) {
      // Skip error if force=true otherwise issue an error.
      if (!force) {
        console->print_error(shcore::str_format(
            "Unable to remove instance '%s' from the ReplicaSet.",
            instance->descr().c_str()));
        throw shcore::Exception::runtime_error(err.what());
      }

      instances_skipped.push_back(instance->descr());
      log_error("Instance '%s' failed to leave the cluster: %s",
                instance->descr().c_str(), err.what());
      console->print_warning(shcore::str_format(
          "An error occurred when trying to remove instance '%s' from the "
          "ReplicaSet. The instance might have been left active and in an "
          "inconsistent state, requiring manual action to fully dissolve the "
          "ReplicaSet.",
          instance->descr().c_str()));
      console->print_info();
    }
  }

  m_rset.disconnect();

  // Print appropriate output message depending if some operation was skipped.
  console->print_info();

  // entire ReplicaSet successfully removed.
  if (instances_skipped.empty() && instances_sync_error.empty()) {
    console->print_info("The ReplicaSet was successfully dissolved.");
    console->print_info(
        "Replication was disabled but user data was left intact.");
    console->print_info();
    return;
  }

  if (!instances_skipped.empty()) {
    // Some instance were skipped and not properly removed from the cluster
    // despite being dissolved.
    std::string warning_msg;
    warning_msg.reserve(240 + (instances_skipped.size() * 50));

    warning_msg
        .append(
            "The ReplicaSet was successfully dissolved, but the following "
            "instance")
        .append((instances_skipped.size() > 1) ? "s were " : " was ")
        .append("skipped: '")
        .append(shcore::str_join(instances_skipped, "', '"))
        .append("'. Please make sure ")
        .append((instances_skipped.size() > 1) ? "these instances are "
                                               : "this instance is ")
        .append(
            "permanently unavailable or take any necessary manual action to "
            "ensure the ReplicaSet is fully dissolved.");

    console->print_warning(warning_msg);
    return;
  }

  // Some instance failed to catch up with cluster transaction and cluster's
  // metadata might not have been removed.
  assert(!instances_sync_error.empty());

  std::string warning_msg;
  warning_msg.reserve(240 + (instances_sync_error.size() * 50));

  warning_msg
      .append(
          "The ReplicaSet was successfully dissolved, but the following "
          "instance")
      .append((instances_sync_error.size() > 1) ? "s were " : " was ")
      .append("unable to catch up with the ReplicaSet transactions: '")
      .append(shcore::str_join(instances_sync_error, "', '"))
      .append("'. Please make sure the ReplicaSet metadata was removed on ")
      .append((instances_sync_error.size() > 1) ? "these instances "
                                                : "this instance ")
      .append("in order to be able to be reused.");

  console->print_warning(warning_msg);
}

}  // namespace mysqlsh::dba::replicaset
