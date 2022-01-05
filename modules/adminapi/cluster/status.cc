/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/status.h"
#include <algorithm>
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/common_status.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

namespace {
template <typename R>
inline bool set_uint(shcore::Dictionary_t dict, const std::string &prop,
                     const R &row, const std::string &field) {
  if (row.has_field(field)) {
    if (row.is_null(field)) {
      (*dict)[prop] = shcore::Value::Null();
    } else {
      (*dict)[prop] = shcore::Value(row.get_uint(field));
    }
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
    if (row.is_null(field)) {
      (*dict)[prop] = shcore::Value::Null();
    } else {
      std::string field_str = row.get_string(field);

      // Strip any newline from the string, especially important for GTID-sets
      field_str.erase(std::remove(field_str.begin(), field_str.end(), '\n'),
                      field_str.end());

      (*dict)[prop] = shcore::Value(field_str);
    }
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
      if (shcore::str_beginswith(ts, "0000-00-00 00:00:00")) {
        (*dict)[prop] = shcore::Value("");
      } else {
        (*dict)[prop] = shcore::Value(ts);
      }
    }
    return true;
  }
  return false;
}

}  // namespace

Status::Status(const Cluster_impl &cluster,
               const mysqlshdk::utils::nullable<uint64_t> &extended)
    : m_cluster(cluster), m_extended(extended) {}

Status::~Status() {}

void Status::connect_to_members() {
  auto ipool = current_ipool();

  for (const auto &inst : m_instances) {
    try {
      m_member_sessions[inst.endpoint] =
          ipool->connect_unchecked_endpoint(inst.endpoint);
    } catch (const shcore::Error &e) {
      m_member_connect_errors[inst.endpoint] = e.format();
    }
  }
}

shcore::Dictionary_t Status::check_group_status(
    const mysqlsh::dba::Instance &instance,
    const std::vector<mysqlshdk::gr::Member> &members, bool has_quorum) {
  shcore::Dictionary_t dict = shcore::make_dict();

  m_no_quorum = !has_quorum;

  int total_in_group = 0;
  int online_count = 0;
  int quorum_size = 0;
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

    total_in_group++;
    if (member.state == mysqlshdk::gr::Member_state::ONLINE ||
        member.state == mysqlshdk::gr::Member_state::RECOVERING)
      quorum_size++;
    if (member.state == mysqlshdk::gr::Member_state::ONLINE) online_count++;
  }

  size_t number_of_failures_tolerated =
      quorum_size > 0 ? (quorum_size - 1) / 2 : 0;

  Cluster_status rs_status;
  std::string desc_status;

  if (online_count == 0) {
    rs_status = Cluster_status::ERROR;
    if (has_quorum)
      desc_status = "There are no ONLINE members in the cluster.";
    else
      desc_status = "Cluster has no quorum as visible from '" +
                    instance.descr() +
                    "' and no ONLINE members that can be used to restore it.";
  } else if (!has_quorum) {
    rs_status = Cluster_status::NO_QUORUM;
    desc_status = "Cluster has no quorum as visible from '" + instance.descr() +
                  "' and cannot process write transactions.";
  } else {
    if (m_cluster.is_fenced_from_writes()) {
      rs_status = Cluster_status::FENCED_WRITES;

      desc_status = "Cluster is fenced from Write Traffic.";
    } else if (m_cluster.is_cluster_set_member() &&
               m_cluster.is_invalidated()) {
      rs_status = Cluster_status::INVALIDATED;

      desc_status = "Cluster was invalidated by the ClusterSet it belongs to.";
    } else if (number_of_failures_tolerated == 0) {
      rs_status = Cluster_status::OK_NO_TOLERANCE;

      desc_status = "Cluster is NOT tolerant to any failures.";
    } else {
      if (missing_from_group > 0 || total_in_group != quorum_size) {
        rs_status = Cluster_status::OK_PARTIAL;
      } else {
        rs_status = Cluster_status::OK;
      }

      if (number_of_failures_tolerated == 1) {
        desc_status = "Cluster is ONLINE and can tolerate up to ONE failure.";
      } else {
        desc_status = "Cluster is ONLINE and can tolerate up to " +
                      std::to_string(number_of_failures_tolerated) +
                      " failures.";
      }
    }
  }

  if (static_cast<int>(m_instances.size()) > online_count) {
    if (m_instances.size() - online_count == 1) {
      desc_status.append(" 1 member is not active.");
    } else {
      desc_status.append(" " +
                         std::to_string(m_instances.size() - online_count) +
                         " members are not active.");
    }
  }

  (*dict)["statusText"] = shcore::Value(desc_status);
  (*dict)["status"] = shcore::Value(to_string(rs_status));

  return dict;
}

const Instance_metadata *Status::instance_with_uuid(const std::string &uuid) {
  for (const auto &i : m_instances) {
    if (i.uuid == uuid) return &i;
  }
  return nullptr;
}

Member_stats_map Status::query_member_stats() {
  Member_stats_map stats;
  auto group_instance = m_cluster.get_cluster_server();

  if (group_instance) {
    try {
      auto member_stats = group_instance->query(
          "SELECT * FROM performance_schema.replication_group_member_stats");

      while (auto row = member_stats->fetch_one_named()) {
        std::string channel = row.get_string("CHANNEL_NAME");
        if (channel == "group_replication_applier") {
          stats[row.get_string("MEMBER_ID")].second =
              mysqlshdk::db::Row_by_name(row);
        } else if (channel == "group_replication_recovery") {
          stats[row.get_string("MEMBER_ID")].first =
              mysqlshdk::db::Row_by_name(row);
        }
      }
    } catch (const mysqlshdk::db::Error &e) {
      throw shcore::Exception::mysql_error_with_code(
          group_instance->descr() + ": " + e.what(), e.code());
    }
  }

  return stats;
}

