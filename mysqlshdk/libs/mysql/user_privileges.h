/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_H_
#define MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

class User_privileges_result;
class IInstance;

/**
 * Holds privileges for the given user.
 */
class User_privileges {
 public:
  /**
   * Specifies a string which matches any schema or any table.
   */
  static const char k_wildcard[];

  /**
   * Gathers privileges for the given user using provided session.
   *
   * @param instance The Instance object used to query the database.
   * @param user The username part for the user account to check.
   * @param host The host part for the user account to check.
   */
  User_privileges(const mysqlshdk::mysql::IInstance &instance,
                  const std::string &user, const std::string &host);

  User_privileges(const User_privileges &) = delete;
  User_privileges(User_privileges &&) = default;
  User_privileges &operator=(const User_privileges &) = delete;
  User_privileges &operator=(User_privileges &&) = default;

  ~User_privileges() = default;

  /**
   * Checks if the given user account exists.
   *
   * @return True if account exists.
   */
  bool user_exists() const;

  /**
   * Validates if the given user account has all required privileges on
   * specified schema.table and provides specific user privileges.
   *
   * Returns an object with the information on the user privileges. For more
   * details see @see User_privileges_result. If required_privileges contains
   * exactly one string "ALL" or "ALL PRIVILEGES", all known privileges are
   * checked. Privileges in required_privileges are not case-sensitive.
   *
   * @param required_privileges The privileges the user needs to have.
   * @param schema The schema to check. By default "*" is used.
   * @param table The table to check. By default "*" is used.
   *
   * @return Result of the user privileges check for the provided input
   *         arguments.
   * @throw std::runtime_error if required_privileges contains an invalid
   *                           privilege.
   * @throw std::runtime_error if required_privileges contains more than one
   *                           privilege and "ALL" is specified.
   */
  User_privileges_result validate(
      const std::set<std::string> &required_privileges,
      const std::string &schema = k_wildcard,
      const std::string &table = k_wildcard) const;

  /**
   * Get the set of roles/users granted to the user.
   *
   * @return set with the roles/user account granted to the target user of the
   *         User_privileges object (empty set if no roles/users are associated
   *         to the user).
   */
  std::set<std::string> get_user_roles() const { return m_roles; }

 private:
  friend class User_privileges_result;

  /**
   * Helper structure, holds information about mapped row.
   */
  struct Mapped_row;

  /**
   * Converts a row from database into helper structure.
   */
  using Row_mapper = void (*)(const db::IRow *const, Mapped_row *);

  /**
   * Parses a result of database query, obtaining information about privileges
   * and user grants.
   *
   * @param result The result of database query.
   * @param map_row Converts a single row into readable data.
   * @param user_role string with the target user/role.
   */
  void parse_privileges(const std::shared_ptr<db::IResult> &result,
                        Row_mapper map_row, const std::string &user_role);

  void set_all_privileges(const mysqlshdk::mysql::IInstance &instance);

  /**
   * Reads global privileges of a user or role.
   *
   * @param instance A Instance object for communication with database.
   * @param user_role string with the target user or role.
   */
  void read_global_privileges(const mysqlshdk::mysql::IInstance &instance,
                              const std::string &user_role);

  /**
   * Reads privileges of a user or role on all schemas.
   *
   * @param instance A Instance object for communication with database.
   * @param user_role string with the target user or role.
   */
  void read_schema_privileges(const mysqlshdk::mysql::IInstance &instance,
                              const std::string &user_role);

  /**
   * Reads privileges of a user or role on all tables.
   *
   * @param instance A Instance object for communication with database.
   * @param user_role string with the target user or role.
   */
  void read_table_privileges(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &user_role);

  /**
   * Reads privileges of a user/role from result of specified query.
   *
   * @param instance A Instance object for communication with database.
   * @param query A query to be executed.
   * @param map_row Converts a single row into readable data.
   * @param user_role string with the target user/role.
   */
  void read_privileges(const mysqlshdk::mysql::IInstance &instance,
                       const char *const query, Row_mapper map_row,
                       const std::string &user_role);

  /**
   * Get the defined mandatory roles.
   *
   * NOTE: the returned roles might not be active.
   *
   * @param instance The Instance object used to query the database.
   * @return a set of strings with the defined mandatory role, each role in the
   *         set has the format '<user>'@'<host>' (empty set if there are no
   *         mandatory roles).
   */
  std::set<std::string> get_mandatory_roles(
      const mysqlshdk::mysql::IInstance &instance) const;

