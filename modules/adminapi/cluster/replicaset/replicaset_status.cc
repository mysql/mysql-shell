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

#include <utility>

#include "modules/adminapi/cluster/replicaset/replicaset_status.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

namespace {
template <typename R>
inline bool set_uint(shcore::Dictionary_t dict, const std::string &prop,
                     const R &row, const std::string &field) {
  if (row.has_field(field)) {
    if (row.is_null(field))
      (*dict)[prop] = shcore::Value::Null();
    else
      (*dict)[prop] = shcore::Value(row.get_uint(field));
    return true;
  }
  return false;
}

template <typename R>
inline bool set_secs(shcore::Dictionary_t dict, const std::string &prop,
                     const R &row, const std::string &field) {
  if (row.has_field(field)) {
    if (row.is_null(field)) {
      (*dict)[prop] = shcore::Value::Null();
    } else {
      (*dict)[prop] = shcore::Value(row.get_int(field) / 1000000.0);
    }
    return true;
  }
  return false;
}

template <typename R>
inline bool set_string(shcore::Dictionary_t dict, const std::string &prop,
                       const R &row, const std::string &field) {
  if (row.has_field(field)) {
    if (row.is_null(field))
      (*dict)[prop] = shcore::Value::Null();
    else
      (*dict)[prop] = shcore::Value(row.get_string(field));
    return true;
  }
  return false;
}

template <typename R>
inline bool set_ts(shcore::Dictionary_t dict, const std::string &prop,
                   const R &row, const std::string &field) {
  if (row.has_field(field)) {
    if (row.is_null(field)) {
      (*dict)[prop] = shcore::Value::Null();
    } else {
      std::string ts = row.get_string(field);
      if (shcore::str_beginswith(ts, "0000-00-00 00:00:00"))
        (*dict)[prop] = shcore::Value("");
      else
        (*dict)[prop] = shcore::Value(ts);
    }
    return true;
  }
  return false;
}

}  // namespace

Replicaset_status::Replicaset_status(
    const GRReplicaSet &replicaset,
    const mysqlshdk::utils::nullable<uint64_t> &extended)
    : m_replicaset(replicaset), m_extended(extended) {}

Replicaset_status::~Replicaset_status() {}

void Replicaset_status::prepare() {
  // Save the reference of the cluster object
  m_cluster = m_replicaset.get_cluster();

  // Sanity check: Verify if the topology type changed and issue an error if
  // needed.
  m_replicaset.sanity_check();

  m_instances = m_replicaset.get_instances();

  // Always connect to members to be able to get an accurate mode, based on
  // their super_ready_only value.
  connect_to_members();
}

void Replicaset_status::connect_to_members() {
  auto group_session = m_cluster->get_group_session();

  mysqlshdk::db::Connection_options group_session_copts(
      group_session->get_connection_options());

  for (const auto &inst : m_instances) {
    mysqlshdk::db::Connection_options opts(inst.endpoint);
    if (opts.uri_endpoint() == group_session_copts.uri_endpoint()) {
      m_member_sessions[inst.endpoint] = group_session;
    } else {
      opts.set_login_options_from(group_session_copts);

      try {
        m_member_sessions[inst.endpoint] =
            mysqlshdk::db::mysql::open_session(opts);
      } catch (const mysqlshdk::db::Error &e) {
        m_member_connect_errors[inst.endpoint] = e.format();
      }
    }
  }
}

