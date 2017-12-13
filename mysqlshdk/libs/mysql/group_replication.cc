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
#include <memory>
#include <random>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <mysqld_error.h>

#include "mysql/group_replication.h"

#include "mysqlshdk/libs/utils/uuid_gen.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/logger.h"

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
    case Member_state::ONLINE:
      return "ONLINE";
    case Member_state::RECOVERING:
      return "RECOVERING";
    case Member_state::OFFLINE:
      return "OFFLINE";
    case Member_state::ERROR:
      return "ERROR";
    case Member_state::UNREACHABLE:
      return "UNREACHABLE";
    case Member_state::MISSING:
      return "(MISSING)";
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
Member_state to_member_state(const std::string &state) {
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
  else if (shcore::str_casecmp("(MISSING)", state.c_str()) == 0 ||
           shcore::str_casecmp("MISSING", state.c_str()) == 0)
    return Member_state::MISSING;
  else
    throw std::runtime_error("Unsupported member state value: " + state);
}

std::string to_string(const Member_role role) {
  switch (role) {
    case Member_role::PRIMARY:
      return "PRIMARY";
    case Member_role::SECONDARY:
      return "SECONDARY";
  }
  throw std::logic_error("invalid role");
}

Member_role to_member_role(const std::string &role) {
  if (shcore::str_casecmp("PRIMARY", role.c_str()) == 0) {
    return Member_role::PRIMARY;
  } else if (shcore::str_casecmp("SECONDARY", role.c_str()) == 0) {
    return Member_role::SECONDARY;
  } else {
    throw std::runtime_error("Unsupported GR member role value: " + role);
  }
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
 * Checks whether the given instance is a primary member of a group.
 *
 * Assumes that the instance is part of a InnoDB cluster.
 *
 * @param  instance the instance to be verified
 * @return          true if the instance is the primary of a single primary
 *                  group or if the group is multi-primary
 */
bool is_primary(const mysqlshdk::mysql::IInstance &instance) {
  try {
    std::shared_ptr<db::IResult> resultset = instance.get_session()->query(
        "SELECT NOT @@group_replication_single_primary_mode OR "
        "    (select variable_value"
        "       from performance_schema.global_status"
        "       where variable_name = 'group_replication_primary_member')"
        "    = @@server_uuid");
    auto row = resultset->fetch_one();
    return (row && row->get_int(0)) != 0;
  } catch (mysqlshdk::db::Error &e) {
    log_warning("Error checking if member is primary: %s (%i)", e.what(),
                e.code());
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      throw std::runtime_error(
          std::string("Group replication not started (MySQL error ") +
          std::to_string(e.code()) + ": " + e.what() + ")");
    } else {
      throw;
    }
  }
}

/**
 * Checks whether the group has enough ONLINE members for a quotum to be
 * reachable, from the point of view of the given instance. If the given
 * instance is not ONLINE itself, an exception will be thrown.
 *
 * @param  instance member instance object to be queried
 * @param  out_unreachable set to the number of unreachable members, unless null
 * @param  out_total set to total number of members, unless null
 * @return          true if quorum is possible, false otherwise
 */
bool has_quorum(const mysqlshdk::mysql::IInstance &instance,
                int *out_unreachable, int *out_total) {
  const char *q =
      "SELECT "
      "  CAST(SUM(IF(member_state = 'UNREACHABLE', 1, 0)) AS SIGNED) AS UNRCH,"
      "  COUNT(*) AS TOTAL,"
      "  (SELECT member_state"
      "      FROM performance_schema.replication_group_members"
      "      WHERE member_id = @@server_uuid) AS my_state"
      "  FROM performance_schema.replication_group_members";

  std::shared_ptr<db::IResult> resultset = instance.get_session()->query(q);
  auto row = resultset->fetch_one();
  if (!row) {
    throw std::runtime_error("Group replication query returned no results");
  }
  if (row->is_null(2) || row->get_string(2).empty()) {
    throw std::runtime_error("Target member appears to not be in a group");
  }
  if (row->get_string(2) != "ONLINE") {
    throw std::runtime_error("Target member is in state "+row->get_string(2));
  }
  int unreachable = row->get_int(0);
  int total = row->get_int(1);
  if (out_unreachable)
    *out_unreachable = unreachable;
  if (out_total)
    *out_total = total;
  return (total - unreachable) > total/2;
}

/**
 * Retrieve the current GR state for the specified server instance.
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A MemberState enum value with the GR state of the specified instance
 *         (i.e., ONLINE, RECOVERING, OFFLINE, ERROR, UNREACHABLE), including
 *         an additional state MISSING indicating that GR monitory information
 *         was not found for the instance (e.g., if GR is not enabled).
 *         For more information, see:
 *         https://dev.mysql.com/doc/refman/5.7/en/group-replication-server-states.html
 */
