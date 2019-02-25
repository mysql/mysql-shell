/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/mysql/group_replication.h"

#include <cassert>
#include <iomanip>
#include <limits>
#include <memory>
#include <set>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <mysqld_error.h>

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/uuid_gen.h"

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
 * Convert string to MemberState enumeration value.
 *
 * @param state String to convert to a MemberState value.
 * @return MemberState value resulting from the string conversion.
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
           shcore::str_casecmp("MISSING", state.c_str()) == 0 || state.empty())
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

std::string to_string(const Topology_mode mode) {
  switch (mode) {
    case Topology_mode::SINGLE_PRIMARY:
      return "Single-Primary";
    case Topology_mode::MULTI_PRIMARY:
      return "Multi-Primary";
    default:
      throw std::logic_error("Unexpected Group Replication mode.");
  }
}

Topology_mode to_topology_mode(const std::string &mode) {
  if (shcore::str_casecmp("Single-Primary", mode) == 0)
    return Topology_mode::SINGLE_PRIMARY;
  else if (shcore::str_casecmp("Multi-Primary", mode) == 0)
    return Topology_mode::MULTI_PRIMARY;
  else
    throw std::runtime_error("Unsupported Group Replication mode: " + mode);
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
    std::string err_msg = "Target member is in state " + row->get_string(2);
    if (is_running_gr_auto_rejoin(instance))
      err_msg += " (running auto-rejoin)";
    throw std::runtime_error(err_msg);
  }
  int unreachable = row->get_int(0);
  int total = row->get_int(1);
  if (out_unreachable) *out_unreachable = unreachable;
  if (out_total) *out_total = total;
  return (total - unreachable) > total / 2;
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
 * @param out_single_primary_mode if not NULL, assigned to true if group is
 *        single primary
 *
 * @return A list of GR_member objects corresponding to the current known
 *         members of the group (from the point of view of the specified
 *         instance).
 */
