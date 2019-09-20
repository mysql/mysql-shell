/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

inline bool is_router_upgrade_required(
    const mysqlshdk::utils::Version &version) {
  // Router 1.0.9 shouldn't matter as they're unlikely to exist in the wild,
  // but we mark it to require an upgrade, to make testing possible.
  if (version <= mysqlshdk::utils::Version("1.0.9")) {
    return true;
  }
  // TODO(alfredo) - uncomment this when MD 2.0 is added
  //   if (version <= mysqlshdk::utils::Version("8.0.19")) {
  //     return true;
  //   }
  return false;
}

shcore::Value router_list(MetadataStorage *md, uint64_t cluster_id,
                          bool only_upgrade_required) {
  auto router_list = shcore::make_dict();

  auto routers_md = md->get_routers(cluster_id);

  for (const auto &router_md : routers_md) {
    auto router = shcore::make_dict();

    mysqlshdk::utils::Version router_version;
    if (router_md.version.is_null()) {
      router_version = mysqlshdk::utils::Version("8.0.18");
      (*router)["version"] = shcore::Value("<= 8.0.18");
    } else {
      router_version = mysqlshdk::utils::Version(*router_md.version);
      (*router)["version"] = shcore::Value(*router_md.version);
    }

    bool upgrade_required = is_router_upgrade_required(router_version);
    if (upgrade_required)
      (*router)["upgradeRequired"] = shcore::Value(upgrade_required);

    if (only_upgrade_required && !upgrade_required) continue;

    std::string label;
    if (router_md.name.empty())
      label = router_md.hostname;
    else
      label = router_md.hostname + "::" + router_md.name;

    (*router)["hostname"] = shcore::Value(router_md.hostname);

    if (router_md.last_checkin.is_null())
      (*router)["lastCheckIn"] = shcore::Value::Null();
    else
      (*router)["lastCheckIn"] = shcore::Value(*router_md.last_checkin);

    if (router_md.ro_port.is_null())
      (*router)["roPort"] = shcore::Value::Null();
    else
      (*router)["roPort"] = shcore::Value(*router_md.ro_port);

    if (router_md.rw_port.is_null())
      (*router)["rwPort"] = shcore::Value::Null();
    else
      (*router)["rwPort"] = shcore::Value(*router_md.rw_port);

    if (router_md.ro_x_port.is_null())
      (*router)["roXPort"] = shcore::Value::Null();
    else
      (*router)["roXPort"] = shcore::Value(*router_md.ro_x_port);

    if (router_md.rw_x_port.is_null())
      (*router)["rwXPort"] = shcore::Value::Null();
    else
      (*router)["rwXPort"] = shcore::Value(*router_md.rw_x_port);

    router_list->set(label, shcore::Value(router));
  }

  return shcore::Value(router_list);
}

}  // namespace dba
}  // namespace mysqlsh
