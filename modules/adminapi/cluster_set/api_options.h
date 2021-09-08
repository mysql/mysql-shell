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
  Cluster_ssl_mode ssl_mode = Cluster_ssl_mode::NONE;
  std::string replication_allowed_host;
};

struct Create_replica_cluster_options : public Interactive_option,
                                        public Timeout_option {
  static const shcore::Option_pack_def<Create_replica_cluster_options>
      &options();
  void set_recovery_verbosity(int value);

  Cluster_set_group_replication_options gr_options;
  Create_replica_cluster_clone_options clone_options;
  bool dry_run = false;
  int recovery_verbosity = isatty(STDOUT_FILENO) ? 2 : 1;
  std::string replication_allowed_host;
};

struct Remove_cluster_options : public Timeout_option {
  static const shcore::Option_pack_def<Remove_cluster_options> &options();

  bool dry_run = false;
  mysqlshdk::null_bool force;
};

struct Status_options {
  static const shcore::Option_pack_def<Status_options> &options();
  void set_extended(uint64_t value);

  uint64_t extended = 0;  // By default 0 (false).
};

struct Invalidate_replica_clusters_option {
  static const shcore::Option_pack_def<Invalidate_replica_clusters_option>
      &options();

  void set_list_option(const std::string &option, const shcore::Value &value);

  std::list<std::string> invalidate_replica_clusters;
};

struct Set_primary_cluster_options : public Invalidate_replica_clusters_option,
                                     public Timeout_option {
  static const shcore::Option_pack_def<Set_primary_cluster_options> &options();

  bool dry_run = false;
};

struct Force_primary_cluster_options
    : public Invalidate_replica_clusters_option {
  static const shcore::Option_pack_def<Force_primary_cluster_options>
      &options();

  bool dry_run = false;
};

struct Rejoin_cluster_options {
  static const shcore::Option_pack_def<Rejoin_cluster_options> &options();

  bool dry_run = false;
};

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_API_OPTIONS_H_
