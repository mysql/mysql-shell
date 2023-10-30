/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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
#include <array>
#include <map>
#include <string_view>
#include <utility>

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "scripting/types.h"

namespace mysqlshdk {
namespace mysql {

namespace {
// Define the fence sysvar list.
constexpr std::array<std::string_view, 3> k_fence_sysvars = {
    "read_only", "super_read_only", "offline_mode"};
}  // namespace

void Auth_options::get(const mysqlshdk::db::Connection_options &copts) {
  user = copts.get_user();
  password = copts.has_password()
                 ? std::optional<std::string>{copts.get_password()}
                 : std::nullopt;
  ssl_options = copts.get_ssl_options();
}

void Auth_options::set(mysqlshdk::db::Connection_options *copts) const {
  copts->set_user(user);
  if (!password.has_value())
    copts->clear_password();
  else
    copts->set_password(*password);
  copts->set_ssl_options(ssl_options);
}

Instance::Instance(const std::shared_ptr<db::ISession> &session)
    : _session(session) {}

void Instance::register_warnings_callback(const Warnings_callback &callback) {
  m_warnings_callback = callback;
}

void Instance::refresh() {
  m_uuid.clear();
  m_group_name.clear();
}

std::string Instance::descr() const { return get_canonical_address(); }

std::string Instance::get_canonical_hostname() const {
  if (m_hostname.empty()) {
    // returns the hostname that should be used to reach this instance
    auto result = query("SELECT COALESCE(@@report_host, @@hostname)");
    auto row = result->fetch_one();
    m_hostname = row->get_as_string(0);
  }
  return m_hostname;
}

int Instance::get_canonical_port() const {
  if (m_port == 0) {
    // returns the hostname that should be used to reach this instance
    auto result = query("SELECT COALESCE(@@report_port, @@port)");
    auto row = result->fetch_one();
    m_port = row->is_null(0) ? 0 : row->get_int(0);
  }
  return m_port;
}

std::optional<int> Instance::get_xport() const {
  if (!m_xport.has_value()) {
    try {
      auto result = query("SELECT @@mysqlx_port");
      auto row = result->fetch_one();

      m_xport = row->get_int(0);
    } catch (const std::exception &) {
      log_info("The X plugin is not enabled on instance '%s'.",
               descr().c_str());

      m_xport = std::nullopt;
    }
  }

  return m_xport;
}

std::string Instance::get_canonical_address() const {
  // returns the canonical address that should be used to reach this instance in
  // the format: canonical_hostname + ':' + canonical_port
  if (m_hostname.empty() || m_port == 0) {
    auto result = query(
        "SELECT COALESCE(@@report_host, @@hostname),"
        "  COALESCE(@@report_port, @@port)");
    auto row = result->fetch_one_or_throw();
    m_hostname = row->get_string(0, "");
    m_port = row->get_int(1);
  }
  return mysqlshdk::utils::make_host_and_port(m_hostname, m_port);
}

uint32_t Instance::get_server_id() const {
  if (m_server_id == 0) {
    m_server_id = queryf_one_int(0, 0, "SELECT @@server_id");
  }

  return m_server_id;
}

const std::string &Instance::get_uuid() const {
  if (m_uuid.empty()) {
    m_uuid = queryf_one_string(0, "", "SELECT @@server_uuid");
  }
  return m_uuid;
}

uint32_t Instance::get_id() const {
  if (!m_id.has_value()) m_id = queryf_one_int(0, 0, "SELECT @@server_id");
  return *m_id;
}

const std::string &Instance::get_group_name() const {
  if (m_group_name.empty()) {
    m_group_name =
        get_sysvar_string("group_replication_group_name").value_or("");
  }
  return m_group_name;
}

const std::string &Instance::get_version_compile_os() const {
  if (m_version_compile_os.empty()) {
    m_version_compile_os =
        get_sysvar_string("version_compile_os", Var_qualifier::GLOBAL)
            .value_or("");
  }
  return m_version_compile_os;
}

const std::string &Instance::get_version_compile_machine() const {
  if (m_version_compile_machine.empty()) {
    m_version_compile_machine =
        get_sysvar_string("version_compile_machine", Var_qualifier::GLOBAL)
            .value_or("");
  }
  return m_version_compile_machine;
}

std::optional<bool> Instance::get_sysvar_bool(std::string_view name,
                                              const Var_qualifier scope) const {
  auto value = get_system_variable(name, scope);
  if (!value.has_value()) return {};

  if (shcore::str_caseeq(*value, "YES", "TRUE", "1", "ON")) return true;
  if (shcore::str_caseeq(*value, "NO", "FALSE", "0", "OFF")) return false;

  throw std::runtime_error(
      shcore::str_format("The variable %.*s is not boolean.",
                         static_cast<int>(name.size()), name.data()));
}

std::optional<std::string> Instance::get_sysvar_string(
    std::string_view name, const Var_qualifier scope) const {
  return get_system_variable(name, scope);
}

std::optional<int64_t> Instance::get_sysvar_int(
    std::string_view name, const Var_qualifier scope) const {
  auto value = get_system_variable(name, scope);
  if (!value.has_value()) return {};

  if (value->empty()) return {};

  try {
    return shcore::lexical_cast<int64_t>(*value);
  } catch (const std::invalid_argument &) {
    throw std::runtime_error(
        shcore::str_format("The variable %.*s is not an integer.",
                           static_cast<int>(name.size()), name.data()));
  }
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

  query(set_stmt);
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

  query(set_stmt);
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

  query(set_stmt);
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

  query(set_stmt);
}

std::optional<std::string> Instance::get_system_variable(
    std::string_view name, const Var_qualifier scope) const {
  shcore::sqlstring query;
  if (scope == Var_qualifier::GLOBAL)
    query = "show GLOBAL variables where ! in (?)"_sql;
  else if (scope == Var_qualifier::SESSION)
    query = "show SESSION variables where ! in (?)"_sql;
  else
    throw std::runtime_error(
        "Invalid variable scope to get variables value, "
        "only GLOBAL and SESSION is supported.");

  query << "variable_name" << name;
  query.done();

  auto result = this->query(query);
  if (!result) return {};

  auto row = result->fetch_one();
  if (!row || row->is_null(1)) return {};

  return row->get_string(1);
}

std::map<std::string, std::optional<std::string>>
Instance::get_system_variables_like(const std::string &pattern,
                                    const Var_qualifier scope) const {
  std::map<std::string, std::optional<std::string>> ret_val;

  std::string query_format;
  if (scope == Var_qualifier::GLOBAL)
    query_format = "SHOW GLOBAL VARIABLES LIKE ?";
  else if (scope == Var_qualifier::SESSION)
    query_format = "SHOW SESSION VARIABLES LIKE ?";
  else
    throw std::runtime_error(
        "Invalid variable scope to get variables value, "
        "only GLOBAL and SESSION is supported.");

  shcore::sqlstring query(query_format.c_str(), 0);
  query << pattern;
  query.done();

  std::shared_ptr<db::IResult> result;
  result = this->query(query);

  auto row = result->fetch_one();
  while (row) {
    if (row->is_null(1))
      ret_val[row->get_string(0)] = std::nullopt;
    else
      ret_val[row->get_string(0)] = row->get_string(1);
    row = result->fetch_one();
  }

  return ret_val;
}

/**
 * Check if the performance schema is enabled on the instance.
 *
 * @return true if performance_schema is enabled and false otherwise.
 */

bool Instance::is_performance_schema_enabled() const {
  return get_sysvar_bool("performance_schema", false);
}

bool Instance::is_ssl_enabled() const {
  bool have_ssl;
  if (get_version() >= mysqlshdk::utils::Version(8, 0, 21)) {
    try {
      auto enabled =
          query(
              "SELECT value FROM performance_schema.tls_channel_status"
              " WHERE channel='mysql_main' AND property = 'Enabled'")
              ->fetch_one_or_throw()
              ->get_string(0);
      have_ssl = shcore::str_caseeq(enabled, "Yes");
    } catch (const shcore::Error &e) {
      if (e.code() != ER_TABLEACCESS_DENIED_ERROR) throw;
      have_ssl = shcore::str_caseeq(get_sysvar_string("have_ssl", ""), "YES");
    }
  } else {
    have_ssl = shcore::str_caseeq(get_sysvar_string("have_ssl", ""), "YES");
  }
  return have_ssl;
}

/**
 * Returns true if a given variable still has the default (compiled) value.
 * @param name string with the name of the variable to check
 * @return true if the given variable has the default (compiled) value and false
 * otherwise.
 * @throw std::runtime_error if performance_schema is not enabled.
 * @throw std::invalid_argument if name cannot be found in the
 * performance_schema variables_info table.
 */

bool Instance::has_variable_compiled_value(std::string_view name) const {
  if (!is_performance_schema_enabled())
    throw std::runtime_error(shcore::str_format(
        "Unable to check if variable '%.*s' has the default (compiled) value "
        "since performance_schema is not enabled.",
        static_cast<int>(name.size()), name.data()));

  auto stmt =
      "SELECT (variable_source = 'COMPILED') FROM "
      "performance_schema.variables_info WHERE variable_name = ?"_sql;
  stmt << name;
  stmt.done();

  auto resultset = query(stmt);
  if (auto row = resultset->fetch_one(); row) {
    return (row->get_int(0) != 0);
  }

  throw std::runtime_error(
      shcore::str_format("Unable to find variable '%.*s' in the "
                         "performance_schema.variables_info table.",
                         static_cast<int>(name.size()), name.data()));
}

/**
 * Get the status of the specified plugin.
 *
 * @param plugin_name string with the name of the plugin to check.
 *
 * @return nullable string with the state of the plugin if available ("ACTIVE"
 *         or "DISABLED") or NULL if the plugin is not available (installed).
 */
std::optional<std::string> Instance::get_plugin_status(
    const std::string &plugin_name) const {
  // Find the state information for the specified plugin.
  auto plugin_state_stmt =
      "SELECT plugin_status FROM information_schema.plugins "
      "WHERE plugin_name = ?"_sql;

  plugin_state_stmt << plugin_name;
  plugin_state_stmt.done();

  auto resultset = query(plugin_state_stmt);
  auto row = resultset->fetch_one();
  if (row) return row->get_string(0);

  // No state information found, return NULL.
  return std::nullopt;
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
  std::string plugin_lib;

  if (plugin_name == "clone") {
    plugin_lib = "mysql_clone";
  } else {
    plugin_lib = plugin_name;
  }

  plugin_lib += get_plugin_library_extension();

  // Install the GR plugin.
  try {
    auto stmt = ("INSTALL PLUGIN ! SONAME ?"_sql << plugin_name << plugin_lib);
    stmt.done();
    execute(stmt);
  } catch (const std::exception &err) {
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
  auto stmt = ("UNINSTALL PLUGIN !"_sql << plugin_name);
  stmt.done();

  // Uninstall the plugin.
  try {
    execute(stmt);
  } catch (const std::exception &err) {
    // Uninstall plugin failed.
    throw std::runtime_error("error uninstalling plugin '" + plugin_name +
                             "': " + err.what());
  }
}

/**
 * Create the specified new user.
 *
 * @param options
 */
void Instance::create_user(std::string_view user, std::string_view host,
                           const Create_user_options &options) const {
  auto user_stmt = ("CREATE USER IF NOT EXISTS ?@?"_sql << user << host).str();

  if (options.password.has_value())
    user_stmt.append(
        (" IDENTIFIED BY /*((*/ ? /*))*/"_sql << *options.password).str());

  if (options.cert_issuer.empty() && options.cert_subject.empty()) {
    user_stmt.append(" REQUIRE NONE");
  } else if (!options.cert_issuer.empty() && !options.cert_subject.empty()) {
    user_stmt.append((" REQUIRE ISSUER ? AND SUBJECT ?"_sql
                      << options.cert_issuer << options.cert_subject)
                         .str());

  } else {
    if (!options.cert_issuer.empty())
      user_stmt.append((" REQUIRE ISSUER ?"_sql << options.cert_issuer).str());
    else
      user_stmt.append(
          (" REQUIRE SUBJECT ?"_sql << options.cert_subject).str());
  }

  if (options.password.has_value() && options.disable_pwd_expire)
    user_stmt.append(" PASSWORD EXPIRE NEVER");

  execute(user_stmt);

  // grant privileges
  for (const auto &grant : options.grants) {
    auto grant_stmt = shcore::str_format(
        "GRANT %s ON %s TO ?@?%s", grant.privileges.c_str(),
        grant.target.c_str(), grant.grant_option ? " WITH GRANT OPTION" : "");

    shcore::sqlstring sql_str(grant_stmt.c_str(), 0);
    (sql_str << user << host).done();
    execute(sql_str);
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
  auto stmt = (if_exists ? "DROP USER IF EXISTS ?@?"_sql : "DROP USER ?@?"_sql);
  (stmt << user << host).done();
  execute(stmt);
}

/**
 * Get all privileges of the given user.
 *
 * @param user string with the username part for the user account to check.
 * @param host string with the host part of the user account to check.
 * @param allow_skip_grants_user Whether a skip-grants user is a valid account.
 * @return User privileges.
 */
std::unique_ptr<User_privileges> Instance::get_user_privileges(
    const std::string &user, const std::string &host,
    bool allow_skip_grants_user) const {
  return std::make_unique<User_privileges>(*this, user, host,
                                           allow_skip_grants_user);
}

std::unique_ptr<User_privileges> Instance::get_current_user_privileges(
    bool allow_skip_grants_user) const {
  std::string user;
  std::string host;

  get_current_user(&user, &host);

  return get_user_privileges(user, host, allow_skip_grants_user);
}

bool Instance::is_read_only(bool super) const {
  // Check if the member is not read_only
  std::shared_ptr<mysqlshdk::db::IResult> result(
      query(super ? "select @@super_read_only"
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
  auto result = query("SELECT CURRENT_USER()");
  auto row = result->fetch_one_or_throw();
  std::string current_account = row->get_string(0);
  shcore::split_account(current_account, current_user, current_host,
                        shcore::Account::Auto_quote::USER_AND_HOST);
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
    queryf("SHOW GRANTS FOR ?@?", username, host);
  } catch (const mysqlshdk::db::Error &err) {
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

/**
 * Set the password for a given mysql user account
 *
 * @param username account username
 * @param hostname account hostname
 * @param password password we want to set for the account
 */
void Instance::set_user_password(const std::string &username,
                                 const std::string &hostname,
                                 const std::string &password) const {
  log_debug("Changing password for user %s@%s", username.c_str(),
            hostname.c_str());
  executef("ALTER USER /*(*/ ?@? /*)*/ IDENTIFIED BY /*((*/ ? /*))*/", username,
           hostname, password);
}

std::optional<bool> Instance::is_set_persist_supported() const {
  // Check if the instance version is >= 8.0.11 to support the SET PERSIST.
  if (get_version() < mysqlshdk::utils::Version(8, 0, 11)) return {};

  // Check the value of persisted_globals_load
  return get_sysvar_bool("persisted_globals_load", Var_qualifier::GLOBAL);
}

std::optional<std::string> Instance::get_persisted_value(
    const std::string &variable_name) const {
  auto result = queryf(
      "SELECT variable_value "
      "FROM performance_schema.persisted_variables "
      "WHERE variable_name = ?",
      variable_name);

  auto row = result->fetch_one();
  if (row) return row->get_string(0);

  return {};
}

std::vector<std::string> Instance::get_fence_sysvars() const {
  std::vector<std::string> result;

  // Create the query to get the fence sysvars.
  std::string str_query = "SELECT";
  for (auto it = k_fence_sysvars.cbegin(); it != k_fence_sysvars.cend(); ++it) {
    if (it != k_fence_sysvars.cbegin()) str_query.append(",");
    str_query.append(" @@").append(*it);
  }

  // Execute the query and add all the enabled sysvars to the result list.
  auto resultset = _session->query(str_query);
  auto row = resultset->fetch_one();
  if (row) {
    for (size_t i = 0; i < k_fence_sysvars.size(); ++i) {
      if (row->get_int(i) != 0) {
        result.push_back(std::string{k_fence_sysvars.at(i)});
      }
    }
    return result;
  } else {
    throw std::logic_error(
        "No result return from query for get_fence_sysvars()");
  }
}

void Instance::suppress_binary_log(bool flag) {
  if (flag) {
    if (m_sql_binlog_suppress_count == 0) execute("SET SESSION sql_log_bin=0");
    m_sql_binlog_suppress_count++;
  } else {
    if (m_sql_binlog_suppress_count <= 0)
      throw std::logic_error("mismatched call to suppress_binary_log()");
    m_sql_binlog_suppress_count--;
    if (m_sql_binlog_suppress_count == 0) execute("SET SESSION sql_log_bin=1");
  }
}

std::shared_ptr<mysqlshdk::db::IResult> Instance::query(const std::string &sql,
                                                        bool buffered) const {
  auto res = get_session()->query(sql, buffered);
  process_result_warnings(sql, *res);

  return res;
}

std::shared_ptr<mysqlshdk::db::IResult> Instance::query_udf(
    const std::string &sql, bool buffered) const {
  auto res = get_session()->query_udf(sql, buffered);
  process_result_warnings(sql, *res);

  return res;
}

void Instance::execute(const std::string &sql) const {
  try {
    get_session()->execute(sql);
  } catch (const mysqlshdk::db::Error &e) {
    throw mysqlshdk::db::Error((descr() + ": " + e.what()).c_str(), e.code(),
                               e.sqlstate());
  }
}

void Instance::process_result_warnings(const std::string &sql,
                                       mysqlshdk::db::IResult &result) const {
  // Call the Warnings_callback if registered
  if (!m_warnings_callback) return;

  // Get all the Warnings
  while (auto warning = result.fetch_one_warning()) {
    std::string warning_level;

    switch (warning->level) {
      case db::Warning::Level::Note:
        warning_level = "NOTE";
        break;
      case db::Warning::Level::Warn:
        warning_level = "WARNING";
        break;
      case db::Warning::Level::Error:
        warning_level = "ERROR";
        break;
    }

    m_warnings_callback(sql, warning->code, warning_level, warning->msg);

    warning = result.fetch_one_warning();
  }
}

std::string Instance::generate_uuid() const {
  // Generate a UUID on the MySQL server.
  std::string get_uuid_stmt = "SELECT UUID()";
  auto resultset = query(get_uuid_stmt);
  auto row = resultset->fetch_one();

  return row->get_string(0);
}

}  // namespace mysql
}  // namespace mysqlshdk
