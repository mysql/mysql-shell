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
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/plugin.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/uuid_gen.h"

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
  if (shcore::str_caseeq("ONLINE", state))
    return Member_state::ONLINE;
  else if (shcore::str_caseeq("RECOVERING", state))
    return Member_state::RECOVERING;
  else if (shcore::str_caseeq("OFFLINE", state))
    return Member_state::OFFLINE;
  else if (shcore::str_caseeq("ERROR", state))
    return Member_state::ERROR;
  else if (shcore::str_caseeq("UNREACHABLE", state))
    return Member_state::UNREACHABLE;
  else if (state.empty() || shcore::str_caseeq("(MISSING)", state) ||
           shcore::str_caseeq("MISSING", state))
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
    case Member_role::NONE:
      return "NONE";
  }
  throw std::logic_error("invalid role");
}

Member_role to_member_role(const std::string &role) {
  if (shcore::str_caseeq("PRIMARY", role)) {
    return Member_role::PRIMARY;
  } else if (shcore::str_caseeq("SECONDARY", role)) {
    return Member_role::SECONDARY;
  } else if (role.empty()) {
    return Member_role::NONE;
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
  if (shcore::str_caseeq("Single-Primary", mode))
    return Topology_mode::SINGLE_PRIMARY;
  else if (shcore::str_caseeq("Multi-Primary", mode))
    return Topology_mode::MULTI_PRIMARY;
  else
    throw std::runtime_error("Unsupported Group Replication mode: " + mode);
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
    std::string query;
    if (instance.get_version() >= utils::Version(8, 0, 2)) {
      query =
          "SELECT NOT @@group_replication_single_primary_mode OR "
          "    (select member_id from "
          "performance_schema.replication_group_members where (member_role = "
          "'PRIMARY')) = @@server_uuid";
    } else {
      query =
          "SELECT NOT @@group_replication_single_primary_mode OR "
          "    (select variable_value"
          "       from performance_schema.global_status"
          "       where variable_name = 'group_replication_primary_member')"
          "    = @@server_uuid";
    }
    auto res = instance.query(query);
    auto row = res->fetch_one();
    return (row && row->get_int(0)) != 0;
  } catch (const mysqlshdk::db::Error &e) {
    log_warning("Error checking if member is primary: %s (%i)", e.what(),
                e.code());
    if (e.code() != ER_UNKNOWN_SYSTEM_VARIABLE) throw;

    throw std::runtime_error(
        shcore::str_format("Group replication not started (MySQL error %d: %s)",
                           e.code(), e.what()));
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
  // Note: ERROR members aren't supposed to be in the members list (except for
  // itself), but there's a GR bug.
  const char *q =
      "SELECT "
      "  CAST(SUM(IF(member_state = 'UNREACHABLE', 1, 0)) AS SIGNED) AS UNRCH,"
      "  COUNT(*) AS TOTAL,"
      "  (SELECT member_state"
      "      FROM performance_schema.replication_group_members"
      "      WHERE member_id = @@server_uuid) AS my_state"
      "  FROM performance_schema.replication_group_members"
      "  WHERE member_id = @@server_uuid OR member_state <> 'ERROR'";

  std::shared_ptr<db::IResult> resultset = instance.query(q);
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
  // replication_group_members will have a single row with member_id = '' and
  // member_state = OFFLINE in 5.7 if the server is started and GR doesn't
  // auto-start.
  static const char *member_state_stmt =
      "SELECT member_state "
      "FROM performance_schema.replication_group_members "
      "WHERE member_id = @@server_uuid OR"
      " (member_id = '' AND member_state = 'OFFLINE')";
  auto resultset = instance.query(member_state_stmt);
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
 * @param out out_has_quorum if not NULL, assigned to true if the instance
 *        is part of a majority group.
 * @param out out_group_view_id if not NULL, assigned to the view_id
 *
 * @return A list of GR_member objects corresponding to the current known
 *         members of the group (from the point of view of the specified
 *         instance).
 */
std::vector<Member> get_members(const mysqlshdk::mysql::IInstance &instance,
                                bool *out_single_primary_mode,
                                bool *out_has_quorum,
                                std::string *out_group_view_id) {
  std::vector<Member> members;
  size_t online_members = 0;
  std::shared_ptr<db::IResult> result;
  const char *query;

  // Note: member_state is supposed to not be possible to be ERROR except
  // for the queried member itself, but that's not true because of a bug in GR

  // 8.0.2 added member_role and member_version columns
  if (instance.get_version() >= utils::Version(8, 0, 2)) {
    query =
        "SELECT m.member_id, m.member_state, m.member_host, m.member_port,"
        " m.member_role, m.member_version, s.view_id"
        " FROM performance_schema.replication_group_members m"
        " LEFT JOIN performance_schema.replication_group_member_stats s"
        "   ON m.member_id = s.member_id"
        "      AND s.channel_name = 'group_replication_applier'"
        " WHERE m.member_id = @@server_uuid OR m.member_state <> 'ERROR'"
        " ORDER BY m.member_id";
  } else {
    // query the old way
    query =
        "SELECT m.member_id, m.member_state, m.member_host, m.member_port,"
        "   IF(g.primary_uuid = '' OR m.member_id = g.primary_uuid,"
        "   'PRIMARY', 'SECONDARY') as member_role,"
        "    NULL as member_version, s.view_id"
        " FROM (SELECT IFNULL(variable_value, '') AS primary_uuid"
        "       FROM performance_schema.global_status"
        "       WHERE variable_name = 'group_replication_primary_member') g,"
        "      performance_schema.replication_group_members m"
        " LEFT JOIN performance_schema.replication_group_member_stats s"
        "   ON m.member_id = s.member_id"
        "     AND s.channel_name = 'group_replication_applier'"
        " WHERE m.member_id = @@server_uuid OR m.member_state <> 'ERROR'"
        " ORDER BY m.member_id";
  }

  try {
    result = instance.query(query);
  } catch (const mysqlshdk::db::Error &e) {
    log_error("Error querying GR member information: %s", e.format().c_str());
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      return {};
    }
    throw;
  }

  db::Row_ref_by_name row = result->fetch_one_named();
  if (!row || row.get_string("member_role").empty()) {
    // no members listed or empty role
    log_debug(
        "Query to replication_group_members from '%s' did not return "
        "group membership data",
        instance.descr().c_str());

    throw std::runtime_error(
        "Group replication does not seem to be active in instance '" +
        instance.descr() + "'");
  }

  while (row) {
    Member member;
    member.uuid = row.get_string("member_id");
    member.state = to_member_state(row.get_string("member_state"));
    member.host = row.get_string("member_host");
    member.port = row.get_int("member_port", 0);
    member.role = to_member_role(row.get_string("member_role"));
    member.version = row.get_string("member_version", "");
    if (out_group_view_id && !row.is_null("view_id")) {
      *out_group_view_id = row.get_string("view_id");
    }

    members.push_back(member);

    if (member.state == Member_state::ONLINE ||
        member.state == Member_state::RECOVERING)
      online_members++;

    row = result->fetch_one_named();
  }

  if (out_single_primary_mode) {
    mysqlshdk::gr::get_group_primary_uuid(instance, out_single_primary_mode);
  }

  assert(
      (members.size() == 1 && (members.front().state == Member_state::OFFLINE ||
                               members.front().state == Member_state::ERROR)) ||
      !out_group_view_id || !out_group_view_id->empty());

  if (out_has_quorum) *out_has_quorum = online_members > members.size() / 2;

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
 * @param  out_has_quorum          set to true if the member is part of a quorum
 * @param  out_is_primary          set to true if the member is a primary
 * @return                         false if the instance is not part of a group
 */
bool get_group_information(const mysqlshdk::mysql::IInstance &instance,
                           Member_state *out_member_state,
                           std::string *out_member_id,
                           std::string *out_group_name,
                           std::string *out_group_view_change_uuid,
                           bool *out_single_primary_mode, bool *out_has_quorum,
                           bool *out_is_primary) {
  try {
    std::string query =
        "SELECT @@group_replication_group_name group_name, "
        "NULLIF(CONCAT(''/*!80026, @@group_replication_view_change_uuid*/), "
        "'') group_view_change_uuid, "
        " @@group_replication_single_primary_mode single_primary, "
        " @@server_uuid, "
        " member_state, "
        " (SELECT "
        "   sum(IF(member_state in ('ONLINE', 'RECOVERING'), 1, 0)) > sum(1)/2 "
        "  FROM performance_schema.replication_group_members"
        "  WHERE member_id = @@server_uuid OR member_state <> 'ERROR'"
        " ) has_quorum,"
        " COALESCE(/*!80002 member_role = 'PRIMARY', NULL AND */"
        "     NOT @@group_replication_single_primary_mode OR"
        "     member_id = (select variable_value"
        "       from performance_schema.global_status"
        "       where variable_name = 'group_replication_primary_member')"
        " ) is_primary"
        " FROM performance_schema.replication_group_members"
        " WHERE member_id = @@server_uuid";

    auto result = instance.query(query);
    const db::IRow *row = result->fetch_one();
    if (!row || row->is_null(0)) return false;

    if (out_group_name) *out_group_name = row->get_string(0);
    if (out_group_view_change_uuid && !row->is_null(1))
      *out_group_view_change_uuid = row->get_string(1);
    if (out_single_primary_mode && !row->is_null(2))
      *out_single_primary_mode = (row->get_int(2) != 0);
    if (out_member_id) *out_member_id = row->get_string(3);
    if (out_member_state)
      *out_member_state = to_member_state(row->get_string(4));
    if (out_has_quorum)
      *out_has_quorum = row->is_null(5) ? false : row->get_int(5) != 0;
    if (out_is_primary) *out_is_primary = row->get_int(6, 0);

    return true;

  } catch (const mysqlshdk::db::Error &e) {
    log_error("Error while querying for group_replication info: %s", e.what());
    switch (e.code()) {
      case ER_BAD_DB_ERROR:
      case ER_NO_SUCH_TABLE:
      case ER_UNKNOWN_SYSTEM_VARIABLE:
        // if GR plugin is not installed, we get unknown sysvar
        // if server doesn't have pfs, we get BAD_DB
        // if server has pfs but not the GR tables, we get NO_SUCH_TABLE
        return false;
      default:
        throw;
    }
  }
}

std::string get_group_primary_uuid(const mysqlshdk::mysql::IInstance &instance,
                                   bool *out_single_primary_mode) {
  std::string query;
  // the column "member_role" was only added in 8.0.2
  if (instance.get_version() >= utils::Version(8, 0, 2)) {
    query =
        "SELECT @@group_replication_single_primary_mode, member_id AS "
        "primary_uuid"
        "   FROM performance_schema.replication_group_members"
        "   WHERE (member_role = 'PRIMARY')";
  } else {
    query =
        "SELECT @@group_replication_single_primary_mode, "
        "       variable_value AS primary_uuid"
        "   FROM performance_schema.global_status"
        "   WHERE variable_name = 'group_replication_primary_member'";
  }

  auto res = instance.query(query);
  if (auto row = res->fetch_one(); row) {
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
      const char *get_gr_protocol_version =
          "SELECT group_replication_get_communication_protocol()";

      log_debug("Executing UDF: %s",
                get_gr_protocol_version + sizeof("SELECT"));  // hide "SELECT "

      auto resultset = instance.query_udf(get_gr_protocol_version);
      auto row = resultset->fetch_one();

      if (row) {
        return (mysqlshdk::utils::Version(row->get_string(0)));
      } else {
        throw std::logic_error(
            "No rows returned when querying the version of Group Replication "
            "communication protocol.");
      }
    } catch (const mysqlshdk::db::Error &error) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
    }
  }
}

