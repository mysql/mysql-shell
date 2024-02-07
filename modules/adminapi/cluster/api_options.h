/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_
#define MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_

#include <optional>
#include <string>
#include <vector>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlsh::dba::cluster {

struct Add_instance_options {
  static const shcore::Option_pack_def<Add_instance_options> &options();

  Recovery_progress_option recovery_progress;
  Join_group_replication_options gr_options;
  Join_cluster_clone_options clone_options;
  std::optional<std::string> label;
  std::string cert_subject;
};

struct Rejoin_instance_options {
  static const shcore::Option_pack_def<Rejoin_instance_options> &options();

  Recovery_progress_option recovery_progress;
  Rejoin_group_replication_options gr_options;
  Join_read_replica_clone_options clone_options;
  int timeout{0};
  bool dry_run{false};
};

struct Remove_instance_options {
  static const shcore::Option_pack_def<Remove_instance_options> &options();

  bool get_force(bool default_value = false) const noexcept {
    return force.value_or(default_value);
  }

  int timeout{0};
  bool dry_run{false};
  std::optional<bool> force;
};

struct Status_options {
  static const shcore::Option_pack_def<Status_options> &options();

  uint64_t extended{0};
};

struct Options_options {
  static const shcore::Option_pack_def<Options_options> &options();

  bool all{false};
};

struct Rescan_options {
  static const shcore::Option_pack_def<Rescan_options> &options();

  std::vector<mysqlshdk::db::Connection_options> add_instances_list;
  std::vector<mysqlshdk::db::Connection_options> remove_instances_list;
  bool auto_add{false};
  bool auto_remove{false};
  bool upgrade_comm_protocol{false};
  std::optional<bool> update_view_change_uuid;
  std::optional<bool> repair_metadata;

 private:
  void set_list_option(std::string_view name, const shcore::Value &value);

  std::optional<bool> m_used_deprecated;
};

struct Set_primary_instance_options {
  static const shcore::Option_pack_def<Set_primary_instance_options> &options();

  std::optional<uint32_t> running_transactions_timeout;
};

// Read-Replicas

enum class Source_type { PRIMARY, SECONDARY, CUSTOM };

struct Replication_sources {
  std::set<Managed_async_channel_source,
           std::greater<Managed_async_channel_source>>
      replication_sources;
  std::optional<Source_type> source_type;
};

struct Add_replica_instance_options {
  static const shcore::Option_pack_def<Add_replica_instance_options> &options();

  int timeout{0};
  bool dry_run{false};
  std::optional<std::string> label;
  std::string cert_subject;

  Recovery_progress_option recovery_progress;
  Replication_sources replication_sources_option;
  Join_read_replica_clone_options clone_options;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_