  /**
   * Read roles/users granted to the user.
   *
   * @param instance The Instance object used to query the database.
   */
  void read_user_roles(const mysqlshdk::mysql::IInstance &instance);

  /**
   * Gets the list of privileges missing on the given user account of a
   * specific set of privileges.
   *
   * @param required_privileges The list of required privileges.
   * @param schema The schema to check.
   * @param table The table to check.
   * @param check_is_grantable Boolean value to indicate whether a check to
   * verify whether the privilege is grantable or not should be done.
   *
   * @return A set of privileges missing from the given list of required
   * privileges. This set is empty if user has all the required privileges.
   */
  std::set<std::string> get_missing_privileges_from_set(
      const std::set<std::string> &required_privileges,
      const std::string &schema, const std::string &table,
      bool check_is_grantable = false) const;

  /**
   * Checks if the given user account has GRANT OPTION privilege on
   * specified schema.table.
   *
   * @param required_privileges The privileges the user needs to have.
   * @param schema The schema to check. By default "*" is used.
   * @param table The table to check. By default "*" is used.
   *
   * @return True if user has the GRANT OPTION.
   */
  bool has_grant_option(const std::set<std::string> &required_privileges,
                        const std::string &schema = k_wildcard,
                        const std::string &table = k_wildcard) const;

  /**
   * Checks if the given user account has all required privileges on specified
   * schema.table and provides any missing ones.
   *
   * @param required_privileges The privileges the user needs to have.
   * @param schema The schema to check. By default "*" is used.
   * @param table The table to check. By default "*" is used.
   *
   * @return A set of privileges which user lacks. This set is empty if user has
   *         all the required privileges.
   */
  std::set<std::string> get_missing_privileges(
      const std::set<std::string> &required_privileges,
      const std::string &schema = k_wildcard,
      const std::string &table = k_wildcard) const;

  std::string m_account;

  bool m_user_exists = false;

  struct Privilege {
    std::string name;
    bool grantable;

    Privilege(std::string &&name_, bool grantable_)
        : name(std::move(name_)), grantable(grantable_) {}

    bool operator<(const Privilege &priv) const { return name < priv.name; }
  };

  // Map with user privileges information:
  // {user/roles -> {schema -> {table -> {privileges}}}}
  std::unordered_map<
      std::string,
      std::unordered_map<std::string,
                         std::unordered_map<std::string, std::set<Privilege>>>>
      m_privileges;

  // Set of roles/users granted.
  std::set<std::string> m_roles;

  // Set of ALL privileges (NOTE: different depending on the server version).
  std::set<std::string> m_all_privileges;
};

/**
 * Contains the results from a user privilege validation on a specific database
 * object.
 */
class User_privileges_result {
 public:
  /**
   * Creates a result of user privilege validation on the specified
   * schema.table.
   *
   * @param privileges Information on all the privileges user has.
   * @param required_privileges The privileges the user needs to have.
   * @param schema The schema to check.
   * @param table The table to check.
   */
  User_privileges_result(
      const User_privileges &privileges,
      const std::set<std::string> &required_privileges,
      const std::string &schema = User_privileges::k_wildcard,
      const std::string &table = User_privileges::k_wildcard);

  /**
   * Checks if the given user account exists.
   *
   * @return True if account exists.
   */
  bool user_exists() const;

  /**
   * Checks if the given user account has GRANT OPTION privilege on the given
   * schema.table.
   *
   * @return True if user has the GRANT OPTION.
   */
  bool has_grant_option() const;

  /**
   * Checks if the given user account has all required privileges on the given
   * schema.table.
   *
   * @return True if user does not have any of the required privileges.
   */
  bool has_missing_privileges() const;

  /**
   * Checks if the given user account has all required privileges on the given
   * schema.table and provides any missing ones.
   *
   * @return A set of privileges which user lacks. This set is empty if user has
   *         all the required privileges.
   */
  const std::set<std::string> &missing_privileges() const;

 private:
  bool m_user_exists = false;

  bool m_has_grant_option = false;

  std::set<std::string> m_missing_privileges;
};

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_USER_PRIVILEGES_H_
