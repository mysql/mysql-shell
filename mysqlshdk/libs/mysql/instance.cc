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

#include <mysqld_error.h>
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

Instance::Instance(std::shared_ptr<db::ISession> session) : _session(session) {}

std::string Instance::descr() const {
  return _session->get_connection_options().as_uri(
      db::uri::formats::only_transport());
}

std::string Instance::get_canonical_hostname() const {
  // returns the hostname that should be used to reach this instance
  auto result = _session->query("SELECT COALESCE(@@report_host, @@hostname)");
  auto row = result->fetch_one();
  return row->get_as_string(0);
}

int Instance::get_canonical_port() const {
  // returns the hostname that should be used to reach this instance
  auto result = _session->query("SELECT COALESCE(@@report_port, @@port)");
  auto row = result->fetch_one();
  return row->is_null(0) ? 0 : row->get_int(0);
}

void Instance::cache_global_sysvars(bool force_refresh) {
  if (force_refresh || _global_sysvars.empty())
    _global_sysvars = get_system_variables({}, Var_qualifier::GLOBAL);
}

utils::nullable<std::string> Instance::get_cached_global_sysvar(
    const std::string &name) const {
  if (_global_sysvars.empty()) {
    throw std::logic_error("cache_global_sysvars() not called yet");
  }
  const auto it = _global_sysvars.find(name);
  if (it != _global_sysvars.end()) {
    return it->second;
  }
  return {};
}

namespace {
bool sysvar_to_bool(const std::string &name, const std::string &str_value) {
  const char *value = str_value.c_str();
  bool ret_val = false;
  if (shcore::str_caseeq(value, "YES") || shcore::str_caseeq(value, "TRUE") ||
      shcore::str_caseeq(value, "1") || shcore::str_caseeq(value, "ON"))
    ret_val = true;
  else if (shcore::str_caseeq(value, "NO") ||
           shcore::str_caseeq(value, "FALSE") ||
           shcore::str_caseeq(value, "0") || shcore::str_caseeq(value, "OFF"))
    ret_val = false;
  else
    throw std::runtime_error("The variable " + name + "is not boolean.");
  return ret_val;
}
}  // namespace

utils::nullable<bool> Instance::get_cached_global_sysvar_as_bool(
    const std::string &name) const {
  auto value = get_cached_global_sysvar(name);
  if (value) return sysvar_to_bool(name, *value);
  return {};
}

utils::nullable<bool> Instance::get_sysvar_bool(
    const std::string &name, const Var_qualifier scope) const {
  utils::nullable<bool> ret_val;

  std::map<std::string, utils::nullable<std::string>> variables =
      get_system_variables({name}, scope);

  if (variables[name]) {
    ret_val = sysvar_to_bool(name, variables[name]);
  }

  return ret_val;
}

utils::nullable<std::string> Instance::get_sysvar_string(
    const std::string &name, const Var_qualifier scope) const {
  return get_system_variables({name}, scope)[name];
}

