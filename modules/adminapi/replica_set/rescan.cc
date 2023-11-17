/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/replica_set/rescan.h"

#include <ranges>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh::dba::replicaset {

namespace {

constexpr std::string_view k_instance_attribute_server_id{"server_id"};

struct UnmanagedInstanceInfo {
  std::string uuid;
  std::string host;
  int port;
};

struct ObsoleteInstanceInfo {
  std::string uuid;
  std::string label;
  std::string endpoint;
  bool invalidated{false};
  bool ghost{false};
};

std::string str_join_natural(const auto &container, auto &&transform) {
  if (container.empty()) return {};
  if (container.size() == 1) return transform(container.front());

  std::string str_final;
  str_final.reserve(container.size() *
                    10);  // assume 10 characters per item (better than nothing)

  auto end = std::prev(container.end());
  for (auto it = container.begin(); it != end; ++it) {
    str_final.append(1, '\'').append(transform(*it)).append("', ");
  }

  str_final.erase(str_final.size() - 2);
  str_final.append(" and '")
      .append(transform(container.back()))
      .append(1, '\'');

  return str_final;
}

}  // namespace

void Rescan::do_run(bool add_unmanaged, bool remove_obsolete) {
  auto console = mysqlsh::current_console();
  console->print_info("Rescanning the ReplicaSet...");
  console->print_info();

  {
    Scoped_instance_list instances(
        m_rset.connect_all_members(0, false, nullptr, true));

    scan_server_ids(instances);
  }

  bool changes_ignored{false};
  scan_topology(add_unmanaged, remove_obsolete, &changes_ignored);

  {
    Scoped_instance_list instances(
        m_rset.connect_all_members(0, false, nullptr, true));

    scan_replication_accounts(instances);
  }

  console->print_info("Rescan finished.");

  if (m_invalidate_rset) {
    log_info("Invalidating ReplicaSet object.");
    m_rset.disconnect();
  }
}

