/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/mod_common.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {

struct Group_replication_options {
  enum Unpack_target {
    NONE,                    // none
    CREATE,                  // all options OK
    CREATE_REPLICA_CLUSTER,  // all but group_name, group_seeds,
                             // manual_start_on_boot
    JOIN,                    // all but group_name
    REJOIN,                  // only memberSslMode, ipAllowlist and localAddress
    REBOOT                   // same as REJOIN plus sslMode
  };

  explicit Group_replication_options() noexcept = default;
  explicit Group_replication_options(Unpack_target t) noexcept : target(t) {}

  void check_option_values(const mysqlshdk::utils::Version &version,
                           int canonical_port);

  /**
   * Read the Group Replication option values from the given Instance.
   *
   * NOTE: This function does not override any GR option previously set, i.e.,
   *       only read a specific option if its value is null.
   *
   * @param instance target Instance object to read the GR options.
   */
  void read_option_values(const mysqlshdk::mysql::IInstance &instance,
                          bool switching_comm_stack = false);

  void set_local_address(const std::string &value);
  void set_exit_state_action(const std::string &value);
  void set_member_weight(int64_t value);
  void set_auto_rejoin_tries(int64_t value);
  void set_expel_timeout(int64_t value);
  void set_consistency(const std::string &option, const std::string &value);
  void set_communication_stack(const std::string &value);
  void set_transaction_size_limit(int64_t value);
  void set_paxos_single_leader(bool value);

  Unpack_target target{NONE};

  Cluster_ssl_mode ssl_mode{Cluster_ssl_mode::NONE};
  std::optional<mysqlshdk::mysql::Auth_options> recovery_credentials;

  std::optional<std::string> group_name;
  std::optional<std::string> view_change_uuid;
  std::optional<std::string> ip_allowlist;
  std::optional<std::string> local_address;
  std::optional<std::string> group_seeds;
  std::optional<std::string> exit_state_action;
  std::optional<std::string> consistency;
  std::optional<int64_t> member_weight;
  std::optional<int64_t> expel_timeout;
  std::optional<int64_t> auto_rejoin_tries;
  std::optional<bool> manual_start_on_boot;
  std::optional<std::string> communication_stack;
  std::optional<int64_t> transaction_size_limit;
  std::optional<bool> paxos_single_leader;
};

void validate_local_address_option(std::string_view address,
                                   std::string_view communication_stack,
                                   int canonical_port);

void validate_ip_allow_list(const mysqlshdk::mysql::IInstance &instance,
                            std::string_view ip_allowlist,
                            const std::string &local_address,
                            std::string_view communication_stack,
                            bool create_cluster);

struct Rejoin_group_replication_options : public Group_replication_options {
  explicit Rejoin_group_replication_options(
      Unpack_target t = Unpack_target::REJOIN)
      : Group_replication_options(t) {}
  static const shcore::Option_pack_def<Rejoin_group_replication_options>
      &options();

  void set_ip_allowlist(const std::string &option, const std::string &value);
};

struct Reboot_group_replication_options
    : public Rejoin_group_replication_options {
  explicit Reboot_group_replication_options(
      Unpack_target t = Unpack_target::REBOOT)
      : Rejoin_group_replication_options(t) {}
  static const shcore::Option_pack_def<Reboot_group_replication_options>
      &options();
};

struct Join_group_replication_options
    : public Rejoin_group_replication_options {
  explicit Join_group_replication_options(Unpack_target t = Unpack_target::JOIN)
      : Rejoin_group_replication_options(t) {}
  static const shcore::Option_pack_def<Join_group_replication_options>
      &options();
};

struct Cluster_set_group_replication_options
    : public Rejoin_group_replication_options {
  explicit Cluster_set_group_replication_options(
      Unpack_target t = Unpack_target::CREATE_REPLICA_CLUSTER)
      : Rejoin_group_replication_options(t) {}
  static const shcore::Option_pack_def<Cluster_set_group_replication_options>
      &options();
};

struct Create_group_replication_options
    : public Join_group_replication_options {
  Create_group_replication_options()
      : Join_group_replication_options(Unpack_target::CREATE) {}
  static const shcore::Option_pack_def<Create_group_replication_options>
      &options();

  void set_group_name(const std::string &value);
  void set_ssl_mode(const std::string &value);

  void set_consistency(const std::string &option, const std::string &value) {
    Group_replication_options::set_consistency(option, value);
  }

  void set_expel_timeout(int64_t value) {
    Group_replication_options::set_expel_timeout(value);
  }
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_GROUP_REPLICATION_OPTIONS_H_