utils::nullable<int64_t> Instance::get_sysvar_int(
    const std::string &name, const Var_qualifier scope) const {
  utils::nullable<int64_t> ret_val;

  auto variables = get_system_variables({name}, scope);

  if (variables[name]) {
    std::string value = variables[name];

    if (!value.empty()) {
      size_t end_pos;
      int64_t int_val = std::stol(value, &end_pos);

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
void Instance::set_sysvar(const std::string &name, const std::string &value,
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
 * Set the specified system variable with DEFAULT value.
 *
 * @param name string with the name of the system variable to set.
 * @param qualifier Var_qualifier with the qualifier to set the system variable.
 */
void Instance::set_sysvar_default(const std::string &name,
                                  const Var_qualifier qualifier) const {
  std::string set_stmt_fmt;
  if (qualifier == Var_qualifier::GLOBAL)
    set_stmt_fmt = "SET GLOBAL ! = DEFAULT";
  else if (qualifier == Var_qualifier::PERSIST)
    set_stmt_fmt = "SET PERSIST ! = DEFAULT";
  else if (qualifier == Var_qualifier::PERSIST_ONLY)
    set_stmt_fmt = "SET PERSIST_ONLY ! = DEFAULT";
  else
    set_stmt_fmt = "SET SESSION ! = DEFAULT";

  shcore::sqlstring set_stmt = shcore::sqlstring(set_stmt_fmt.c_str(), 0);
  set_stmt << name;
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
void Instance::set_sysvar(const std::string &name, const int64_t value,
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
void Instance::set_sysvar(const std::string &name, const bool value,
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

std::map<std::string, utils::nullable<std::string>>
Instance::get_system_variables(const std::vector<std::string> &names,
                               const Var_qualifier scope) const {
  std::map<std::string, utils::nullable<std::string>> ret_val;

  std::shared_ptr<db::IResult> result;
  if (!names.empty()) {
    std::string query_format;
    if (scope == Var_qualifier::GLOBAL)
      query_format = "show GLOBAL variables where ! in (?";
    else if (scope == Var_qualifier::SESSION)
      query_format = "show SESSION variables where ! in (?";
    else
      throw std::runtime_error(
          "Invalid variable scope to get variables value, "
          "only GLOBAL and SESSION is supported.");

    ret_val[names[0]] = utils::nullable<std::string>();

    for (size_t index = 1; index < names.size(); index++) {
      query_format.append(", ?");
      ret_val[names[index]] = utils::nullable<std::string>();
    }

    query_format.append(")");

    shcore::sqlstring query(query_format.c_str(), 0);

    query << "variable_name";

    for (const auto &name : names) query << name;

    query.done();

    result = _session->query(query);
  } else {
    if (scope == Var_qualifier::GLOBAL)
      result = _session->query("SHOW GLOBAL VARIABLES");
    else if (scope == Var_qualifier::SESSION)
      result = _session->query("SHOW SESSION VARIABLES");
    else
      throw std::runtime_error(
          "Invalid variable scope to get variables value, "
          "only GLOBAL and SESSION is supported.");
  }

  auto row = result->fetch_one();
  while (row) {
    if (row->is_null(1))
      ret_val[row->get_string(0)] = nullptr;
    else
      ret_val[row->get_string(0)] = row->get_string(1);
    row = result->fetch_one();
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

const std::string &Instance::get_version_compile_os() const {
  if (m_version_compile_os.empty()) {
    m_version_compile_os = *get_sysvar_string("version_compile_os");
  }
  return m_version_compile_os;
}

std::string Instance::get_plugin_library_extension() const {
  std::string instance_os = shcore::str_upper(get_version_compile_os());

  if (instance_os.find("WIN") != std::string::npos) {
    return ".dll";
  } else {
    return ".so";
  }
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
  std::string plugin_lib = plugin_name + get_plugin_library_extension();

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
  // Create the user
  static const std::string create_stmt_fmt =
      "CREATE USER IF NOT EXISTS ?@? IDENTIFIED BY /*(*/ ? /*)*/";
  shcore::sqlstring create_stmt = shcore::sqlstring(create_stmt_fmt.c_str(), 0);
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
    shcore::sqlstring grant_stmt = shcore::sqlstring(grant_stmt_fmt.c_str(), 0);
    grant_stmt << user;
    grant_stmt << host;
    grant_stmt.done();
    _session->execute(grant_stmt);
  }
}

/**
 * Drop the specified user.
 *
 * @param user string with the username part for the user account to drop.
 * @param host string with the host part of the user account to drop.
 */
void Instance::drop_user(const std::string &user, const std::string &host,
                         bool if_exists) const {
  // Drop the user
  std::string drop_stmt_fmt =
      if_exists ? "DROP USER IF EXISTS ?@?" : "DROP USER ?@?";
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
 * Get all privileges of the given user.
 *
 * @param user string with the username part for the user account to check.
 * @param host string with the host part of the user account to check.
 * @return User privileges.
 */
std::unique_ptr<User_privileges> Instance::get_user_privileges(
    const std::string &user, const std::string &host) const {
  return std::unique_ptr<User_privileges>(
      new User_privileges(_session, user, host));
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
  if (_version == utils::Version()) _version = _session->get_server_version();
  return _version;
}

/**
 * Gets the current account of a session
 *
 * @param session object which represents the session to the instance
 * @param current_user output value for the current username
 * @param current_host output value for the current hostname
 *
 * @return nothing
 */
void Instance::get_current_user(std::string *current_user,
                                std::string *current_host) const {
  auto result = _session->query("SELECT CURRENT_USER()");
  auto row = result->fetch_one();
  std::string current_account = row->get_string(0);
  shcore::split_account(current_account, current_user, current_host, true);
}

/**
 * Check if an user exists
 *
 * @param session object which represents the session to the instance
 * @param username account username to check
 * @param hostname account hostname to check
 *
 * @return a boolean value which is true if the account exists or false
 * otherwise
 */
bool Instance::user_exists(const std::string &username,
                           const std::string &hostname) const {
  std::string host = hostname;
  // Host '%' is used by default if not provided in the user account.
  if (host.empty()) host = "%";

  // Query to check if the user exists
  try {
    _session->queryf("SHOW GRANTS FOR ?@?", username, host);
  } catch (mysqlshdk::db::Error &err) {
    log_debug("=> %s", err.what());
    // the current_account doesn't have enough privileges to execute the query
    if (err.code() == ER_TABLEACCESS_DENIED_ERROR) {
      // Get the current username and hostname
      // https://dev.mysql.com/doc/refman/5.7/en/information-functions.html
      // #function_current-user
      std::string current_user, current_host;
      get_current_user(&current_user, &current_host);

      std::string error_msg =
          "Session account '" + current_user + "'@'" + current_host +
          "' does not have all the required privileges to execute this "
          "operation. For more information, see the online documentation.";

      log_warning("%s", error_msg.c_str());
      throw std::runtime_error(error_msg);
    } else if (err.code() == ER_NONEXISTING_GRANT) {
      return false;
    } else {
      log_error("Unexpected error while checking if user exists: %s",
                err.what());
      throw;
    }
  }
  return true;
}

}  // namespace mysql
}  // namespace mysqlshdk
