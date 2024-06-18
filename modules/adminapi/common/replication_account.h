/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_REPLICATION_ACCOUNT_H_
#define MODULES_ADMINAPI_COMMON_REPLICATION_ACCOUNT_H_

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/undo.h"

namespace mysqlsh::dba {

class Cluster_impl;
class Cluster_set_impl;
class Replica_set_impl;

struct Group_replication_options;
struct Replication_auth_options;

class Replication_account {
 public:
  enum class Account_type { GROUP_REPLICATION, READ_REPLICA };

  struct Auth_options_host {
    mysqlshdk::mysql::Auth_options auth;
    std::string host;
  };
  struct User_host {
    std::string user;
    std::string host;
  };
  struct User_hosts {
    std::string user;
    std::vector<std::string> hosts;
    bool from_md{false};
  };

  static constexpr std::string_view k_group_recovery_user_prefix{
      "mysql_innodb_cluster_"};
  static constexpr std::string_view k_group_recovery_old_user_prefix{
      "mysql_innodb_cluster_r"};

  static constexpr std::string_view k_async_recovery_user_prefix{
      "mysql_innodb_rs_"};

  static std::string make_replication_user_name(uint32_t server_id,
                                                std::string_view user_prefix,
                                                bool server_id_hexa = false);

 public:
  explicit Replication_account(Cluster_impl &cluster) noexcept
      : m_topo{cluster} {}
  explicit Replication_account(Cluster_set_impl &cluster_set) noexcept
      : m_topo{cluster_set} {}
  explicit Replication_account(Replica_set_impl &replica_set) noexcept
      : m_topo{replica_set} {}

  mysqlshdk::mysql::Sql_undo_list record_undo(auto &&cb) {
    m_undo = mysqlshdk::mysql::Sql_undo_list{};
    cb(*this);
    return std::exchange(*m_undo, {});
  }

  Auth_options_host create_replication_user(
      const mysqlshdk::mysql::IInstance &target,
      std::string_view auth_cert_subject,
      Replication_account::Account_type account_type,
      bool only_on_target = false, mysqlshdk::mysql::Auth_options creds = {},
      bool print_recreate_node = true, bool dry_run = false) const;

  Auth_options_host create_cluster_replication_user(
      const mysqlshdk::mysql::IInstance &target, std::string_view account_host,
      Replication_auth_type member_auth_type, std::string_view auth_cert_issuer,
      std::string_view auth_cert_subject, bool dry_run);

  User_host recreate_replication_user(const mysqlshdk::mysql::IInstance &target,
                                      std::string_view auth_cert_subject,
                                      bool dry_run = false) const;

  void drop_all_replication_users();

  void ensure_metadata_has_recovery_accounts(bool allow_non_standard_format);

  void update_replication_allowed_host(const std::string &host);

  void restore_recovery_account_all_members(bool reset_password = true);

  void create_local_replication_user(
      const mysqlshdk::mysql::IInstance &target,
      std::string_view auth_cert_subject,
      const Group_replication_options &gr_options,
      bool propagate_credentials_donors, bool dry_run = false);

  void create_replication_users_at_instance(
      const mysqlshdk::mysql::IInstance &target);

  static std::string create_recovery_account(
      const mysqlshdk::mysql::IInstance &primary,
      const mysqlshdk::mysql::IInstance &target,
      const Replication_auth_options &repl_auth_options,
      std::string_view repl_allowed_host);

  std::string create_recovery_account(
      const mysqlshdk::mysql::IInstance &target_instance,
      std::string_view auth_cert_subject);

  User_host recreate_recovery_account(
      const mysqlshdk::mysql::IInstance &primary,
      const mysqlshdk::mysql::IInstance &target,
      const std::string_view auth_cert_subject);

  bool drop_replication_user(const mysqlshdk::mysql::IInstance *target,
                             std::string endpoint, std::string server_uuid,
                             uint32_t server_id, bool dry_run);

  void drop_replication_user(const Cluster_impl &cluster);

  void drop_replication_user(std::string_view server_uuid,
                             const mysqlshdk::mysql::IInstance *target);

  void drop_read_replica_replication_user(
      const mysqlshdk::mysql::IInstance &target, bool dry_run = false);

  void drop_replication_users();

  User_hosts get_replication_user(const mysqlshdk::mysql::IInstance &target,
                                  bool is_read_replica);

  Auth_options_host refresh_replication_user(
      const mysqlshdk::mysql::IInstance &target, bool dry_run);

  Auth_options_host refresh_replication_user(
      const mysqlshdk::mysql::IInstance &target, const Cluster_id &cluster_id,
      bool dry_run);

  std::unordered_map<uint32_t, std::string> get_mismatched_recovery_accounts();

  std::vector<User_host> get_unused_recovery_accounts(
      const std::unordered_map<uint32_t, std::string>
          &mismatched_recovery_accounts);

 private:
  std::string get_replication_user_host() const;

  template <class T>
    requires std::is_same_v<T, Cluster_impl> ||
             std::is_same_v<T, Cluster_set_impl> ||
             std::is_same_v<T, Replica_set_impl> bool
  topo_holds() const {
    return std::holds_alternative<std::reference_wrapper<T>>(m_topo);
  }

  template <class T>
    requires std::is_same_v<T, Cluster_impl> ||
             std::is_same_v<T, Cluster_set_impl> ||
             std::is_same_v<T, Replica_set_impl>
  const auto &topo_as() const {
    return std::get<std::reference_wrapper<T>>(m_topo).get();
  }

 private:
  std::variant<std::reference_wrapper<Cluster_impl>,
               std::reference_wrapper<Cluster_set_impl>,
               std::reference_wrapper<Replica_set_impl>>
      m_topo;
  std::optional<mysqlshdk::mysql::Sql_undo_list> m_undo;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_REPLICATION_ACCOUNT_H_
