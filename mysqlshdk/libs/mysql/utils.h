/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_UTILS_H_
#define MYSQLSHDK_LIBS_MYSQL_UTILS_H_

#include <mysqld_error.h>

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/enumset.h"

namespace mysqlshdk {
namespace mysql {
namespace schema {
std::set<std::string> get_tables(const mysql::IInstance &instance,
                                 const std::string &schema);
std::set<std::string> get_views(const mysql::IInstance &instance,
                                const std::string &schema);
}  // namespace schema

enum class Account_attribute { Grants };

inline constexpr size_t kPASSWORD_LENGTH = 32;

using Account_attribute_set =
    utils::Enum_set<Account_attribute, Account_attribute::Grants>;

void clone_user(const IInstance &source_instance,
                const IInstance &dest_instance, const std::string &user,
                const std::string &host);

void clone_user(const IInstance &instance, const std::string &orig_user,
                const std::string &orig_host, const std::string &new_user,
                const std::string &new_host);

void clone_user(const IInstance &instance, const std::string &orig_user,
                const std::string &orig_host, const std::string &new_user,
                const std::string &new_host, const std::string &password,
                Account_attribute_set flags =
                    Account_attribute_set(Account_attribute::Grants));

size_t iterate_users(const IInstance &instance, const std::string &user_filter,
                     const std::function<bool(std::string, std::string)> &cb);

using Privilege_list = std::vector<std::string>;

Privilege_list get_global_grants(const IInstance &instance,
                                 const std::string &user,
                                 const std::string &host);

std::vector<std::pair<std::string, Privilege_list>> get_user_restrictions(
    const IInstance &instance, const std::string &user,
    const std::string &host);

/** Drops all accounts for a given username */
void drop_all_accounts_for_user(const IInstance &instance,
                                const std::string &user);
/**
 * Get the list with all the hostnames for a given user account
 * @param instance the target instance
 * @param user the user account name
 * @return list with all hostnames for a given user account.
 */
std::vector<std::string> get_all_hostnames_for_user(const IInstance &instance,
                                                    const std::string &user);

void create_user_with_random_password(
    const IInstance &instance, std::string_view user,
    const std::vector<std::string> &hosts,
    const IInstance::Create_user_options &options, std::string *out_password);

void create_user(const IInstance &instance, std::string_view user,
                 const std::vector<std::string> &hosts,
                 const IInstance::Create_user_options &options);

void set_random_password(const IInstance &instance, const std::string &user,
                         const std::vector<std::string> &hosts,
                         std::string *out_password);

std::string generate_password(size_t password_length = kPASSWORD_LENGTH);

void drop_view_or_table(const IInstance &instance, const std::string &schema,
                        const std::string &name, bool if_exists);

void drop_table_or_view(const IInstance &instance, const std::string &schema,
                        const std::string &name, bool if_exists);

void copy_schema(const mysql::IInstance &instance, const std::string &name,
                 const std::string &target, bool use_existing_schema,
                 bool move_tables);

void copy_data(const mysql::IInstance &instance, const std::string &name,
               const std::string &target);

void create_indicator_tag(const mysql::IInstance &instance,
                          std::string_view name);
void drop_indicator_tag(const mysql::IInstance &instance,
                        std::string_view name);
bool check_indicator_tag(const mysql::IInstance &instance,
                         std::string_view name);

mysqlshdk::db::Ssl_options read_ssl_client_options(
    const mysqlshdk::mysql::IInstance &instance, bool set_cert, bool set_ca);

struct Error_log_entry {
  std::string logged;
  uint64_t thread_id;
  std::string prio;
  std::string error_code;
  std::string subsystem;
  std::string data;
};

bool query_server_errors(const mysql::IInstance &instance,
                         const std::string &start_time,
                         const std::string &end_time,
                         const std::vector<std::string> &subsystems,
                         const std::function<void(const Error_log_entry &)> &f);

/**
 * Queries per-thread variables.
 *
 * @param instance target instance where to perform the query.
 * @param thread_command the target thread command name.
 * @param user the target user associated with the thread.
 * @param var_name_filter the LIKE filter for the variables names.
 * @param cb callback to call for each variable found together with its value.
 *
 * NOTE: first argument of callback is const& in order to avoid false positive
 *       -Werror=maybe-uninitialized with GCC12, release build + gcov
 */
size_t iterate_thread_variables(
    const mysqlshdk::mysql::IInstance &instance,
    std::string_view thread_command, std::string_view user,
    std::string_view var_name_filter,
    const std::function<bool(const std::string &, std::string)> &cb);

inline void assert_transaction_is_open(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  try {
    // if there's an active transaction, this will throw
    session->execute(
        "/*NOTRACE;THROW(1568)*/SET TRANSACTION ISOLATION LEVEL REPEATABLE "
        "READ");
    // no exception -> transaction is not active
    assert(false);
  } catch ([[maybe_unused]] const mysqlshdk::db::Error &e) {
    // make sure correct error is reported
    assert(e.code() == ER_CANT_CHANGE_TX_CHARACTERISTICS);
  } catch (...) {
    // any other exception means that something else went wrong (or debug
    // exception)
  }
}

/**
 * Converts MySQL system variable to boolean.
 *
 * @param name Name of the variable.
 * @param value Value of the variable.
 *
 * @returns Boolean value of the variable.
 * @throws std::runtime_error If value cannot be converted to boolean.
 */
bool sysvar_to_bool(std::string_view name, std::string_view value);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_UTILS_H_
