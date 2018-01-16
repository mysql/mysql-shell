/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <map>
#include <utility>

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "scripting/types.h"

namespace mysqlshdk {
namespace mysql {

Instance::Instance(std::shared_ptr<db::ISession> session) : _session(session) {
}

utils::nullable<bool> Instance::get_sysvar_bool(
    const std::string &name, const Var_qualifier scope) const {
  utils::nullable<bool> ret_val;

  std::map<std::string, utils::nullable<std::string>> variables =
      get_system_variables({name}, scope);

  if (variables[name]) {
    std::string str_value = variables[name];
    const char *value = str_value.c_str();

    if (shcore::str_caseeq(value, "YES") || shcore::str_caseeq(value, "TRUE") ||
        shcore::str_caseeq(value, "1") || shcore::str_caseeq(value, "ON"))
      ret_val = true;
    else if (shcore::str_caseeq(value, "NO") ||
             shcore::str_caseeq(value, "FALSE") ||
             shcore::str_caseeq(value, "0") || shcore::str_caseeq(value, "OFF"))
      ret_val = false;
    else
      throw std::runtime_error("The variable " + name + "is not boolean.");
  }

  return ret_val;
}

utils::nullable<std::string> Instance::get_sysvar_string(
    const std::string& name, const Var_qualifier scope) const {
  return get_system_variables({name}, scope)[name];
}

utils::nullable<int64_t> Instance::get_sysvar_int(
    const std::string& name, const Var_qualifier scope) const {
  utils::nullable<int64_t> ret_val;

  auto variables = get_system_variables({name}, scope);

  if (variables[name]) {
    std::string value = variables[name];

    if (!value.empty()) {
      size_t end_pos;
      int64_t int_val = std::stoi(value, &end_pos);

      if (end_pos == value.size())
        ret_val = int_val;
      else
        throw std::runtime_error("The variable " + name +
                                 " is not an integer.");
    }
  }

  return ret_val;
}

/**
 * Set the specified system variable with the given value.
 *
 * @param name string with the name of the system variable to set.
 * @param value string with the value to set for the variable.
 * @param qualifier Var_qualifier with the qualifier to set the system variable.
 */
void Instance::set_sysvar(const std::string &name,
                          const std::string &value,
                          const Var_qualifier qualifier) const {
  std::string set_stmt_fmt;
  if (qualifier == Var_qualifier::GLOBAL)
    set_stmt_fmt = "SET GLOBAL ! = ?";
  else if (qualifier == Var_qualifier::PERSIST)
    set_stmt_fmt = "SET PERSIST ! = ?";
  else if (qualifier == Var_qualifier::PERSIST_ONLY)
    set_stmt_fmt = "SET PERSIST_ONLY ! = ?";
  else
    set_stmt_fmt = "SET SESSION ! = ?";

  shcore::sqlstring set_stmt = shcore::sqlstring(set_stmt_fmt.c_str(), 0);
  set_stmt << name;
  set_stmt << value;
  set_stmt.done();
  _session->execute(set_stmt);
}

/**
 * Set the specified system variable with the given value.
 *
 * @param name string with the name of the system variable to set.
 * @param value integer value to set for the variable.
 * @param qualifier Var_qualifier with the qualifier to set the system variable.
 */
void Instance::set_sysvar(const std::string &name,
                          const int64_t value,
                          const Var_qualifier qualifier) const {
  std::string set_stmt_fmt;
  if (qualifier == Var_qualifier::GLOBAL)
    set_stmt_fmt = "SET GLOBAL ! = ?";
  else if (qualifier == Var_qualifier::PERSIST)
    set_stmt_fmt = "SET PERSIST ! = ?";
  else if (qualifier == Var_qualifier::PERSIST_ONLY)
    set_stmt_fmt = "SET PERSIST_ONLY ! = ?";
  else
    set_stmt_fmt = "SET SESSION ! = ?";

  shcore::sqlstring set_stmt = shcore::sqlstring(set_stmt_fmt.c_str(), 0);
  set_stmt << name;
  set_stmt << value;
  set_stmt.done();
  _session->execute(set_stmt);
}

/**
 * Set the specified system variable with the given value.
 *
 * @param name string with the name of the system variable to set.
 * @param value boolean value to set for the variable.
 * @param qualifier Var_qualifier with the qualifier to set the system variable.
 */
void Instance::set_sysvar(const std::string &name,
                          const bool value,
                          const Var_qualifier qualifier) const {
  std::string str_value = value ? "ON" : "OFF";
  std::string set_stmt_fmt;
  if (qualifier == Var_qualifier::GLOBAL)
    set_stmt_fmt = "SET GLOBAL ! = ?";
  else if (qualifier == Var_qualifier::PERSIST)
    set_stmt_fmt = "SET PERSIST ! = ?";
  else if (qualifier == Var_qualifier::PERSIST_ONLY)
    set_stmt_fmt = "SET PERSIST_ONLY ! = ?";
  else
    set_stmt_fmt = "SET SESSION ! = ?";

  shcore::sqlstring set_stmt = shcore::sqlstring(set_stmt_fmt.c_str(), 0);
  set_stmt << name;
  set_stmt << str_value;
  set_stmt.done();
  _session->execute(set_stmt);
}

std::map<std::string, utils::nullable<std::string> >
Instance::get_system_variables(const std::vector<std::string>& names,
                               const Var_qualifier scope) const {
  std::map<std::string, utils::nullable<std::string> > ret_val;

  if (!names.empty()) {
    std::string query_format;
    if (scope == Var_qualifier::GLOBAL)
      query_format = "show GLOBAL variables where ! in (?";
    else if (scope == Var_qualifier::SESSION)
      query_format = "show SESSION variables where ! in (?";
    else
      throw std::runtime_error("Invalid variable scope to get variables value, "
                               "only GLOBAL and SESSION is supported.");

    ret_val[names[0]] = utils::nullable<std::string>();

    for (size_t index = 1; index < names.size(); index++) {
      query_format.append(", ?");
      ret_val[names[index]] = utils::nullable<std::string>();
    }

    query_format.append(")");

    shcore::sqlstring query(query_format.c_str(), 0);

    query << "variable_name";

    for (const auto &name : names)
      query << name;

    query.done();

    auto result = _session->query(query);
    auto row = result->fetch_one();

    while (row) {
      ret_val[row->get_string(0)] = row->get_string(1);
      row = result->fetch_one();
    }
  }

  return ret_val;
}

/**
 * Get the status of the specified plugin.
 *
 * @param plugin_name string with the name of the plugin to check.
 *
 * @return nullable string with the state of the plugin if available ("ACTIVE"
 *         or "DISABLED") or NULL if the plugin is not available (installed).
 */
utils::nullable<std::string> Instance::get_plugin_status(
    const std::string &plugin_name) const {
  // Find the state information for the specified plugin.
  std::string plugin_state_stmt_fmt =
      "SELECT plugin_status "
      "FROM information_schema.plugins "
      "WHERE plugin_name = ?";
  shcore::sqlstring plugin_state_stmt =
      shcore::sqlstring(plugin_state_stmt_fmt.c_str(), 0);
  plugin_state_stmt << plugin_name;
  plugin_state_stmt.done();
  auto resultset = _session->query(plugin_state_stmt);
  auto row = resultset->fetch_one();
  if (row)
    return utils::nullable<std::string>(row->get_string(0));
  else
    // No state information found, return NULL.
    return utils::nullable<std::string>();
}

/**
 * Install the specified plugin.
 *
 * @param plugin_name string with the name of the plugin to install.
 *
 * @throw std::runtime_error if an error occurs installing the plugin.
 */
void Instance::install_plugin(const std::string &plugin_name) const {
  // Determine the extension of the plugin library.
  std::string instance_os =
      shcore::str_upper(get_sysvar_string("version_compile_os"));
  std::string plugin_lib = plugin_name;
  if (instance_os.find("WIN") != std::string::npos) {
    plugin_lib += ".dll";
  } else {
    plugin_lib += ".so";
  }

  // Install the GR plugin.
  try {
    std::string stmt_fmt = "INSTALL PLUGIN ! SONAME ?";
    shcore::sqlstring stmt = shcore::sqlstring(stmt_fmt.c_str(), 0);
    stmt << plugin_name;
    stmt << plugin_lib;
    stmt.done();
    _session->execute(stmt);
  } catch (std::exception &err) {
    // Install plugin failed.
    throw std::runtime_error("error installing plugin '" + plugin_name +
                             "': " + err.what());
  }
}

/**
 * Uninstall the specified plugin.
 *
 * @param plugin_name string with the name of the plugin to install.
 *
 * @throw std::runtime_error if an error occurs uninstalling the plugin.
 */
void Instance::uninstall_plugin(const std::string &plugin_name) const {
  std::string stmt_fmt = "UNINSTALL PLUGIN !";
  shcore::sqlstring stmt = shcore::sqlstring(stmt_fmt.c_str(), 0);
  stmt << plugin_name;
  stmt.done();
  // Uninstall the plugin.
  try {
    _session->execute(stmt);
  } catch (std::exception &err) {
    // Uninstall plugin failed.
    throw std::runtime_error("error uninstalling plugin '" + plugin_name +
                             "': " + err.what());
  }
}

/**
 * Create the specified new user.
 *
 * @param user string with the username part for the user account to create.
 * @param host string with the host part of the user account to create.
 * @param pwd string with the password for the user account.
 * @param grants vector of tuples with with the privileges to grant on objects.
 *               The tuple has three elements: the first is a string with a
 *               comma separated list of the privileges to grant (e.g., "SELECT,
 *               INSERT, UPDATE"), the second is a string with the target
 *               object to grant those privileges on (e.g., "mysql.*"), and the
 *               third is a boolean value indicating if the grants will be
 *               given with the "GRANT OPTION" privilege (true) or not (false).
 */
void Instance::create_user(
    const std::string &user, const std::string &host, const std::string &pwd,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants)
    const {
  // NOTE: An implicit COMMIT is executed by CREATE USER and GRANT before
  // executing: https://dev.mysql.com/doc/en/implicit-commit.html
  try {
    // Create the user
    std::string create_stmt_fmt = "CREATE USER ?@? IDENTIFIED BY ?";
    shcore::sqlstring create_stmt =
        shcore::sqlstring(create_stmt_fmt.c_str(), 0);
    create_stmt << user;
    create_stmt << host;
    create_stmt << pwd;
    create_stmt.done();
    _session->execute(create_stmt);

    // Grant privileges
    for (size_t i = 0; i < grants.size(); i++) {
      std::string grant_stmt_fmt = "GRANT " + std::get<0>(grants[i]) + " ON " +
                                   std::get<1>(grants[i]) + " TO ?@?";
      if (std::get<2>(grants[i]))
        grant_stmt_fmt = grant_stmt_fmt + " WITH GRANT OPTION";
      shcore::sqlstring grant_stmt =
          shcore::sqlstring(grant_stmt_fmt.c_str(), 0);
      grant_stmt << user;
      grant_stmt << host;
      grant_stmt.done();
      _session->execute(grant_stmt);
    }
    _session->execute("COMMIT");
  } catch (std::exception &err) {
    _session->execute("ROLLBACK");
    throw;
  }
}

/**
 * Drop the specified user.
 *
 * @param user string with the username part for the user account to drop.
 * @param host string with the host part of the user account to drop.
 */
void Instance::drop_user(const std::string &user,
                         const std::string &host) const {
  // Drop the user
  std::string drop_stmt_fmt = "DROP USER ?@?";
  shcore::sqlstring drop_stmt = shcore::sqlstring(drop_stmt_fmt.c_str(), 0);
  drop_stmt << user;
  drop_stmt << host;
  drop_stmt.done();
  _session->execute(drop_stmt);
}

/**
 * Drop all the users with the matching regular expression.
 *
 * A supported MySQL regular expression must be specified and it will be used
 * to match the user name (not the host part) of the user accounts to remove.
 * For more information about supported MySQL regular expressions, see:
 * https://dev.mysql.com/doc/en/regexp.html
 *
 * @param regexp String with the regular expression to match the user name of
 *               the account to remove. It must be a supported MySQL regular
 *               expression.
 *
 */
void Instance::drop_users_with_regexp(const std::string &regexp) const {
  // Get all users matching the provided regular expression.
  std::string users_to_drop_stmt_fmt =
      "SELECT GRANTEE FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
      "WHERE GRANTEE REGEXP ?";
  shcore::sqlstring users_to_drop_stmt =
      shcore::sqlstring(users_to_drop_stmt_fmt.c_str(), 0);
  users_to_drop_stmt << regexp;
  users_to_drop_stmt.done();
  auto resultset = _session->query(users_to_drop_stmt);
  auto row = resultset->fetch_one();
  std::vector<std::string> user_accounts;
  while (row) {
    user_accounts.push_back(row->get_string(0));
    row = resultset->fetch_one();
  }
  // Remove all matching users.
  std::string drop_stmt = "DROP USER IF EXISTS ";
  for (std::string user_account : user_accounts)
    _session->execute(drop_stmt + user_account);
}

/**
 * Check the user.
 *
 * Verify if the given user exists and if it possesses the specified
 * privileges.
 *
 * @param user string with the username part for the user account to check.
 * @param host string with the host part of the user account to check.
 * @param privileges vector of strings with the list of privileges to check.
 * @param on_db string with the target schema to check the privileges.
 * @param on_obj string with the target object (in the schema) to check the
 *               privileges.
 * @return tuple with three elements: first, boolean value indicating if the
 *         user exists (true) or not (false); second, string with a comma
 *         separated list of the missing privileges; third, boolean value
 *         indicating if the user has GRANT OPTION (true) or not (false).
 */
std::tuple<bool, std::string, bool> Instance::check_user(
    const std::string &user, const std::string &host,
    const std::vector<std::string> &privileges, const std::string &on_db,
    const std::string &on_obj) const {
  std::tuple<bool, std::string, bool> result(false, "", false);
  // Validate given privileges.
  std::vector<std::string> priv_needed(privileges);
  std::transform(priv_needed.begin(), priv_needed.end(), priv_needed.begin(),
                 shcore::str_upper);
  bool all_priv = false;
  for (const std::string &priv : priv_needed) {
    // Check if ALL [PRIVILEGES] is used.
    if (priv.compare("ALL") == 0 || priv.compare("ALL PRIVILEGES") == 0) {
      if (priv_needed.size() != 1)
        throw std::runtime_error{"Invalid list of privileges. '" + priv +
                                 "' already include all the other "
                                 "privileges and it must be specified alone."};
      all_priv = true;
      priv_needed = kAllPrivileges;
      break;
    }
    // Check if all specified privileges are valid.
    if (std::find(kAllPrivileges.begin(), kAllPrivileges.end(), priv) ==
        kAllPrivileges.end())
      throw std::runtime_error{"Invalid privilege in the privileges list: '" +
                               priv + "'."};
  }

  // Get the grants for the user on *.*
  std::map<std::string,
           std::map<std::string, std::pair<std::vector<std::string>, bool>>>
      user_grants;
  std::string user_priv_stmt_fmt =
      "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
      "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
      "WHERE GRANTEE = \"?@?\"";
  shcore::sqlstring user_priv_stmt =
      shcore::sqlstring(user_priv_stmt_fmt.c_str(), 0);
  user_priv_stmt << user;
  user_priv_stmt << host;
  user_priv_stmt.done();
  auto resultset = _session->query(user_priv_stmt);
  auto row = resultset->fetch_one();
  std::vector<std::string> grants{};
  bool is_grantable = false;
  // Check if the user exists (all have at least one privilege: USAGE)
  if (!row) {
    // If user does not exist, then all privileges are also missing.
    std::get<1>(result) = shcore::str_join(privileges, ",");
    return result;
  } else {
    // User exists.
    std::get<0>(result) = true;
  }
  // Get grants on *.*
  while (row) {
    grants.push_back(row->get_string(0));
    if (row->get_string(1).compare("YES") == 0)
      is_grantable = true;
    row = resultset->fetch_one();
  }
  user_grants["*"]["*"] =
      std::pair<std::vector<std::string>, bool>(grants, is_grantable);
  resultset.reset();

  // Get the grants for the user on specific schemas: <schema>.*
  std::string db_priv_stmt_fmt =
      "SELECT TABLE_SCHEMA, PRIVILEGE_TYPE, IS_GRANTABLE "
      "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
      "WHERE GRANTEE = \"?@?\" ORDER BY TABLE_SCHEMA";
  shcore::sqlstring db_priv_stmt =
      shcore::sqlstring(db_priv_stmt_fmt.c_str(), 0);
  db_priv_stmt << user;
  db_priv_stmt << host;
  db_priv_stmt.done();
  resultset = _session->query(db_priv_stmt);
  row = resultset->fetch_one();
  std::string db_name = "";
  is_grantable = false;
  while (row) {
    if (db_name.compare(row->get_string(0)) != 0) {
      if (!db_name.empty())
        user_grants[db_name]["*"] =
            std::pair<std::vector<std::string>, bool>(grants, is_grantable);
      db_name = row->get_string(0);
      grants.clear();
      is_grantable = false;
    }
    grants.push_back(row->get_string(1));
    if (row->get_string(2).compare("YES") == 0)
      is_grantable = true;
    row = resultset->fetch_one();
  }
  if (!db_name.empty())
    user_grants[db_name]["*"] =
        std::pair<std::vector<std::string>, bool>(grants, is_grantable);
  resultset.reset();

  // Get the grants for the user on a specific tables: <schema>.<table>
  std::string tbl_priv_stmt_fmt =
      "SELECT TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE, IS_GRANTABLE "
      "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
      "WHERE GRANTEE = \"?@?\" ORDER BY TABLE_SCHEMA, TABLE_NAME";
  shcore::sqlstring tbl_priv_stmt =
      shcore::sqlstring(tbl_priv_stmt_fmt.c_str(), 0);
  tbl_priv_stmt << user;
  tbl_priv_stmt << host;
  tbl_priv_stmt.done();
  resultset = _session->query(tbl_priv_stmt);
  row = resultset->fetch_one();
  db_name = "";
  std::string tbl_name = "";
  is_grantable = false;
  while (row) {
    if (db_name.compare(row->get_string(0)) != 0 ||
        tbl_name.compare(row->get_string(1)) != 0) {
      if (!db_name.empty() && !tbl_name.empty())
        user_grants[db_name][tbl_name] =
            std::pair<std::vector<std::string>, bool>(grants, is_grantable);
      db_name = row->get_string(0);
      tbl_name = row->get_string(1);
      grants.clear();
      is_grantable = false;
    }
    grants.push_back(row->get_string(2));
    if (row->get_string(3).compare("YES") == 0)
      is_grantable = true;
    row = resultset->fetch_one();
  }
  if (!db_name.empty() && !tbl_name.empty())
    user_grants[db_name][tbl_name] =
        std::pair<std::vector<std::string>, bool>(grants, is_grantable);

  // Check privileges for the user.
  std::vector<std::string> priv_missing{};
  bool has_grant_option = false;
  for (unsigned int i = 0; i < priv_needed.size(); i++) {
    // Verify if privilege is available for *.*
    const std::vector<std::string> *v = &user_grants["*"]["*"].first;
    if (std::find(v->begin(), v->end(), priv_needed[i]) != v->end()) {
      bool grant_option = user_grants["*"]["*"].second;
      has_grant_option = grant_option ? grant_option : has_grant_option;
      continue;  // privilege found on *.*
    }
    // Verify privileges for schema if specified (not "*").
    if (on_db.compare("*") != 0 && user_grants.count(on_db) > 0) {
      if (user_grants[on_db].count("*") > 0) {
        v = &user_grants[on_db]["*"].first;
        bool grant_option = user_grants[on_db]["*"].second;
        if (std::find(v->begin(), v->end(), priv_needed[i]) != v->end()) {
          has_grant_option = grant_option ? grant_option : has_grant_option;
          continue;  // privilege found on <db_name>.*
        }
      }
      if (user_grants[on_db].count(on_obj) > 0) {
        v = &user_grants[on_db][on_obj].first;
        bool grant_option = user_grants[on_db][on_obj].second;
        if (std::find(v->begin(), v->end(), priv_needed[i]) != v->end()) {
          has_grant_option = grant_option ? grant_option : has_grant_option;
          continue;  // privilege found on <db_name>.<tbl_name>
        }
      }
    }
    // Privilege not found for user, add it to the missing list.
    priv_missing.push_back(priv_needed[i]);
  }

  // Set missing privilege and GRANT OPTION value.
  if (all_priv && priv_missing.size() == kAllPrivileges.size())
    // Set missing to 'ALL [PRIVILEGES]'
    std::get<1>(result) = shcore::str_upper(privileges[0]);
  else
    std::get<1>(result) = shcore::str_join(priv_missing, ",");
  std::get<2>(result) = has_grant_option;

  return result;
}


bool Instance::is_read_only(bool super) const {
  // Check if the member is not read_only
  std::shared_ptr<mysqlshdk::db::IResult> result(
      _session->query(super ? "select @@super_read_only"
                            : "select @@read_only or @@super_read_only"));
  const mysqlshdk::db::IRow *row(result->fetch_one());
  if (row) {
    return (row->get_int(0) != 0);
  }
  throw std::logic_error("unexpected null result from query");
}

utils::Version Instance::get_version() const {
  if (_version == utils::Version())
    _version = _session->get_server_version();
  return _version;
}

}  // namespace mysql
}  // namespace mysqlshdk
