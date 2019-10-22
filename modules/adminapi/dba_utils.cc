/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba_utils.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/global_topology.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

namespace {

std::string find_cluster_member_uri_of_role(
    const std::shared_ptr<Instance> &instance, mysqlshdk::gr::Member_role role,
    bool *out_single_primary) {
  bool has_quorum = false;
  const auto members =
      mysqlshdk::gr::get_members(*instance, out_single_primary, &has_quorum);

  if (!has_quorum) {
    throw shcore::Exception("Group has no quorum",
                            SHERR_DBA_GROUP_HAS_NO_QUORUM);
  }

  const auto &instance_uuid = instance->get_uuid();
  std::string member_uuid;

  for (const auto &member : members) {
    if (role == member.role &&
        (mysqlshdk::gr::Member_state::ONLINE == member.state ||
         mysqlshdk::gr::Member_state::RECOVERING == member.state)) {
      member_uuid = member.uuid;

      if (instance_uuid == member_uuid) {
        break;
      }
    }
  }

  return member_uuid;
}

std::string find_replicaset_member_uri_of_role(
    const std::shared_ptr<Instance> &instance, topology::Node_role role,
    bool *out_single_primary) {
  const auto md = std::make_shared<mysqlsh::dba::MetadataStorage>(instance);
  const auto &instance_uuid = instance->get_uuid();

  Cluster_metadata cmd;

  if (!md->get_cluster_for_server_uuid(instance_uuid, &cmd)) {
    throw shcore::Exception::runtime_error(
        "Could not get the InnoDB ReplicaSet metadata.");
  }

  Instance_pool::Auth_options auth_opts;
  auth_opts.get(instance->get_connection_options());
  Scoped_instance_pool ipool(md, false, auth_opts);

  topology::Server_global_topology topology("");
  topology.load_cluster(md.get(), cmd);
  topology.check_servers(true);

  switch (topology.type()) {
    case Global_topology_type::SINGLE_PRIMARY_TREE:
      if (out_single_primary) {
        *out_single_primary = true;
      }
      break;

    case Global_topology_type::NONE:
      throw shcore::Exception::logic_error(
          "The InnoDB ReplicaSet has wrong topology.");
  }

  std::string member_uuid;

  for (const auto &server : topology.servers()) {
    if (server.role() == role &&
        topology::Node_status::ONLINE == server.status()) {
      member_uuid = server.get_primary_member()->uuid;

      if (instance_uuid == member_uuid) {
        break;
      }
    }
  }

  return member_uuid;
}

std::string find_member_uri_of_role(const std::shared_ptr<Instance> &instance,
                                    bool secondary, bool xproto,
                                    bool *out_single_primary,
                                    Cluster_type *out_type) {
  MetadataStorage md{instance};
  mysqlshdk::utils::Version version;

  if (!md.check_version(&version)) {
    throw shcore::Exception(
        "Metadata schema of an InnoDB cluster or ReplicaSet not found",
        SHERR_DBA_METADATA_MISSING);
  }

  const auto &instance_uuid = instance->get_uuid();
  Cluster_type type = Cluster_type::NONE;

  if (!md.check_instance_type(instance_uuid, version, &type) ||
      Cluster_type::NONE == type) {
    throw shcore::Exception::runtime_error(
        "Could not determine whether the instance is an InnoDB cluster or "
        "ReplicaSet.");
  }

  if (out_type) {
    *out_type = type;
  }

  const auto member_uuid =
      Cluster_type::GROUP_REPLICATION == type
          ? find_cluster_member_uri_of_role(
                instance,
                secondary ? mysqlshdk::gr::Member_role::SECONDARY
                          : mysqlshdk::gr::Member_role::PRIMARY,
                out_single_primary)
          : find_replicaset_member_uri_of_role(
                instance,
                secondary ? topology::Node_role::SECONDARY
                          : topology::Node_role::PRIMARY,
                out_single_primary);

  if (instance_uuid == member_uuid) {
    return instance->get_connection_options().uri_endpoint();
  } else {
    if (member_uuid.empty()) {
      // nobody has the right role
      return "";
    }

    log_info("Looking up metadata for member %s", member_uuid.c_str());

    const auto imd = md.get_instance_by_uuid(member_uuid);

    return xproto ? imd.xendpoint : imd.endpoint;
  }
}

}  // namespace

std::string find_primary_member_uri(const std::shared_ptr<Instance> &instance,
                                    bool xproto, bool *out_single_primary,
                                    Cluster_type *out_type) {
  return find_member_uri_of_role(instance, false, xproto, out_single_primary,
                                 out_type);
}

std::string find_secondary_member_uri(const std::shared_ptr<Instance> &instance,
                                      bool xproto, bool *out_single_primary,
                                      Cluster_type *out_type) {
  return find_member_uri_of_role(instance, true, xproto, out_single_primary,
                                 out_type);
}

}  // namespace dba
}  // namespace mysqlsh