Member_state get_member_state(const mysqlshdk::mysql::IInstance &instance) {
  static const char *member_state_stmt =
      "SELECT member_state "
      "FROM performance_schema.replication_group_members "
      "WHERE member_id = @@server_uuid";
  auto session = instance.get_session();
  auto resultset = session->query(member_state_stmt);
  auto row = resultset->fetch_one();
  if (row) {
    return to_member_state(row->get_string(0));
  } else {
    return Member_state::MISSING;
  }
}

/**
 * Retrieve all the current members of the group.
 *
 * Note: each member information is returned as a GR_member object which
 * includes information about the member address with the format "<host>:<port>"
 * and its state (i.e., ONLINE, RECOVERING, OFFLINE, ERROR, UNREACHABLE,
 * MISSING).
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A list of GR_member objects corresponding to the current known
 *         members of the group (from the point of view of the specified
 *         instance).
 */
std::vector<Member> get_members(const mysqlshdk::mysql::IInstance &instance) {
  std::vector<Member> members;

  // 8.0.2 added a member_role column
  if (instance.get_version() >= utils::Version(8, 0, 2)) {
    std::shared_ptr<db::IResult> result(instance.get_session()->query(
        "SELECT member_id, member_state, member_host, member_port, member_role"
        " FROM performance_schema.replication_group_members"));
    {
      const db::IRow *row = result->fetch_one();
      if (!row || row->get_string(4).empty()) {
        // no members listed or empty role
        log_debug(
            "Query to replication_group_members from '%s' returned invalid "
            "data",
            instance.get_session()->get_connection_options().as_uri().c_str());
        throw std::runtime_error(
            "Instance does not seem to belong to any replication group");
      }
      while (row) {
        Member member;
        member.uuid = row->get_string(0);
        member.state = to_member_state(row->get_string(1));
        member.host = row->get_string(2);
        member.gr_port = row->get_int(3);
        member.role = to_member_role(row->get_string(4));
        members.push_back(member);

        row = result->fetch_one();
      }
    }
  } else {
    // query the old way
    std::shared_ptr<db::IResult> result(instance.get_session()->query(
        "SELECT member_id, member_state, member_host, member_port,"
        "   IF(NOT @@group_replication_single_primary_mode OR"
        "     member_id = (select variable_value"
        "       from performance_schema.global_status"
        "       where variable_name = 'group_replication_primary_member'),"
        "   'PRIMARY', 'SECONDARY') as member_role"
        " FROM performance_schema.replication_group_members"));
    {
      const db::IRow *row = result->fetch_one();
      while (row) {
        Member member;
        member.uuid = row->get_string(0);
        member.state = to_member_state(row->get_string(1));
        member.host = row->get_string(2);
        member.gr_port = row->get_int(3);
        members.push_back(member);

        row = result->fetch_one();
      }
    }
  }
  return members;
}

/**
 * Fetch various basic info bits from the group the given instance is member of
 *
 * This function will return false if the member is not part of a group, but
 * throw an exception if it cannot determine either way or an unexpected error
 * occurs.
 *
 * @param  instance                the instance to query info from
 * @param  out_member_state        set to the state of the member being queried
 * @param  out_member_id           set to the server_uuid of the member
 * @param  out_group_name          set to the name of the group of the member
 * @param  out_single_primary_mode set to true if group is single primary
 * @return                         false if the instance is not part of a group
 */
bool get_group_information(const mysqlshdk::mysql::IInstance &instance,
                           Member_state *out_member_state,
                           std::string *out_member_id,
                           std::string *out_group_name,
                           bool *out_single_primary_mode) {
  try {
    std::shared_ptr<db::IResult> result(instance.get_session()->query(
        "SELECT @@group_replication_group_name, "
        "   @@group_replication_single_primary_mode, "
        "   @@server_uuid, "
        "   member_state "
        " FROM performance_schema.replication_group_members"
        " WHERE member_id = @@server_uuid"));
    const db::IRow *row = result->fetch_one();
    if (row && !row->is_null(0)) {
      if (out_group_name)
        *out_group_name = row->get_string(0);
      if (!row->is_null(1) && out_single_primary_mode)
        *out_single_primary_mode = (row->get_int(1) != 0);
      if (out_member_id)
        *out_member_id = row->get_string(2);
      if (out_member_state)
        *out_member_state = to_member_state(row->get_string(3));
      return true;
    }
    return false;
  } catch (mysqlshdk::db::Error &e) {
    log_error("Error while querying for group_replication info: %s", e.what());
    if (e.code() == ER_BAD_DB_ERROR || e.code() == ER_NO_SUCH_TABLE ||
        e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      // if GR plugin is not installed, we get unknwon sysvar
      // if server doesn't have pfs, we get BAD_DB
      // if server has pfs but not the GR tables, we get NO_SUCH_TABLE
      return false;
    }
    throw;
  }
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
