/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <cassert>
#include <iomanip>
#include <limits>
#include <random>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "gr/group_replication.h"

#include "mysqlshdk/libs/utils/uuid_gen.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace {
const char *kErrorPluginDisabled =
    "Group Replication plugin is %s and "
    "cannot be enabled on runtime. Please "
    "enable the plugin and restart the server.";
const char *kErrorReadOnlyTimeout =
    "Timeout waiting for super_read_only to be "
    "unset after call to start Group "
    "Replication plugin.";
}  // namespace

namespace mysqlshdk {
namespace gr {

/**
 * Convert MemberState enumeration values to string.
 *
 * @param state MemberState value to convert to string.
 * @return string representing the MemberState value.
 */
std::string to_string(const Member_state state) {
  switch (state) {
    case Member_state ::ONLINE:
      return "ONLINE";
    case Member_state ::RECOVERING:
      return "RECOVERING";
    case Member_state ::OFFLINE:
      return "OFFLINE";
    case Member_state ::ERROR:
      return "ERROR";
    case Member_state ::UNREACHABLE:
      return "UNREACHABLE";
    case Member_state ::NOT_FOUND:
      return "NOT_FOUND";
    default:
      throw std::logic_error("Unexpected member state.");
  }
}

/**
 * Convert MemberState enumeration values to string.
 *
 * @param state MemberState value to convert to string.
 * @return string representing the MemberState value.
 */
Member_state to_member_state(const std::string state) {
  if (shcore::str_casecmp("ONLINE", state.c_str()) == 0)
    return Member_state::ONLINE;
  else if (shcore::str_casecmp("RECOVERING", state.c_str()) == 0)
    return Member_state::RECOVERING;
  else if (shcore::str_casecmp("OFFLINE", state.c_str()) == 0)
    return Member_state::OFFLINE;
  else if (shcore::str_casecmp("ERROR", state.c_str()) == 0)
    return Member_state::ERROR;
  else if (shcore::str_casecmp("UNREACHABLE", state.c_str()) == 0)
    return Member_state::UNREACHABLE;
  else if (shcore::str_casecmp("NOT_FOUND", state.c_str()) == 0)
    return Member_state::NOT_FOUND;
  else
    throw std::runtime_error("Unsupported member state value: " + state);
}

/**
 * Verify if the specified server instance is already a member of a GR group.
 *
 * NOTE: the instance might be a member of a GR group but not be active (e.g.,
 *       OFFLINE).
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A boolean value indicating if the specified instance already belongs
 *         to a GR group.
 */
bool is_member(const mysqlshdk::mysql::IInstance &instance) {
  std::string is_member_stmt =
      "SELECT group_name "
      "FROM performance_schema.replication_connection_status "
      "WHERE channel_name = 'group_replication_applier'";
  auto session = instance.get_session();
  auto resultset = session->query(is_member_stmt);
  auto row = resultset->fetch_one();
  if (row && !row->get_string(0).empty())
    return true;
  else
    return false;
}

/**
 * Verify if the specified server instance is already a member of the specified
 * GR group.
 *
 * NOTE: the instance might be a member of a GR group but not be active (e.g.,
 *       OFFLINE).
 *
 * @param instance session object to connect to the target instance.
 * @param group_name string with the name of the GR group to check.
 *
 * @return A boolean value indicating if the specified instance already belongs
 *          to the specified GR group.
 */
bool is_member(const mysqlshdk::mysql::IInstance &instance,
               const std::string &group_name) {
  std::string is_member_stmt_fmt =
      "SELECT group_name "
      "FROM performance_schema.replication_connection_status "
      "WHERE channel_name = 'group_replication_applier' AND group_name = ?";
  shcore::sqlstring is_member_stmt =
      shcore::sqlstring(is_member_stmt_fmt.c_str(), 0);
  is_member_stmt << group_name;
  is_member_stmt.done();
  auto session = instance.get_session();
  auto resultset = session->query(is_member_stmt);
  auto row = resultset->fetch_one();
  if (row)
    return true;
  else
    return false;
}

/**
 * Retrieve the current GR state for the specified server instance.
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A MemberState enum value with the GR state of the specified instance
 *         (i.e., ONLINE, RECOVERING, OFFLINE, ERROR, UNREACHABLE), including
 *         an additional state NOT_FOUND indicating that GR monitory information
 *         was not found for the instance (e.g., if GR is not enabled).
 *         For more information, see:
 *         https://dev.mysql.com/doc/refman/5.7/en/group-replication-server-states.html
 */
Member_state get_member_state(const mysqlshdk::mysql::IInstance &instance) {
  std::string member_state_stmt =
      "SELECT member_state "
      "FROM performance_schema.replication_group_members AS m "
      "JOIN performance_schema.replication_group_member_stats AS s "
      "ON m.member_id = s.member_id AND m.member_id = @@server_uuid";
  auto session = instance.get_session();
  auto resultset = session->query(member_state_stmt);
  auto row = resultset->fetch_one();
  if (row) {
    return to_member_state(row->get_string(0));
  } else {
    return Member_state::NOT_FOUND;
  }
}

/**
 * Retrieve all the current members of the group.
 *
 * Note: each member information is returned as a GR_member object which
 * includes information about the member address with the format "<host>:<port>"
 * and its state (i.e., ONLINE, RECOVERING, OFFLINE, ERROR, UNREACHABLE,
 * NOT_FOUND).
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A list of GR_member objects corresponding to the current known
 *         members of the group (from the point of view of the specified
 *         instance).
 */
std::vector<Member> get_members(
    const mysqlshdk::mysql::IInstance & /*instance*/) {
  // TODO(pjesus)
  assert(0);
  return {};
}

/**
 * Check if the Group Replication plugin is installed, and if not try to
 * install it. The option file with the instance configuration is updated
 * accordingly if provided.
 *
 * @param instance session object to connect to the target instance.
 *
 * @throw std::runtime_error if the GR plugin is DISABLED (cannot be installed
 *        online) or if an error occurs installing the GR plugin.
 *
 * @return A boolean value indicating if the GR plugin was installed (true) or
 *         if it is already installed and ACTIVE (false).
 */
bool install_plugin(const mysqlshdk::mysql::IInstance &instance) {
  // Get GR plugin state.
  utils::nullable<std::string> plugin_state =
      instance.get_plugin_status(kPluginName);

  // Install the GR plugin if no state info is available (not installed).
  bool res = false;
  if (plugin_state.is_null()) {
    instance.install_plugin(kPluginName);
    res = true;

    // Check the GR plugin state after installation;
    plugin_state = instance.get_plugin_status(kPluginName);
  } else if ((*plugin_state).compare(kPluginActive) == 0) {
    // GR plugin is already active.
    return false;
  }

  if (!plugin_state.is_null()) {
    // Raise an exception if the plugin is not active (disabled, deleted or
    // inactive), cannot be enabled online.
    if ((*plugin_state).compare(kPluginActive) != 0)
      throw std::runtime_error(
          shcore::str_format(kErrorPluginDisabled, plugin_state->c_str()));
  }
  // TODO(pjesus): Handle changes to the configuration file if provided.
  // Requires the new Config File library to be implemented.

  // GR Plugin installed and not disabled (active).
  return res;
}

/**
 * Check if the Group Replication plugin is installed and uninstall it if
 * needed. The option file with the instance configuration is updated
 * accordingly if provided.
 *
 * @param instance session object to connect to the target instance.
 *
 * @throw std::runtime_error if an error occurs uninstalling the GR plugin.
 *
 * @return A boolean value indicating if the GR plugin was uninstalled (true)
 *         or if it is already not available (false).
 */
bool uninstall_plugin(const mysqlshdk::mysql::IInstance &instance) {
  // Get GR plugin state.
  utils::nullable<std::string> plugin_state =
      instance.get_plugin_status(kPluginName);

  if (!plugin_state.is_null()) {
    // Uninstall the GR plugin if state info is available (installed).
    instance.uninstall_plugin(kPluginName);
    return true;
  } else {
    // GR plugin is not installed.
    return false;
  }
  // TODO(pjesus): Handle changes to the configuration file if provided.
  // Requires the new Config File library to be implemented.
}

/**
 * Retrieve all Group Replication configurations (variables) for the target
 * server instance.
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A Configuration object with all the current GR configurations
 *         (variables) and their respective value.
 */
// NOTE: Requires the Configuration library.
// Configuration get_configurations(
//     const mysqlshdk::mysql::IInstance &instance) {}

/**
 * Update the given GR configurations (variables) on the target server
 * instance.
 *
 * @param instance session object to connect to the target instance.
 * @param configs Configuration object with the configurations (variables) and
 *                their respective (new) value to set on the instance.
 * @param persist boolean value indicating if the given configurations
 *                (variables) will be persisted, using the new SET PERSIST
 *                feature (only available for MySQL 8 server instances).
 */
// NOTE: Requires the Configuration library.
// void update_configurations(const mysqlshdk::mysql::IInstance &instance,
//                           const Configuration &configs,
//                           const bool persist) {}

/**
 * Execute the CHANGE MASTER statement to configure Group Replication.
 *
 * @param instance session object to connect to the target instance.
 * @param rpl_user string with the username that will be used for replication.
 *                 Note: this user should already exist with the proper
 *                       privileges.
 * @param rpl_pwd string with the password for the replication user.
 */
void do_change_master(const mysqlshdk::mysql::IInstance & /*instance*/,
                      const std::string & /*rpl_user*/,
                      const std::string & /*rpl_pwd*/) {
  // TODO(pjesus)
  assert(0);
}

/**
 * Start the Group Replication.
 *
 * If the serve is indicated to be the bootstrap member, this function
 * will wait for SUPER READ ONLY to be disabled (i.e., server to be ready
 * to accept write transactions).
 *
 * Note: All Group Replication configurations must be properly set before
 *       using this function.
 *
 * @param instance session object to connect to the target instance.
 * @param bootstrap boolean value indicating if the operation is executed for
 *                  the first member, to bootstrap the group.
 *                  Note: If bootstrap = true, the proper GR bootstrap setting
 *                  is set and unset, respectively at the begin and end of
 *                  the operation, and the function wait for SUPER READ ONLY
 *                  to be unset on the instance.
 * @param read_only_timeout integer value with the timeout value in seconds
 *                          to wait for SUPER READ ONLY to be disabled
 *                          for a bootstrap server. By default the value is
 *                          900 seconds (i.e., 15 minutes).
 *
 * @throw std::runtime_error if SUPER READ ONLY is still enabled for the
 *        bootstrap server after the given 'read_only_timeout'.
 */
void start_group_replication(const mysqlshdk::mysql::IInstance &instance,
                             const bool bootstrap,
                             const uint16_t read_only_timeout) {
  if (bootstrap)
    instance.set_sysvar("group_replication_bootstrap_group", true,
                        mysqlshdk::mysql::VarScope::GLOBAL);
  try {
    auto session = instance.get_session();
    session->execute("START GROUP_REPLICATION");
  } catch (std::exception &err) {
    // Try to set the group_replication_bootstrap_group to OFF if the
    // statement to start GR failed and only then throw the error.
    try {
      if (bootstrap)
        instance.set_sysvar("group_replication_bootstrap_group", false,
                            mysqlshdk::mysql::VarScope::GLOBAL);
      // Ignore any error trying to set the bootstrap variable to OFF.
    } catch (...) {
    }
    throw;
  }
  if (bootstrap) {
    instance.set_sysvar("group_replication_bootstrap_group", false,
                        mysqlshdk::mysql::VarScope::GLOBAL);
    // Wait for SUPER READ ONLY to be OFF.
    // Required for MySQL versions < 5.7.20.
    mysqlshdk::utils::nullable<bool> read_only = instance.get_sysvar_bool(
        "super_read_only", mysqlshdk::mysql::VarScope::GLOBAL);
    uint16_t waiting_time = 0;
    while (*read_only && waiting_time < read_only_timeout) {
      shcore::sleep_ms(1000);
      waiting_time += 1;
      read_only = instance.get_sysvar_bool("super_read_only",
                                           mysqlshdk::mysql::VarScope::GLOBAL);
    }
    // Throw an error is SUPPER READ ONLY is ON.
    if (*read_only)
      throw std::runtime_error(kErrorReadOnlyTimeout);
  }
}

/**
 * Stop the Group Replication.
 *
 * @param instance session object to connect to the target instance.
 */
void stop_group_replication(const mysqlshdk::mysql::IInstance &instance) {
  auto session = instance.get_session();
  session->execute("STOP GROUP_REPLICATION");
}

/**
 * Generate a UUID to use for the group name.
 *
 * The UUID generated is a string representation of 5 hexadecimal numbers
 * with the format aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee.
 *
 * @return A string with a new UUID to be used for the group name.
 */
std::string generate_group_name() {
  // Generate a UUID.
  return get_string_uuid();
}

/**
 * Check the specified replication user, namely if it has the required
 * privileges to use for Group Replication.
 *
 * @param instance session object to connect to the target instance.
 * @param user string with the username that will be used.
 * @param host string with the host part for the user account. If none is
 *             provide (empty) then use '%'.
 *
 * @return A tuple containing the result of the check operation including
 *         the details about the requisites that were not meet (e.g., privilege
 *         missing). The return tuple has three elements: first, boolean value
 *         indicating if the user exists (true) or not (false); second, string
 *         with a comma separated list of the missing privileges; third,
 *         boolean value indicating if the user has GRANT OPTION (true) or not
 *         (false).
 */
std::tuple<bool, std::string, bool> check_replication_user(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host) {
  // Check if user has REPLICATION SLAVE on *.*
  std::vector<std::string> gr_grants{"REPLICATION SLAVE"};
  return instance.check_user(user, host, gr_grants, "*", "*");
}

/**
 * Create a replication (recovery) user with the required privileges for
 * Group Replication.
 *
 * @param instance session object to connect to the target instance.
 * @param user string with the username that will be used.
 * @param host string with the host part for the user account. If none is
 *             provide (empty) then use '%'.
 * @param pwd string with the password for the user.
 */
void create_replication_user(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &user, const std::string &host,
                             const std::string &pwd) {
  // Create the user with the required privileges: REPLICATION SLAVE on *.*.
  std::vector<std::tuple<std::string, std::string, bool>> gr_grants = {
      std::make_tuple("REPLICATION SLAVE", "*.*", false)};
  instance.create_user(user, host, pwd, gr_grants);
}

/**
 * Check the compliance of the current data to use Group Replication.
 * More specifically, verify if all database tables use the InnoDB engine
 * and have a primary key.
 *
 * @param instance session object to connect to the target instance.
 * @param max_errors non negative integer [0, 65535] with the maximum number of
 *                   compliance errors to return, once this value is reached
 *                   the check is stopped. By default, the value is 0, meaning
 *                   that no maximum limit will be used.
 *
 * @return A map containing the result of the check operation including
 *         the details about the data compliance that were not meet.
 */
std::map<std::string, std::string> check_data_compliance(
    const mysqlshdk::mysql::IInstance & /*instance*/,
    const uint16_t /*max_errors*/) {
  // TODO(pjesus)
  // NOTE: Consider returning vector of tuples, like
  // std::tuple<std::string, IssueType> where IssueType is an enum
  assert(0);
  return {};
}

/**
 * Check the server system variables requirements in order to be able to
 * use Group Replication.
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A map containing the result of the check operation including
 *         the details about the server variables that do not meet the needed
 *         requirements to use GR.
 */
std::map<std::string, std::string> check_server_variables(
    const mysqlshdk::mysql::IInstance & /*instance*/) {
  // TODO(pjesus)
  // NOTE: Review the type of object to return.
  assert(0);
  return {};
}

}  // namespace gr
}  // namespace mysqlshdk
