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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_API_OPTIONS_H_
#define MODULES_ADMINAPI_CLUSTER_SET_API_OPTIONS_H_

#include <string>

#include "modules/adminapi/common/api_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

struct Create_cluster_set_options {
  static const shcore::Option_pack_def<Create_cluster_set_options> &options();
  void set_ssl_mode(const std::string &value);

  bool dry_run = false;
  Cluster_ssl_mode ssl_mode;
};

struct Create_replica_cluster_options : public Interactive_option {
  static const shcore::Option_pack_def<Create_replica_cluster_options>
      &options();
  void set_recovery_verbosity(int value);
  void set_timeout(int value);

  Cluster_set_group_replication_options gr_options;
  Create_replica_cluster_clone_options clone_options;
  bool dry_run = false;
  int recovery_verbosity = isatty(STDOUT_FILENO) ? 2 : 1;
  int timeout = 0;
};

struct Remove_cluster_options {
  static const shcore::Option_pack_def<Remove_cluster_options> &options();
  void set_timeout(int value);

  bool dry_run = false;
  int timeout = 0;
  mysqlshdk::null_bool force;
};

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_API_OPTIONS_H_
