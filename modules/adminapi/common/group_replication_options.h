/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_GROUP_REPLICATION_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_GROUP_REPLICATION_OPTIONS_H_

#include <string>
#include <vector>
#include "modules/mod_common.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"
#include "scripting/types.h"

namespace mysqlsh {
namespace dba {

struct Group_replication_options {
  enum Unpack_target {
    NONE,    // none
    CREATE,  // all options OK
    JOIN,    // all but group_name
    REJOIN   // only memberSslMode and ipWhitelist
  };

  Group_replication_options() : target(NONE) {}

  explicit Group_replication_options(Unpack_target t) : target(t) {}

  template <typename Unpacker>
  Unpacker &unpack(Unpacker *options) {
    do_unpack(options);
    return *options;
  }

  void check_option_values(const mysqlshdk::utils::Version &version);

  Unpack_target target;

  mysqlshdk::utils::nullable<std::string> group_name;
  mysqlshdk::utils::nullable<std::string> ssl_mode;
  mysqlshdk::utils::nullable<std::string> ip_whitelist;
  mysqlshdk::utils::nullable<std::string> local_address;
  mysqlshdk::utils::nullable<std::string> group_seeds;
  mysqlshdk::utils::nullable<std::string> exit_state_action;
  mysqlshdk::utils::nullable<std::string> failover_consistency;
  mysqlshdk::utils::nullable<int64_t> member_weight;
  mysqlshdk::utils::nullable<int64_t> expel_timeout;

 private:
  void do_unpack(shcore::Option_unpacker *unpacker);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GROUP_REPLICATION_OPTIONS_H_
