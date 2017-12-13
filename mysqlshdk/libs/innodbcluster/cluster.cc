/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>

#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace innodbcluster {

void check_quorum(mysqlshdk::mysql::Instance *instance) {
  int unreachable, total;
  if (!gr::has_quorum(*instance, &unreachable, &total))
    throw cluster_error(
        Error::Group_has_no_quorum,
        shcore::str_format(
            "%i out of %i members of the InnoDB cluster are unreachable, which "
            "is not sufficient for a quorum to be reached.",
            unreachable, total));
}

std::string to_string(Error code) {
  switch (code) {
    case Error::Metadata_missing:
      return "Cluster metadata not found";
    case Error::Metadata_version_too_old:
      return "Cluster metadata version too old";
    case Error::Metadata_version_too_new:
      return "Unsupported metadata version";
    case Error::Metadata_inconsistent:
      return "Inconsistency found in metadata";
    case Error::Metadata_out_of_sync:
      return "Metadata inconsistent or out of date";
    case Error::Group_replication_not_active:
      return "Group Replication not active";
    case Error::Group_has_no_quorum:
      return "Insufficient ONLINE members for a quorum";
  }
  throw std::logic_error("Unknown cluster error");
}

Cluster_group_client::Cluster_group_client(
    std::shared_ptr<Metadata> md, std::shared_ptr<db::ISession> session,
    bool recovery_mode)
    : _metadata(md), _instance(new mysqlshdk::mysql::Instance(session)) {
  gr::Member_state member_state;
  std::string member_id;

  if (!_metadata->exists())
    throw cluster_error(Error::Metadata_missing,
                        "Cluster metadata schema not found");

  // TODO(alfredo) add check for metadata version
  // if (!_metadata->version_check()) {
  //   throw metadata_unsupported(
  //      "Metadata version is %s, but >= and < is required")
  // }

  // Skip basic checks that would fail if we're in recovery mode
  // (like GR not running or no quorum)
  if (!recovery_mode) {
    // Validate that the target session is actually to an instance that is part
    // of a InnoDB cluster
    if (!gr::get_group_information(*_instance, &member_state, &member_id,
                                   &_group_name, &_single_primary_mode)) {
      throw cluster_error(Error::Group_replication_not_active,
          "Instance is not a member of a InnoDB cluster");
    }
    if (member_state == gr::Member_state::OFFLINE) {
      throw cluster_error(Error::Group_replication_not_active,
          "Target member is OFFLINE in group_replication");
    }

    check_quorum(_instance.get());

    // TODO(alfredo) check that the group_session member is actually member of
    // the cluster in the metadata it holds
    // TODO(alfredo) check that GR is running and that the member is ONLINE
    // TODO(alfredo) check if group_id matches the one in metadata
  }
}

std::vector<Instance_info> Cluster_group_client::get_online_primaries() {
  std::vector<Instance_info> instances(
      _metadata->get_group_instances(_group_name));
  std::vector<gr::Member> members(gr::get_members(*_instance));

  std::vector<Instance_info> online_instances;
  // keep only instances that are ONLINE
  for (const auto &instance : instances) {
    if (std::find_if(members.begin(), members.end(),
                     [instance](const gr::Member &m) {
                       return instance.uuid == m.uuid &&
                              m.role == gr::Member_role::PRIMARY &&
                              m.state == gr::Member_state::ONLINE;
                     }) != members.end()) {
      online_instances.push_back(instance);
    }
  }
  return online_instances;
}

std::vector<Instance_info> Cluster_group_client::get_online_secondaries() {
  std::vector<Instance_info> instances(
      _metadata->get_group_instances(_group_name));
  std::vector<gr::Member> members(gr::get_members(*_instance));

  std::vector<Instance_info> online_instances;
  // keep only instances that are ONLINE
  for (const auto &instance : instances) {
    if (std::find_if(members.begin(), members.end(),
                     [instance](const gr::Member &m) {
                       return instance.uuid == m.uuid &&
                              m.role == gr::Member_role::SECONDARY &&
                              m.state == gr::Member_state::ONLINE;
                     }) != members.end()) {
      online_instances.push_back(instance);
    }
  }
  return online_instances;
}

namespace {
std::string pick_candidate_uri(
    Protocol_type type,
    const std::vector<mysqlshdk::innodbcluster::Instance_info> &candidates) {
  mysqlshdk::innodbcluster::Instance_info i(
      candidates[rand() % candidates.size()]);

  std::string redirect_uri;
  if (type == Protocol_type::Classic) {
    redirect_uri = "mysql://" + i.classic_endpoint;
    if (redirect_uri.empty()) {
      throw std::runtime_error(
          "MySQL connection endpoint not set in metadata for "
          "target instance " +
          i.name);
    }
  } else {
    redirect_uri = "mysqlx://" + i.x_endpoint;
    if (redirect_uri.empty()) {
      throw std::runtime_error(
          "X protocol endpoint not set in metadata for target "
          "instance " + i.name);
    }
  }
  return redirect_uri;
}
}  // namespace

std::string Cluster_group_client::find_uri_to_any_primary(
    Protocol_type type) {
  std::vector<mysqlshdk::innodbcluster::Instance_info> candidates;
  candidates = get_online_primaries();
  if (candidates.empty()) {
    throw std::runtime_error("No primaries found in InnoDB cluster group");
  }

  return pick_candidate_uri(type, candidates);
}

std::string Cluster_group_client::find_uri_to_any_secondary(
    Protocol_type type) {
  if (!_single_primary_mode) {
    throw std::invalid_argument(
        "Cluster is multi-primary and thus does not have secondaries");
  }

  std::vector<mysqlshdk::innodbcluster::Instance_info> candidates;
  candidates = get_online_secondaries();
  if (candidates.empty()) {
    throw std::runtime_error("No secondaries found in InnoDB cluster group");
  }

  return pick_candidate_uri(type, candidates);
}

}  // namespace innodbcluster
}  // namespace mysqlshdk
