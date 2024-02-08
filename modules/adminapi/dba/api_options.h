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

#ifndef MODULES_ADMINAPI_DBA_API_OPTIONS_H_
#define MODULES_ADMINAPI_DBA_API_OPTIONS_H_

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlsh {
namespace dba {

struct Common_sandbox_options {
  Common_sandbox_options();

  static const shcore::Option_pack_def<Common_sandbox_options> &options();
  void set_sandbox_dir(const std::string &value);

  std::string sandbox_dir;
};

struct Stop_sandbox_options : public Common_sandbox_options {
  static const shcore::Option_pack_def<Stop_sandbox_options> &options();
  std::optional<std::string> password;
};

struct Deploy_sandbox_options : public Stop_sandbox_options {
  static const shcore::Option_pack_def<Deploy_sandbox_options> &options();

  std::optional<int> xport;
  std::string allow_root_from{"%"};
  bool ignore_ssl_error = false;
  shcore::Array_t mysqld_options;
};

struct Check_instance_configuration_options {
  static const shcore::Option_pack_def<Check_instance_configuration_options>
      &options();

  std::string mycnf_path;
};

struct Configure_instance_options {
  explicit Configure_instance_options(Cluster_type type) noexcept
      : cluster_type{type} {}

  static const shcore::Option_pack_def<Configure_instance_options> &options();

  void set_password_expiration(const shcore::Value &value);

  Cluster_type cluster_type;

  std::string cluster_admin;
  std::optional<std::string> cluster_admin_password;
  std::optional<std::string> cluster_admin_cert_issuer;
  std::optional<std::string> cluster_admin_cert_subject;
  std::optional<int64_t> cluster_admin_password_expiration;
  std::optional<bool> restart;
  std::optional<int64_t> replica_parallel_workers;
  std::string mycnf_path;
  std::string output_mycnf_path;
};

struct Configure_cluster_instance_options : public Configure_instance_options {
  Configure_cluster_instance_options() noexcept
      : Configure_instance_options{Cluster_type::GROUP_REPLICATION} {}

  static const shcore::Option_pack_def<Configure_cluster_instance_options>
      &options();

  void set_replica_parallel_workers(int64_t value);
  void set_mycnf_path(const std::string &value);
  void set_output_mycnf_path(const std::string &value);
};

struct Configure_replicaset_instance_options
    : public Configure_instance_options {
  Configure_replicaset_instance_options() noexcept
      : Configure_instance_options{Cluster_type::ASYNC_REPLICATION} {}

  static const shcore::Option_pack_def<Configure_replicaset_instance_options>
      &options();

  void set_replica_parallel_workers(int64_t value);
};

struct Replication_auth_options {
  static const shcore::Option_pack_def<Replication_auth_options> &options();

  void set_auth_type(const std::string &value);
  void set_cert_issuer(const std::string &value);
  void set_cert_subject(const std::string &value);

  Replication_auth_type member_auth_type = Replication_auth_type::PASSWORD;
  std::string cert_issuer;
  std::string cert_subject;
};

struct Create_cluster_options : public Force_options {
  static const shcore::Option_pack_def<Create_cluster_options> &options();

  bool get_adopt_from_gr(bool default_value = false) const noexcept {
    return adopt_from_gr.value_or(default_value);
  }

  Create_group_replication_options gr_options;
  Create_cluster_clone_options clone_options;
  Replication_auth_options member_auth_options;
  std::optional<bool> adopt_from_gr;
  std::optional<bool> multi_primary;
  bool dry_run = false;

  std::string replication_allowed_host;
};

struct Create_replicaset_options {
  static const shcore::Option_pack_def<Create_replicaset_options> &options();
  void set_instance_label(const std::string &optionvalue);
  void set_ssl_mode(const std::string &value);

  bool adopt = false;
  bool dry_run = false;
  bool gtid_set_is_complete = false;
  std::string instance_label;

  std::string replication_allowed_host;

  Replication_auth_options member_auth_options;
  Cluster_ssl_mode ssl_mode = Cluster_ssl_mode::NONE;
};

struct Drop_metadata_schema_options {
  static const shcore::Option_pack_def<Drop_metadata_schema_options> &options();

  std::optional<bool> force;
};

struct Reboot_cluster_options {
  static const shcore::Option_pack_def<Reboot_cluster_options> &options();
  void check_option_values(const mysqlshdk::utils::Version &version,
                           int canonical_port, const std::string &comm_stack);
  void set_primary(std::string value);
  void set_switch_communication_stack(const std::string &value);
  void set_timeout(uint32_t timeout_seconds);

  bool get_force(bool default_value = false) const noexcept {
    return force.value_or(default_value);
  }

  bool get_dry_run(bool default_value = false) const noexcept {
    return dry_run.value_or(default_value);
  }

  std::string get_primary(std::string default_value = {}) const noexcept {
    return primary.value_or(std::move(default_value));
  }

  std::chrono::seconds get_timeout() const;

  std::optional<bool> force;
  std::optional<bool> dry_run;
  std::optional<std::string> primary;
  std::optional<std::string> switch_communication_stack;
  std::optional<std::chrono::seconds> timeout;
  Reboot_group_replication_options gr_options;
};

struct Upgrade_metadata_options {
  static const shcore::Option_pack_def<Upgrade_metadata_options> &options();

  bool dry_run = false;
};

}  // namespace dba
}  // namespace mysqlsh
#endif  // MODULES_ADMINAPI_DBA_API_OPTIONS_H_
