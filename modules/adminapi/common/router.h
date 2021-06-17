/*
 * Copyright (c) 2019, 2021 Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_ROUTER_H_
#define MODULES_ADMINAPI_COMMON_ROUTER_H_

#include <array>
#include <string>

#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

constexpr auto k_router_option_invalidated_cluster_routing_policy =
    "invalidated_cluster_policy";
constexpr auto k_router_option_target_cluster = "target_cluster";
constexpr auto k_router_option_target_cluster_primary = "primary";

constexpr std::array<decltype(k_router_option_target_cluster), 2>
    k_router_options = {k_router_option_invalidated_cluster_routing_policy,
                        k_router_option_target_cluster};

class Cluster_set_impl;

shcore::Value router_list(MetadataStorage *md, const Cluster_id &cluster_id,
                          bool only_upgrade_required);

shcore::Value clusterset_list_routers(MetadataStorage *md,
                                      const Cluster_set_id &clusterset_id,
                                      const std::string &router);

shcore::Dictionary_t router_options(MetadataStorage *md,
                                    const std::string &clusterset_id,
                                    const std::string &router_id = "");

void validate_router_option(const Cluster_set_impl &cluster_set,
                            std::string name, const shcore::Value &value);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ROUTER_H_
