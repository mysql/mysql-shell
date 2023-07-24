/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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
#include <string_view>
#include <utility>

#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/metadata_storage.h"

namespace mysqlsh {
namespace dba {

inline constexpr auto k_router_option_invalidated_cluster_routing_policy =
    "invalidated_cluster_policy";
inline constexpr auto
    k_router_option_invalidated_cluster_routing_policy_drop_all = "drop_all";
inline constexpr auto
    k_router_option_invalidated_cluster_routing_policy_accept_ro = "accept_ro";

inline constexpr auto k_router_option_target_cluster = "target_cluster";
inline constexpr auto k_router_option_target_cluster_primary = "primary";
inline constexpr auto k_router_option_stats_updates_frequency =
    "stats_updates_frequency";
inline constexpr auto k_router_option_use_replica_primary_as_rw =
    "use_replica_primary_as_rw";
inline constexpr auto k_router_option_unreachable_quorum_allowed_traffic =
    "unreachable_quorum_allowed_traffic";
inline constexpr auto k_router_option_tags = "tags";

inline constexpr auto k_router_option_read_only_targets = "read_only_targets";
inline constexpr auto k_router_option_read_only_targets_all = "all";
inline constexpr auto k_router_option_read_only_targets_read_replicas =
    "read_replicas";
inline constexpr auto k_router_option_read_only_targets_secondaries =
    "secondaries";

inline constexpr auto k_default_router_option_read_only_targets =
    k_router_option_read_only_targets_secondaries;

inline constexpr std::array<decltype(k_router_option_target_cluster), 6>
    k_clusterset_router_options = {
        k_router_option_invalidated_cluster_routing_policy,
        k_router_option_target_cluster,
        k_router_option_stats_updates_frequency,
        k_router_option_use_replica_primary_as_rw,
        k_router_option_tags,
        k_router_option_read_only_targets};

extern const std::map<std::string, shcore::Value>
    k_default_clusterset_router_options;

inline constexpr std::array<std::string_view, 4> k_cluster_router_options = {
    k_router_option_tags, k_router_option_read_only_targets,
    k_router_option_stats_updates_frequency,
    k_router_option_unreachable_quorum_allowed_traffic};

extern const std::map<std::string, shcore::Value>
    k_default_cluster_router_options;

inline constexpr std::array<decltype(k_router_option_target_cluster), 2>
    k_replicaset_router_options = {k_router_option_tags,
                                   k_router_option_stats_updates_frequency};

extern const std::map<std::string, shcore::Value>
    k_default_replicaset_router_options;

shcore::Value router_list(MetadataStorage *md, const Cluster_id &cluster_id,
                          bool only_upgrade_required);

shcore::Value clusterset_list_routers(MetadataStorage *md,
                                      const Cluster_set_id &clusterset_id,
                                      const std::string &router);

shcore::Dictionary_t router_options(MetadataStorage *md, Cluster_type type,
                                    const std::string &id,
                                    const std::string &router_label = "");

shcore::Value validate_router_option(const Base_cluster_impl &cluster,
                                     const std::string &name,
                                     const shcore::Value &value);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ROUTER_H_
