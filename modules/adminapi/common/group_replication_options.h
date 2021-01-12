/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/instance_pool.h"
#include "modules/mod_common.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"

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

  /**
   * Read the Group Replication option values from the given Instance.
   *
   * NOTE: This function does not override any GR option previously set, i.e.,
   *       only read a specific option if its value is null.
   *
   * @param instance target Instance object to read the GR options.
   */
  void read_option_values(const mysqlshdk::mysql::IInstance &instance);

  Unpack_target target;

  mysqlshdk::utils::nullable<mysqlshdk::mysql::Auth_options>
      recovery_credentials;

  mysqlshdk::null_string group_name;
  mysqlshdk::null_string ssl_mode;
  mysqlshdk::null_string ip_allowlist;
  mysqlshdk::null_string local_address;
  mysqlshdk::null_string group_seeds;
  mysqlshdk::null_string exit_state_action;
  mysqlshdk::null_string consistency;
  mysqlshdk::utils::nullable<int64_t> member_weight;
  mysqlshdk::utils::nullable<int64_t> expel_timeout;
  mysqlshdk::utils::nullable<int64_t> auto_rejoin_tries;
  mysqlshdk::null_bool manual_start_on_boot;

  std::string ip_allowlist_option_name;

 private:
  void do_unpack(shcore::Option_unpacker *unpacker);
};

struct Join_group_replication_options : public Group_replication_options {
  explicit Join_group_replication_options(Unpack_target t = Unpack_target::JOIN)
      : Group_replication_options(t) {}
  static const shcore::Option_pack_def<Join_group_replication_options>
      &options();

  void set_ssl_mode(const std::string &value);
  void set_ip_allowlist(const std::string &option, const std::string &value);
  void set_local_address(const std::string &value);
  void set_exit_state_action(const std::string &value);
  void set_member_weight(int64_t value);
  void set_consistency(const std::string &option, const std::string &value);
  void set_group_seeds(const std::string &value);
  void set_expel_timeout(int64_t value);
  void set_auto_rejoin_tries(int64_t value);
};

struct Create_group_replication_options
    : public Join_group_replication_options {
  Create_group_replication_options()
      : Join_group_replication_options(Unpack_target::CREATE) {}
  static const shcore::Option_pack_def<Create_group_replication_options>
      &options();
  void set_group_name(const std::string &value);
  void set_manual_start_on_boot(bool value);
};

struct Rejoin_group_replication_options : public Group_replication_options {
  Rejoin_group_replication_options()
      : Group_replication_options(Unpack_target::REJOIN) {}
  static const shcore::Option_pack_def<Rejoin_group_replication_options>
      &options();

  void set_ssl_mode(const std::string &value);
  void set_ip_allowlist(const std::string &value);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GROUP_REPLICATION_OPTIONS_H_