shcore::Dictionary_t Replicaset_status::check_group_status(
    const mysqlsh::dba::Instance &instance,
    const std::vector<mysqlshdk::gr::Member> &members) {
  shcore::Dictionary_t dict = shcore::make_dict();

  int unreachable_in_group;
  int total_in_group;
  bool has_quorum = mysqlshdk::gr::has_quorum(instance, &unreachable_in_group,
                                              &total_in_group);
  m_no_quorum = !has_quorum;

  // count inconsistencies in the group vs metadata
  int missing_from_group = 0;
  int missing_from_md = 0;

  for (const auto &inst : m_instances) {
    if (std::find_if(members.begin(), members.end(),
                     [&inst](const mysqlshdk::gr::Member &member) {
                       return member.uuid == inst.uuid;
                     }) == members.end()) {
      missing_from_group++;
    }
  }

  for (const auto &member : members) {
    if (std::find_if(m_instances.begin(), m_instances.end(),
                     [&member](const Instance_metadata &inst) {
                       return member.uuid == inst.uuid;
                     }) != m_instances.end()) {
      missing_from_md++;
    }
  }

  size_t online_count = total_in_group - unreachable_in_group;
  size_t number_of_failures_tolerated =
      online_count > 0 ? (online_count - 1) / 2 : 0;

  ReplicaSetStatus::Status rs_status;
  std::string desc_status;

  if (!has_quorum) {
    rs_status = ReplicaSetStatus::NO_QUORUM;
    desc_status = "Cluster has no quorum as visible from '" + instance.descr() +
                  "' and cannot process write transactions.";
  } else {
    if (number_of_failures_tolerated == 0) {
      rs_status = ReplicaSetStatus::OK_NO_TOLERANCE;

      desc_status = "Cluster is NOT tolerant to any failures.";
    } else {
      if (missing_from_group + unreachable_in_group > 0)
        rs_status = ReplicaSetStatus::OK_PARTIAL;
      else
        rs_status = ReplicaSetStatus::OK;

      if (number_of_failures_tolerated == 1)
        desc_status = "Cluster is ONLINE and can tolerate up to ONE failure.";
      else
        desc_status = "Cluster is ONLINE and can tolerate up to " +
                      std::to_string(number_of_failures_tolerated) +
                      " failures.";
    }
  }

  if (m_instances.size() > online_count) {
    if (m_instances.size() - online_count == 1)
      desc_status.append(" 1 member is not active");
    else
      desc_status.append(" " +
                         std::to_string(m_instances.size() - online_count) +
                         " members are not active");
  }

  (*dict)["statusText"] = shcore::Value(desc_status);
  (*dict)["status"] = shcore::Value(ReplicaSetStatus::describe(rs_status));

  return dict;
}

const Instance_metadata *Replicaset_status::instance_with_uuid(
    const std::string &uuid) {
  for (const auto &i : m_instances) {
    if (i.uuid == uuid) return &i;
  }
  return nullptr;
}

Member_stats_map Replicaset_status::query_member_stats() {
  Member_stats_map stats;
  auto group_instance = m_cluster->get_target_instance();

  auto member_stats = group_instance->query(
      "SELECT * FROM performance_schema.replication_group_member_stats");

  while (auto row = member_stats->fetch_one_named()) {
    std::string channel = row.get_string("CHANNEL_NAME");
    if (channel == "group_replication_applier")
      stats[row.get_string("MEMBER_ID")].second =
          mysqlshdk::db::Row_by_name(row);
    else if (channel == "group_replication_recovery")
      stats[row.get_string("MEMBER_ID")].first =
          mysqlshdk::db::Row_by_name(row);
  }

  return stats;
}

void Replicaset_status::collect_last_error(
    shcore::Dictionary_t dict, const mysqlshdk::db::Row_ref_by_name &row,
    const std::string &prefix, const std::string &key_prefix) {
  if (row.has_field(prefix + "ERROR_NUMBER") &&
      row.get_uint(prefix + "ERROR_NUMBER") != 0) {
    set_uint(dict, key_prefix + "Errno", row, prefix + "ERROR_NUMBER");
    set_string(dict, key_prefix + "Error", row, prefix + "ERROR_MESSAGE");
    set_ts(dict, key_prefix + "ErrorTimestamp", row,
           prefix + "ERROR_TIMESTAMP");
  }
}

shcore::Value Replicaset_status::collect_last(
    const mysqlshdk::db::Row_ref_by_name &row, const std::string &prefix,
    const std::string &what) {
  shcore::Dictionary_t tx = shcore::make_dict();

  set_string(tx, "transaction", row, prefix + "_TRANSACTION");
  set_ts(tx, "originalCommitTimestamp", row,
         prefix + "_TRANSACTION_ORIGINAL_COMMIT_TIMESTAMP");

  set_ts(tx, "immediateCommitTimestamp", row,
         prefix + "_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP");
  set_ts(tx, "startTimestamp", row,
         prefix + "_TRANSACTION_START_" + what + "_TIMESTAMP");
  set_ts(tx, "endTimestamp", row,
         prefix + "_TRANSACTION_END_" + what + "_TIMESTAMP");

  set_secs(tx, "originalCommitToEndTime", row,
           "LAST_ORIGINAL_COMMIT_TO_END_" + what + "_TIME");
  set_secs(tx, "immediateCommitToEndTime", row,
           "LAST_IMMEDIATE_COMMIT_TO_END_" + what + "_TIME");
  set_secs(tx, shcore::str_lower(what) + "Time", row, "LAST_" + what + "_TIME");

  set_uint(tx, "retries", row, prefix + "_TRANSACTION_RETRIES_COUNT");

  collect_last_error(tx, row, prefix + "_TRANSACTION_LAST_TRANSIENT_",
                     "lastTransient");
  return shcore::Value(tx);
}