void Rescan::scan_topology(bool add_unmanaged, bool remove_obsolete,
                           bool *changes_ignored) {
  log_info("Scanning ReplicaSet topology...");

  auto interactive = current_shell_options()->get().wizards;

  auto primary = m_rset.get_primary_master();

  if (changes_ignored) *changes_ignored = false;

  // get list of new and obsolete instances
  std::vector<UnmanagedInstanceInfo> unmanaged_instances;
  std::vector<ObsoleteInstanceInfo> obsolete_instances;
  {
    std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
        ghost_slaves;
    auto slaves = Replica_set_impl::get_valid_slaves(*primary, &ghost_slaves);

    auto known_instances = m_rset.get_instances_from_metadata();

    for (const auto &shost : slaves) {
      const auto &s = std::get<0>(shost);

      auto it =
          std::find_if(known_instances.begin(), known_instances.end(),
                       [&s](const auto &i) { return (i.uuid == s.uuid); });

      if (it != known_instances.end()) continue;

      unmanaged_instances.push_back(UnmanagedInstanceInfo{
          .uuid = s.uuid, .host = s.host, .port = s.port});
    }

    for (const auto &i : known_instances) {
      if (i.uuid == primary->get_uuid()) continue;

      auto it = std::find_if(slaves.begin(), slaves.end(), [&i](const auto &s) {
        return (i.uuid == std::get<0>(s).uuid);
      });

      if (it != slaves.end()) continue;

      auto ghost = std::any_of(
          ghost_slaves.begin(), ghost_slaves.end(),
          [&i](const auto &g) { return (i.uuid == std::get<0>(g).uuid); });

      obsolete_instances.push_back(
          ObsoleteInstanceInfo{.uuid = i.uuid,
                               .label = i.label,
                               .endpoint = i.endpoint,
                               .invalidated = i.invalidated,
                               .ghost = ghost});
    }
  }

  if (unmanaged_instances.empty() && obsolete_instances.empty()) {
    log_info("No updates required.");
    return;
  }

  auto console = mysqlsh::current_console();

  const auto &md = m_rset.get_metadata_storage();
  MetadataStorage::Transaction trx(md);

  // can't add new instances if auth type is other than PASSWORD
  if (!unmanaged_instances.empty() &&
      (m_rset.query_cluster_auth_type() != Replication_auth_type::PASSWORD)) {
    console->print_info(
        "New instances were discovered in the ReplicaSet but ignored because "
        "the ReplicaSet requires SSL certificate authentication.\nPlease stop "
        "the asynchronous channel on those instances and then add them to the "
        "ReplicaSet using <<<addInstance>>>() with the appropriate "
        "authentication options.");

    if (changes_ignored) *changes_ignored = true;

    // but we want to be able to proceed, so assume we don't have any new
    // instances
    unmanaged_instances.clear();
  }

  // add new instances
  if (!unmanaged_instances.empty()) {
    auto add_instance = [this, &md, &primary](const UnmanagedInstanceInfo &i) {
      auto endpoint = mysqlshdk::utils::make_host_and_port(i.host, i.port);
      log_info("Adding instance '%s' to the metadata.", endpoint.c_str());

      const Scoped_instance target_instance(
          m_rset.connect_target_instance(endpoint));

      Instance_metadata inst = query_instance_info(*target_instance, false);
      inst.cluster_id = m_rset.get_id();
      inst.primary_master = false;
      inst.instance_type = Instance_type::ASYNC_MEMBER;
      inst.replica_type = Replica_type::NONE;
      inst.master_id = md->get_instance_by_uuid(primary->get_uuid()).id;
      inst.master_uuid = primary->get_uuid();

      md->record_async_member_added(inst);
    };

    if (add_unmanaged) {
      for (const auto &i : unmanaged_instances) {
        auto endpoint = mysqlshdk::utils::make_host_and_port(i.host, i.port);
        console->print_info(shcore::str_format(
            "Adding instance '%s' to the ReplicaSet metadata...",
            endpoint.c_str()));

        add_instance(i);
      }
    } else if (interactive) {
      for (const auto &i : unmanaged_instances) {
        auto endpoint = mysqlshdk::utils::make_host_and_port(i.host, i.port);
        if (console->confirm(
                shcore::str_format(
                    "The instance '%s' is part of the replication topology but "
                    "isn't present in the metadata. Do you wish to add it?",
                    endpoint.c_str()),
                Prompt_answer::NO) == Prompt_answer::NO) {
          if (changes_ignored) *changes_ignored = true;
          continue;
        }

        console->print_info("Adding instance to the ReplicaSet metadata...");
        add_instance(i);
      }
    } else {
      if (changes_ignored) *changes_ignored = true;
      if (unmanaged_instances.size() == 1) {
        const auto &i = unmanaged_instances.front();
        console->print_info(shcore::str_format(
            "The instance '%s' is part of the replication topology but isn't "
            "present in the metadata. Use the \"addUnmanaged\" option to "
            "automatically "
            "add the instance to the ReplicaSet metadata.",
            mysqlshdk::utils::make_host_and_port(i.host, i.port).c_str()));

      } else {
        auto instance_list =
            str_join_natural(unmanaged_instances, [](const auto &i) {
              return mysqlshdk::utils::make_host_and_port(i.host, i.port);
            });

        console->print_info(shcore::str_format(
            "The instances %s are part of the ReplicaSet but aren't present in "
            "the metadata. Use the \"addUnmanaged\" option to automatically "
            "add them to the ReplicaSet metadata.",
            instance_list.c_str()));
      }
    }
  }

  // remove obsolete instances
  if (!obsolete_instances.empty()) {
    auto remove_instance = [this, &md, &console](std::string_view endpoint,
                                                 std::string_view uuid,
                                                 bool print_endpoint) {
      if (print_endpoint)
        console->print_info(shcore::str_format(
            "Removing instance '%.*s' from the ReplicaSet metadata...",
            static_cast<int>(endpoint.size()), endpoint.data()));
      else
        console->print_info(
            "Removing instance from the ReplicaSet metadata...");

      md->remove_instance(endpoint);

      // invalidate the replicaset object if we just removed the server from it
      if (!m_invalidate_rset &&
          (m_rset.get_cluster_server()->get_uuid() == uuid))
        m_invalidate_rset = true;
    };

    if (remove_obsolete) {
      for (const auto &i : obsolete_instances) {
        if (i.invalidated) {
          console->print_warning(shcore::str_format(
              "Ignoring instance '%s' because it's INVALIDATED. Please rejoin "
              "or remove it from the ReplicaSet.",
              i.endpoint.c_str()));

          continue;
        }

        remove_instance(i.endpoint, i.uuid, true);
      }
    } else if (interactive) {
      for (const auto &i : obsolete_instances) {
        if (i.invalidated) {
          console->print_warning(shcore::str_format(
              "Ignoring instance '%s' because it's INVALIDATED. Please rejoin "
              "or remove it from the ReplicaSet.",
              i.endpoint.c_str()));

          continue;
        }

        if (i.ghost) {
          console->print_warning(
              shcore::str_format("Replication is not active in instance '%s', "
                                 "however, it is configured.",
                                 i.endpoint.c_str()));
        }

        if (console->confirm(
                shcore::str_format("The instance '%s' is no longer part of the "
                                   "ReplicaSet but is still present in the "
                                   "metadata. Do you wish to remove it?",
                                   i.label.c_str()),
                Prompt_answer::NO) == Prompt_answer::NO) {
          if (changes_ignored) *changes_ignored = true;
          continue;
        }

        remove_instance(i.endpoint, i.uuid, false);
      }
    } else {
      // discard invalidated
      std::erase_if(obsolete_instances,
                    [](const auto &i) { return i.invalidated; });

      if (!obsolete_instances.empty()) {
        if (changes_ignored) *changes_ignored = true;

        if (obsolete_instances.size() == 1) {
          console->print_info(shcore::str_format(
              "The instance '%s' is no longer part of the ReplicaSet but is "
              "still present in the metadata. Use the \"removeObsolete\" "
              "option to automatically remove the instance.",
              obsolete_instances.front().label.c_str()));
        } else {
          auto instance_list = str_join_natural(
              obsolete_instances,
              [](const auto &i) { return i.invalidated ? "" : i.label; });

          console->print_info(shcore::str_format(
              "The instances %s are no longer part of the ReplicaSet but are "
              "still present in the metadata. Use the \"removeObsolete\" "
              "option to automatically remove them.",
              instance_list.c_str()));
        }
      }
    }
  }

  trx.commit();
}

