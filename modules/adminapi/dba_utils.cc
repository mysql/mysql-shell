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
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

namespace {
std::string find_member_uri_of_role(const std::shared_ptr<Instance> &instance,
                                    mysqlshdk::gr::Member_role role,
                                    bool xproto, bool *out_single_primary) {
  mysqlsh::dba::MetadataStorage md(instance);
  if (!md.check_version()) {
    throw shcore::Exception("InnoDB cluster metadata schema not found",
                            SHERR_DBA_METADATA_MISSING);
  }

  // check if we have the desired role
  bool session_has_the_right_role = false;
  std::string uuid = instance->get_uuid();
  std::string other_uuid;
  bool has_quorum = false;
  auto members =
      mysqlshdk::gr::get_members(*instance, out_single_primary, &has_quorum);
  if (!has_quorum) {
    throw shcore::Exception("Group has no quorum",
                            SHERR_DBA_GROUP_HAS_NO_QUORUM);
  }

  for (const auto &member : members) {
    if (member.role == role) {
      if (uuid == member.uuid) {
        session_has_the_right_role = true;
        break;
      }
      other_uuid = member.uuid;
    }
  }

  if (!session_has_the_right_role) {
    // nobody has the right role
    if (other_uuid.empty()) {
      return "";
    }

    log_info("Looking up metadata for member %s", other_uuid.c_str());
    mysqlsh::dba::Instance_metadata imd = md.get_instance_by_uuid(other_uuid);

    return xproto ? imd.xendpoint : imd.endpoint;
  }
  return instance->get_connection_options().uri_endpoint();
}
}  // namespace

std::string find_primary_member_uri(const std::shared_ptr<Instance> &instance,
                                    bool xproto, bool *out_single_primary) {
  return find_member_uri_of_role(instance, mysqlshdk::gr::Member_role::PRIMARY,
                                 xproto, out_single_primary);
}

std::string find_secondary_member_uri(const std::shared_ptr<Instance> &instance,
                                      bool xproto, bool *out_single_primary) {
  return find_member_uri_of_role(instance,
                                 mysqlshdk::gr::Member_role::SECONDARY, xproto,
                                 out_single_primary);
}

}  // namespace dba
}  // namespace mysqlsh