void Status::collect_last_error(shcore::Dictionary_t dict,
                                const mysqlshdk::db::Row_ref_by_name &row,
                                const std::string &prefix,
                                const std::string &key_prefix) {
  if (row.has_field(prefix + "ERROR_NUMBER") &&
      row.get_uint(prefix + "ERROR_NUMBER") != 0) {
    set_uint(dict, key_prefix + "Errno", row, prefix + "ERROR_NUMBER");
    set_string(dict, key_prefix + "Error", row, prefix + "ERROR_MESSAGE");
    set_ts(dict, key_prefix + "ErrorTimestamp", row,
           prefix + "ERROR_TIMESTAMP");
  }
}

shcore::Value Status::collect_last(const mysqlshdk::db::Row_ref_by_name &row,
                                   const std::string &prefix,
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

shcore::Value Status::collect_current(const mysqlshdk::db::Row_ref_by_name &row,
                                      const std::string &prefix,
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

shcore::Value Status::connection_status(
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

  if (auto v = collect_current(row, "QUEUEING", "QUEUE")) {
    (*dict)["currentlyQueueing"] = v;
  }

  return shcore::Value(dict);
}

shcore::Value Status::coordinator_status(
    const mysqlshdk::db::Row_ref_by_name &row) {
  shcore::Dictionary_t dict = shcore::make_dict();

  set_uint(dict, "threadId", row, "THREAD_ID");

  collect_last_error(dict, row);

  auto last = collect_last(row, "LAST_PROCESSED", "BUFFER");
  if (!last.as_map()->empty()) (*dict)["lastProcessed"] = shcore::Value(last);

  if (auto v = collect_current(row, "PROCESSING", "BUFFER")) {
    (*dict)["currentlyProcessing"] = v;
  }

  return shcore::Value(dict);
}

shcore::Value Status::applier_status(
    const mysqlshdk::db::Row_ref_by_name &row) {
  shcore::Dictionary_t dict = shcore::make_dict();
  set_uint(dict, "threadId", row, "THREAD_ID");

  collect_last_error(dict, row);

  auto last = collect_last(row, "LAST_APPLIED", "APPLY");
  if (!last.as_map()->empty()) (*dict)["lastApplied"] = shcore::Value(last);

  set_string(dict, "lastSeen", row, "LAST_SEEN_TRANSACTION");

  if (auto v = collect_current(row, "APPLYING", "APPLY")) {
    (*dict)["currentlyApplying"] = v;
  }

  return shcore::Value(dict);
}

/**
 * Similar to collect_local_status(), but only includes basic/important
 * stats that should be displayed in the default output.
 */
void Status::collect_basic_local_status(shcore::Dictionary_t dict,
                                        const mysqlsh::dba::Instance &instance,
                                        bool is_primary) {
  using mysqlshdk::utils::Version;

  auto version = instance.get_version();

  shcore::Dictionary_t applier_node = shcore::make_dict();
  shcore::Array_t applier_workers = shcore::make_array();

#define TSDIFF(prefix, start, end) \
  "TIMEDIFF(" prefix "_" end ", " prefix "_" start ")"

  std::string sql;

  if (version >= Version(8, 0, 0)) {
    if (m_cluster.is_cluster_set_member()) {
      // PRIMARY of PC has no relevant replication lag info
      // PRIMARY of RC shows lag from clusterset_replication channel
      // SECONDARY members show replication from gr_applier channel
      std::string channel_name;

      if (is_primary) {
        if (!m_cluster.is_primary_cluster()) {
          channel_name = k_clusterset_async_channel_name;
        }
      } else {
        channel_name = mysqlshdk::gr::k_gr_applier_channel;
      }

      if (!channel_name.empty()) {
        mysqlshdk::mysql::Replication_channel channel;

        if (mysqlshdk::mysql::get_channel_status(instance, channel_name,
                                                 &channel)) {
          dict->set("replicationLagFromOriginalSource",
                    shcore::Value(channel.repl_lag_from_original));
          dict->set("replicationLagFromImmediateSource",
                    shcore::Value(channel.repl_lag_from_immediate));
        } else {
          dict->set("replicationLagFromOriginalSource", shcore::Value::Null());
          dict->set("replicationLagFromImmediateSource", shcore::Value::Null());
        }
      }
    } else {
      sql = "SELECT ";
      sql +=
          "IF(c.LAST_QUEUED_TRANSACTION=''"
          " OR c.LAST_QUEUED_TRANSACTION=a.LAST_APPLIED_TRANSACTION"
          " OR a.LAST_APPLIED_TRANSACTION_END_APPLY_TIMESTAMP <"
          "  a.LAST_APPLIED_TRANSACTION_ORIGINAL_COMMIT_TIMESTAMP,"
          "NULL, " TSDIFF("LAST_APPLIED_TRANSACTION",
                          "ORIGINAL_COMMIT_TIMESTAMP",
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

        if (!lag.empty() && lag != "00:00:00.000000") {
          (*dict)["replicationLag"] = shcore::Value(lag);
        } else {
          (*dict)["replicationLag"] = shcore::Value::Null();
        }
      }
    }
  }

#undef TSDIFF
}

/**
 * Collect per instance replication stats collected from performance_schema
 * tables.
 */
void Status::collect_local_status(shcore::Dictionary_t dict,
                                  const mysqlsh::dba::Instance &instance,
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

  if (!applier_workers->empty()) {
    (*applier_node)["workers"] = shcore::Value(applier_workers);
  }

  if (!recovery_workers->empty() && recovering) {
    (*recovery_node)["workers"] = shcore::Value(recovery_workers);
  }

  if (!applier_node->empty()) {
    (*dict)["transactions"] = shcore::Value(applier_node);
  }

  if (!recovery_node->empty() && recovering) {
    (*dict)["recovery"] = shcore::Value(recovery_node);
  }

  (*dict)["version"] = shcore::Value(version.get_full());
}

void Status::feed_metadata_info(shcore::Dictionary_t dict,
                                const Instance_metadata &info) {
  (*dict)["address"] = shcore::Value(info.endpoint);
  (*dict)["role"] = shcore::Value("HA");
}

void Status::feed_member_info(
    shcore::Dictionary_t dict, const mysqlshdk::gr::Member &member,
    const mysqlshdk::utils::nullable<bool> &super_read_only,
    const std::vector<std::string> &fence_sysvars,
    mysqlshdk::gr::Member_state self_state, bool is_auto_rejoin_running) {
  (*dict)["readReplicas"] = shcore::Value(shcore::make_dict());

  if (!m_extended.is_null() && *m_extended >= 1) {
    // Set fenceSysVars array.
    shcore::Array_t fence_array = shcore::make_array();
    for (const auto &sysvar : fence_sysvars) {
      fence_array->push_back(shcore::Value(sysvar));
    }

    (*dict)["fenceSysVars"] = shcore::Value(fence_array);

    (*dict)["memberId"] = shcore::Value(member.uuid);
  }

  // memberRole instance Role as reported by GR (Primary/Secondary)
  (*dict)["memberRole"] = shcore::Value(mysqlshdk::gr::to_string(member.role));

  if ((!m_extended.is_null() && *m_extended >= 1) ||
      member.state != self_state) {
    // memberState is from the point of view of the member itself
    (*dict)["memberState"] =
        shcore::Value(mysqlshdk::gr::to_string(self_state));
  }

  // status is the status from the point of view of the quorum
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

void Status::feed_member_stats(shcore::Dictionary_t dict,
                               const mysqlshdk::db::Row_by_name &stats) {
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

    for (const auto &applier : channel.appliers) {
      if (applier.last_error.code != 0) {
        dict->set("applierErrorNumber", shcore::Value(applier.last_error.code));
        dict->set("applierError", shcore::Value(applier.last_error.message));
        break;
      }
    }
  }
  return shcore::Value(dict);
}

shcore::Value clone_progress(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &begin_time) {
  shcore::Dictionary_t dict = shcore::make_dict();
  mysqlshdk::mysql::Clone_status status =
      mysqlshdk::mysql::check_clone_status(instance, begin_time);

  dict->set("cloneState", shcore::Value(status.state));
  dict->set("cloneStartTime", shcore::Value(status.begin_time));
  if (status.error_n) dict->set("errorNumber", shcore::Value(status.error_n));
  if (!status.error.empty()) dict->set("error", shcore::Value(status.error));

  if (!status.stages.empty()) {
    auto stage = status.stages[status.current_stage()];
    dict->set("currentStage", shcore::Value(stage.stage));
    dict->set("currentStageState", shcore::Value(stage.state));

    if (stage.work_estimated > 0) {
      dict->set(
          "currentStageProgress",
          shcore::Value(stage.work_completed * 100.0 / stage.work_estimated));
    }
  } else {
    // When the check for clone progress is executed right after clone has
    // started there's a tiny gap of time on which P_S.clone_progress hasn't
    // been populated yet. If we ran into that scenario, we must manually
    // populate the dictionary fields with undefined values to avoid a segfault.
    dict->set("currentStage", shcore::Value());
    dict->set("currentStageState", shcore::Value());
    dict->set("currentStageProgress", shcore::Value());
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
      info = clone_progress(instance, join_timestamp);
      break;

    case mysqlshdk::gr::Group_member_recovery_status::CLONE_ERROR:
      status = "Clone error";
      info = clone_progress(instance, join_timestamp);
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

struct Instance_metadata_info {
  Instance_metadata md;
  std::string actual_server_uuid;
};

void check_parallel_appliers(
    shcore::Array_t issues, const mysqlshdk::utils::Version &instance_version,
    const Parallel_applier_options &parallel_applier_options) {
  auto current_values =
      parallel_applier_options.get_current_settings(instance_version);
  auto required_values =
      parallel_applier_options.get_required_values(instance_version);

  for (const auto &setting : required_values) {
    std::string current_value = current_values[std::get<0>(setting)].get_safe();

    if (!current_value.empty() && current_value != std::get<1>(setting)) {
      issues->push_back(shcore::Value(
          "NOTE: The required parallel-appliers settings are not enabled "
          "on the instance. Use dba.configureInstance() to fix it."));
      break;
    }
  }
}

void check_channel_error(shcore::Array_t issues,
                         const std::string &channel_label,
                         const mysqlshdk::mysql::Replication_channel &channel) {
  using mysqlshdk::mysql::Replication_channel;

  switch (channel.status()) {
    case Replication_channel::OFF:
    case Replication_channel::ON:
    case Replication_channel::RECEIVER_OFF:
    case Replication_channel::APPLIER_OFF:
    case Replication_channel::CONNECTING:
      break;
    case Replication_channel::CONNECTION_ERROR:
      issues->push_back(shcore::Value(
          "ERROR: " + channel_label +
          " channel receiver stopped with an error: " +
          mysqlshdk::mysql::to_string(channel.receiver.last_error)));
      break;
    case Replication_channel::APPLIER_ERROR:
      for (const auto &a : channel.appliers) {
        if (a.last_error.code != 0) {
          issues->push_back(
              shcore::Value("ERROR: " + channel_label +
                            " channel applier stopped with an error: " +
                            mysqlshdk::mysql::to_string(a.last_error)));
          break;
        }
      }
      break;
  }
}

void check_unrecognized_channels(shcore::Array_t issues, Instance *instance,
                                 bool is_cluster_set_member) {
  if (instance) {
    auto channels =
        mysqlshdk::mysql::get_incoming_channel_names(*instance, false);
    for (const std::string &name : channels) {
      if (!(name == mysqlshdk::gr::k_gr_applier_channel ||
            name == mysqlshdk::gr::k_gr_recovery_channel ||
            (is_cluster_set_member &&
             name == k_clusterset_async_channel_name))) {
        issues->push_back(shcore::Value(
            "ERROR: Unrecognized replication channel '" + name +
            "' found. Unmanaged replication channels are not supported."));
      }
    }
  }
}

shcore::Array_t instance_diagnostics(
    Instance *instance, const Cluster_impl *cluster,
    const Instance_metadata_info &instance_md,
    const mysqlshdk::mysql::Replication_channel &recovery_channel,
    const mysqlshdk::mysql::Replication_channel &applier_channel,
    const mysqlshdk::utils::nullable<bool> &super_read_only,
    const mysqlshdk::gr::Member &minfo, mysqlshdk::gr::Member_state self_state,
    const Parallel_applier_options &parallel_applier_options) {
  shcore::Array_t issues = shcore::make_array();

  // split-brain
  if ((minfo.state == mysqlshdk::gr::Member_state::UNREACHABLE ||
       minfo.state == mysqlshdk::gr::Member_state::MISSING) &&
      (self_state == mysqlshdk::gr::Member_state::ONLINE ||
       self_state == mysqlshdk::gr::Member_state::RECOVERING)) {
    issues->push_back(shcore::Value(
        "ERROR: split-brain! Instance is not part of the majority group, "
        "but has state " +
        to_string(self_state)));
  }
  // SECONDARY (or PRIMARY of replica cluster) that's not SRO
  if (cluster->is_cluster_set_member() &&
      !(minfo.role == mysqlshdk::gr::Member_role::PRIMARY &&
        cluster->is_primary_cluster()) &&
      self_state != mysqlshdk::gr::Member_state::RECOVERING &&
      !super_read_only.is_null() && !*super_read_only) {
    issues->push_back(shcore::Value(
        "WARNING: Instance is NOT the global PRIMARY but super_read_only "
        "option is OFF. Errant transactions and inconsistencies may be "
        "accidentally introduced."));
  } else if (minfo.role == mysqlshdk::gr::Member_role::SECONDARY &&
             self_state != mysqlshdk::gr::Member_state::RECOVERING &&
             !super_read_only.is_null() && !*super_read_only) {
    issues->push_back(
        shcore::Value("WARNING: Instance is NOT a PRIMARY but super_read_only "
                      "option is OFF."));
  }

  check_channel_error(issues, "GR Recovery", recovery_channel);
  check_channel_error(issues, "GR Applier", applier_channel);

  check_unrecognized_channels(issues, instance,
                              cluster->is_cluster_set_member());

  if (self_state == mysqlshdk::gr::Member_state::OFFLINE) {
    issues->push_back(shcore::Value("NOTE: group_replication is stopped."));
  } else if (self_state == mysqlshdk::gr::Member_state::ERROR) {
    issues->push_back(
        shcore::Value("ERROR: group_replication has stopped with an error."));
  }

  if (instance_md.actual_server_uuid != instance_md.md.uuid) {
    issues->push_back(
        shcore::Value("WARNING: server_uuid for instance has changed from "
                      "its last known value. Use cluster.rescan() to update "
                      "the metadata."));
  } else if (instance_md.md.cluster_id.empty() &&
             self_state != mysqlshdk::gr::Member_state::RECOVERING) {
    issues->push_back(
        shcore::Value("WARNING: Instance is not managed by InnoDB cluster. Use "
                      "cluster.rescan() to repair."));
  } else if (instance_md.md.server_id == 0) {
    issues->push_back(shcore::Value(
        "NOTE: instance server_id is not registered in the metadata. Use "
        "cluster.rescan() to update the metadata."));
  }

  // Check if parallel-appliers are not configured. The requirement was
  // introduced in 8.0.23 so only check if the version is equal or higher to
  // that.
  mysqlshdk::utils::Version instance_version;
  if (minfo.version.empty()) {
    if (instance) instance_version = instance->get_version();
  } else {
    instance_version = mysqlshdk::utils::Version(minfo.version);
  }

  if (instance_version >= mysqlshdk::utils::Version(8, 0, 23)) {
    check_parallel_appliers(issues, instance_version, parallel_applier_options);
  }

  return issues;
}

void check_comm_protocol_upgrade_possible(
    shcore::Array_t issues,
    const std::vector<mysqlshdk::gr::Member> &member_info,
    const mysqlshdk::utils::Version &protocol_version) {
  std::vector<mysqlshdk::utils::Version> all_protocol_versions;
  // Check if protocol upgrade is possible
  // If there's no version field in member_info it's because it was queried on
  // an old version, but in that case it means there's at least 1 server
  // < 8.0.16 so an upgrade isn't possible anyway
  for (const auto &m : member_info) {
    if (!m.version.empty()) {
      auto version = mysqlshdk::gr::get_max_supported_group_protocol_version(
          mysqlshdk::utils::Version(m.version));

      all_protocol_versions.emplace_back(version);
    }
  }

  if (!all_protocol_versions.empty()) {
    std::sort(all_protocol_versions.begin(), all_protocol_versions.end());

    // get the lowest protocol version supported by the group members
    auto highest_possible_version = all_protocol_versions.front();
    if (highest_possible_version > protocol_version) {
      issues->emplace_back(shcore::Value(
          "Group communication protocol in use is version " +
          protocol_version.get_full() + " but it is possible to upgrade to " +
          highest_possible_version.get_full() +
          ". Message fragmentation for large transactions can only be "
          "enabled after upgrade. Use "
          "Cluster.rescan({upgradeCommProtocol:true}) to upgrade."));
    }
  }
}

shcore::Array_t group_diagnostics(
    const Cluster_impl *cluster,
    const std::vector<mysqlshdk::gr::Member> &member_info,
    const mysqlshdk::utils::Version &protocol_version) {
  shcore::Array_t issues = shcore::make_array();

  if (protocol_version && !member_info.empty())
    check_comm_protocol_upgrade_possible(issues, member_info, protocol_version);

  switch (cluster->cluster_availability()) {
    case Cluster_availability::NO_QUORUM:
      issues->push_back(
          shcore::Value("ERROR: Could not find ONLINE members forming a "
                        "quorum. Cluster will be unable to perform updates "
                        "until it's restored."));
      break;
    case Cluster_availability::OFFLINE:
    case Cluster_availability::SOME_UNREACHABLE:
      issues->push_back(shcore::Value(
          "ERROR: Cluster members are reachable but they're all OFFLINE."));
      break;
    case Cluster_availability::UNREACHABLE:
      issues->push_back(shcore::Value(
          "ERROR: Could not connect to any ONLINE members but "
          "there are unreachable instances that could still be ONLINE."));
      break;
    case Cluster_availability::ONLINE:
    case Cluster_availability::ONLINE_NO_PRIMARY:
      break;
  }

  if (cluster->is_fenced_from_writes()) {
    issues->push_back(
        shcore::Value("WARNING: Cluster is fenced from Write traffic. Use "
                      "cluster.unfenceWrites() to unfence the Cluster."));
  }

  return issues;
}

shcore::Array_t validate_instance_recovery_user(
    std::shared_ptr<Instance> instance,
    const std::map<std::string, std::string> &endpoints,
    const std::string &actual_server_uuid, mysqlshdk::gr::Member_state state) {
  auto issues = shcore::make_array();

  if (state != mysqlshdk::gr::Member_state::ONLINE &&
      state != mysqlshdk::gr::Member_state::RECOVERING)
    return issues;

  if (instance && !endpoints.empty()) {
    auto it = endpoints.find(actual_server_uuid);
    if (it != endpoints.end()) {
      std::string recovery_user = mysqlshdk::mysql::get_replication_user(
          *instance, mysqlshdk::gr::k_gr_recovery_channel);

      // it->second is recovery user from MD
      if (!recovery_user.empty()) {
        bool recovery_is_valid =
            shcore::str_beginswith(
                recovery_user,
                mysqlshdk::gr::k_group_recovery_old_user_prefix) ||
            shcore::str_beginswith(recovery_user,
                                   mysqlshdk::gr::k_group_recovery_user_prefix);

        if (it->second.empty() && recovery_is_valid) {
          issues->push_back(
              shcore::Value("WARNING: The replication recovery account in use "
                            "by the instance is not stored in the metadata. "
                            "Use Cluster.rescan() to update the metadata."));
        } else if (it->second != recovery_user) {
          if (recovery_is_valid) {
            issues->push_back(shcore::Value(
                "WARNING: The replication recovery account in use "
                "by the instance is not stored in the metadata. "
                "Use Cluster.rescan() to update the metadata."));
          } else {
            issues->push_back(shcore::Value(
                "WARNING: Unsupported recovery account '" + recovery_user +
                "' has been found for instance '" + instance->descr() +
                "'. Operations such as "
                "Cluster.resetRecoveryAccountsPassword() and "
                "Cluster.addInstance() may fail. Please remove and add "
                "the instance back to the Cluster to ensure a supported "
                "recovery "
                "account is used."));
          }
        }
      } else {
        issues->push_back(shcore::Value(
            "WARNING: Recovery user account not found for server address: " +
            instance->descr() + " with UUID " + instance->get_uuid()));
      }
    }
  }

  return issues;
}

}  // namespace

shcore::Dictionary_t Status::get_topology(
    const std::vector<mysqlshdk::gr::Member> &member_info) {
  using mysqlshdk::gr::Member_role;
  using mysqlshdk::gr::Member_state;
  using mysqlshdk::mysql::Replication_channel;

  Member_stats_map member_stats = query_member_stats();

  shcore::Dictionary_t dict = shcore::make_dict();

  auto get_member = [&member_info](const std::string &uuid) {
    for (const auto &m : member_info) {
      if (m.uuid == uuid) return m;
    }
    return mysqlshdk::gr::Member();
  };

  std::vector<Instance_metadata_info> instances;

  // add placeholders for unmanaged members
  for (const auto &m : member_info) {
    bool found = false;
    for (const auto &i : m_instances) {
      if (i.uuid == m.uuid) {
        Instance_metadata_info mdi;
        mdi.md = i;
        mdi.actual_server_uuid = m.uuid;
        instances.emplace_back(std::move(mdi));
        found = true;
        break;
      }
    }
    if (!found) {
      // if instance in MD was not found by uuid, then search by address
      for (const auto &i : m_instances) {
        if (i.address == m.host + ":" + std::to_string(m.port)) {
          log_debug(
              "Instance with address=%s is in group, but UUID doesn't match "
              "group=%s MD=%s",
              i.endpoint.c_str(), m.uuid.c_str(), i.uuid.c_str());

          Instance_metadata_info mdi;
          mdi.md = i;
          mdi.actual_server_uuid = m.uuid;
          instances.emplace_back(std::move(mdi));
          found = true;
          break;
        }
      }
    }
    if (!found) {
      Instance_metadata_info mdi;
      mdi.md.address = m.host + ":" + std::to_string(m.port);
      mdi.md.label = mdi.md.address;
      mdi.md.uuid = m.uuid;
      mdi.md.endpoint = mdi.md.address;
      mdi.actual_server_uuid = mdi.md.uuid;

      log_debug("Instance %s with uuid=%s found in group but not in MD",
                mdi.md.address.c_str(), m.uuid.c_str());

      auto group_instance = m_cluster.get_cluster_server();

      mysqlshdk::db::Connection_options opts(mdi.md.endpoint);
      mysqlshdk::db::Connection_options group_session_copts(
          group_instance->get_connection_options());
      opts.set_login_options_from(group_session_copts);
      try {
        m_member_sessions[mdi.md.endpoint] = Instance::connect(opts);
      } catch (const shcore::Error &e) {
        m_member_connect_errors[mdi.md.endpoint] = e.format();
      }

      instances.emplace_back(std::move(mdi));
    }
  }
  // look for instances in MD but not in group
  for (const auto &i : m_instances) {
    bool found = false;
    for (const auto &m : member_info) {
      if (m.uuid == i.uuid ||
          i.endpoint == m.host + ":" + std::to_string(m.port)) {
        found = true;
        break;
      }
    }
    if (!found) {
      log_debug("Instance with uuid=%s address=%s is in MD but not in group",
                i.uuid.c_str(), i.endpoint.c_str());

      auto &instance(m_member_sessions[i.endpoint]);

      Instance_metadata_info mdi;
      mdi.md = i;
      if (instance) {
        mdi.actual_server_uuid = instance->get_uuid();
      } else {
        // fallback to assuming the server_uuid is OK if we can't connect to
        // the instance
        mdi.actual_server_uuid = i.uuid;
      }
      instances.emplace_back(std::move(mdi));
    }
  }

  std::map<std::string, std::string> endpoints;
  if (m_cluster.get_metadata_storage()->real_version().get_major() > 1) {
    endpoints =
        m_cluster.get_metadata_storage()->get_instances_with_recovery_accounts(
            m_cluster.get_id());
  }

  for (const auto &inst : instances) {
    shcore::Dictionary_t member = shcore::make_dict();
    mysqlshdk::gr::Member minfo(get_member(inst.actual_server_uuid));
    mysqlshdk::gr::Member_state self_state =
        mysqlshdk::gr::Member_state::MISSING;

    auto &instance(m_member_sessions[inst.md.endpoint]);

    mysqlshdk::utils::nullable<bool> super_read_only;
    std::vector<std::string> fence_sysvars;
    bool auto_rejoin = false;

    Replication_channel applier_channel;
    Replication_channel recovery_channel;

    Parallel_applier_options parallel_applier_options;

    if (instance) {
      // Get the current parallel-applier options
      parallel_applier_options = Parallel_applier_options(*instance);

      // Get super_read_only value of each instance to set the mode accurately.
      super_read_only = instance->get_sysvar_bool("super_read_only");

      // Check if auto-rejoin is running.
      auto_rejoin = mysqlshdk::gr::is_running_gr_auto_rejoin(*instance);

      self_state = mysqlshdk::gr::get_member_state(*instance);

      minfo.version = instance->get_version().get_base();

      if (!m_extended.is_null()) {
        if (*m_extended >= 1) {
          fence_sysvars = instance->get_fence_sysvars();

          auto workers = parallel_applier_options.replica_parallel_workers;

          if (parallel_applier_options.replica_parallel_workers.get_safe() >
              0) {
            (*member)["applierWorkerThreads"] = shcore::Value(*workers);
          }
        }

        if (*m_extended >= 3) {
          collect_local_status(member, *instance,
                               minfo.state == Member_state::RECOVERING);
        }
        if (minfo.state == Member_state::ONLINE)
          collect_basic_local_status(member, *instance,
                                     minfo.role == Member_role::PRIMARY);

        shcore::Value recovery_info;
        if (minfo.state == Member_state::RECOVERING) {
          std::string status;
          // Get the join timestamp from the Metadata
          shcore::Value join_time;
          m_cluster.get_metadata_storage()->query_instance_attribute(
              instance->get_uuid(), k_instance_attribute_join_time, &join_time);

          std::tie(status, recovery_info) = recovery_status(
              *instance,
              join_time.type == shcore::String ? join_time.as_string() : "");
          if (!status.empty()) {
            (*member)["recoveryStatusText"] = shcore::Value(status);
          }
        }

        // Include recovery channel info if RECOVERING or if there's an error
        if (mysqlshdk::mysql::get_channel_status(
                *instance, mysqlshdk::gr::k_gr_recovery_channel,
                &recovery_channel) &&
            *m_extended > 0) {
          if (minfo.state == Member_state::RECOVERING ||
              recovery_channel.status() != Replication_channel::OFF) {
            mysqlshdk::mysql::Replication_channel_master_info master_info;
            mysqlshdk::mysql::Replication_channel_relay_log_info relay_info;

            mysqlshdk::mysql::get_channel_info(
                *instance, mysqlshdk::gr::k_gr_recovery_channel, &master_info,
                &relay_info);

            if (!recovery_info) recovery_info = shcore::Value::new_map();

            (*recovery_info.as_map())["recoveryChannel"] = shcore::Value(
                channel_status(&recovery_channel, &master_info, &relay_info, "",
                               *m_extended - 1, true, false));
          }
        }
        if (recovery_info) (*member)["recovery"] = recovery_info;

        // Include applier channel info ONLINE and channel not ON
        // or != RECOVERING and channel not OFF
        if (mysqlshdk::mysql::get_channel_status(
                *instance, mysqlshdk::gr::k_gr_applier_channel,
                &applier_channel) &&
            *m_extended > 0) {
          if ((self_state == Member_state::ONLINE &&
               applier_channel.status() != Replication_channel::ON) ||
              (self_state != Member_state::RECOVERING &&
               self_state != Member_state::ONLINE &&
               applier_channel.status() != Replication_channel::OFF)) {
            mysqlshdk::mysql::Replication_channel_master_info master_info;
            mysqlshdk::mysql::Replication_channel_relay_log_info relay_info;

            mysqlshdk::mysql::get_channel_info(
                *instance, mysqlshdk::gr::k_gr_applier_channel, &master_info,
                &relay_info);

            (*member)["applierChannel"] = shcore::Value(
                channel_status(&applier_channel, &master_info, &relay_info, "",
                               *m_extended - 1, false, false));
          }
        }
      }

    } else {
      (*member)["shellConnectError"] =
          shcore::Value(m_member_connect_errors[inst.md.endpoint]);
    }
    feed_metadata_info(member, inst.md);
    feed_member_info(member, minfo, super_read_only, fence_sysvars, self_state,
                     auto_rejoin);

    shcore::Array_t issues = instance_diagnostics(
        instance.get(), &m_cluster, inst, recovery_channel, applier_channel,
        super_read_only, minfo, self_state, parallel_applier_options);

    auto ret_val = validate_instance_recovery_user(
        instance, endpoints, inst.actual_server_uuid, self_state);

    issues->insert(issues->end(), std::make_move_iterator(ret_val->begin()),
                   std::make_move_iterator(ret_val->end()));

    if (issues && !issues->empty()) {
      (*member)["instanceErrors"] = shcore::Value(issues);
    }

    if ((!m_extended.is_null() && *m_extended >= 2) &&
        member_stats.find(inst.md.uuid) != member_stats.end()) {
      shcore::Dictionary_t mdict = member;

      auto dict_for = [mdict](const std::string &key) {
        if (!mdict->has_key(key)) {
          (*mdict)[key] = shcore::Value(shcore::make_dict());
        }
        return mdict->get_map(key);
      };

      if (member_stats[inst.md.uuid].first) {
        feed_member_stats(dict_for("recovery"),
                          member_stats[inst.md.uuid].first);
      }
      if (member_stats[inst.md.uuid].second) {
        feed_member_stats(dict_for("transactions"),
                          member_stats[inst.md.uuid].second);
      }
    }

    (*dict)[inst.md.label] = shcore::Value(member);
  }

  return dict;
}

shcore::Dictionary_t Status::collect_replicaset_status() {
  shcore::Dictionary_t tmp = shcore::make_dict();
  shcore::Dictionary_t ret = shcore::make_dict();

  auto group_instance = m_cluster.get_cluster_server();
  bool gr_running =
      m_cluster.cluster_availability() != Cluster_availability::OFFLINE &&
      m_cluster.cluster_availability() !=
          Cluster_availability::SOME_UNREACHABLE &&
      m_cluster.cluster_availability() != Cluster_availability::UNREACHABLE;

  std::string topology_mode =
      mysqlshdk::gr::to_string(m_cluster.get_cluster_topology_type());

  // Set Cluster name
  (*ret)["name"] = shcore::Value("default");
  (*ret)["topologyMode"] = shcore::Value(topology_mode);

  mysqlshdk::utils::Version protocol_version;
  if (group_instance && gr_running) {
    try {
      protocol_version =
          mysqlshdk::gr::get_group_protocol_version(*group_instance);
    } catch (const shcore::Exception &e) {
      if (e.code() == ER_CANT_INITIALIZE_UDF)
        log_warning("Can't get protocol version from %s: %s",
                    group_instance->descr().c_str(), e.format().c_str());
      else
        throw;
    }
  }

  if ((!m_extended.is_null() && *m_extended >= 1)) {
    (*ret)["groupName"] = shcore::Value(m_cluster.get_group_name());
    if (group_instance &&
        group_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 26))
      (*ret)["groupViewChangeUuid"] =
          shcore::Value(*group_instance->get_sysvar_string(
              "group_replication_view_change_uuid"));
    (*ret)["GRProtocolVersion"] =
        protocol_version ? shcore::Value(protocol_version.get_full())
                         : shcore::Value::Null();
  }

  auto ssl_mode = group_instance ? *group_instance->get_sysvar_string(
                                       "group_replication_ssl_mode",
                                       mysqlshdk::mysql::Var_qualifier::GLOBAL)
                                 : "";
  if (!ssl_mode.empty()) {
    (*ret)["ssl"] = shcore::Value(ssl_mode);
  }

  bool single_primary;
  std::string view_id;
  bool has_quorum;
  std::vector<mysqlshdk::gr::Member> member_info;

  if (group_instance && gr_running) {
    member_info = mysqlshdk::gr::get_members(*group_instance, &single_primary,
                                             &has_quorum, &view_id);

    tmp = check_group_status(*group_instance, member_info, has_quorum);
    (*ret)["statusText"] = shcore::Value(tmp->get_string("statusText"));
    (*ret)["status"] = shcore::Value(tmp->get_string("status"));
    if (!m_extended.is_null() && *m_extended >= 1)
      (*ret)["groupViewId"] = shcore::Value(view_id);

    std::shared_ptr<Instance> primary_instance;
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
                primary_instance = s->second;
              }
              (*ret)["primary"] = shcore::Value(primary->endpoint);
            }
            break;
          }
        }
      }
    }
  } else {
    if (m_cluster.cluster_availability() == Cluster_availability::OFFLINE) {
      (*ret)["status"] = shcore::Value("OFFLINE");
      (*ret)["statusText"] =
          shcore::Value("All members of the group are OFFLINE");
    } else {
      (*ret)["status"] = shcore::Value("UNREACHABLE");
      (*ret)["statusText"] =
          shcore::Value("Could not connect to any ONLINE members");
    }
  }

  (*ret)["topology"] = shcore::Value(get_topology(member_info));

  {
    auto issues = group_diagnostics(&m_cluster, member_info, protocol_version);

    if (issues && !issues->empty())
      (*ret)["clusterErrors"] = shcore::Value(issues);
  }

  return ret;
}

