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

#ifndef MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_
#define MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_

#include <string>
#include <vector>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

struct Add_instance_options : public Password_interactive_options {
  static const shcore::Option_pack_def<Add_instance_options> &options();
  void set_wait_recovery(int value);

  Join_group_replication_options gr_options;
  Join_cluster_clone_options clone_options;
  mysqlshdk::null_string label;
  int wait_recovery = isatty(STDOUT_FILENO) ? 3 : 2;
};

struct Rejoin_instance_options : public Password_interactive_options {
  static const shcore::Option_pack_def<Rejoin_instance_options> &options();

  Rejoin_group_replication_options gr_options;
};

struct Remove_instance_options : public Password_interactive_options {
  static const shcore::Option_pack_def<Remove_instance_options> &options();

  mysqlshdk::null_bool force;
};

struct Status_options {
  static const shcore::Option_pack_def<Status_options> &options();
  void set_extended(uint64_t value);
  void set_query_members(bool value);

  uint64_t extended = 0;  // By default 0 (false).
};

struct Options_options {
  static const shcore::Option_pack_def<Options_options> &options();

  bool all = false;
};

struct Rescan_options : public Interactive_option {
  static const shcore::Option_pack_def<Rescan_options> &options();
  void set_list_option(const std::string &option, const shcore::Value &value);
  void set_update_topology_mode(bool value);

  mysqlshdk::null_bool update_topology_mode;
  std::vector<mysqlshdk::db::Connection_options> add_instances_list;
  std::vector<mysqlshdk::db::Connection_options> remove_instances_list;
  bool auto_add = false;
  bool auto_remove = false;
  bool upgrade_comm_protocol = false;
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
#endif  // MODULES_ADMINAPI_CLUSTER_API_OPTIONS_H_