void Rescan::scan_replication_accounts(const Scoped_instance_list &instances) {
  log_info("Scanning ReplicaSet instances replication accounts...");

  auto console = mysqlsh::current_console();

  auto primary = m_rset.get_primary_master();

  const auto &md = m_rset.get_metadata_storage();
  MetadataStorage::Transaction trx(md);

  std::string host = "%";
  if (shcore::Value allowed_host;
      md->query_cluster_attribute(m_rset.get_id(),
                                  k_cluster_attribute_replication_allowed_host,
                                  &allowed_host) &&
      allowed_host.get_type() == shcore::String &&
      !allowed_host.as_string().empty()) {
    host = allowed_host.as_string();
  }

  bool updated_account{false};

  // make sure that the account format is correct
  for (const auto &i : instances.list()) {
    if (i->get_uuid() == primary->get_uuid()) continue;

    auto replication_user =
        mysqlshdk::mysql::get_replication_user(*i, k_replicaset_channel_name);

    auto replication_intended_user =
        Base_cluster_impl::make_replication_user_name(
            i->get_server_id(), k_async_cluster_user_name);

    if (replication_intended_user == replication_user) continue;

    updated_account = true;
    console->print_info(shcore::str_format(
        "Updating replication account for instance '%s'.", i->descr().c_str()));

    const auto [new_auth_options, new_host] = m_rset.create_replication_user(
        i.get(), m_rset.query_cluster_instance_auth_cert_subject(*i), false);

    {
      Async_replication_options repl_options;
      repl_options.repl_credentials = new_auth_options;

      async_update_replica_credentials(i.get(), k_replicaset_channel_name,
                                       repl_options, false);
    }

    md->update_instance_repl_account(
        i->get_uuid(), Cluster_type::ASYNC_REPLICATION, Replica_type::NONE,
        new_auth_options.user, new_host);
  }

  // make sure that the MD has the account
  for (const auto &i : instances.list()) {
    if (i->get_uuid() == primary->get_uuid()) continue;

    auto replication_user =
        mysqlshdk::mysql::get_replication_user(*i, k_replicaset_channel_name);

    auto account = md->get_instance_repl_account(
        i->get_uuid(), Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);
    if ((std::get<0>(account) == replication_user) &&
        (std::get<1>(account) == host))
      continue;

    updated_account = true;
    console->print_info(shcore::str_format(
        "Updating replication account for instance '%s'.", i->descr().c_str()));

    md->update_instance_repl_account(
        i->get_uuid(), Cluster_type::ASYNC_REPLICATION, Replica_type::NONE,
        replication_user, host);
  }

  trx.commit();

  if (!updated_account) log_info("No updates required.");
}

void Rescan::scan_server_ids(const Scoped_instance_list &instances) {
  log_info("Scanning ReplicaSet instances server_uuid and server_id...");

  auto console = mysqlsh::current_console();

  const auto &md = m_rset.get_metadata_storage();
  MetadataStorage::Transaction trx(md);

  bool updated_id{false};
  for (const auto &i : instances.list()) {
    auto i_md = md->get_instance_by_address(i->get_canonical_address());

    if ((i_md.uuid == i->get_uuid()) && (i_md.server_id == i->get_server_id()))
      continue;

    updated_id = true;
    console->print_info(shcore::str_format(
        "Updating server id for instance '%s'.", i->descr().c_str()));

    if (i_md.uuid != i->get_uuid())
      md->update_instance_uuid(m_rset.get_id(), i_md.id, i->get_uuid());

    if (i_md.server_id != i->get_server_id())
      md->update_instance_attribute(i->get_uuid(),
                                    k_instance_attribute_server_id,
                                    shcore::Value{i->get_server_id()}, false);
  }

  trx.commit();

  if (!updated_id) log_info("No updates required.");
}

}  // namespace mysqlsh::dba::replicaset
