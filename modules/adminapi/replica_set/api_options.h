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

#ifndef MODULES_ADMINAPI_REPLICA_SET_API_OPTIONS_H_
#define MODULES_ADMINAPI_REPLICA_SET_API_OPTIONS_H_

#include <string>

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
namespace replicaset {

struct Wait_recovery_option {
  static const shcore::Option_pack_def<Wait_recovery_option> &options();

  void set_wait_recovery(int value);

  int wait_recovery = isatty(STDOUT_FILENO) ? 3 : 2;
};

struct Rejoin_instance_options : public Wait_recovery_option,
                                 public Timeout_option,
                                 public Interactive_option {
  static const shcore::Option_pack_def<Rejoin_instance_options> &options();

  Join_replicaset_clone_options clone_options;
  bool dry_run = false;
};

struct Add_instance_options : public Rejoin_instance_options {
  static const shcore::Option_pack_def<Add_instance_options> &options();

  Async_replication_options ar_options;
  std::string instance_label;
};

struct Gtid_wait_timeout_option {
  static const shcore::Option_pack_def<Gtid_wait_timeout_option> &options();

  int timeout() const;

 private:
  mysqlshdk::utils::nullable<int> m_timeout;
};

struct Remove_instance_options : public Gtid_wait_timeout_option {
  static const shcore::Option_pack_def<Remove_instance_options> &options();

  mysqlshdk::null_bool force;
};

struct Status_options {
  static const shcore::Option_pack_def<Status_options> &options();
  void set_extended(int value);

  int extended = 0;  // By default 0 (false).
};

struct Set_primary_instance_options : public Gtid_wait_timeout_option {
  static const shcore::Option_pack_def<Set_primary_instance_options> &options();

  bool dry_run = false;
};

struct Force_primary_instance_options : public Set_primary_instance_options {
  static const shcore::Option_pack_def<Force_primary_instance_options>
      &options();

  bool invalidate_instances = false;
};

}  // namespace replicaset
}  // namespace dba
}  // namespace mysqlsh
#endif  // MODULES_ADMINAPI_REPLICA_SET_API_OPTIONS_H_