shcore::Value Replicaset_status::collect_current(
    const mysqlshdk::db::Row_ref_by_name &row, const std::string &prefix,
    const std::string &what) {
  if (row.has_field(prefix + "_TRANSACTION")) {
    std::string gtid = row.get_string(prefix + "_TRANSACTION");
    if (!gtid.empty()) {
      shcore::Dictionary_t tx = shcore::make_dict();
      (*tx)["transaction"] = shcore::Value(gtid);
      set_ts(tx, "originalCommitTimestamp", row,
             prefix + "_TRANSACTION_ORIGINAL_COMMIT_TIMESTAMP");
      set_ts(tx, "immediateCommitTimestamp", row,
             prefix + "_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP");
      set_ts(tx, "startTimestamp", row,
             prefix + "_TRANSACTION_START_" + what + "_TIMESTAMP");

      set_secs(tx, "originalCommitToNowTime", row,
               "CURRENT_ORIGINAL_COMMIT_TO_NOW_TIME");
      set_secs(tx, "immediateCommitToNowTime", row,
               "CURRENT_IMMEDIATE_COMMIT_TO_NOW_TIME");

      set_uint(tx, "retries", row, prefix + "_TRANSACTION_RETRIES_COUNT");

      collect_last_error(tx, row, prefix + "_TRANSACTION_LAST_TRANSIENT_",
                         "lastTransient");
      return shcore::Value(tx);
    }
  }
  return shcore::Value();
}

shcore::Value Replicaset_status::connection_status(
    const mysqlshdk::db::Row_ref_by_name &row) {
  shcore::Dictionary_t dict = shcore::make_dict();

  // lookup label of the server
  // (*dict)["source"] = string_from_row(("SOURCE_UUID"));
  set_uint(dict, "threadId", row, "THREAD_ID");

  set_string(dict, "receivedTransactionSet", row, "RECEIVED_TRANSACTION_SET");

  collect_last_error(dict, row);

  set_ts(dict, "lastHeartbeatTimestamp", row, "LAST_HEARTBEAT_TIMESTAMP");
  set_uint(dict, "receivedHeartbeats", row, "COUNT_RECEIVED_HEARTBEATS");

  auto last = collect_last(row, "LAST_QUEUED", "QUEUE");
  if (!last.as_map()->empty()) (*dict)["lastQueued"] = shcore::Value(last);

  if (auto v = collect_current(row, "QUEUEING", "QUEUE"))
    (*dict)["currentlyQueueing"] = v;

  return shcore::Value(dict);
}

shcore::Value Replicaset_status::coordinator_status(
    const mysqlshdk::db::Row_ref_by_name &row) {
  shcore::Dictionary_t dict = shcore::make_dict();

  set_uint(dict, "threadId", row, "THREAD_ID");

  collect_last_error(dict, row);

  auto last = collect_last(row, "LAST_PROCESSED", "BUFFER");
  if (!last.as_map()->empty()) (*dict)["lastProcessed"] = shcore::Value(last);

  if (auto v = collect_current(row, "PROCESSING", "BUFFER"))
    (*dict)["currentlyProcessing"] = v;

  return shcore::Value(dict);
}

shcore::Value Replicaset_status::applier_status(
    const mysqlshdk::db::Row_ref_by_name &row) {
  shcore::Dictionary_t dict = shcore::make_dict();
  set_uint(dict, "threadId", row, "THREAD_ID");

  collect_last_error(dict, row);

  auto last = collect_last(row, "LAST_APPLIED", "APPLY");
  if (!last.as_map()->empty()) (*dict)["lastApplied"] = shcore::Value(last);

  set_string(dict, "lastSeen", row, "LAST_SEEN_TRANSACTION");

  if (auto v = collect_current(row, "APPLYING", "APPLY"))
    (*dict)["currentlyApplying"] = v;

  return shcore::Value(dict);
}

/**
 * Similar to collect_local_status(), but only includes basic/important
 * stats that should be displayed in the default output.
 */
