/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/common_status.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/parallel_applier_options.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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

Status::Status(const std::shared_ptr<Cluster_impl> &cluster,
               std::optional<uint64_t> extended)
    : m_cluster(cluster), m_extended(extended) {}

Status::~Status() = default;

void Status::connect_to_members() {
  auto ipool = current_ipool();

  for (const auto &inst : m_instances) {
    try {
      if (inst.instance_type == Instance_type::READ_REPLICA) {
        m_read_replica_sessions[inst.endpoint] =
            ipool->connect_unchecked_endpoint(inst.endpoint);
      } else {
        m_member_sessions[inst.endpoint] =
            ipool->connect_unchecked_endpoint(inst.endpoint);
      }
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
  int cluster_instance_count = 0;

  for (const auto &inst : m_instances) {
    if (inst.instance_type != Instance_type::GROUP_MEMBER) continue;

    cluster_instance_count++;

    if (std::find_if(members.begin(), members.end(),
                     [&inst](const mysqlshdk::gr::Member &member) {
                       return member.uuid == inst.uuid;
                     }) == members.end()) {
      missing_from_group++;
    }
  }

  for (const auto &member : members) {
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
    if (m_cluster->is_fenced_from_writes()) {
      rs_status = Cluster_status::FENCED_WRITES;

      desc_status = "Cluster is fenced from Write Traffic.";
    } else if (m_cluster->is_cluster_set_member() &&
               m_cluster->is_invalidated()) {
      rs_status = Cluster_status::INVALIDATED;

      desc_status = "Cluster was invalidated by the ClusterSet it belongs to.";
    } else if (number_of_failures_tolerated == 0) {
      if (missing_from_group > 0 || total_in_group != quorum_size)
        rs_status = Cluster_status::OK_NO_TOLERANCE_PARTIAL;
      else
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

  if (cluster_instance_count > online_count) {
    if (cluster_instance_count - online_count == 1) {
      desc_status.append(" 1 member is not active.");
    } else {
      desc_status.append(" " +
                         std::to_string(cluster_instance_count - online_count) +
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
  auto group_instance = m_cluster->get_cluster_server();

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

static constexpr const char *k_calculate_lag_query = R"*(
SELECT
    IF( /* IF  replication connection or sql thread is not running, we return
           'null', replication isn't working properly
        */
        applier_coordinator_status.SERVICE_STATE = 'OFF'
        OR conn_status.SERVICE_STATE = 'OFF',
        'null',
        IF(
            /* When the last queued gtid and last applied gtid is the same,
               then the applier applied everything in the queue
               In addition when the applying transaction is 0 (e.g. due to
               replication restart) we also have nothing to execute
            */
            GTID_SUBTRACT(conn_status.LAST_QUEUED_TRANSACTION,
            applier_status.LAST_APPLIED_TRANSACTION) = ''
            OR UNIX_TIMESTAMP(applier_status.APPLYING_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP) = 0,
            'applier_queue_applied',
            /* Replication lag is the diff between now and the time it was
               committed by its source
            */
            TIMEDIFF(
                NOW(6),
                applier_status.APPLYING_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP
            )
        )
    ) AS cluster_member_lag
FROM
    performance_schema.replication_connection_status AS conn_status
    JOIN performance_schema.replication_applier_status_by_worker AS applier_status ON applier_status.channel_name = conn_status.channel_name
    JOIN performance_schema.replication_applier_status_by_coordinator AS applier_coordinator_status ON applier_coordinator_status.channel_name = conn_status.channel_name
WHERE
    conn_status.channel_name = 'group_replication_applier'
ORDER BY
    /* As we have a row returned per worker thread, we need to obtain the
       worker that is executing the oldest transaction to find replication lag.
       Because workers can be idle, we have to put a lower priority on them,
       hence the ordering by 0-EXECUTING, 1-IDLE first
    */
    IF(GTID_SUBTRACT(conn_status.LAST_QUEUED_TRANSACTION,
      applier_status.LAST_APPLIED_TRANSACTION) = ''
            OR UNIX_TIMESTAMP(applier_status.APPLYING_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP) = 0,
            '1-IDLE', '0-EXECUTING') ASC,
    applier_status.APPLYING_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP ASC
LIMIT
    1;
)*";

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
    if (m_cluster->is_cluster_set_member()) {
      // PRIMARY of PC has no relevant replication lag info
      // PRIMARY of RC shows lag from clusterset_replication channel
      // SECONDARY members show replication from gr_applier channel
      std::string channel_name;

      if (is_primary) {
        if (!m_cluster->is_primary_cluster()) {
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
      auto result = instance.query(k_calculate_lag_query);
      auto row = result->fetch_one_named();
      if (row) {
        std::string lag = row.get_string("cluster_member_lag", "");

        if (lag == "null" || lag.empty()) {
          (*dict)["replicationLag"] = shcore::Value::Null();
        } else {
          (*dict)["replicationLag"] = shcore::Value(lag);
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

  if (info.hidden_from_router) {
    (*dict)["hiddenFromRouter"] = shcore::Value::True();
  }
}

void Status::feed_member_info(shcore::Dictionary_t dict,
                              const mysqlshdk::gr::Member &member,
                              std::optional<bool> offline_mode,
                              std::optional<bool> super_read_only,
                              const std::vector<std::string> &fence_sysvars,
                              mysqlshdk::gr::Member_state self_state,
                              bool is_auto_rejoin_running,
                              const shcore::Dictionary_t read_replicas) {
  (*dict)["readReplicas"] = shcore::Value(read_replicas);

  if (m_extended.value_or(0) >= 1) {
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

  if ((m_extended.value_or(0) >= 1) || member.state != self_state) {
    // memberState is from the point of view of the member itself
    (*dict)["memberState"] =
        shcore::Value(mysqlshdk::gr::to_string(self_state));
  }

  // status is the status from the point of view of the quorum
  (*dict)["status"] = shcore::Value(mysqlshdk::gr::to_string(member.state));

  // Set the instance mode (read-only or read-write).
  if (!offline_mode.has_value() || !super_read_only.has_value() ||
      member.state != mysqlshdk::gr::Member_state::ONLINE) {
    // offline_mode or super_read_only is null if it could not be obtained from
    // the instance.
    (*dict)["mode"] = shcore::Value("n/a");
  } else {
    // We deal with offline mode the same as n/a and set mode to read-only if
    // there is no quorum otherwise according to the instance super_read_only
    // value.
    if (*offline_mode)
      (*dict)["mode"] = shcore::Value("n/a");
    else
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

void check_parallel_appliers(
    shcore::Array_t issues, const mysqlshdk::utils::Version &instance_version,
    const Parallel_applier_options &parallel_applier_options) {
  auto current_values =
      parallel_applier_options.get_current_settings(instance_version);
  auto required_values =
      parallel_applier_options.get_required_values(instance_version);

  for (const auto &setting : required_values) {
    auto current_value = current_values[std::get<0>(setting)].value_or("");

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
          "ERROR: receiver thread of " + channel_label +
          " channel stopped with an error: " +
          mysqlshdk::mysql::to_string(channel.receiver.last_error)));
      break;
    case Replication_channel::APPLIER_ERROR:
      for (const auto &a : channel.appliers) {
        if (a.last_error.code != 0) {
          issues->push_back(
              shcore::Value("ERROR: applier thread of " + channel_label +
                            " channel stopped with an error: " +
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

void check_host_metadata(shcore::Array_t issues, Instance *instance,
                         const Instance_metadata_info &instance_md) {
  auto address = instance->get_canonical_address();

  if (!mysqlshdk::utils::are_endpoints_equal(instance_md.md.endpoint,
                                             address)) {
    issues->push_back(
        shcore::Value("ERROR: Metadata for this instance does not match "
                      "hostname reported by instance (metadata=" +
                      instance_md.md.endpoint + ", actual=" + address +
                      "). Use rescan() to update the metadata."));
  }
}

void check_transaction_size_limit(shcore::Array_t issues, Instance *instance,
                                  int64_t cluster_transaction_size_limit) {
  // Check if the instance's value for group_replication_transaction_size_limit
  // matches the Cluster's one
  int64_t instance_transaction_size_limit =
      instance->get_sysvar_int(kGrTransactionSizeLimit).value_or(0);

  if (cluster_transaction_size_limit != -1 &&
      instance_transaction_size_limit != cluster_transaction_size_limit) {
    issues->push_back(shcore::Value(
        "WARNING: The value of 'group_replication_transaction_size_limit' does "
        "not match the Cluster's configured value. Use Cluster.rescan() to "
        "repair."));
  }
}

void check_auth_type_instance_ssl(shcore::Array_t issues,
                                  const Instance &instance,
                                  const Cluster_impl &cluster) {
  auto auth_type = cluster.query_cluster_auth_type();
  if (auth_type == Replication_auth_type::PASSWORD) return;

  // if we can be more specific
  switch (auth_type) {
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (cluster.query_cluster_instance_auth_cert_subject(instance).empty()) {
        auto msg = shcore::str_format(
            "WARNING: memberAuthType is set to '%s' but there's no "
            "'certSubject' configured for this instance, which prevents all "
            "other instances from reaching it, compromising the Cluster. "
            "Please remove the instance from the Cluster and use the most "
            "recent version of the shell to re-add it back.",
            to_string(auth_type).c_str());
        issues->push_back(shcore::Value(std::move(msg)));
        return;
      }
    default:
      break;
  }

  // check if instance user was created correctly
  bool has_issuer{false}, has_subject{false};
  {
    auto instance_repl_account =
        cluster.get_metadata_storage()->get_instance_repl_account(
            instance.get_uuid(), Cluster_type::GROUP_REPLICATION,
            Replica_type::GROUP_MEMBER);

    if (std::get<0>(instance_repl_account).empty()) {
      // if this happens, a warning is already shown to the user in
      // validate_instance_recovery_user(), so there's no point showing a second
      // error message related to the same thing, even though it almost surely
      // means that the user was created with an older version of the shell
      return;
    }

    if (auto server = cluster.get_cluster_server(); server) {
      auto res = server->queryf(
          "SELECT coalesce(length(x509_issuer)), "
          "coalesce(length(x509_subject)) FROM mysql.user WHERE (user = ?)",
          std::get<0>(instance_repl_account));

      if (auto row = res->fetch_one()) {
        has_issuer = row->get_int(0) > 0;
        has_subject = row->get_int(1) > 0;
      }
    }
  }

  DBUG_EXECUTE_IF("fail_recovery_user_status_check_issuer",
                  { has_issuer = false; });
  DBUG_EXECUTE_IF("fail_recovery_user_status_check_subject",
                  { has_subject = false; });

  switch (auth_type) {
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      if (!has_issuer) {
        auto msg = shcore::str_format(
            "WARNING: memberAuthType is set to '%s' but there's no "
            "'certIssuer' configured for this instance recovery user, which "
            "prevents all other instances from reaching it, compromising the "
            "Cluster. Please remove the instance from the Cluster and use the "
            "most recent version of the shell to re-add it back.",
            to_string(auth_type).c_str());
        issues->push_back(shcore::Value(std::move(msg)));
      }
      break;
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      if (!has_issuer || !has_subject) {
        auto msg = shcore::str_format(
            "WARNING: memberAuthType is set to '%s' but there's no "
            "'certIssuer' and/or 'certSubject' configured for this instance "
            "recovery user, which prevents all other instances from reaching "
            "it, compromising the Cluster. Please remove the instance from the "
            "Cluster and use the most recent version of the shell to re-add it "
            "back.",
            to_string(auth_type).c_str());
        issues->push_back(shcore::Value(std::move(msg)));
      }
      break;
    default:
      break;
  }
}

shcore::Array_t instance_diagnostics(
    Instance *instance, const Cluster_impl *cluster,
    const Instance_metadata_info &instance_md,
    const mysqlshdk::mysql::Replication_channel &recovery_channel,
    const mysqlshdk::mysql::Replication_channel &applier_channel,
    std::optional<bool> super_read_only, const mysqlshdk::gr::Member &minfo,
    mysqlshdk::gr::Member_state self_state,
    const Parallel_applier_options &parallel_applier_options,
    int64_t cluster_transaction_size_limit) {
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
      super_read_only.has_value() && !*super_read_only) {
    issues->push_back(shcore::Value(
        "WARNING: Instance is NOT the global PRIMARY but super_read_only "
        "option is OFF. Errant transactions and inconsistencies may be "
        "accidentally introduced."));
  } else if (minfo.role == mysqlshdk::gr::Member_role::SECONDARY &&
             self_state != mysqlshdk::gr::Member_state::RECOVERING &&
             super_read_only.has_value() && !*super_read_only) {
    issues->push_back(
        shcore::Value("WARNING: Instance is NOT a PRIMARY but super_read_only "
                      "option is OFF."));
  }

  check_channel_error(issues, "Group Replication Recovery", recovery_channel);
  check_channel_error(issues, "Group Replication Applier", applier_channel);

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

  try {
    mysqlshdk::utils::split_host_and_port(instance_md.md.grendpoint);
  } catch (const std::invalid_argument &e) {
    issues->push_back(shcore::Value(
        "ERROR: Invalid or missing information of Group Replication's network "
        "address in metadata. Use Cluster.rescan() to update the metadata."));
  }

  // check if value of report_host matches what's in the metadata
  if (instance) check_host_metadata(issues, instance, instance_md);

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

  if (instance) {
    check_transaction_size_limit(issues, instance,
                                 cluster_transaction_size_limit);
  }

  // check if instance (if secondary) has a seemingly correct SSL settings
  if (instance) check_auth_type_instance_ssl(issues, *instance, *cluster);

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
      std::string str = "Group communication protocol in use is version " +
                        protocol_version.get_full() +
                        " but it is possible to upgrade to " +
                        highest_possible_version.get_full() + ".";

      if (protocol_version < mysqlshdk::utils::Version(8, 0, 16)) {
        str += " Message fragmentation for large transactions";
        if (highest_possible_version == mysqlshdk::utils::Version(8, 0, 27)) {
          str += " and Single Consensus Leader";
        }

        str += " can only be enabled after upgrade.";
      } else {
        str += " Single Consensus Leader can only be enabled after upgrade.";
      }

      str += " Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade.";

      issues->emplace_back(shcore::Value(str));
    }
  }
}

void check_view_change_uuid_md(shcore::Array_t issues,
                               const Cluster_impl &cluster) {
  auto group_instance = cluster.get_cluster_server();

  // If there's primary we can't check anything, exit
  if (!group_instance) return;

  // Check if group_replication_view_change_uuid is set on the cluster and if
  // so, if it matches the value stored in the Metadata
  auto view_change_uuid = cluster.get_cluster_server()->get_sysvar_string(
      "group_replication_view_change_uuid", "");

  // Not in use, exit
  if (view_change_uuid.empty() || view_change_uuid == "AUTOMATIC") return;

  auto md_view_change_uuid = cluster.get_view_change_uuid();

  if (md_view_change_uuid.empty()) {
    issues->push_back(shcore::Value(
        "WARNING: The Cluster's group_replication_view_change_uuid is not "
        "stored in the Metadata. Please use <Cluster>.rescan() to update the "
        "metadata."));
  } else {
    if (md_view_change_uuid != view_change_uuid) {
      issues->push_back(shcore::Value(
          "WARNING: The Cluster's group_replication_view_change_uuid value "
          "in use does not match the value stored in the Metadata. Please "
          "use <Cluster>.rescan() to update the metadata."));
    }
  }
}

shcore::Array_t cluster_diagnostics(
    const Cluster_impl &cluster,
    const std::vector<mysqlshdk::gr::Member> &member_info,
    const mysqlshdk::utils::Version &protocol_version) {
  shcore::Array_t issues = shcore::make_array();

  if (protocol_version && !member_info.empty())
    check_comm_protocol_upgrade_possible(issues, member_info, protocol_version);

  // Verify Metadata consistency regarding group_replication_view_change_uuid
  check_view_change_uuid_md(issues, cluster);

  switch (cluster.cluster_availability()) {
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

  if (cluster.is_fenced_from_writes()) {
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

  if (!instance || endpoints.empty()) return issues;

  auto it = endpoints.find(actual_server_uuid);
  if (it == endpoints.end()) return issues;

  std::string recovery_user = mysqlshdk::mysql::get_replication_user(
      *instance, mysqlshdk::gr::k_gr_recovery_channel);

  // it->second is recovery user from MD
  if (!recovery_user.empty()) {
    bool recovery_is_valid =
        shcore::str_beginswith(
            recovery_user, mysqlshdk::gr::k_group_recovery_old_user_prefix) ||
        shcore::str_beginswith(recovery_user,
                               mysqlshdk::gr::k_group_recovery_user_prefix);

    if (it->second.empty() && recovery_is_valid) {
      issues->push_back(
          shcore::Value("WARNING: The replication recovery account in use "
                        "by the instance is not stored in the metadata. "
                        "Use Cluster.rescan() to update the metadata."));
    } else if (it->second != recovery_user) {
      if (recovery_is_valid) {
        issues->push_back(
            shcore::Value("WARNING: The replication recovery account in use "
                          "by the instance is not stored in the metadata. "
                          "Use Cluster.rescan() to update the metadata."));
      } else {
        issues->push_back(shcore::Value(
            "WARNING: Unsupported recovery account '" + recovery_user +
            "' has been found for instance '" + instance->descr() +
            "'. Operations such as Cluster.resetRecoveryAccountsPassword() and "
            "Cluster.addInstance() may fail. Please remove and add "
            "the instance back to the Cluster to ensure a supported recovery "
            "account is used."));
      }
    }
  } else {
    issues->push_back(shcore::Value(
        "WARNING: Recovery user account not found for server address: " +
        instance->descr() + " with UUID " + instance->get_uuid()));
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
  std::vector<Read_replica_info> read_replicas;

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
        if (!mysqlshdk::utils::are_endpoints_equal(
                i.address,
                mysqlshdk::utils::make_host_and_port(m.host, m.port)))
          continue;

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
    if (!found) {
      Instance_metadata_info mdi;
      mdi.md.address = mysqlshdk::utils::make_host_and_port(m.host, m.port);
      mdi.md.label = mdi.md.address;
      mdi.md.uuid = m.uuid;
      mdi.md.endpoint = mdi.md.address;
      mdi.actual_server_uuid = mdi.md.uuid;

      log_debug("Instance %s with uuid=%s found in group but not in MD",
                mdi.md.address.c_str(), m.uuid.c_str());

      auto group_instance = m_cluster->get_cluster_server();

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
      if (i.instance_type == Instance_type::READ_REPLICA) {
        Read_replica_info read_replica = {};

        read_replica.md = i;

        Replication_sources replication_sources =
            m_cluster->get_read_replica_replication_sources(i.uuid);

        if (replication_sources.source_type != Source_type::CUSTOM) {
          read_replica.managed_channel_info.automatic_sources = true;
        } else {
          read_replica.managed_channel_info.sources.swap(
              replication_sources.replication_sources);
        }

        read_replicas.emplace_back(read_replica);
        found = true;
        break;
      } else {
        if (m.uuid == i.uuid ||
            mysqlshdk::utils::are_endpoints_equal(
                i.endpoint,
                mysqlshdk::utils::make_host_and_port(m.host, m.port))) {
          found = true;
          break;
        }
      }
    }
    if (!found) {
      log_debug("Instance with uuid=%s address=%s is in MD but not in group",
                i.uuid.c_str(), i.endpoint.c_str());

      auto &instance = m_member_sessions[i.endpoint];

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
  if (m_cluster->get_metadata_storage()->real_version().get_major() > 1) {
    endpoints =
        m_cluster->get_metadata_storage()->get_instances_with_recovery_accounts(
            m_cluster->get_id());
  }

  auto mismatched_recovery_accounts =
      m_cluster->get_mismatched_recovery_accounts();

  // Flag to mark the primary instance was already feeded with rogue
  // read-replicas info. Used to avoid all members being fed with the same
  // read-replica when in multi-primary mode
  bool already_feeded_primary = false;

  for (const auto &inst : instances) {
    shcore::Dictionary_t member = shcore::make_dict();
    mysqlshdk::gr::Member minfo(get_member(inst.actual_server_uuid));
    mysqlshdk::gr::Member_state self_state =
        mysqlshdk::gr::Member_state::MISSING;

    auto &instance = m_member_sessions[inst.md.endpoint];

    std::optional<bool> super_read_only;
    std::optional<bool> offline_mode;
    std::vector<std::string> fence_sysvars;
    bool auto_rejoin = false;

    Replication_channel applier_channel;
    Replication_channel recovery_channel;

    Parallel_applier_options parallel_applier_options;

    if (instance) {
      // Get the current parallel-applier options
      parallel_applier_options = Parallel_applier_options(*instance);

      // Get super_read_only value of each instance to set the mode
      // accurately.
      super_read_only = instance->get_sysvar_bool("super_read_only");

      // Get offline_mode value of each instance to set the mode accurately.
      offline_mode = instance->get_sysvar_bool("offline_mode");

      // Check if auto-rejoin is running.
      auto_rejoin = mysqlshdk::gr::is_running_gr_auto_rejoin(*instance);

      self_state = mysqlshdk::gr::get_member_state(*instance);

      minfo.version = instance->get_version().get_base();

      if (m_extended.has_value()) {
        if (*m_extended >= 1) {
          fence_sysvars = instance->get_fence_sysvars();

          auto workers = parallel_applier_options.replica_parallel_workers;

          if (parallel_applier_options.replica_parallel_workers.value_or(0) >
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
          m_cluster->get_metadata_storage()->query_instance_attribute(
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

    bool is_primary = minfo.role == Member_role::PRIMARY;

    feed_member_info(
        member, minfo, offline_mode, super_read_only, fence_sysvars, self_state,
        auto_rejoin,
        get_read_replicas_info(inst, read_replicas,
                               is_primary && !already_feeded_primary));

    if (is_primary) already_feeded_primary = true;

    {
      shcore::Array_t issues = instance_diagnostics(
          instance.get(), m_cluster.get(), inst, recovery_channel,
          applier_channel, super_read_only, minfo, self_state,
          parallel_applier_options, *m_cluster_transaction_size_limit);

      if (offline_mode.value_or(false)) {
        issues->push_back(
            shcore::Value("WARNING: Instance has 'offline_mode' enabled."));
      } else if (instance && instance->is_set_persist_supported()) {
        auto value = instance->get_persisted_value("offline_mode");
        if (value.has_value() && shcore::str_caseeq(*value, "ON")) {
          issues->push_back(shcore::Value(
              "WARNING: Instance has 'offline_mode' enabled and persisted. In "
              "the event that this instance becomes a primary, Shell or other "
              "members will be prevented from connecting to it disrupting the "
              "Cluster's normal functioning."));
        }
      }

      if (instance) {
        auto ret_val = validate_instance_recovery_user(
            instance, endpoints, inst.actual_server_uuid, self_state);

        // if the last check returned an error, the next check will too, so
        // there's no need to show the user two different msgs that kind of
        // point to the same thing (although for different reasons)
        if (ret_val->empty()) {
          auto it = mismatched_recovery_accounts.find(instance->get_id());
          if (it != mismatched_recovery_accounts.end()) {
            auto msg = shcore::str_format(
                "WARNING: Incorrect recovery account (%s) being used. Use "
                "Cluster.rescan() to repair.",
                it->second.c_str());
            ret_val->push_back(shcore::Value(std::move(msg)));
          }
        }

        issues->insert(issues->end(), std::make_move_iterator(ret_val->begin()),
                       std::make_move_iterator(ret_val->end()));
      }

      // check if primary has unused recovery accounts
      if (minfo.role == Member_role::PRIMARY) {
        auto ret_val =
            validate_recovery_accounts_unused(mismatched_recovery_accounts);
        issues->insert(issues->end(), std::make_move_iterator(ret_val->begin()),
                       std::make_move_iterator(ret_val->end()));
      }

      if (issues && !issues->empty()) {
        (*member)["instanceErrors"] = shcore::Value(std::move(issues));
      }
    }

    if (inst.md.tags && !inst.md.tags->empty()) {
      (*member)["tags"] = shcore::Value(inst.md.tags);
    }

    if ((m_extended.value_or(0) >= 2) &&
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

  auto group_instance = m_cluster->get_cluster_server();
  bool gr_running =
      m_cluster->cluster_availability() != Cluster_availability::OFFLINE &&
      m_cluster->cluster_availability() !=
          Cluster_availability::SOME_UNREACHABLE &&
      m_cluster->cluster_availability() != Cluster_availability::UNREACHABLE;

  std::string topology_mode =
      mysqlshdk::gr::to_string(m_cluster->get_cluster_topology_type());

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

  if (m_extended.value_or(0) >= 1) {
    (*ret)["groupName"] = shcore::Value(m_cluster->get_group_name());

    if (group_instance &&
        group_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 26)) {
      (*ret)["groupViewChangeUuid"] =
          shcore::Value(group_instance->get_sysvar_string(
              "group_replication_view_change_uuid", ""));
    }

    (*ret)["GRProtocolVersion"] =
        protocol_version ? shcore::Value(protocol_version.get_full())
                         : shcore::Value::Null();

    // Add the communicationStack
    //
    // Always add the value of the communicationStack regardless of the version
    // in use, because:
    //
    //   - Even before the 'MySQL' communication stack was introduced, 'XCOM'
    //   was the name of the communication stack used by GR
    //   - It slightly educates users that there's a possibility of having a
    //   different communication stack.
    if (group_instance) {
      (*ret)[kCommunicationStack] =
          shcore::Value(get_communication_stack(*group_instance));
    }

    // Add the value of paxosSingleLeader, when supported
    if (group_instance &&
        supports_paxos_single_leader(group_instance->get_version())) {
      auto value = get_paxos_single_leader_enabled(*group_instance);

      if (value.has_value()) {
        std::string_view paxos_single_leader = *value == true ? "ON" : "OFF";

        (*ret)[kPaxosSingleLeader] = shcore::Value(paxos_single_leader);
      }
    }
  }

  {
    auto ssl_mode = group_instance ? group_instance->get_sysvar_string(
                                         "group_replication_ssl_mode", "")
                                   : "";
    if (!ssl_mode.empty()) {
      (*ret)["ssl"] = shcore::Value(ssl_mode);
    }
  }

  bool single_primary = true;
  std::vector<mysqlshdk::gr::Member> member_info;

  if (group_instance && gr_running) {
    bool has_quorum = true;
    std::string view_id;

    member_info = mysqlshdk::gr::get_members(*group_instance, &single_primary,
                                             &has_quorum, &view_id);

    tmp = check_group_status(*group_instance, member_info, has_quorum);
    (*ret)["statusText"] = shcore::Value(tmp->get_string("statusText"));
    (*ret)["status"] = shcore::Value(tmp->get_string("status"));
    if (m_extended.value_or(0) >= 1)
      (*ret)["groupViewId"] = shcore::Value(view_id);

    std::shared_ptr<Instance> primary_instance;
    {
      if (single_primary) {
        // In single primary mode we need to add the "primary" field
        (*ret)["primary"] = shcore::Value("?");
        for (const auto &member : member_info) {
          if (member.role != mysqlshdk::gr::Member_role::PRIMARY) continue;
          const Instance_metadata *primary = instance_with_uuid(member.uuid);
          if (primary) {
            if (auto s = m_member_sessions.find(primary->endpoint);
                s != m_member_sessions.end()) {
              primary_instance = s->second;
            }
            (*ret)["primary"] = shcore::Value(primary->endpoint);
          }
          break;
        }
      }
    }
  } else {
    if (m_cluster->cluster_availability() == Cluster_availability::OFFLINE) {
      (*ret)["status"] = shcore::Value("OFFLINE");
      (*ret)["statusText"] =
          shcore::Value("All members of the group are OFFLINE");
    } else {
      (*ret)["status"] = shcore::Value("UNREACHABLE");
      (*ret)["statusText"] =
          shcore::Value("Could not connect to any ONLINE members");
    }
  }

  auto issues = cluster_diagnostics(*m_cluster, member_info, protocol_version);

  // Get the Cluster's transaction size_limit stored in the Metadata if the
  // Cluster is standalone or a Primary. Otherwise, it's a Replica so it
  // should be the value of the primary member
  bool is_replica_cluster =
      m_cluster->is_cluster_set_member() && !m_cluster->is_primary_cluster();

  if (group_instance) {
    m_cluster_transaction_size_limit =
        m_cluster->get_cluster_server()
            ->get_sysvar_int(kGrTransactionSizeLimit)
            .value_or(0);
  }

  if (!is_replica_cluster) {
    shcore::Value value;
    if (!m_cluster->get_metadata_storage()->query_cluster_attribute(
            m_cluster->get_id(), k_cluster_attribute_transaction_size_limit,
            &value)) {
      issues->push_back(
          shcore::Value("WARNING: Cluster's transaction size limit is not "
                        "registered in the metadata. Use "
                        "cluster.rescan() to update the metadata."));

    } else {
      m_cluster_transaction_size_limit = value.as_int();
    }
  }

  // If the cluster is operating in multi-primary mode and paxosSingleLeader
  // is enabled, add a NOTE informing the Cluster won't use a single
  // consensus leader in that case
  bool is_active_multi_primary =
      group_instance && gr_running && !single_primary;

  if (is_active_multi_primary &&
      supports_paxos_single_leader(group_instance->get_version()) &&
      get_paxos_single_leader_enabled(*group_instance).value_or(false)) {
    issues->push_back(
        shcore::Value("NOTE: The Cluster is configured to use a Single "
                      "Consensus Leader, however, this setting is ineffective "
                      "since the Cluster is operating in multi-primary mode."));
  }

  if (issues && !issues->empty())
    (*ret)["clusterErrors"] = shcore::Value(issues);

  (*ret)["topology"] = shcore::Value(get_topology(member_info));

  return ret;
}

shcore::Array_t Status::validate_recovery_accounts_unused(
    const std::unordered_map<uint32_t, std::string>
        &mismatched_recovery_accounts) const {
  auto issues = shcore::make_array();

  auto accounts =
      m_cluster->get_unused_recovery_accounts(mismatched_recovery_accounts);
  if (accounts.empty()) return issues;

  std::string msg{accounts.size() == 1
                      ? "WARNING: Detected an unused recovery account: "
                      : "WARNING: Detected unused recovery accounts: "};

  for (const auto &account : accounts)
    msg.append(std::get<0>(account)).append(", ");
  msg.erase(msg.size() - 2);  // remove last ", "

  msg.append(". Use Cluster.rescan() to clean up.");

  issues->push_back(shcore::Value(std::move(msg)));
  return issues;
}

shcore::Array_t Status::read_replica_diagnostics(
    Instance *instance, const Read_replica_info &rr_info,
    bool is_primary) const {
  using mysqlshdk::mysql::Replication_channel;

  shcore::Array_t instance_errors = shcore::make_array();

  auto append_error = [&instance_errors](const std::string &msg) {
    if (!instance_errors) {
      instance_errors = shcore::make_array();
    }
    instance_errors->push_back(shcore::Value(msg));
  };

  if (!instance) {
    append_error(
        "ERROR: Could not connect to the Read-Replica: The instance is "
        "unreachable.");

    return instance_errors;
  }

  // Check if super_read_only is disabled
  if (!instance->is_read_only(true)) {
    append_error(
        "WARNING: Instance is a Read-Replica but super_read_only option is "
        "OFF. Use Cluster.rejoinInstance() to fix it.");
  }

  // Check if the read-replica doesn't have any configured
  // replicationSources
  Replication_sources replication_sources =
      m_cluster->get_read_replica_replication_sources(instance->get_uuid());

  if (!replication_sources.source_type.has_value() ||
      (*replication_sources.source_type == Source_type::CUSTOM &&
       replication_sources.replication_sources.empty())) {
    append_error(
        "WARNING: Read Replica's replicationSources is empty. Use "
        "Cluster.setInstanceOption() with the option "
        "'replicationSources' to update the instance's sources.");
  }

  // Check if the channel is missing
  if (rr_info.managed_channel_info.channel_name.empty()) {
    append_error(
        "WARNING: Read Replica's replication channel is missing. Use "
        "Cluster.rejoinInstance() to restore it.");

    return instance_errors;
  }

  if (!rr_info.managed_channel_info.automatic_failover) {
    append_error(
        "WARNING: Read Replica's replication channel is misconfigured: "
        "automatic failover is disabled. Use Cluster.rejoinInstance() to "
        "fix it.");
  }

  // Check if the read-replica doesn't have any active sources
  if (rr_info.managed_channel_info.sources.empty()) {
    append_error(
        "WARNING: Read Replica has no active sources. Use "
        "Cluster.rejoinInstance() to fix it.");
  }

  // Check if the read-replica active sources match the replicationSources
  bool sources_mismatch = false;

  if (replication_sources.source_type == Source_type::CUSTOM) {
    if (replication_sources.replication_sources !=
        rr_info.managed_channel_info.sources) {
      sources_mismatch = true;
    }
  } else {
    sources_mismatch =
        (replication_sources.source_type == Source_type::PRIMARY &&
         !is_primary);
  }

  if (sources_mismatch) {
    append_error(
        "WARNING: Read Replica's replication channel is misconfigured: the "
        "current sources don't match the configured ones. Use "
        "Cluster.rejoinInstance() to fix it.");
  }

  // Check the replication channel status
  if (mysqlshdk::mysql::Replication_channel channel;
      mysqlshdk::mysql::get_channel_status(
          *instance, mysqlsh::dba::k_read_replica_async_channel_name,
          &channel)) {
    switch (channel.status()) {
      case mysqlshdk::mysql::Replication_channel::OFF:
      case mysqlshdk::mysql::Replication_channel::APPLIER_OFF:
      case mysqlshdk::mysql::Replication_channel::RECEIVER_OFF: {
        append_error(
            "WARNING: Read Replica's replication channel is stopped. Use "
            "Cluster.rejoinInstance() to restore it.");
        break;
      }
      case mysqlshdk::mysql::Replication_channel::CONNECTION_ERROR: {
        append_error(
            "WARNING: Read Replica's replication channel stopped with a "
            "connection error: '" +
            channel.receiver.last_error.message +
            "'. Use Cluster.rejoinInstance() to restore it.");
        break;
      }
      case mysqlshdk::mysql::Replication_channel::APPLIER_ERROR: {
        std::string applier_error_msg;

        for (const auto &applier : channel.appliers) {
          if (applier.state == Replication_channel::Applier::OFF &&
              applier.last_error.code != 0) {
            applier_error_msg = applier.last_error.message;
            break;
          }
        }

        append_error(
            "WARNING: Read Replica's replication channel stopped with an "
            "error: '" +
            applier_error_msg +
            "'. Use Cluster.rejoinInstance() to restore it.");
        break;
      }
      case mysqlshdk::mysql::Replication_channel::ON:
      case mysqlshdk::mysql::Replication_channel::CONNECTING:
        break;
    }
  }

  return instance_errors;
}

shcore::Dictionary_t Status::feed_read_replica_info(const Read_replica_info &rr,
                                                    bool is_primary) {
  shcore::Dictionary_t rr_dict = shcore::make_dict();

  rr_dict->set("address", shcore::Value(rr.md.endpoint));

  // Get the replication channel status
  auto &rr_instance = m_read_replica_sessions[rr.md.endpoint];

  std::string status;
  if (!rr_instance) {
    status = to_string(mysqlshdk::mysql::Read_replica_status::UNREACHABLE);
  } else {
    status = to_string(mysqlshdk::mysql::get_read_replica_status(*rr_instance));
  }

  rr_dict->set("status", shcore::Value(status));

  if (rr_instance) {
    rr_dict->set("version",
                 shcore::Value(rr_instance->get_version().get_full()));
  }

  rr_dict->set("role", shcore::Value("READ_REPLICA"));

  if (m_extended.value_or(0) >= 1) {
    shcore::Array_t source_list = shcore::make_array();

    // Check whether the channel was configured to follow the
    // primary/secondary or a list of sources
    bool automatic_sources = rr.managed_channel_info.automatic_sources;

    if (automatic_sources && m_extended.value_or(0) < 2) {
      shcore::Value source_attribute;
      if (m_cluster->get_metadata_storage()->query_instance_attribute(
              rr.md.uuid, k_instance_attribute_read_replica_replication_sources,
              &source_attribute)) {
        if (source_attribute.type == shcore::String) {
          source_list->push_back(shcore::Value(source_attribute.as_string()));
        }
      }
    } else {
      for (const auto &source : rr.managed_channel_info.sources) {
        if (m_extended.value_or(0) >= 2) {
          shcore::Dictionary_t source_list_extended = shcore::make_dict();
          (*source_list_extended)["address"] =
              shcore::Value(source.to_string());
          (*source_list_extended)["weight"] = shcore::Value(source.weight);

          source_list->push_back(shcore::Value(source_list_extended));
        } else {
          source_list->push_back(shcore::Value(source.to_string()));
        }
      }
    }

    rr_dict->set("replicationSources", shcore::Value(source_list));

    // SSL info
    if (rr_instance) {
      auto instance_repl_account =
          m_cluster->get_metadata_storage()->get_instance_repl_account(
              rr_instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
              Replica_type::READ_REPLICA);

      std::string ssl_cipher, ssl_version;
      mysqlshdk::mysql::iterate_thread_variables(
          *(m_cluster->get_metadata_storage()->get_md_server()),
          "Binlog Dump GTID", instance_repl_account.first, "Ssl%",
          [&ssl_cipher, &ssl_version](const std::string &var_name,
                                      const std::string &var_value) {
            if (var_name == "Ssl_cipher") {
              ssl_cipher = std::move(var_value);
            } else if (var_name == "Ssl_version") {
              ssl_version = std::move(var_value);
            }

            return (ssl_cipher.empty() || ssl_version.empty());
          });

      if (!(ssl_cipher.empty() && ssl_version.empty())) {
        if (ssl_cipher.empty()) {
          rr_dict->set("replicationSsl", shcore::Value(std::move(ssl_version)));
        } else if (ssl_version.empty()) {
          rr_dict->set("replicationSsl", shcore::Value(std::move(ssl_cipher)));
        } else {
          rr_dict->set("replicationSsl",
                       shcore::Value(shcore::str_format(
                           "%s %s", ssl_cipher.c_str(), ssl_version.c_str())));
        }
      }

      mysqlshdk::mysql::Replication_channel_master_info master_info;
      mysqlshdk::mysql::Replication_channel_relay_log_info relay_info;

      if (mysqlshdk::mysql::get_channel_info(*rr_instance,
                                             k_read_replica_async_channel_name,
                                             &master_info, &relay_info)) {
        auto channel_stats_map =
            channel_status(&rr.repl_channel_info, &master_info, &relay_info, "",
                           m_extended.value_or(0), false, true);

        auto store_dict = [&](std::initializer_list<std::string> keys) {
          for (const auto &key : keys) {
            if (channel_stats_map->has_key(key)) {
              rr_dict->set(key, channel_stats_map->at(key));
            }
          }
        };

        store_dict({"applierStatus", "applierThreadState",
                    "applierWorkerThreads", "receiverStatus",
                    "receiverThreadState", "replicationLag"});

        if (m_extended.value_or(0) >= 2) {
          store_dict({"applierQueuedTransactionSetSize",
                      "applierQueuedTransactionSet",
                      "receiverTimeSinceLastMessage", "coordinatorState",
                      "coordinatorThreadState"});
        }

        if (m_extended.value_or(0) == 3) {
          store_dict({"options"});
        }
      } else {
        log_info("Failed to obtain information from replication channel '%s'",
                 k_read_replica_async_channel_name);
      }
    }
  }

  // Run the diagnostics
  shcore::Array_t instance_errors = nullptr;
  instance_errors = read_replica_diagnostics(rr_instance.get(), rr, is_primary);

  if (!instance_errors->empty()) {
    rr_dict->set("instanceErrors", shcore::Value(instance_errors));
  }

  return rr_dict;
}

shcore::Dictionary_t Status::get_read_replicas_info(
    const Instance_metadata_info &instance_md,
    const std::vector<Read_replica_info> &read_replicas, bool is_primary) {
  shcore::Dictionary_t dict = shcore::make_dict();

  std::vector<Read_replica_info> member_read_replicas, rogue_read_replicas;

  // if there are no read-replicas for this member and it's not the primary
  // (to place rogue read-replicas under it) return right away
  if (!is_primary && read_replicas.empty()) {
    return dict;
  }

  // Get the list of read-replicas for this member
  for (auto rr_info : read_replicas) {
    auto &rr_session = m_read_replica_sessions[rr_info.md.endpoint];

    if (rr_session) {
      if (!rr_info.managed_channel_info.automatic_sources &&
          rr_info.managed_channel_info.sources.empty()) {
        rogue_read_replicas.push_back(rr_info);
        continue;
      }

      // Get the current source member
      mysqlshdk::mysql::Replication_channel channel = {};

      if (mysqlshdk::mysql::get_channel_status(
              *rr_session, k_read_replica_async_channel_name,
              &rr_info.repl_channel_info)) {
        rr_info.current_source_server_uuid =
            std::move(rr_info.repl_channel_info.source_uuid);

        Managed_async_channel channel_config = {};
        channel_config.channel_name = k_read_replica_async_channel_name;

        if (!get_managed_connection_failover_configuration(*rr_session,
                                                           &channel_config)) {
          log_info(
              "Failed to get the information for the Read-Replica managed "
              "replication channel at '%s'",
              rr_info.md.endpoint.c_str());
          rogue_read_replicas.push_back(rr_info);
          continue;
        }

        rr_info.managed_channel_info = std::move(channel_config);

      } else {
        // Instance is a rogue
        rogue_read_replicas.push_back(rr_info);
        continue;
      }

      // If the current source is empty, the instance is a rogue
      // The reason why the source is empty might be because the replica was in
      // error state and the channel was then stopped
      if (rr_info.current_source_server_uuid.empty()) {
        rogue_read_replicas.push_back(rr_info);
        continue;
      }

      // If the current source does not belong to the cluster anymore, the
      // instance must be considered rogue. The instance might have
      // entered an error state and during that period the current source was
      // removed from the cluster
      try {
        // Attempt to get the current source from the Cluster
        m_cluster->get_metadata_storage()->get_instance_by_uuid(
            rr_info.current_source_server_uuid);
      } catch (const shcore::Exception &e) {
        if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) {
          log_info("Error querying metadata for %s: %s\n",
                   rr_info.current_source_server_uuid.c_str(), e.what());
        }

        // The source does not belong to the Cluster anymore, place the
        // read-replica under the primary, as rogue
        rogue_read_replicas.push_back(rr_info);
        continue;
      }

      if (rr_info.current_source_server_uuid ==
          instance_md.actual_server_uuid) {
        member_read_replicas.push_back(rr_info);
        continue;
      }
    } else {
      rogue_read_replicas.push_back(rr_info);
      continue;
    }
  }

  // If this is the primary member, place rogue read-replicas under it
  if (is_primary) {
    for (auto const &rr : rogue_read_replicas) {
      shcore::Dictionary_t rr_dict = feed_read_replica_info(rr, is_primary);

      (*dict)[rr.md.label] = shcore::Value(rr_dict);
    }
  }

  // If there are no other read-replicas for this member, return right away
  if (member_read_replicas.empty()) {
    return dict;
  }

  for (auto const &rr : member_read_replicas) {
    shcore::Dictionary_t rr_dict = feed_read_replica_info(rr, is_primary);

    (*dict)[rr.md.label] = shcore::Value(rr_dict);
  }

  return dict;
}

void Status::prepare() {
  // Sanity check: Verify if the topology type changed and issue an error if
  // needed.
  if (m_cluster->get_cluster_server()) m_cluster->sanity_check();

  m_instances = m_cluster->get_all_instances();

  // Always connect to members to be able to get an accurate mode, based
  // on their super_ready_only value.
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
  auto group_instance = m_cluster->get_cluster_server();

  if (group_instance) {
    auto state = get_replication_group_state(
        *group_instance, get_gr_instance_type(*group_instance));

    bool show_warning = (state.source_state != ManagedInstance::OnlineRW &&
                         state.source_state != ManagedInstance::OnlineRO);

    if (show_warning) {
      std::string warning =
          "The cluster description may be inaccurate as it was generated "
          "from an instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*replicaset_dict)["warning"] = shcore::Value(std::move(warning));
    }
  }

  return shcore::Value(replicaset_dict);
}

shcore::Value Status::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster->get_name());

  // If the Cluster belongs to a ClusterSet, include relevant ClusterSet
  // information:
  //   - domainName
  //   - clusterRole
  //   - clusterSetReplicationStatus
  if (m_cluster->is_cluster_set_member()) {
    auto cs_md = m_cluster->get_clusterset_metadata();
    Cluster_set_metadata cset;

    m_cluster->get_metadata_storage()->get_cluster_set(cs_md.cluster_set_id,
                                                       true, &cset, nullptr);

    (*dict)["domainName"] = shcore::Value(cset.domain_name);
    (*dict)["clusterRole"] =
        shcore::Value(m_cluster->is_primary_cluster() ? "PRIMARY" : "REPLICA");

    // Get the ClusterSet replication channel status
    Cluster_channel_status ch_status =
        m_cluster->get_cluster_set_object()->get_replication_channel_status(
            *m_cluster);

    // If the Cluster is a Replica, add the status right away
    if (!m_cluster->is_primary_cluster()) {
      (*dict)["clusterSetReplicationStatus"] =
          shcore::Value(to_string(ch_status));
    } else {
      // If it's a Primary, add the status in case is suspicious (not Missing
      // or Stopped)
      if (ch_status != Cluster_channel_status::MISSING &&
          ch_status != Cluster_channel_status::STOPPED) {
        (*dict)["clusterSetReplicationStatus"] =
            shcore::Value(to_string(ch_status));
      }
    }
  }

  // Get the default replicaSet options
  (*dict)["defaultReplicaSet"] = shcore::Value(get_default_replicaset_status());

  if (m_cluster->get_cluster_server()) {
    // Gets the metadata version
    if (m_extended.value_or(0) >= 1) {
      auto version = mysqlsh::dba::metadata::installed_version(
          m_cluster->get_cluster_server());
      (*dict)["metadataVersion"] = shcore::Value(version.get_base());
    }

    // Iterate all replicasets and get the status for each one

    std::string addr = m_cluster->get_cluster_server()->get_canonical_address();
    (*dict)["groupInformationSourceMember"] = shcore::Value(addr);
  }

  auto md_server = m_cluster->get_metadata_storage()->get_md_server();

  // metadata server, if its a different one
  if (md_server &&
      (!m_cluster->get_cluster_server() ||
       md_server->get_uuid() != m_cluster->get_cluster_server()->get_uuid())) {
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
