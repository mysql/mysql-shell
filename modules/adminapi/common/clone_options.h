/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_CLONE_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_CLONE_OPTIONS_H_

#include <string>
#include <vector>
#include "modules/mod_common.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"
#include "scripting/types.h"

namespace mysqlsh {
namespace dba {

class Cluster_impl;

enum class Member_recovery_method { AUTO, INCREMENTAL, CLONE };

struct Clone_options {
  enum Unpack_target {
    NONE,            // none
    CREATE_CLUSTER,  // only disableClone
    JOIN_CLUSTER,    // all but disableClone
    JOIN_REPLICASET  // ReplicaSet only options
  };

  Clone_options() : target(NONE) {}

  explicit Clone_options(Unpack_target t) : target(t) {}

  template <typename Unpacker>
  Unpacker &unpack(Unpacker *options) {
    do_unpack(options);
    return *options;
  }

  void check_option_values(const mysqlshdk::utils::Version &version,
                           const Cluster_impl *cluster = nullptr);

  Unpack_target target;

  mysqlshdk::utils::nullable<bool> disable_clone;
  bool gtid_set_is_complete = false;
  mysqlshdk::utils::nullable<Member_recovery_method> recovery_method;
  std::string recovery_method_str_invalid;
  mysqlshdk::utils::nullable<std::string> clone_donor;

 private:
  void do_unpack(shcore::Option_unpacker *unpacker);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_CLONE_OPTIONS_H_