void Status::prepare() {
  // Sanity check: Verify if the topology type changed and issue an error if
  // needed.
  if (m_cluster.get_cluster_server()) m_cluster.sanity_check();

  m_instances = m_cluster.get_instances();

  // Always connect to members to be able to get an accurate mode, based on
  // their super_ready_only value.
  connect_to_members();
}

shcore::Value Status::get_default_replicaset_status() {
  shcore::Dictionary_t dict = shcore::make_dict();

  // Get the Cluster status
  shcore::Dictionary_t replicaset_dict;

  replicaset_dict = collect_replicaset_status();

  // TODO(alfredo) - this needs to be reviewed, probably not necessary
  // Check if the Cluster group session is established to an instance with
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
  auto group_instance = m_cluster.get_cluster_server();

  if (group_instance) {
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

shcore::Value Status::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // If the Cluster belongs to a ClusterSet, include relevant ClusterSet
  // information:
  //   - domainName
  //   - clusterRole
  //   - clusterSetReplicationStatus
  if (m_cluster.is_cluster_set_member()) {
    auto cs_md = m_cluster.get_clusterset_metadata();
    Cluster_set_metadata cset;

    m_cluster.get_metadata_storage()->get_cluster_set(cs_md.cluster_set_id,
                                                      true, &cset, nullptr);

    (*dict)["domainName"] = shcore::Value(cset.domain_name);
    (*dict)["clusterRole"] =
        shcore::Value(m_cluster.is_primary_cluster() ? "PRIMARY" : "REPLICA");

    // Get the ClusterSet replication channel status
    auto cluster_cpy = std::make_unique<Cluster_impl>(m_cluster);

    Cluster_channel_status ch_status =
        cluster_cpy->get_cluster_set()->get_replication_channel_status(
            m_cluster);

    // If the Cluster is a Replica, add the status right away
    if (!m_cluster.is_primary_cluster()) {
      (*dict)["clusterSetReplicationStatus"] =
          shcore::Value(to_string(ch_status));
    } else {
      // If it's a Primary, add the status in case is suspicious (not Missing or
      // Stopped)
      if (ch_status != Cluster_channel_status::MISSING &&
          ch_status != Cluster_channel_status::STOPPED) {
        (*dict)["clusterSetReplicationStatus"] =
            shcore::Value(to_string(ch_status));
      }
    }
  }

  // Get the default replicaSet options
  (*dict)["defaultReplicaSet"] = shcore::Value(get_default_replicaset_status());

  if (m_cluster.get_cluster_server()) {
    // Gets the metadata version
    if (!m_extended.is_null() && *m_extended >= 1) {
      auto version = mysqlsh::dba::metadata::installed_version(
          m_cluster.get_cluster_server());
      (*dict)["metadataVersion"] = shcore::Value(version.get_base());
    }

    // Iterate all replicasets and get the status for each one

    std::string addr = m_cluster.get_cluster_server()->get_canonical_address();
    (*dict)["groupInformationSourceMember"] = shcore::Value(addr);
  }

  auto md_server = m_cluster.get_metadata_storage()->get_md_server();

  // metadata server, if its a different one
  if (md_server &&
      (!m_cluster.get_cluster_server() ||
       md_server->get_uuid() != m_cluster.get_cluster_server()->get_uuid())) {
    (*dict)["metadataServer"] =
        shcore::Value(md_server->get_canonical_address());
  }

  return shcore::Value(dict);
}

void Status::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Status::finish() {}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