void Replicaset_status::collect_basic_local_status(
    shcore::Dictionary_t dict, const mysqlsh::dba::Instance &instance) {
  using mysqlshdk::utils::Version;

  auto version = instance.get_version();

  shcore::Dictionary_t applier_node = shcore::make_dict();
  shcore::Array_t applier_workers = shcore::make_array();

#define TSDIFF(prefix, start, end) \
  "TIMEDIFF(" prefix "_" end ", " prefix "_" start ")"

  std::string sql;

  if (version >= Version(8, 0, 0)) {
    sql = "SELECT ";
    sql +=
        "IF(c.LAST_QUEUED_TRANSACTION=''"
        " OR c.LAST_QUEUED_TRANSACTION=a.LAST_APPLIED_TRANSACTION"
        " OR a.LAST_APPLIED_TRANSACTION_END_APPLY_TIMESTAMP <"
        "  a.LAST_APPLIED_TRANSACTION_ORIGINAL_COMMIT_TIMESTAMP,"
        "NULL, " TSDIFF("LAST_APPLIED_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP",
                        "END_APPLY_TIMESTAMP") ")";
    sql += " AS LAST_ORIGINAL_COMMIT_TO_END_APPLY_TIME";
    sql +=
        " FROM performance_schema.replication_applier_status_by_worker a"
        " JOIN performance_schema.replication_connection_status c"
        "   ON a.channel_name = c.channel_name"
        " WHERE a.channel_name = 'group_replication_applier'";

    // this can return multiple rows per channel for
    // multi-threaded applier, otherwise just one. If MT, we also
    // get stuff in the coordinator table
    auto result = instance.query(sql);
    auto row = result->fetch_one_named();
    if (row) {
      std::string lag =
          row.get_string("LAST_ORIGINAL_COMMIT_TO_END_APPLY_TIME", "");

      if (!lag.empty() && lag != "00:00:00.000000")
        (*dict)["replicationLag"] = shcore::Value(lag);
      else
        (*dict)["replicationLag"] = shcore::Value::Null();
    }
  }

#undef TSDIFF
}

/**
 * Collect per instance replication stats collected from performance_schema
 * tables.
 */
