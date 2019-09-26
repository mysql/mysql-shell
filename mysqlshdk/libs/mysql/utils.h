/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_UTILS_H_
#define MYSQLSHDK_LIBS_MYSQL_UTILS_H_

#include <memory>
#include <string>
#include <tuple>
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

static constexpr size_t kPASSWORD_LENGTH = 32;

using Account_attribute_set =
    utils::Enum_set<Account_attribute, Account_attribute::Grants>;

void clone_user(const IInstance &instance, const std::string &orig_user,
                const std::string &orig_host, const std::string &new_user,
                const std::string &new_host, const std::string &password,
                Account_attribute_set flags =
                    Account_attribute_set(Account_attribute::Grants));

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
    const IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants,
    std::string *out_password, bool disable_pwd_expire = false);

void create_user_with_password(
    const IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants,
    const std::string &password, bool disable_pwd_expire = false);

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
}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_UTILS_H_