mysqlshdk::utils::Version get_max_supported_group_protocol_version(
    const mysqlshdk::utils::Version &server_version) {
  if (server_version < mysqlshdk::utils::Version(8, 0, 16)) {
    return mysqlshdk::utils::Version(5, 7, 14);
  } else if (server_version < mysqlshdk::utils::Version(8, 0, 27)) {
    // Protocol version for any server >= 8.0.16 && < 8.0.27 is 8.0.16
    return mysqlshdk::utils::Version(8, 0, 16);
  }

  // we have to assume protocol version for any server >= 8.0.27 is 8.0.27
  return mysqlshdk::utils::Version(8, 0, 27);
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
    log_debug("Executing UDF: %s",
              query.c_str() + sizeof("SELECT"));  // hide "SELECT "

    instance.query_udf(query);
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

bool is_protocol_upgrade_possible(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &skip_server_uuid,
    mysqlshdk::utils::Version *out_protocol_version) {
  std::vector<Member> group_members = get_members(instance);

  bool upgrade_required = false;

  // Get the current protocol version in use by the group
  mysqlshdk::utils::Version protocol_version_group =
      get_group_protocol_version(instance);
  // Check if any of the group_members supports a higher protocol version
  for (const auto &member : group_members) {
    // If version is not available, the instance is < 8.0, so an upgrade is not
    // required.
    if (member.version.empty()) {
      return false;
    }

    // If the instance is leaving the cluster we must skip checking it against
    // the cluster
    if (!skip_server_uuid.empty() && (skip_server_uuid == member.uuid)) {
      continue;
    } else {
      // Check if protocol version in use is lower than than the instance's
      // protocol version. If it is, the protocol version must be upgraded
      mysqlshdk::utils::Version ver(get_max_supported_group_protocol_version(
          mysqlshdk::utils::Version(member.version)));
      if (protocol_version_group < ver) {
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
    log_info(
        "Group Replication protocol version upgrade required (current=%s, "
        "new=%s)",
        protocol_version_group.get_full().c_str(),
        out_protocol_version->get_full().c_str());
  }

  return upgrade_required;
}

bool install_group_replication_plugin(
    const mysqlshdk::mysql::IInstance &instance,
    mysqlshdk::config::Config *config) {
  return install_plugin(k_gr_plugin_name, instance, config);
}

bool uninstall_group_replication_plugin(
    const mysqlshdk::mysql::IInstance &instance,
    mysqlshdk::config::Config *config) {
  return uninstall_plugin(k_gr_plugin_name, instance, config);
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
std::map<std::string, std::optional<std::string>> get_all_configurations(
    const mysqlshdk::mysql::IInstance &instance) {
  // Get all GR system variables.
  std::map<std::string, std::optional<std::string>> gr_vars =
      instance.get_system_variables_like(
          "group_replication_%", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get all auto_increment variables.
  std::map<std::string, std::optional<std::string>> auto_inc_vars =
      instance.get_system_variables_like(
          "auto_increment_%", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Merge variable and return the result.
  gr_vars.insert(auto_inc_vars.begin(), auto_inc_vars.end());
  return gr_vars;
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
 */
void start_group_replication(const mysqlshdk::mysql::IInstance &instance,
                             const bool bootstrap) {
  if (bootstrap)
    instance.set_sysvar("group_replication_bootstrap_group", true,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  try {
    instance.execute("START GROUP_REPLICATION");
  } catch (const std::exception &) {
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
  }
}

/**
 * Stop the Group Replication.
 *
 * @param instance session object to connect to the target instance.
 */
void stop_group_replication(const mysqlshdk::mysql::IInstance &instance) {
  instance.execute("STOP GROUP_REPLICATION");
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
  const std::set<std::string> gr_grants{"REPLICATION SLAVE"};
  return instance.get_user_privileges(user, host)->validate(gr_grants);
}

mysqlshdk::mysql::Auth_options create_recovery_user(
    const std::string &username, const mysqlshdk::mysql::IInstance &primary,
    const std::vector<std::string> &hosts,
    const Create_recovery_user_options &options) {
  assert(!hosts.empty());
  assert(!username.empty());

  mysqlshdk::mysql::IInstance::Create_user_options user_options;

  // setup user grants
  if (options.clone_supported)
    user_options.grants.push_back(
        {"REPLICATION SLAVE, BACKUP_ADMIN", "*.*", false});
  else
    user_options.grants.push_back({"REPLICATION SLAVE", "*.*", false});

  if (options.auto_failover)
    user_options.grants.push_back({"SELECT", "performance_schema.*", false});

  if (options.mysql_comm_stack_supported)
    user_options.grants.push_back({"GROUP_REPLICATION_STREAM", "*.*", false});

  if (primary.get_version() >= utils::Version(8, 0, 0)) {
    // Recovery accounts must have CONNECTION_ADMIN too, otherwise, if a group
    // members has offline_mode enabled the member is evicted from the group
    // since the recovery account connections are disconnected and blocked.
    // This has greater impact when using "MySQL" communication stack
    user_options.grants.push_back({"CONNECTION_ADMIN", "*.*", false});
  }

  mysqlshdk::mysql::Auth_options creds;

  try {
    // Accounts are created at the primary replica regardless of who will use
    // them, since they'll get replicated everywhere.

    if (options.requires_password && !options.password.has_value()) {
      // finalize user options
      user_options.disable_pwd_expire = true;
      user_options.cert_issuer = options.cert_issuer;
      user_options.cert_subject = options.cert_subject;

      // re-create replication with a new generated password
      std::string new_pwd;
      mysqlshdk::mysql::create_user_with_random_password(
          primary, username, hosts, user_options, &new_pwd);

      creds.password = std::move(new_pwd);

    } else {
      // finalize user options
      if (options.requires_password) {
        user_options.password = *options.password;
        user_options.disable_pwd_expire = true;
      }
      user_options.cert_issuer = options.cert_issuer;
      user_options.cert_subject = options.cert_subject;

      // re-create replication with a given password
      mysqlshdk::mysql::create_user(primary, username, hosts, user_options);

      creds.password = user_options.password;
    }

    creds.user = username;
    creds.ssl_options = primary.get_connection_options().get_ssl_options();

  } catch (std::exception &e) {
    throw std::runtime_error(shcore::str_format(
        "Unable to create the Group Replication recovery account: %s",
        e.what()));
  }

  return creds;
}

bool is_group_replication_delayed_starting(
    const mysqlshdk::mysql::IInstance &instance) {
  try {
    return instance
               .query(
                   "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME "
                   "= "
                   "'thread/group_rpl/THD_delayed_initialization'")
               ->fetch_one()
               ->get_uint(0) != 0;
  } catch (const std::exception &e) {
    log_warning("Error checking GR state: %s", e.what());
    return false;
  }
}

bool is_active_member(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &host, const int port) {
  bool ret_val = false;
  shcore::sqlstring is_active_member_stmt;

  if (!host.empty() && port != 0) {
    std::string is_active_member_stmt_fmt =
        "SELECT count(*) "
        "FROM performance_schema.replication_group_members "
        "WHERE MEMBER_HOST = ? AND MEMBER_PORT = ? "
        "AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')";
    is_active_member_stmt =
        shcore::sqlstring(is_active_member_stmt_fmt.c_str(), 0);
    is_active_member_stmt << host;
    is_active_member_stmt << port;
    is_active_member_stmt.done();

  } else {
    std::string is_active_member_stmt_fmt =
        "select count(*) "
        "FROM performance_schema.replication_group_members "
        "WHERE MEMBER_ID = @@server_uuid AND MEMBER_STATE NOT "
        "IN ('OFFLINE', 'UNREACHABLE')";
    is_active_member_stmt =
        shcore::sqlstring(is_active_member_stmt_fmt.c_str(), 0);
    is_active_member_stmt.done();
  }

  try {
    auto result = instance.query(is_active_member_stmt);
    auto row = result->fetch_one();
    if (row) {
      if (row->get_int(0) != 0) {
        log_debug("Instance type check: %s: GR is active",
                  instance.descr().c_str());
        ret_val = true;
      } else {
        log_debug("Instance type check: %s: GR is installed but not active",
                  instance.descr().c_str());
      }
    }
  } catch (const mysqlshdk::db::Error &e) {
    if (e.code() == ER_UNKNOWN_SYSTEM_VARIABLE) {
      log_warning("Group replication not started (MySQL error: %s (%i)",
                  e.what(), e.code());
    } else {
      log_warning("Error checking if Group Replication is active: %s (%i)",
                  e.what(), e.code());
      throw;
    }
  }
  return ret_val;
}

void wait_member_online(const mysqlshdk::mysql::IInstance &instance,
                        int32_t timeout_ms) {
  uint32_t sleep_time = 0.5 * 1000;  // half second

  while (true) {
    if (mysqlshdk::gr::get_member_state(instance) ==
        mysqlshdk::gr::Member_state::ONLINE) {
      return;
    } else {
      if (timeout_ms <= 0) {
        throw std::runtime_error("Timeout waiting for '" + instance.descr() +
                                 "' to become ONLINE in the Group");
      }

      shcore::sleep_ms(sleep_time);
      timeout_ms -= sleep_time;
    }
  }
}

void update_auto_increment(mysqlshdk::config::Config *config,
                           const Topology_mode &topology_mode,
                           uint64_t group_size) {
  assert(config != nullptr);

  if (topology_mode == Topology_mode::SINGLE_PRIMARY) {
    // Set auto-increment for single-primary topology:
    // - auto_increment_increment = 1
    // - auto_increment_offset = 2
    config->set("auto_increment_increment", std::optional<int64_t>{1});
    config->set("auto_increment_offset", std::optional<int64_t>{2});
  } else if (topology_mode == Topology_mode::MULTI_PRIMARY) {
    // Set auto-increment for multi-primary topology:
    // - auto_increment_increment = n;
    // - auto_increment_offset = 1 + server_id % n;
    // where n is the size of the GR group if > 7, otherwise n = 7.
    // NOTE: We are assuming that there is only one handler for each instance.
    std::vector<std::string> handler_names = config->list_handler_names();
    int64_t size = static_cast<int64_t>((group_size == 0) ? handler_names.size()
                                                          : group_size);
    int64_t n = (size > 7) ? size : 7;
    config->set("auto_increment_increment", std::optional<int64_t>{n});

    // Each instance has a different server_id therefore each handler is set
    // individually here.
    for (std::string handler_name : handler_names) {
      auto server_id = config->get_int("server_id", handler_name);
      int64_t offset = 1 + (*server_id) % n;
      config->set_for_handler("auto_increment_offset",
                              std::optional<int64_t>{offset}, handler_name);
    }
  }
}

void update_group_seeds(mysqlshdk::config::Config *config,
                        const std::map<std::string, std::string> &group_seeds) {
  std::vector<std::string> handler_names = config->list_handler_names();

  // Each instance might have a different group_seed value thus each handler is
  // set individually.
  for (const std::string &handler_name : handler_names) {
    std::string gr_group_seeds_new_value;

    // assemble group_seeds list with all GR endpoints except their own
    for (const auto &it : group_seeds) {
      if (it.first != config->get_handler(handler_name)->get_server_uuid()) {
        if (!gr_group_seeds_new_value.empty())
          gr_group_seeds_new_value.append(",");
        gr_group_seeds_new_value.append(it.second);
      }
    }

    config->set_for_handler("group_replication_group_seeds",
                            gr_group_seeds_new_value, handler_name);
  }
}

void set_as_primary(const mysqlshdk::mysql::IInstance &instance,
                    const std::string &uuid,
                    std::optional<uint32_t> running_transactions_timeout) {
  std::string query;
  if (running_transactions_timeout) {
    query = ("SELECT group_replication_set_as_primary(?, ?)"_sql
             << uuid << *running_transactions_timeout)
                .str();
  } else {
    query = ("SELECT group_replication_set_as_primary(?)"_sql << uuid).str();
  }

  try {
    log_debug("Executing UDF: %s",
              query.c_str() + sizeof("SELECT"));  // hide "SELECT "
    instance.query_udf(query);
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

void switch_to_multi_primary_mode(const mysqlshdk::mysql::IInstance &instance) {
  std::string query = "SELECT group_replication_switch_to_multi_primary_mode()";

  try {
    log_debug("Executing UDF: %s",
              query.c_str() + sizeof("SELECT"));  // hide "SELECT "
    instance.query_udf(query);
  } catch (const mysqlshdk::db::Error &error) {
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
    log_debug("Executing UDF: %s",
              query.c_str() + sizeof("SELECT"));  // hide "SELECT "
    instance.query_udf(query);
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}

bool is_running_gr_auto_rejoin(const mysqlshdk::mysql::IInstance &instance) {
  bool result = false;

  try {
    auto row =
        instance
            .query(
                "SELECT PROCESSLIST_STATE FROM performance_schema.threads "
                "WHERE NAME = 'thread/group_rpl/THD_autorejoin'")
            ->fetch_one();
    // if the query doesn't return empty, then auto-rejoin is running.
    result = row != nullptr;
  } catch (const std::exception &e) {
    log_error("Error checking GR auto-rejoin procedure state: %s", e.what());
    throw;
  }

  return result;
}

void check_instance_version_compatibility(
    const mysqlshdk::mysql::IInstance &instance,
    mysqlshdk::utils::Version lowest_cluster_version) {
  auto gr_allow_lower_version_join = instance.get_sysvar_bool(
      "group_replication_allow_local_lower_version_join", false);

  // If gr_allow_lower_version_join is NULL then it means the variable is not
  // available, e.g., if the GR plugin is not installed which happens by default
  // on 5.7 servers. In that case, we apply the expected behaviour for the
  // default variable value (false).
  if (gr_allow_lower_version_join) return;

  mysqlshdk::utils::Version target_version = instance.get_version();

  auto supports_downgrade = [](const utils::Version &ver) {
    return ver >= utils::Version(8, 0, 35);
  };

  if (target_version <= utils::Version(8, 0, 16)) {
    if (target_version.get_major() < lowest_cluster_version.get_major()) {
      throw std::runtime_error(
          "Instance major version '" +
          std::to_string(target_version.get_major()) +
          "' cannot be lower than the cluster lowest major version '" +
          std::to_string(lowest_cluster_version.get_major()) + "'.");
    }
  } else {
    // upgrades are allowed
    if (target_version >= lowest_cluster_version) return;

    // downgrades within the same series are allowed, but only starting
    // from 8.0.35
    if (supports_downgrade(target_version) &&
        supports_downgrade(lowest_cluster_version) &&
        target_version.numeric_version_series() ==
            lowest_cluster_version.numeric_version_series())
      return;

    // downgrades between series are not allowed
    throw std::runtime_error(
        "Instance version '" + target_version.get_base() +
        "' cannot be lower than the cluster lowest version '" +
        lowest_cluster_version.get_base() + "'.");
  }
}

bool is_instance_only_read_compatible(
    const mysqlshdk::mysql::IInstance &instance,
    mysqlshdk::utils::Version lowest_cluster_version) {
  mysqlshdk::utils::Version version = instance.get_version();

  if (version >= mysqlshdk::utils::Version(8, 0, 17) &&
      lowest_cluster_version.get_major() >= 8 &&
      version > lowest_cluster_version) {
    return true;
  }
  return false;
}

Group_member_recovery_status detect_recovery_status(
    const mysqlshdk::mysql::IInstance &instance, const std::string &start_time,
    bool *out_recovering) {
  std::string clone_state;
  std::string member_state;
  bool recovery = false;

  try {
    if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 17)) {
      auto result = instance.queryf(
          "SELECT"
          " (SELECT count(*)"
          "   FROM performance_schema.replication_applier_status_by_worker a"
          "   JOIN performance_schema.replication_connection_status c"
          "     ON a.channel_name = c.channel_name"
          "   WHERE a.channel_name = 'group_replication_recovery'"
          "     AND ((a.service_state = 'ON' OR a.last_error_message <> '')"
          "          OR (c.service_state = 'ON' OR c.last_error_message <> ''))"
          " ) recovery,"
          " (SELECT member_state"
          "   FROM performance_schema.replication_group_members"
          "   WHERE member_id = @@server_uuid) member_state,"
          " (SELECT concat(state, '@', begin_time)"
          "   FROM performance_schema.clone_status"
          "   WHERE begin_time >= ?) clone",
          start_time.empty() ? "0000-00-00 00:00:00" : start_time);

      auto row = result->fetch_one_or_throw();
      recovery = row->get_int(0, 0);
      member_state = row->get_string(1, "OFFLINE");
      clone_state = row->get_string(2, "");
    }
  } catch (const mysqlshdk::db::Error &e) {
    log_debug("Error checking recovery state at %s: %s",
              instance.descr().c_str(), e.what());
  }
  if (member_state.empty()) {
    auto result = instance.query(
        "SELECT"
        " (SELECT count(*)"
        "   FROM performance_schema.replication_applier_status_by_worker a"
        "   JOIN performance_schema.replication_connection_status c"
        "     ON a.channel_name = c.channel_name"
        "   WHERE a.channel_name = 'group_replication_recovery'"
        "     AND ((a.service_state = 'ON' OR a.last_error_message <> '')"
        "          OR (c.service_state = 'ON' OR c.last_error_message <> ''))"
        " ) recovery,"
        " (SELECT member_state"
        "   FROM performance_schema.replication_group_members"
        "   WHERE member_id = @@server_uuid) member_state");

    auto row = result->fetch_one_or_throw();
    recovery = row->get_int(0, 0);
    member_state = row->get_string(1, "OFFLINE");
  }

  if (out_recovering) *out_recovering = member_state == "RECOVERING";

  log_debug2(
      "Recovery status of %s: member_state=%s, clone_state=%s, recovery=%i",
      instance.descr().c_str(), member_state.c_str(), clone_state.c_str(),
      recovery);

  // If CLONE is used, the server will restart once it's finished.
  // If the server has auto-rejoin enabled, a normal recovery can start right
  // after the restart. In that case, we report the normal recovery.
  if (member_state == "RECOVERING") {
    if (recovery)
      return Group_member_recovery_status::DISTRIBUTED;
    else if (!clone_state.empty())
      return Group_member_recovery_status::CLONE;
    else
      return Group_member_recovery_status::UNKNOWN;
  } else if (member_state == "ERROR") {
    if (recovery)
      return Group_member_recovery_status::DISTRIBUTED_ERROR;
    else if (!clone_state.empty())
      return Group_member_recovery_status::CLONE_ERROR;
    else
      return Group_member_recovery_status::UNKNOWN_ERROR;
  } else if (member_state == "ONLINE") {
    return Group_member_recovery_status::DONE_ONLINE;
  } else if (member_state == "OFFLINE") {
    // GR may stop during a CLONE, while it's restarting
    // GR stopped?
    log_info(
        "Member %s is in state %s, when recovery was expected "
        "(clone_state=%s).",
        instance.descr().c_str(), member_state.c_str(), clone_state.c_str());
    if (clone_state.empty())
      return Group_member_recovery_status::DONE_OFFLINE;
    else
      return Group_member_recovery_status::CLONE;
  } else {
    log_error("Member %s is in state %s, when recovery was expected",
              instance.descr().c_str(), member_state.c_str());
    throw std::logic_error("Unexpected member state " + member_state);
  }
}

bool is_endpoint_supported_by_gr(const std::string &endpoint,
                                 const mysqlshdk::utils::Version &version) {
  const bool supports_ipv6 = (version >= mysqlshdk::utils::Version(8, 0, 14));
  try {
    // split host from port, and make sure endpoint has both
    mysqlshdk::db::Connection_options conn_opt =
        shcore::get_connection_options(endpoint, false);

    if (!conn_opt.has_host() || !conn_opt.has_port()) {
      throw shcore::Exception::argument_error(
          shcore::str_format("Invalid address format: '%s'", endpoint.c_str()));
    }
    const bool is_ipv4 = mysqlshdk::utils::Net::is_ipv4(conn_opt.get_host());
    if (is_ipv4) {
      // IPv4 is supported by all versions
      return true;
    }
    const bool is_ipv6 = mysqlshdk::utils::Net::is_ipv6(conn_opt.get_host());
    if (is_ipv6) {
      return supports_ipv6;
    }
    // Do not try to resolve the hostname. Even if we can resolve it, there is
    // no guarantee that name resolution is the same across cluster instances
    // and the instance where we are running the ngshell.
    return true;
  } catch (const std::invalid_argument &) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid address format: '%s'", endpoint.c_str()));
  }
}

namespace {
void execute_member_action_udf(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &query,
                               const std::string &reason) {
  // Get the value of super_read_only to disable it if enabled.
  // Changing a GR member action configuration is only possible with
  // super_read_only disabled.
  bool super_read_only = instance.get_sysvar_bool("super_read_only", false);

  if (super_read_only) {
    log_info(
        "Disabling super_read_only mode on instance '%s' to '%s' a Group "
        "Replication member action configuration.",
        instance.descr().c_str(), reason.c_str());
    instance.set_sysvar("super_read_only", false);
  }

  shcore::on_leave_scope reset_super_read_only([&instance, super_read_only]() {
    if (super_read_only) {
      log_info("Restoring super_read_only mode on instance '%s'",
               instance.descr().c_str());
      instance.set_sysvar("super_read_only", true);
    }
  });

  try {
    assert(shcore::str_ibeginswith(query, "select "));
    log_debug("Executing UDF: %s",
              query.c_str() + sizeof("SELECT"));  // hide "SELECT "
    instance.query_udf(query);
  } catch (const mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }
}
}  // namespace

void disable_member_action(const mysqlshdk::mysql::IInstance &instance,
                           const std::string &action_name,
                           const std::string &stage) {
  shcore::sqlstring query = shcore::sqlstring(
      "SELECT group_replication_disable_member_action(?, ?)", false);

  query << action_name;
  query << stage;
  query.done();

  execute_member_action_udf(instance, query, "change");
}

void enable_member_action(const mysqlshdk::mysql::IInstance &instance,
                          const std::string &action_name,
                          const std::string &stage) {
  shcore::sqlstring query = shcore::sqlstring(
      "SELECT group_replication_enable_member_action(?, ?)", false);

  query << action_name;
  query << stage;
  query.done();

  execute_member_action_udf(instance, query.str(), "change");
}

bool get_member_action_status(const mysqlshdk::mysql::IInstance &instance,
                              const std::string &action_name,
                              bool *out_enabled) {
  shcore::sqlstring query = shcore::sqlstring(
      "SELECT enabled FROM performance_schema.replication_group_member_actions "
      "WHERE name = ?",
      0);

  query << action_name;
  query.done();

  auto resultset = instance.query(query);
  auto row = resultset->fetch_one();

  if (row) {
    if (out_enabled) *out_enabled = row->get_int(0);
    return true;
  } else {
    return false;
  }
}

void reset_member_actions(const mysqlshdk::mysql::IInstance &instance) {
  std::string query = "SELECT group_replication_reset_member_actions()";

  execute_member_action_udf(instance, query, "reset");
}

}  // namespace gr
}  // namespace mysqlshdk
