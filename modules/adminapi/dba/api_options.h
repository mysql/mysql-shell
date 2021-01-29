/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_DBA_API_OPTIONS_H_
#define MODULES_ADMINAPI_DBA_API_OPTIONS_H_

#include <string>
#include <vector>

#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"

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
  mysqlshdk::null_string password;
};

struct Deploy_sandbox_options : public Stop_sandbox_options {
  static const shcore::Option_pack_def<Deploy_sandbox_options> &options();

  mysqlshdk::utils::nullable<int> xport;
  std::string allow_root_from{"%"};
  bool ignore_ssl_error = false;
  shcore::Array_t mysqld_options;
};

struct Check_instance_configuration_options
    : public Password_interactive_options {
  static const shcore::Option_pack_def<Check_instance_configuration_options>
      &options();

  std::string mycnf_path;
};

struct Configure_instance_options : public Password_interactive_options {
  static const shcore::Option_pack_def<Configure_instance_options> &options();

  bool local = false;
  Cluster_type cluster_type;

  std::string cluster_admin;
  mysqlshdk::null_string cluster_admin_password;
  mysqlshdk::null_bool restart;
  mysqlshdk::utils::nullable<int64_t> replica_parallel_workers;
  std::string mycnf_path;
  std::string output_mycnf_path;
  mysqlshdk::null_bool clear_read_only;
};

struct Configure_cluster_local_instance_options
    : public Configure_instance_options {
  Configure_cluster_local_instance_options();
  static const shcore::Option_pack_def<Configure_cluster_local_instance_options>
      &options();
  void set_mycnf_path(const std::string &value);
  void set_output_mycnf_path(const std::string &value);
  void set_clear_read_only(bool value);
};

struct Configure_cluster_instance_options
    : public Configure_cluster_local_instance_options {
  Configure_cluster_instance_options();
  static const shcore::Option_pack_def<Configure_cluster_instance_options>
      &options();
  void set_replica_parallel_workers(int64_t value);
};

struct Configure_replicaset_instance_options
    : public Configure_instance_options {
  Configure_replicaset_instance_options();
  static const shcore::Option_pack_def<Configure_replicaset_instance_options>
      &options();
  void set_replica_parallel_workers(int64_t value);
};

struct Create_cluster_options : public Force_interactive_options {
  static const shcore::Option_pack_def<Create_cluster_options> &options();
  void set_multi_primary(const std::string &option, bool value);
  void set_clear_read_only(bool value);

  Create_group_replication_options gr_options;
  Create_cluster_clone_options clone_options;
  bool adopt_from_gr = false;
  mysqlshdk::null_bool multi_primary;
  mysqlshdk::null_bool clear_read_only;
  bool dry_run = false;
};

struct Create_replicaset_options : public Interactive_option {
  static const shcore::Option_pack_def<Create_replicaset_options> &options();
  void set_instance_label(const std::string &optionvalue);

  bool adopt = false;
  bool dry_run = false;
  bool gtid_set_is_complete = false;
  std::string instance_label;
  // TODO(rennox): This is here but is not really used (options never set)
  Async_replication_options ar_options;
};

struct Drop_metadata_schema_options {
  static const shcore::Option_pack_def<Drop_metadata_schema_options> &options();

  mysqlshdk::null_bool force;
  mysqlshdk::null_bool clear_read_only;
};

struct Reboot_cluster_options {
  static const shcore::Option_pack_def<Reboot_cluster_options> &options();
  void set_user(const std::string &option, const std::string &value);
  void set_password(const std::string &option, const std::string &value);
  void set_clear_read_only(bool value);

  std::vector<std::string> remove_instances;
  std::vector<std::string> rejoin_instances;
  mysqlshdk::null_bool clear_read_only;
  mysqlshdk::null_string user;
  mysqlshdk::null_string password;
};

struct Upgrade_metadata_options : public Interactive_option {
  static const shcore::Option_pack_def<Upgrade_metadata_options> &options();

  bool dry_run = false;
};

}  // namespace dba
}  // namespace mysqlsh
#endif  // MODULES_ADMINAPI_DBA_API_OPTIONS_H_