void Replicaset_status::collect_local_status(
    shcore::Dictionary_t dict, const mysqlsh::dba::Instance &instance,
    bool recovering) {
  using mysqlshdk::utils::Version;

  auto version = instance.get_version();

  shcore::Dictionary_t recovery_node = shcore::make_dict();
  shcore::Dictionary_t applier_node = shcore::make_dict();
  shcore::Array_t recovery_workers = shcore::make_array();
  shcore::Array_t applier_workers = shcore::make_array();

#define TSDIFF(prefix, start, end) \
  "TIMESTAMPDIFF(MICROSECOND, " prefix "_" start ", " prefix "_" end ")"

#define TSDIFF_NOW(prefix, start) \
  "TIMESTAMPDIFF(MICROSECOND, " prefix "_" start ", NOW(6))"

  std::string sql;

  sql = "SELECT *";

  if (version >= Version(8, 0, 0)) {
    sql += ",";

    sql += TSDIFF("LAST_APPLIED_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP",
                  "END_APPLY_TIMESTAMP");
    sql += " AS LAST_ORIGINAL_COMMIT_TO_END_APPLY_TIME,";
    sql += TSDIFF("LAST_APPLIED_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP",
                  "END_APPLY_TIMESTAMP");
    sql += " AS LAST_IMMEDIATE_COMMIT_TO_END_APPLY_TIME,";
    sql += TSDIFF("LAST_APPLIED_TRANSACTION", "START_APPLY_TIMESTAMP",
                  "END_APPLY_TIMESTAMP");
    sql += " AS LAST_APPLY_TIME,";

    sql += TSDIFF_NOW("APPLYING_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_ORIGINAL_COMMIT_TO_NOW_TIME,";
    sql += TSDIFF_NOW("APPLYING_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_IMMEDIATE_COMMIT_TO_NOW_TIME";
  }
  sql += " FROM performance_schema.replication_applier_status_by_worker";

  // this can return multiple rows per channel for
  // multi-threaded applier, otherwise just one. If MT, we also
  // get stuff in the coordinator table
  auto result = instance.query(sql);
  auto row = result->fetch_one_named();
  while (row) {
    std::string channel_name = row.get_string("CHANNEL_NAME");
    if (channel_name == "group_replication_recovery") {
      recovery_workers->push_back(applier_status(row));
    }
    if (channel_name == "group_replication_applier" &&
        row.get_string("SERVICE_STATE") != "OFF") {
      applier_workers->push_back(applier_status(row));
    }
    row = result->fetch_one_named();
  }

  sql = "SELECT *";
  if (version >= Version(8, 0, 0)) {
    sql += ",";
    sql += TSDIFF("LAST_PROCESSED_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP",
                  "END_BUFFER_TIMESTAMP");
    sql += " AS LAST_ORIGINAL_COMMIT_TO_END_BUFFER_TIME,";
    sql += TSDIFF("LAST_PROCESSED_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP",
                  "END_BUFFER_TIMESTAMP");
    sql += " AS LAST_IMMEDIATE_COMMIT_TO_END_BUFFER_TIME,";
    sql += TSDIFF("LAST_PROCESSED_TRANSACTION", "START_BUFFER_TIMESTAMP",
                  "END_BUFFER_TIMESTAMP");
    sql += " AS LAST_BUFFER_TIME,";

    sql += TSDIFF_NOW("PROCESSING_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_ORIGINAL_COMMIT_TO_NOW_TIME,";
    sql += TSDIFF_NOW("PROCESSING_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_IMMEDIATE_COMMIT_TO_NOW_TIME";
  }
  sql += " FROM performance_schema.replication_applier_status_by_coordinator";
  result = instance.query(sql);
  row = result->fetch_one_named();
  while (row) {
    std::string channel_name = row.get_string("CHANNEL_NAME");
    if (channel_name == "group_replication_recovery") {
      (*recovery_node)["coordinator"] = coordinator_status(row);
    }
    if (channel_name == "group_replication_applier" &&
        row.get_string("SERVICE_STATE") != "OFF") {
      (*applier_node)["coordinator"] = coordinator_status(row);
    }
    row = result->fetch_one_named();
  }

  sql = "SELECT *";
  if (version >= Version(8, 0, 0)) {
    sql += ",";
    sql += TSDIFF("LAST_QUEUED_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP",
                  "END_QUEUE_TIMESTAMP");
    sql += " AS LAST_ORIGINAL_COMMIT_TO_END_QUEUE_TIME,";
    sql += TSDIFF("LAST_QUEUED_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP",
                  "END_QUEUE_TIMESTAMP");
    sql += " AS LAST_IMMEDIATE_COMMIT_TO_END_QUEUE_TIME,";
    sql += TSDIFF("LAST_QUEUED_TRANSACTION", "START_QUEUE_TIMESTAMP",
                  "END_QUEUE_TIMESTAMP");
    sql += " AS LAST_QUEUE_TIME,";

    sql += TSDIFF_NOW("QUEUEING_TRANSACTION", "ORIGINAL_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_ORIGINAL_COMMIT_TO_NOW_TIME,";
    sql += TSDIFF_NOW("QUEUEING_TRANSACTION", "IMMEDIATE_COMMIT_TIMESTAMP");
    sql += " AS CURRENT_IMMEDIATE_COMMIT_TO_NOW_TIME";
  }
  sql += " FROM performance_schema.replication_connection_status";
  result = instance.query(sql);
  row = result->fetch_one_named();
  while (row) {
    std::string channel_name = row.get_string("CHANNEL_NAME");
    if (channel_name == "group_replication_recovery") {
      (*recovery_node)["connection"] = connection_status(row);
    }
    if (channel_name == "group_replication_applier" &&
        row.get_string("SERVICE_STATE") != "OFF") {
      (*applier_node)["connection"] = connection_status(row);
    }
    row = result->fetch_one_named();
  }

  if (!applier_workers->empty())
    (*applier_node)["workers"] = shcore::Value(applier_workers);
  if (!recovery_workers->empty() && recovering)
    (*recovery_node)["workers"] = shcore::Value(recovery_workers);

  if (!applier_node->empty())
    (*dict)["transactions"] = shcore::Value(applier_node);
  if (!recovery_node->empty() && recovering)
    (*dict)["recovery"] = shcore::Value(recovery_node);

  if (version < Version(8, 0, 2)) {
    (*dict)["version"] = shcore::Value(version.get_full());
  }
}

void Replicaset_status::feed_metadata_info(shcore::Dictionary_t dict,
                                           const Instance_metadata &info) {
  (*dict)["address"] = shcore::Value(info.endpoint);
  (*dict)["role"] = shcore::Value("HA");
}

void Replicaset_status::feed_member_info(
    shcore::Dictionary_t dict, const mysqlshdk::gr::Member &member,
    const mysqlshdk::utils::nullable<bool> &super_read_only,
    const std::vector<std::string> &fence_sysvars,
    bool is_auto_rejoin_running) {
  (*dict)["readReplicas"] = shcore::Value(shcore::make_dict());

  if (!m_extended.is_null() && *m_extended >= 1) {
    // Set fenceSysVars array.
    shcore::Array_t fence_array = shcore::make_array();
    for (const auto &sysvar : fence_sysvars) {
      fence_array->push_back(shcore::Value(sysvar));
    }

    (*dict)["fenceSysVars"] = shcore::Value(fence_array);

    (*dict)["memberId"] = shcore::Value(member.uuid);
    (*dict)["memberRole"] =
        shcore::Value(mysqlshdk::gr::to_string(member.role));
    (*dict)["memberState"] =
        shcore::Value(mysqlshdk::gr::to_string(member.state));
  }

  (*dict)["status"] = shcore::Value(mysqlshdk::gr::to_string(member.state));

  // Set the instance mode (read-only or read-write).
  if (super_read_only.is_null()) {
    // super_read_only is null if it could not be obtained from the instance.
    (*dict)["mode"] = shcore::Value("n/a");
  } else {
    // Set mode to read-only if there is no quorum otherwise according to the
    // instance super_read_only value.
    (*dict)["mode"] =
        shcore::Value((m_no_quorum || *super_read_only) ? "R/O" : "R/W");
  }

  // Display autoRejoinRunning attribute by default for each member, but only
  // if running (true).
  if (is_auto_rejoin_running) {
    (*dict)["autoRejoinRunning"] = shcore::Value(is_auto_rejoin_running);
  }

  if (!member.version.empty()) {
    (*dict)["version"] = shcore::Value(member.version);
  }
}

void Replicaset_status::feed_member_stats(
    shcore::Dictionary_t dict, const mysqlshdk::db::Row_by_name &stats) {
  set_uint(dict, "inQueueCount", stats, "COUNT_TRANSACTIONS_IN_QUEUE");
  set_uint(dict, "checkedCount", stats, "COUNT_TRANSACTIONS_CHECKED");
  set_uint(dict, "conflictsDetectedCount", stats, "COUNT_CONFLICTS_DETECTED");
  set_string(dict, "committedAllMembers", stats,
             "TRANSACTIONS_COMMITTED_ALL_MEMBERS");
  set_string(dict, "lastConflictFree", stats, "LAST_CONFLICT_FREE_TRANSACTION");
  set_uint(dict, "inApplierQueueCount", stats,
           "COUNT_TRANSACTIONS_REMOTE_IN_APPLIER_QUEUE");
  set_uint(dict, "appliedCount", stats, "COUNT_TRANSACTIONS_REMOTE_APPLIED");
  set_uint(dict, "proposedCount", stats, "COUNT_TRANSACTIONS_LOCAL_PROPOSED");
  set_uint(dict, "rollbackCount", stats, "COUNT_TRANSACTIONS_LOCAL_ROLLBACK");
}

namespace {
shcore::Value distributed_progress(
    const mysqlshdk::mysql::IInstance &instance) {
  shcore::Dictionary_t dict = shcore::make_dict();
  mysqlshdk::mysql::Replication_channel channel;
  if (mysqlshdk::mysql::get_channel_status(
          instance, mysqlshdk::gr::k_gr_recovery_channel, &channel)) {
    dict->set("state",
              shcore::Value(mysqlshdk::mysql::to_string(channel.status())));

    if (channel.receiver.last_error.code != 0) {
      dict->set("receiverErrorNumber",
                shcore::Value(channel.receiver.last_error.code));
      dict->set("receiverError",
                shcore::Value(channel.receiver.last_error.message));
    }

    if (!channel.appliers.empty() && channel.appliers[0].last_error.code != 0) {
      dict->set("applierErrorNumber",
                shcore::Value(channel.appliers[0].last_error.code));
      dict->set("applierError",
                shcore::Value(channel.appliers[0].last_error.message));
    }
  }
  return shcore::Value(dict);
}

shcore::Value clone_progress(const mysqlshdk::mysql::IInstance &instance) {
  shcore::Dictionary_t dict = shcore::make_dict();
  mysqlshdk::mysql::Clone_status status =
      mysqlshdk::mysql::check_clone_status(instance);

  dict->set("cloneState", shcore::Value(status.state));
  dict->set("cloneStartTime", shcore::Value(status.begin_time));
  if (status.error_n) dict->set("errorNumber", shcore::Value(status.error_n));
  if (!status.error.empty()) dict->set("error", shcore::Value(status.error));

  auto stage = status.stages[status.current_stage()];
  dict->set("currentStage", shcore::Value(stage.stage));
  dict->set("currentStageState", shcore::Value(stage.state));
  if (stage.work_estimated > 0) {
    dict->set(
        "currentStageProgress",
        shcore::Value(stage.work_completed * 100.0 / stage.work_estimated));
  }

  return shcore::Value(dict);
}

std::pair<std::string, shcore::Value> recovery_status(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &join_timestamp) {
  std::string status;
  shcore::Value info;

  mysqlshdk::gr::Group_member_recovery_status recovery =
      mysqlshdk::gr::detect_recovery_status(instance, join_timestamp);

  switch (recovery) {
    case mysqlshdk::gr::Group_member_recovery_status::DISTRIBUTED:
      status = "Distributed recovery in progress";
      info = distributed_progress(instance);
      break;

    case mysqlshdk::gr::Group_member_recovery_status::DISTRIBUTED_ERROR:
      status = "Distributed recovery error";
      info = distributed_progress(instance);
      break;

    case mysqlshdk::gr::Group_member_recovery_status::CLONE:
      status = "Cloning in progress";
      info = clone_progress(instance);
      break;

    case mysqlshdk::gr::Group_member_recovery_status::CLONE_ERROR:
      status = "Clone error";
      info = clone_progress(instance);
      break;

    case mysqlshdk::gr::Group_member_recovery_status::UNKNOWN:
      status = "Recovery in progress";
      break;

    case mysqlshdk::gr::Group_member_recovery_status::UNKNOWN_ERROR:
      status = "Recovery error";
      break;

    case mysqlshdk::gr::Group_member_recovery_status::DONE_OFFLINE:
    case mysqlshdk::gr::Group_member_recovery_status::DONE_ONLINE:
      return {"", shcore::Value()};
  }

  return {status, info};
}
}  // namespace

shcore::Dictionary_t Replicaset_status::get_topology(
    const std::vector<mysqlshdk::gr::Member> &member_info,
    const mysqlsh::dba::Instance * /* primary_instance */) {
  Member_stats_map member_stats = query_member_stats();

  shcore::Dictionary_t dict = shcore::make_dict();

  auto get_member = [&member_info](const std::string &uuid) {
    for (const auto &m : member_info) {
      if (m.uuid == uuid) return m;
    }
    return mysqlshdk::gr::Member();
  };

  for (const auto &inst : m_instances) {
    shcore::Dictionary_t member = shcore::make_dict();
    mysqlshdk::gr::Member minfo(get_member(inst.uuid));

    mysqlsh::dba::Instance instance(m_member_sessions[inst.endpoint]);

    mysqlshdk::utils::nullable<bool> super_read_only;
    std::vector<std::string> fence_sysvars;
    bool auto_rejoin = false;

    if (instance.get_session()) {
      // Get super_read_only value of each instance to set the mode accurately.
      super_read_only = instance.get_sysvar_bool("super_read_only");

      // Check if auto-rejoin is running.
      auto_rejoin = mysqlshdk::gr::is_running_gr_auto_rejoin(instance);

      if (!m_extended.is_null()) {
        if (*m_extended >= 1) {
          fence_sysvars = instance.get_fence_sysvars();
        }

        if (*m_extended >= 3) {
          collect_local_status(
              member, instance,
              minfo.state == mysqlshdk::gr::Member_state::RECOVERING);
        } else {
          if (minfo.state == mysqlshdk::gr::Member_state::ONLINE)
            collect_basic_local_status(member, instance);
        }

        if (minfo.state == mysqlshdk::gr::Member_state::RECOVERING) {
          std::string status;
          shcore::Value info;
          // Get the join timestamp from the Metadata
          shcore::Value join_time;
          m_cluster->get_metadata_storage()->query_instance_attribute(
              instance.get_uuid(), k_instance_attribute_join_time, &join_time);

          std::tie(status, info) = recovery_status(
              instance,
              join_time.type == shcore::String ? join_time.as_string() : "");
          if (!status.empty()) {
            (*member)["recoveryStatusText"] = shcore::Value(status);
          }
          if (info) (*member)["recovery"] = info;
        }
      }
    } else {
      (*member)["shellConnectError"] =
          shcore::Value(m_member_connect_errors[inst.endpoint]);
    }

    feed_metadata_info(member, inst);
    feed_member_info(member, minfo, super_read_only, fence_sysvars,
                     auto_rejoin);

    if ((!m_extended.is_null() && *m_extended >= 2) &&
        member_stats.find(inst.uuid) != member_stats.end()) {
      shcore::Dictionary_t mdict = member;

      auto dict_for = [mdict](const std::string &key) {
        if (!mdict->has_key(key)) {
          (*mdict)[key] = shcore::Value(shcore::make_dict());
        }
        return mdict->get_map(key);
      };

      if (member_stats[inst.uuid].first) {
        feed_member_stats(dict_for("recovery"), member_stats[inst.uuid].first);
      }
      if (member_stats[inst.uuid].second) {
        feed_member_stats(dict_for("transactions"),
                          member_stats[inst.uuid].second);
      }
    }

    (*dict)[inst.label] = shcore::Value(member);
  }

  return dict;
}

shcore::Dictionary_t Replicaset_status::collect_replicaset_status() {
  shcore::Dictionary_t tmp = shcore::make_dict();
  shcore::Dictionary_t ret = shcore::make_dict();

  auto group_instance = m_cluster->get_target_instance();

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid =
      mysqlshdk::gr::get_group_primary_uuid(*group_instance, nullptr);

  std::string topology_mode =
      !gr_primary_uuid.empty()
          ? mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY)
          : mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  // Set ReplicaSet name
  (*ret)["name"] = shcore::Value(m_replicaset.get_name());
  (*ret)["topologyMode"] = shcore::Value(topology_mode);

  if (!m_extended.is_null() && *m_extended >= 1) {
    (*ret)["groupName"] =
        shcore::Value(m_replicaset.get_cluster()->get_group_name());

    try {
      (*ret)["GRProtocolVersion"] = shcore::Value(
          mysqlshdk::gr::get_group_protocol_version(*group_instance)
              .get_full());
    } catch (const shcore::Exception &error) {
      // The UDF may fail with MySQL Error 1123 if any of the members is
      // RECOVERING or the group does not have quorum In such scenario we cannot
      // provide the "GRProtocolVersion" information
      if (error.code() != ER_CANT_INITIALIZE_UDF) {
        throw;
      }
    }
  }

  auto ssl_mode = group_instance->get_sysvar_string(
      "group_replication_ssl_mode", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  if (ssl_mode)
    (*ret)["ssl"] = shcore::Value(*ssl_mode);
  else
    (*ret)["ssl"] = shcore::Value::Null();

  bool single_primary;
  std::vector<mysqlshdk::gr::Member> member_info(
      mysqlshdk::gr::get_members(*group_instance, &single_primary));

  tmp = check_group_status(*group_instance, member_info);
  (*ret)["statusText"] = shcore::Value(tmp->get_string("statusText"));
  (*ret)["status"] = shcore::Value(tmp->get_string("status"));

  bool has_primary = false;
  std::shared_ptr<mysqlshdk::db::ISession> primary_session;
  {
    if (single_primary) {
      // In single primary mode we need to add the "primary" field
      (*ret)["primary"] = shcore::Value("?");
      for (const auto &member : member_info) {
        if (member.role == mysqlshdk::gr::Member_role::PRIMARY) {
          const Instance_metadata *primary = instance_with_uuid(member.uuid);
          if (primary) {
            auto s = m_member_sessions.find(primary->endpoint);
            if (s != m_member_sessions.end()) {
              has_primary = true;
              primary_session = s->second;
            }
            (*ret)["primary"] = shcore::Value(primary->endpoint);
          }
          break;
        }
      }
    }
  }
  mysqlsh::dba::Instance primary_instance(primary_session);
  (*ret)["topology"] = shcore::Value(
      get_topology(member_info, has_primary ? &primary_instance : nullptr));

  return ret;
}

shcore::Value Replicaset_status::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  // Get the ReplicaSet status
  shcore::Dictionary_t replicaset_dict;

  replicaset_dict = collect_replicaset_status();

  // Check if the ReplicaSet group session is established to an instance with
  // a state different than
  //   - Online R/W
  //   - Online R/O
  //
  // Possibly with the state:
  //
  //   - RECOVERING
  //   - OFFLINE
  //   - ERROR
  //
  // If that's the case, a warning must be added to the resulting JSON object
  {
    auto group_instance = m_cluster->get_target_instance();

    auto state = get_replication_group_state(
        *group_instance, get_gr_instance_type(*group_instance));

    bool show_warning = (state.source_state != ManagedInstance::OnlineRW &&
                         state.source_state != ManagedInstance::OnlineRO);

    if (show_warning) {
      std::string warning =
          "The cluster description may be inaccurate as it was generated from "
          "an instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*replicaset_dict)["warning"] = shcore::Value(warning);
    }
  }

  return shcore::Value(replicaset_dict);
}

void Replicaset_status::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Replicaset_status::finish() {
  // Reset all auxiliary (temporary) data used for the operation execution.
  m_member_sessions.clear();
  m_instances.clear();
  m_member_connect_errors.clear();
}

}  // namespace dba
}  // namespace mysqlsh