std::vector<Member> get_members(const mysqlshdk::mysql::IInstance &instance,
                                bool *out_single_primary_mode) {
  std::vector<Member> members;

  // 8.0.2 added member_role and member_version columns
  if (instance.get_version() >= utils::Version(8, 0, 2)) {
    std::shared_ptr<db::IResult> result(instance.get_session()->query(
        "SELECT member_id, member_state, member_host, member_port, "
        "member_role, member_version, @@group_replication_single_primary_mode "
        "FROM performance_schema.replication_group_members"));
    {
      const db::IRow *row = result->fetch_one();
      if (!row || row->get_string(4).empty()) {
        // no members listed or empty role
        log_debug(
            "Query to replication_group_members from '%s' returned invalid "
            "data",
            instance.get_session()->get_connection_options().as_uri().c_str());
        if (mysqlshdk::gr::is_running_gr_auto_rejoin(instance))
          throw std::runtime_error(
              "Instance '" + instance.get_connection_options().uri_endpoint() +
              "' does not seem to belong to any replication group but "
              "is currently running auto-rejoin.");
        else
          throw std::runtime_error(
              "Instance '" + instance.get_connection_options().uri_endpoint() +
              "' does not seem to belong to any replication group");
      }
      while (row) {
        Member member;
        member.uuid = row->get_string(0);
        member.state = to_member_state(row->get_string(1));
        member.host = row->get_string(2);
        member.gr_port = row->get_int(3);
        member.role = to_member_role(row->get_string(4));
        member.version = row->get_string(5);
        if (out_single_primary_mode) *out_single_primary_mode = row->get_int(6);
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
        "   'PRIMARY', 'SECONDARY') as member_role,"
        "    @@group_replication_single_primary_mode"
        " FROM performance_schema.replication_group_members"));
    {
      const db::IRow *row = result->fetch_one();
      while (row) {
        Member member;
        member.uuid = row->get_string(0);
        member.state = to_member_state(row->get_string(1));
        member.host = row->get_string(2);
        member.gr_port = row->get_int(3);
        member.role = to_member_role(row->get_string(4));
        if (out_single_primary_mode) *out_single_primary_mode = row->get_int(5);
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
      if (out_group_name) *out_group_name = row->get_string(0);
      if (!row->is_null(1) && out_single_primary_mode)
        *out_single_primary_mode = (row->get_int(1) != 0);
      if (out_member_id) *out_member_id = row->get_string(2);
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

std::string get_group_primary_uuid(const std::shared_ptr<db::ISession> &session,
                                   bool *out_single_primary_mode) {
  auto res = session->query(
      "SELECT @@group_replication_single_primary_mode, "
      "       variable_value AS primary_uuid"
      "   FROM performance_schema.global_status"
      "   WHERE variable_name = 'group_replication_primary_member'");
  const db::IRow *row = res->fetch_one();
  if (row) {
    if (out_single_primary_mode) *out_single_primary_mode = row->get_int(0);
    if (row->is_null(1)) return "";
    return row->get_string(1);
  }
  throw std::logic_error("GR status query returned no rows");
}

mysqlshdk::utils::Version get_group_protocol_version(
    const mysqlshdk::mysql::IInstance &instance) {
  // MySQL versions in the domain [5.7.14, 8.0.15] map to GCS protocol version
  // 1 (5.7.14) so we can return it immediately and avoid an error when
  // executing the UDF to obtain the protocol version since the UDF is only
  // available for versions >= 8.0.16
  if (instance.get_version() < mysqlshdk::utils::Version(8, 0, 16)) {
    return mysqlshdk::utils::Version(5, 7, 14);
  } else {
    try {
      static const char *get_gr_protocol_version =
          "SELECT group_replication_get_communication_protocol()";

      log_debug("Executing UDF: %s", get_gr_protocol_version);

      auto session = instance.get_session();
      auto resultset = session->query(get_gr_protocol_version);
      auto row = resultset->fetch_one();

      if (row) {
        return (mysqlshdk::utils::Version(row->get_string(0)));
      } else {
        throw std::logic_error(
            "No rows returned when querying the version of Group Replication "
            "communication protocol.");
      }
    } catch (mysqlshdk::db::Error &error) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
    }
  }
}

void set_group_protocol_version(const mysqlshdk::mysql::IInstance &instance,
                                mysqlshdk::utils::Version version) {
  std::string query;

  shcore::sqlstring query_fmt(
      "SELECT group_replication_set_communication_protocol(?)", 0);
  query_fmt << version.get_full();
  query_fmt.done();
  query = query_fmt.str();

  try {
    log_debug("Executing UDF: %s", query.c_str());

    instance.get_session()->query(query);
  } catch (mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

bool is_protocol_downgrade_required(
    mysqlshdk::utils::Version current_group_version,
    const mysqlshdk::mysql::IInstance &instance) {
  if (current_group_version >= mysqlshdk::utils::Version(8, 0, 16)) {
    if (instance.get_version() < current_group_version) {
      log_debug(
          "Group Replication protocol version downgrade required (to "
          "instance version: %s)",
          instance.get_version().get_full().c_str());
      return true;
    }
  }
  return false;
}

bool is_protocol_upgrade_required(
    const mysqlshdk::mysql::IInstance &instance,
    mysqlshdk::utils::nullable<std::string> server_uuid,
    mysqlshdk::utils::Version *out_protocol_version) {
  std::vector<Member> group_members = get_members(instance);

  bool upgrade_required = false;

  // Get the current protocol version in use by the group
  mysqlshdk::utils::Version protocol_version_group =
      get_group_protocol_version(instance);

  // Check if any of the group_members has a version >= 8.0.16
  for (const auto &member : group_members) {
    // If version is not available, the instance is < 8.0, so an upgrade is not
    // required.
    if (member.version.empty()) {
      return false;
    }

    // If the instance is leaving the cluster we must skip checking it against
    // the cluster
    if (!server_uuid.is_null() && (*server_uuid == member.uuid)) {
      continue;
    } else {
      // If the instance is >= 8.0.16, then check if
      // protocol version in use is lower than than the instance version If it
      // is, the protocol version must be upgraded
      mysqlshdk::utils::Version ver(member.version);

      if (ver >= mysqlshdk::utils::Version(8, 0, 16) &&
          protocol_version_group < ver) {
        upgrade_required = true;

        if (*out_protocol_version == mysqlshdk::utils::Version() ||
            *out_protocol_version > ver) {
          *out_protocol_version = ver;
        }
      } else {
        return false;
      }
    }
  }

  if (upgrade_required) {
    log_debug(
        "Group Replication protocol version upgrade required (to "
        "version: "
        "%s)",
        out_protocol_version->get_full().c_str());
  }

  return upgrade_required;
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
bool install_plugin(const mysqlshdk::mysql::IInstance &instance,
                    mysqlshdk::config::Config *config) {
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

  if (!plugin_state.is_null() &&
      (*plugin_state).compare(kPluginDisabled) == 0) {
    // If the plugin is disabled then try to activate and install it, but it
    // can only be done if the option file is available.
    if (config &&
        config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      // Enable the GR plugin on the configuration file.
      auto cfg_file_handler =
          dynamic_cast<mysqlshdk::config::Config_file_handler *>(
              config->get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
      auto gr_plugin_status = cfg_file_handler->get_string(kPluginName);
      cfg_file_handler->set_now(kPluginName,
                                utils::nullable<std::string>("ON"));

      try {
        // Uninstall the GR plugin so it can be installed again after being
        // enabled on the option file.
        instance.uninstall_plugin(kPluginName);

        // Reinstall the GR plugin.
        instance.install_plugin(kPluginName);

        // Check the GR plugin state after the attempting to activate it;
        plugin_state = instance.get_plugin_status(kPluginName);
      } catch (const std::exception &err) {
        // restore previous value on configuration file
        cfg_file_handler->set_now(kPluginName, gr_plugin_status);
        throw;
      }
    }
  }

  if (!plugin_state.is_null()) {
    // Raise an exception if the plugin is not active (disabled, deleted or
    // inactive), cannot be enabled online.
    if ((*plugin_state).compare(kPluginActive) != 0)
      throw std::runtime_error(
          shcore::str_format(kErrorPluginDisabled, plugin_state->c_str()));
  }

  // GR Plugin installed and not disabled (active).
  return res;
}

/**
 * Check if the Group Replication plugin is installed and uninstall it if
 * needed. The option file with the instance configuration is updated
 * accordingly if provided (plugin disabled).
 *
 * @param instance session object to connect to the target instance.
 *
 * @throw std::runtime_error if an error occurs uninstalling the GR plugin.
 *
 * @return A boolean value indicating if the GR plugin was uninstalled (true)
 *         or if it is already not available (false).
 */
bool uninstall_plugin(const mysqlshdk::mysql::IInstance &instance,
                      mysqlshdk::config::Config *config) {
  // Get GR plugin state.
  utils::nullable<std::string> plugin_state =
      instance.get_plugin_status(kPluginName);

  if (!plugin_state.is_null()) {
    // Uninstall the GR plugin if state info is available (installed).
    instance.uninstall_plugin(kPluginName);

    // If the config file handler is available disable the plugin.
    if (config &&
        config->has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      mysqlshdk::config::Config_file_handler *cfg_file_handler =
          static_cast<mysqlshdk::config::Config_file_handler *>(
              config->get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
      cfg_file_handler->set_now(kPluginName,
                                utils::nullable<std::string>("OFF"));
    }
    return true;
  } else {
    // GR plugin is not installed.
    return false;
  }
}

/**
 * Retrieve all Group Replication configurations (variables) for the target
 * server instance.
 *
 * @param instance session object to connect to the target instance.
 *
 * @return A map with all the current GR configurations (variables) and their
 *         respective values.
 */
std::map<std::string, utils::nullable<std::string>> get_all_configurations(
    const mysqlshdk::mysql::IInstance &instance) {
  // Get all GR system variables.
  std::map<std::string, utils::nullable<std::string>> gr_vars =
      instance.get_system_variables_like(
          "group_replication_%", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get all auto_increment variables.
  std::map<std::string, utils::nullable<std::string>> auto_inc_vars =
      instance.get_system_variables_like(
          "auto_increment_%", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Merge variable and return the result.
  gr_vars.insert(auto_inc_vars.begin(), auto_inc_vars.end());
  return gr_vars;
}

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
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  try {
    auto session = instance.get_session();
    session->execute("START GROUP_REPLICATION");
  } catch (std::exception &err) {
    // Try to set the group_replication_bootstrap_group to OFF if the
    // statement to start GR failed and only then throw the error.
    try {
      if (bootstrap)
        instance.set_sysvar("group_replication_bootstrap_group", false,
                            mysqlshdk::mysql::Var_qualifier::GLOBAL);
      // Ignore any error trying to set the bootstrap variable to OFF.
    } catch (...) {
    }
    throw;
  }
  if (bootstrap) {
    instance.set_sysvar("group_replication_bootstrap_group", false,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
    // Wait for SUPER READ ONLY to be OFF.
    // Required for MySQL versions < 5.7.20.
    mysqlshdk::utils::nullable<bool> read_only = instance.get_sysvar_bool(
        "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);
    uint16_t waiting_time = 0;
    while (*read_only && waiting_time < read_only_timeout) {
      shcore::sleep_ms(1000);
      waiting_time += 1;
      read_only = instance.get_sysvar_bool(
          "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }
    // Throw an error is SUPPER READ ONLY is ON.
    if (*read_only) throw std::runtime_error(kErrorReadOnlyTimeout);
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
 *             provide (empty) then use '%' and localhost.
 *
 * @return A User_privileges_result instance containing the result of the check
 *         operation including the details about the requisites that were not
 *         met (e.g., privilege missing).
 */
mysql::User_privileges_result check_replication_user(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host) {
  // Check if user has REPLICATION SLAVE on *.*
  static const std::set<std::string> gr_grants{"REPLICATION SLAVE"};
  return instance.get_user_privileges(user, host)->validate(gr_grants);
}

void create_replication_random_user_pass(
    const mysqlshdk::mysql::IInstance &instance, std::string *out_user,
    const std::vector<std::string> &hosts, std::string *out_pwd) {
  std::string tstamp = mysqlshdk::utils::Random::get()->get_time_string();
  // make sure the tstamp size is always 10 digits
  if (tstamp.size() <= 10) {
    tstamp = shcore::str_rjust(tstamp, 10, '0');
  } else {
    tstamp = tstamp.substr(tstamp.size() - 10);
  }

  // Generate the username string
  std::string base_user = "mysql_innodb_cluster_rplusr";
  *out_user = base_user.substr(0, 32 - tstamp.size()) + tstamp;

  // If the hostname is '%', we must create the user in 'localhost' too
  // otherwise the user may be treated as an anonymous user:
  // https://dev.mysql.com/doc/refman/en/adding-users.html
  static const std::vector<std::string> default_hosts{"%", "localhost"};
  create_replication_user_random_pass(
      instance, *out_user, hosts.empty() ? default_hosts : hosts, out_pwd);
}

/**
 * Create a replication (recovery) user with the required privileges for
 * Group Replication and a randomly generated password.
 *
 * @param instance session object to connect to the target instance.
 * @param user string with the username that will be used.
 * @param hosts list of strings with the host part for the user account. If none
 * is provide (empty) then use '%'.
 * @param out_pwd generated password is assigned
 */
void create_replication_user_random_pass(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts, std::string *out_pwd) {
  // Create the user with the required privileges: REPLICATION SLAVE on *.*.
  static std::vector<std::tuple<std::string, std::string, bool>> gr_grants = {
      std::make_tuple("REPLICATION SLAVE", "*.*", false)};

  // F5. If 'ipWhitelist' is not used on the "ipWhitelist commands", thus
  // getting the default value of 'AUTOMATIC', then the replication-user is
  // created using the wildcard '%' for the host name value.

  mysql::create_user_with_random_password(
      instance.get_session(), user,
      hosts.empty() ? std::vector<std::string>({"%"}) : hosts, gr_grants,
      out_pwd);
}

/**
 * Get the replication user used for recovery.
 *
 * This function returns the replication user used in the (last)
 * CHANGE MASTER TO statement FOR CHANNEL 'group_replication_recovery'.
 *
 * NOTE: The correct execution of this function requires the variable
 *       master_info_repository=TABLE to be set which is a requirement for
 *       Group Replication.
 *
 * @param instance instance object of target member to obtain the
 *                 replication user.
 * @return a string with the replication (recovery) user set fot the specified
 *         instance. Note: If no replication user was specified an empty string
 *         is returned.
 */
std::string get_recovery_user(const mysqlshdk::mysql::IInstance &instance) {
  std::string rpl_user;
  std::shared_ptr<db::IResult> result(instance.get_session()->query(
      "SELECT User_name FROM mysql.slave_master_info "
      "WHERE Channel_name = 'group_replication_recovery'"));
  auto row = result->fetch_one();
  if (row) rpl_user = row->get_string(0);
  return rpl_user;
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
 * Auxiliary function that is used to validate a given invalid config against
 * a given handler and list of valid_values
 * @param values a vector with the values to the variable with
 * @param allowed_values if true, the values list is considered the list of
 *        allowed values, if false the list of forbidden values.
 * @param handler handler that is going to be used for the validation
 * @param change Invalid config struct with the name and expected values already
 *        initialized.
 * @param change_type type of change to be used for the invalid_config
 * @param restart boolean value that is true if the variable requires a restart
 *        to change and false otherwise.
 * @param set_cur_val If true, the current_val field of the invalid config
 *        will be set with the value read from the handler if it isn't
 *        initialized yet.
 *        Note: This is necessary so that the invalid config has a value on the
 *        current value field for logging even if the value is the one expected.
 */
void check_variable_compliance(const std::vector<std::string> &values,
                               bool allowed_values,
                               const config::IConfig_handler &handler,
                               Invalid_config *change, Config_type change_type,
                               bool restart, bool set_cur_val) {
  std::string value;
  // convert value to a string that we can lookup in the valid_values vector
  try {
    auto nullable_value = handler.get_string(change->var_name);
    if (nullable_value.is_null())
      value = k_no_value;
    else
      value = shcore::str_upper(*nullable_value);
  } catch (const std::out_of_range &err) {
    // variable is not defined
    value = k_value_not_set;
  }
  if (set_cur_val && change->current_val == k_must_be_initialized)
    change->current_val = value;
  // If the value is not on the list of allowed values or if it is on the list
  // of forbidden values, then the configuration is not valid.
  auto found_it = std::find(values.begin(), values.end(), value);
  if ((found_it == values.end() && allowed_values) ||
      (found_it != values.end() && !allowed_values)) {
    change->current_val = value;
    change->types.set(change_type);
    change->restart = restart;
  }
}

/**
 * Auxiliary function that does the logging of an invalid config.
 * @param change The Invalid config object
 */
void log_invalid_config(const Invalid_config &change) {
  if (change.types.empty()) {
    log_debug("OK: '%s' value '%s' is compatible with InnoDB Cluster.",
              change.var_name.c_str(), change.current_val.c_str());
  } else {
    log_debug(
        "FAIL: '%s' value '%s' is not compatible with InnoDB Cluster. "
        "Required value: '%s'.",
        change.var_name.c_str(), change.current_val.c_str(),
        change.required_val.c_str());
  }
}

void check_server_variables_compatibility(
    const mysqlshdk::config::Config &config,
    std::vector<Invalid_config> *out_invalid_vec) {
  // create a vector for all the variables required values. Each entry is
  // a string with the name of the variable, then a vector of strings with the
  // accepted values and finally a boolean value that says if the variable
  // requires a restart to change or not.
  // NOTE: The order of the variables in the vector is important since it
  // is used by the configure operation to set the correct variable values and
  // as such it serves as a workaround for BUG#27629719, which requires
  // some GR required variables to be set in a certain order, namely
  // enforce_gtid_consistency before gtid_mode.
  std::vector<std::tuple<std::string, std::vector<std::string>, bool>>
      requirements{std::make_tuple("binlog_format",
                                   std::vector<std::string>{"ROW"}, false),
                   std::make_tuple("binlog_checksum",
                                   std::vector<std::string>{"NONE"}, false),
                   std::make_tuple("log_slave_updates",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("enforce_gtid_consistency",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("gtid_mode",
                                   std::vector<std::string>{"ON", "1"}, true),
                   std::make_tuple("master_info_repository",
                                   std::vector<std::string>{"TABLE"}, true),
                   std::make_tuple("relay_log_info_repository",
                                   std::vector<std::string>{"TABLE"}, true),
                   std::make_tuple("transaction_write_set_extraction",
                                   std::vector<std::string>{"XXHASH64", "2",
                                                            "MURMUR32", "1"},
                                   true)};

  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    // Add an extra requirement for the report_port
    std::string report_port = *(
        config.get_string("port", mysqlshdk::config::k_dft_cfg_server_handler));
    std::vector<std::string> report_port_vec = {report_port};
    requirements.emplace_back("report_port", report_port_vec, false);

    // Check if MTS is enabled (slave_parallel_workers > 0) and if so, add
    // extra requirements.
    utils::nullable<int64_t> slave_p_workers = config.get_int(
        "slave_parallel_workers", mysqlshdk::config::k_dft_cfg_server_handler);
    if (!slave_p_workers.is_null() && *slave_p_workers > 0) {
      std::vector<std::string> slave_parallel_vec = {"LOGICAL_CLOCK"};
      std::vector<std::string> slave_commit_vec = {"ON", "1"};
      requirements.emplace_back("slave_parallel_type", slave_parallel_vec,
                                false);
      requirements.emplace_back("slave_preserve_commit_order", slave_commit_vec,
                                false);
    }
  }

  for (auto &req : requirements) {
    std::string var_name;
    std::vector<std::string> valid_values;
    bool restart;
    std::tie(var_name, valid_values, restart) = req;
    log_debug("Checking if '%s' is compatible with InnoDB Cluster.",
              var_name.c_str());
    // assuming the expected value is the first of the valid values list
    Invalid_config change = Invalid_config(var_name, valid_values.at(0));
    // If config object has has a config handler
    if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
      check_variable_compliance(
          valid_values, true,
          *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
          &change, Config_type::CONFIG, false, true);
    }

    // If config object has has a server handler
    if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
      check_variable_compliance(
          valid_values, true,
          *config.get_handler(mysqlshdk::config::k_dft_cfg_server_handler),
          &change, Config_type::SERVER, restart, true);
    }

    log_invalid_config(change);
    // if there are any changes to be made, add them to the vector of changes
    if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
  }
}

void check_server_id_compatibility(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::config::Config &config,
    std::vector<Invalid_config> *out_invalid_vec) {
  // initialize change object with default values for this specific variable
  Invalid_config change = Invalid_config("server_id", "<unique ID>");

  log_debug("Checking if 'server_id' is compatible with InnoDB Cluster.");
  // If config object has has a config handler
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
    std::vector<std::string> forbidden_values{"0", k_no_value, k_value_not_set};
    check_variable_compliance(
        forbidden_values, false,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler), &change,
        Config_type::CONFIG, false, true);
  }

  // The test for the server_id on the server_handler is special since
  // the 1 value can be both allowed or not depending on being the default
  // value or not. As such we will not make use of the check_variable_compliance
  // function.
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    auto server_id = config.get_int(
        "server_id", mysqlshdk::config::k_dft_cfg_server_handler);
    // server id cannot be null on the server, so we can read its value without
    // any problems
    if (*server_id == 0) {
      // if server_id is 0, then it is not valid for gr usage and must be
      // changed
      change.current_val = "0";
      change.types.set(Config_type::SERVER);
      change.restart = true;
      change.val_type = shcore::Value_type::Integer;
    } else if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 3) &&
               instance.has_variable_compiled_value("server_id")) {
      // Starting from MySQL 8.0.3, server_id = 1 by default (to enable
      // binary logging). For this versions we check if the default value was
      // changed by the user. Otherwise server_id is 0 (not set) by default
      // for previous server versions (it cannot be 0 for any version).
      change.current_val = std::to_string(*server_id);
      change.types.set(Config_type::SERVER);
      change.restart = true;
      change.val_type = shcore::Value_type::Integer;
    } else {
      // If no invalid config was found, store the current variable value to
      // be used for the debug message.
      if (change.types.empty()) change.current_val = std::to_string(*server_id);
    }
  }
  // if there are any changes to be made, add them to the vector of changes
  log_invalid_config(change);
  if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
}

void check_log_bin_compatibility(const mysqlshdk::mysql::IInstance &instance,
                                 const mysqlshdk::config::Config &config,
                                 std::vector<Invalid_config> *out_invalid_vec) {
  log_debug("Checking if 'log_bin' is compatible with InnoDB Cluster.");
  // If config object has has a config handler
  if (config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
    // On MySQL 8.0.3 the binary log is enabled by default, so there is no need
    // to add the log_bin option to the config file. However on 5.7 there is.
    if (instance.get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
      Invalid_config change = Invalid_config("log_bin", k_no_value);
      std::vector<std::string> forbidden_values{k_value_not_set};
      check_variable_compliance(
          forbidden_values, false,
          *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
          &change, Config_type::CONFIG, false, true);
      // if there are any changes to be made, add them to the vector of changes
      if (!change.types.empty()) {
        log_debug(
            "FAIL: '%s' value '%s' is not compatible with InnoDB Cluster. "
            "Required value: '%s'.",
            change.var_name.c_str(), change.current_val.c_str(),
            change.required_val.c_str());
        out_invalid_vec->push_back(std::move(change));
      }
    }
    // We must also check that neither of the skip-log-bin or disable-log-bin
    // options are set.
    Invalid_config change_skip =
        Invalid_config("skip_log_bin", k_value_not_set);
    Invalid_config change_disable =
        Invalid_config("disable_log_bin", k_value_not_set);
    std::vector<std::string> allowed_skip_dis_values{k_value_not_set};
    check_variable_compliance(
        allowed_skip_dis_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
        &change_skip, Config_type::CONFIG, false, true);
    check_variable_compliance(
        allowed_skip_dis_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_file_handler),
        &change_disable, Config_type::CONFIG, false, true);

    log_invalid_config(change_disable);
    // if there are any changes to be made, add them to the vector of changes
    if (!change_disable.types.empty())
      out_invalid_vec->push_back(std::move(change_disable));
    log_invalid_config(change_skip);
    if (!change_skip.types.empty())
      out_invalid_vec->push_back(std::move(change_skip));
  }

  if (config.has_handler(mysqlshdk::config::k_dft_cfg_server_handler)) {
    // create invalid_config with default values for server, which are
    // different from the ones for the config file.
    Invalid_config change = Invalid_config("log_bin", "ON");
    std::vector<std::string> valid_values{"1", "ON"};
    check_variable_compliance(
        valid_values, true,
        *config.get_handler(mysqlshdk::config::k_dft_cfg_server_handler),
        &change, Config_type::SERVER, true, true);
    // If the log_bin value is not on the valid values, then the configuration
    // is not valid.
    if (!change.types.empty()) {
      // If the configuration is not valid on the server, and no config file
      // handler was provided, we must add an invalid config to fix the
      // value on the configuration file since the value cannot cannot be
      // persisted. If the config file handler exists, we already checked
      // if an invalid config is required.
      if (!config.has_handler(mysqlshdk::config::k_dft_cfg_file_handler)) {
        out_invalid_vec->emplace_back("log_bin", k_value_not_set, k_no_value,
                                      Config_types(Config_type::CONFIG), false,
                                      shcore::Value_type::String);
      }
    }
    log_invalid_config(change);
    // if there are any changes to be made, add them to the vector of changes
    if (!change.types.empty()) out_invalid_vec->push_back(std::move(change));
  }
}

bool is_group_replication_delayed_starting(
    const mysqlshdk::mysql::IInstance &instance) {
  try {
    return instance.get_session()
               ->query(
                   "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME "
                   "= "
                   "'thread/group_rpl/THD_delayed_initialization'")
               ->fetch_one()
               ->get_uint(0) != 0;
  } catch (std::exception &e) {
    log_warning("Error checking GR state: %s", e.what());
    return false;
  }
}

bool is_active_member(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &host, const int port) {
  std::string is_active_member_stmt_fmt =
      "SELECT Member_state "
      "FROM performance_schema.replication_group_members "
      "WHERE Member_host = ? AND Member_port = ? "
      "AND Member_state NOT IN ('OFFLINE', 'UNREACHABLE')";
  shcore::sqlstring is_active_member_stmt =
      shcore::sqlstring(is_active_member_stmt_fmt.c_str(), 0);
  is_active_member_stmt << host;
  is_active_member_stmt << port;
  is_active_member_stmt.done();
  auto session = instance.get_session();
  auto resultset = session->query(is_active_member_stmt);
  auto row = resultset->fetch_one();
  if (row)
    return true;
  else
    return false;
}

void update_auto_increment(mysqlshdk::config::Config *config,
                           const Topology_mode &topology_mode) {
  assert(config != nullptr);

  if (topology_mode == Topology_mode::SINGLE_PRIMARY) {
    // Set auto-increment for single-primary topology:
    // - auto_increment_increment = 1
    // - auto_increment_offset = 2
    config->set("auto_increment_increment", utils::nullable<int64_t>{1});
    config->set("auto_increment_offset", utils::nullable<int64_t>{2});
  } else if (topology_mode == Topology_mode::MULTI_PRIMARY) {
    // Set auto-increment for multi-primary topology:
    // - auto_increment_increment = n;
    // - auto_increment_offset = 1 + server_id % n;
    // where n is the size of the GR group if > 7, otherwise n = 7.
    // NOTE: We are assuming that there is only one handler for each instance.
    std::vector<std::string> handler_names = config->list_handler_names();
    int64_t group_size = handler_names.size();
    int64_t n = (group_size > 7) ? group_size : 7;
    config->set("auto_increment_increment", utils::nullable<int64_t>{n});

    // Each instance has a different server_id therefore each handler is set
    // individually here.
    for (std::string handler_name : handler_names) {
      mysqlshdk::utils::nullable<int64_t> server_id =
          config->get_int("server_id", handler_name);
      int64_t offset = 1 + *server_id % n;
      config->set("auto_increment_offset", utils::nullable<int64_t>{offset},
                  handler_name);
    }
  }
}

void set_as_primary(const mysqlshdk::mysql::IInstance &instance,
                    const std::string &uuid) {
  shcore::sqlstring query("SELECT group_replication_set_as_primary(?)", 0);
  query << uuid;
  query.done();

  try {
    log_debug("Executing UDF: %s", query.str().c_str());
    instance.get_session()->query(query);
  } catch (mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

void switch_to_multi_primary_mode(const mysqlshdk::mysql::IInstance &instance) {
  std::string query = "SELECT group_replication_switch_to_multi_primary_mode()";

  try {
    log_debug("Executing UDF: %s", query.c_str());
    instance.get_session()->query(query);
  } catch (mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

void switch_to_single_primary_mode(const mysqlshdk::mysql::IInstance &instance,
                                   const std::string &uuid) {
  std::string query;

  if (!uuid.empty()) {
    shcore::sqlstring query_fmt(
        "SELECT group_replication_switch_to_single_primary_mode(?)", 0);
    query_fmt << uuid;
    query_fmt.done();
    query = query_fmt.str();
  } else {
    query = "SELECT group_replication_switch_to_single_primary_mode()";
  }

  try {
    log_debug("Executing UDF: %s", query.c_str());
    instance.get_session()->query(query);
  } catch (mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

bool is_running_gr_auto_rejoin(const mysqlshdk::mysql::IInstance &instance) {
  auto session = instance.get_session();
  bool result = false;
  if (session) {
    try {
      auto row =
          session
              ->query(
                  "SELECT PROCESSLIST_STATE FROM performance_schema.threads "
                  "WHERE NAME = 'thread/group_rpl/THD_autorejoin'")
              ->fetch_one();
      // if the query doesn't return empty, then auto-rejoin is running.
      result = row != nullptr;
    } catch (const std::exception &e) {
      log_error("Error checking GR auto-rejoin procedure state: %s", e.what());
      throw;
    }
  } else {
    throw(std::runtime_error("Session no longer exists."));
  }
  return result;
}

}  // namespace gr
}  // namespace mysqlshdk
