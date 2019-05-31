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

#include "mysqlshdk/libs/mysql/replication.h"
#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

std::string to_string(Replication_channel::Status status) {
  switch (status) {
    case Replication_channel::OFF:
      return "OFF";
    case Replication_channel::ON:
      return "ON";
    case Replication_channel::RECEIVER_OFF:
      return "RECEIVER_OFF";
    case Replication_channel::APPLIER_OFF:
      return "APPLIER_OFF";
    case Replication_channel::CONNECTING:
      return "CONNECTING";
    case Replication_channel::CONNECTION_ERROR:
      return "CONNECTION_ERROR";
    case Replication_channel::APPLIER_ERROR:
      return "APPLIER_ERROR";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Receiver::State state) {
  switch (state) {
    case Replication_channel::Receiver::ON:
      return "OK";
    case Replication_channel::Receiver::OFF:
      return "OFF";
    case Replication_channel::Receiver::CONNECTING:
      return "CONNECTING";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Coordinator::State state) {
  switch (state) {
    case Replication_channel::Coordinator::ON:
      return "OK";
    case Replication_channel::Coordinator::OFF:
      return "OFF";
    case Replication_channel::Coordinator::NONE:
      return "N/A";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Applier::State state) {
  switch (state) {
    case Replication_channel::Applier::ON:
      return "OK";
    case Replication_channel::Applier::OFF:
      return "OFF";
  }
  throw std::logic_error("internal error");
}

std::string to_string(const Replication_channel::Error &error) {
  if (error.code != 0) {
    return shcore::str_format("%s (%i) at %s", error.message.c_str(),
                              error.code, error.timestamp.c_str());
  }
  return "";
}

std::string format_status(const Replication_channel &channel, bool verbose) {
  std::string msg;
  if (!verbose && channel.status() == Replication_channel::ON) {
    msg = "Channel '" + channel.channel_name + "' is running.";
  } else if (!verbose && channel.status() != Replication_channel::OFF) {
    msg = "Channel '" + channel.channel_name + "' is stopped.";
  } else {
    msg = "Channel: " + channel.channel_name;
    if (verbose) {
      msg += "\nUser: " + channel.user;
    }
    msg += "\nReceiver state: " + to_string(channel.receiver.state);
    if (channel.receiver.last_error.code != 0)
      msg += "\n\tLast error: " + to_string(channel.receiver.last_error);
    if (channel.coordinator.state != Replication_channel::Coordinator::NONE) {
      msg += "\nCoordinator state: " + to_string(channel.coordinator.state);
      if (channel.coordinator.last_error.code != 0)
        msg += "\n\tLast error: " + to_string(channel.coordinator.last_error);
    }
    if (channel.appliers.size() == 1) {
      msg += "\nApplier state: " + to_string(channel.appliers.front().state);
      if (channel.appliers.front().last_error.code != 0)
        msg +=
            "\n\tLast error: " + to_string(channel.appliers.front().last_error);
    } else {
      int i = 0;
      for (const auto &a : channel.appliers) {
        msg +=
            "\nApplier " + std::to_string(i) + " state: " + to_string(a.state);
        if (a.last_error.code != 0)
          msg += "\n\tLast error: " + to_string(a.last_error);
        ++i;
      }
    }
  }

  return msg;
}

namespace {
void extract_error(Replication_channel::Error *error,
                   const mysqlshdk::db::Row_ref_by_name &row,
                   const std::string &prefix) {
  if (!row.is_null(prefix + "errno")) {
    error->code = row.get_int(prefix + "errno");
    error->message = row.get_string(prefix + "errmsg");
    error->timestamp = row.get_string(prefix + "errtime");
  }
}

void unserialize_channel_info(const mysqlshdk::db::Row_ref_by_name &row,
                              Replication_channel *channel) {
  channel->channel_name = row.get_string("channel_name");
  channel->user = row.get_string("user");
  channel->host = row.get_string("host");
  channel->port = row.get_int("port");
  channel->source_uuid = row.get_string("source_uuid");
  channel->group_name =
      row.is_null("group_name") ? "" : row.get_string("group_name");
  channel->last_heartbeat_timestamp =
      row.get_string("last_heartbeat_timestamp");

  if (!row.is_null("io_state")) {
    std::string state = row.get_string("io_state");
    if (state == "ON")
      channel->receiver.state = Replication_channel::Receiver::ON;
    else if (state == "OFF")
      channel->receiver.state = Replication_channel::Receiver::OFF;
    else if (state == "CONNECTING")
      channel->receiver.state = Replication_channel::Receiver::CONNECTING;
    else
      assert(0);

    channel->receiver.thread_state = row.get_string("io_thread_state", "");

    extract_error(&channel->receiver.last_error, row, "io_");
  }

  if (!row.is_null("co_state")) {
    std::string state = row.get_string("co_state");
    if (state == "ON")
      channel->coordinator.state = Replication_channel::Coordinator::ON;
    else if (state == "OFF")
      channel->coordinator.state = Replication_channel::Coordinator::OFF;
    else
      assert(0);

    channel->coordinator.thread_state = row.get_string("co_thread_state", "");

    extract_error(&channel->coordinator.last_error, row, "co_");
  }
}

void unserialize_channel_applier_info(const mysqlshdk::db::Row_ref_by_name &row,
                                      Replication_channel *channel) {
  if (!row.is_null("w_state")) {
    Replication_channel::Applier applier;
    std::string state = row.get_string("w_state");
    if (state == "ON")
      applier.state = Replication_channel::Applier::ON;
    else if (state == "OFF")
      applier.state = Replication_channel::Applier::OFF;
    else
      assert(0);

    applier.thread_state = row.get_string("w_thread_state", "");
    applier.last_applied_trx_delay = row.has_field("w_last_trx_delay")
                                         ? row.get_uint("w_last_trx_delay")
                                         : 0;

    extract_error(&applier.last_error, row, "w_");
    channel->appliers.push_back(applier);
  }
}

static const char *k_base_channel_query = R"*(
  SELECT
    c.channel_name, c.host, c.port, c.user,
    s.source_uuid, s.group_name, s.last_heartbeat_timestamp,
    s.service_state io_state, st.processlist_state io_thread_state,
    s.last_error_number io_errno, s.last_error_message io_errmsg,
    s.last_error_timestamp io_errtime,
    co.service_state co_state, cot.processlist_state co_thread_state,
    co.last_error_number co_errno, co.last_error_message co_errmsg,
    co.last_error_timestamp co_errtime,
    w.service_state w_state, wt.processlist_state w_thread_state,
    w.last_error_number w_errno, w.last_error_message w_errmsg,
    w.last_error_timestamp w_errtime /*!80011 ,
    CAST(w.last_applied_transaction_end_apply_timestamp
      - w.last_applied_transaction_original_commit_timestamp as UNSIGNED)
      as w_last_trx_delay */
  FROM performance_schema.replication_connection_configuration c
  JOIN performance_schema.replication_connection_status s
    ON c.channel_name = s.channel_name
  LEFT JOIN performance_schema.replication_applier_status_by_coordinator co
    ON c.channel_name = co.channel_name
  JOIN performance_schema.replication_applier_status a
    ON c.channel_name = a.channel_name
  JOIN performance_schema.replication_applier_status_by_worker w
    ON c.channel_name = w.channel_name
  LEFT JOIN performance_schema.threads st
    ON s.thread_id = st.thread_id
  LEFT JOIN performance_schema.threads cot
    ON co.thread_id = cot.thread_id
  LEFT JOIN performance_schema.threads wt
    ON w.thread_id = wt.thread_id
)*";

}  // namespace

std::vector<Replication_channel> get_incoming_channels(
    const mysqlshdk::mysql::IInstance &instance) {
  std::vector<Replication_channel> channels;

  auto result = instance.query(std::string(k_base_channel_query) +
                               "ORDER BY channel_name");

  if (auto row = result->fetch_one_named()) {
    Replication_channel channel;
    unserialize_channel_info(row, &channel);
    unserialize_channel_applier_info(row, &channel);
    while ((row = result->fetch_one_named())) {
      if (row.get_string("channel_name") != channel.channel_name) {
        channels.push_back(channel);
        channel.appliers.clear();
        unserialize_channel_info(row, &channel);
      }
      unserialize_channel_applier_info(row, &channel);
    }
    channels.push_back(channel);
  }

  return channels;
}

bool get_channel_status(const mysqlshdk::mysql::IInstance &instance,
                        const std::string &channel_name,
                        Replication_channel *out_channel) {
  auto result = instance.queryf(
      std::string(k_base_channel_query) + "WHERE c.channel_name = ?",
      channel_name);

  if (auto row = result->fetch_one_named()) {
    Replication_channel channel;
    unserialize_channel_info(row, out_channel);
    unserialize_channel_applier_info(row, out_channel);
    while ((row = result->fetch_one_named())) {
      if (out_channel) unserialize_channel_applier_info(row, out_channel);
    }
    return true;
  }
  return false;
}

std::vector<Slave_host> get_slaves(
    const mysqlshdk::mysql::IInstance &instance) {
  std::vector<Slave_host> slaves;

  auto result = instance.query("SHOW SLAVE HOSTS");
  while (auto row = result->fetch_one_named()) {
    Slave_host host;
    host.host = row.get_string("Host");
    host.port = row.get_uint("Port");
    host.uuid = row.get_string("Slave_UUID");
    slaves.push_back(host);
  }

  return slaves;
}

Replication_channel::Status Replication_channel::status() const {
  bool applier_off = false;

  int num_applier_off = 0;
  for (const auto &applier : appliers) {
    if (applier.state == Applier::OFF) {
      if (applier.last_error.code != 0) {
        // error is the strongest status
        return APPLIER_ERROR;
      } else {
        num_applier_off++;
      }
    }
  }
  if (num_applier_off == static_cast<int>(appliers.size()))
    applier_off = true;
  else if (num_applier_off > 0)
    return APPLIER_ERROR;  // some ON and some OFF is treated as ERROR

  if (coordinator.state == Coordinator::OFF) {
    if (coordinator.last_error.code != 0) {
      return APPLIER_ERROR;
    }
    // If we have a coordinator and it's OFF but the appliers are not, then
    // it's an error
    if (!applier_off) {
      return APPLIER_ERROR;
    }
  }

  if (receiver.state == Receiver::OFF) {
    if (receiver.last_error.code != 0) {
      return CONNECTION_ERROR;
    }
    if (applier_off) return OFF;
    return RECEIVER_OFF;
  } else if (receiver.state == Receiver::CONNECTING) {
    if (receiver.last_error.code != 0) {
      return CONNECTION_ERROR;
    }
    if (applier_off) return APPLIER_OFF;
    return CONNECTING;
  } else {
    if (applier_off) return APPLIER_OFF;
  }
  return ON;
}

Replication_channel wait_replication_done_connecting(
    const mysqlshdk::mysql::IInstance &slave, const std::string &channel_name,
    int timeout) {
  time_t start_time = time(nullptr);
  // check that the connection thread for the channel switches to ON
  // and if not, throw an exception

  Replication_channel channel;

  do {
    if (!get_channel_status(slave, channel_name, &channel)) {
      throw std::runtime_error(
          "Replication channel could not be found at the instance");
    } else {
      if (channel.receiver.state == Replication_channel::Receiver::CONNECTING &&
          channel.receiver.last_error.code == 0) {
        // channel is still connecting, try again after a little
        shcore::sleep_ms(200);
      } else {
        return channel;
      }
    }
  } while (time(nullptr) - start_time < timeout);

  return channel;
}

bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &gtid_set, int timeout) {
  // NOTE: According to the GR team the max supported timeout value for
  //       WAIT_FOR_EXECUTED_GTID_SET() is 18446744073.7096 seconds
  //       (2^64/1000000000). Therefore, a type with a max value that include
  //       that range should be used.
  auto result = instance.queryf("SELECT WAIT_FOR_EXECUTED_GTID_SET(?, ?)",
                                gtid_set, timeout);
  auto row = result->fetch_one();
  // WAIT_FOR_EXECUTED_GTID_SET() returns 0 for success, 1 for timeout,
  // otherwise an error is generated.
  return row->get_int(0) == 0;
}

bool wait_for_gtid_set_from(const mysqlshdk::mysql::IInstance &target,
                            const mysqlshdk::mysql::IInstance &source,
                            int timeout) {
  auto result = source.query("SELECT @@GLOBAL.GTID_EXECUTED");
  auto row = result->fetch_one();
  if (!row || row->is_null(0) || row->get_string(0).empty()) return true;

  return wait_for_gtid_set(target, row->get_string(0), timeout);
}

size_t estimate_gtid_set_size(const std::string &gtid_set) {
  size_t count = 0;
  shcore::str_itersplit(
      gtid_set,
      [&count](const std::string &s) {
        size_t p = s.find(':');
        if (p != std::string::npos) {
          size_t begin, end;
          switch (sscanf(&s[p + 1], "%zd-%zd", &begin, &end)) {
            case 2:
              count += end - begin + 1;
              break;
            case 1:
              count++;
              break;
          }
        }
        return true;
      },
      ",");
  return count;
}

std::string get_executed_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  bool include_received) {
  // session GTID_EXECUTED is different from the global one in 5.7
  if (include_received) {
    return server.queryf_one_string(
        0, "",
        "SELECT CONCAT(@@GLOBAL.GTID_EXECUTED, "
        "   (SELECT "
        "     COALESCE(CONCAT(',', group_concat(received_transaction_set)), '')"
        "    FROM performance_schema.replication_connection_status))");
  } else {
    return server.queryf_one_string(0, "", "SELECT @@GLOBAL.GTID_EXECUTED");
  }
}

std::string get_purged_gtid_set(const mysqlshdk::mysql::IInstance &server) {
  return server.queryf_one_string(0, "", "SELECT @@GLOBAL.GTID_PURGED");
}

std::string get_received_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  const std::string &channel_name) {
  return server.queryf_one_string(
      0, "",
      "SELECT GROUP_CONCAT(received_transaction_set)"
      "   FROM performance_schema.replication_connection_status"
      "   WHERE channel_name = ?",
      channel_name);
}

Gtid_set_relation compare_gtid_sets(const mysqlshdk::mysql::IInstance &server,
                                    const std::string &gtidset_a,
                                    const std::string &gtidset_b,
                                    std::string *out_missing_from_a,
                                    std::string *out_missing_from_b) {
  if (out_missing_from_a) *out_missing_from_a = gtidset_b;
  if (out_missing_from_b) *out_missing_from_b = gtidset_a;

  if (gtidset_a.empty()) {
    if (gtidset_b.empty())
      return Gtid_set_relation::EQUAL;
    else
      return Gtid_set_relation::CONTAINED;
  } else if (gtidset_b.empty()) {
    return Gtid_set_relation::CONTAINS;
  }

  // Set some session tempvars for caching
  server.executef("SET @gtidset_a=?", gtidset_a);
  server.executef("SET @gtidset_b=?", gtidset_b);

  std::string a_sub_b = server.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(@gtidset_a, @gtidset_b)");
  std::string b_sub_a = server.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(@gtidset_b, @gtidset_a)");

  if (out_missing_from_a) *out_missing_from_a = b_sub_a;
  if (out_missing_from_b) *out_missing_from_b = a_sub_b;

  if (a_sub_b.empty() && b_sub_a.empty()) {
    return Gtid_set_relation::EQUAL;
  } else if (a_sub_b.empty() && !b_sub_a.empty()) {
    return Gtid_set_relation::CONTAINED;
  } else if (!a_sub_b.empty() && b_sub_a.empty()) {
    return Gtid_set_relation::CONTAINS;
  } else {
    std::string ab_intersection =
        server.queryf_one_string(0, "",
                                 "SELECT GTID_SUBTRACT(@gtidset_a, "
                                 "GTID_SUBTRACT(@gtidset_a, @gtidset_b))");
    if (ab_intersection.empty())
      return Gtid_set_relation::DISJOINT;
    else
      return Gtid_set_relation::INTERSECTS;
  }
}

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &master,
    const mysqlshdk::mysql::IInstance &slave, bool include_received,
    std::string *out_missing_gtids, std::string *out_errant_gtids) {
  auto master_gtid = get_executed_gtid_set(master, include_received);
  auto master_purged_gtid = get_purged_gtid_set(master);
  auto slave_gtid = get_executed_gtid_set(slave, include_received);
  return check_replica_gtid_state(master, master_gtid, master_purged_gtid,
                                  slave_gtid, out_missing_gtids,
                                  out_errant_gtids);
}

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &master_gtidset, const std::string &master_purged_gtidset,
    const std::string &slave_gtidset, std::string *out_missing_gtids,
    std::string *out_errant_gtids) {
  Gtid_set_relation rel =
      compare_gtid_sets(instance, master_gtidset, slave_gtidset,
                        out_errant_gtids, out_missing_gtids);

  switch (rel) {
    case Gtid_set_relation::DISJOINT:
    case Gtid_set_relation::INTERSECTS:
    case Gtid_set_relation::CONTAINED:
      return Replica_gtid_state::DIVERGED;

    case Gtid_set_relation::EQUAL:
      if (slave_gtidset.empty()) {
        return Replica_gtid_state::NEW;
      }
      return Replica_gtid_state::IDENTICAL;

    case Gtid_set_relation::CONTAINS:
      // If purged has more gtids than the executed on the slave
      // it means some data will not be recoverable
      if (master_purged_gtidset.empty() ||
          instance
              .queryf("SELECT GTID_SUBTRACT(?, ?) = ''", master_purged_gtidset,
                      slave_gtidset)
              ->fetch_one_or_throw()
              ->get_int(0)) {
        if (slave_gtidset.empty()) {
          return Replica_gtid_state::NEW;
        }
        return Replica_gtid_state::RECOVERABLE;
      } else {
        return Replica_gtid_state::IRRECOVERABLE;
      }
  }

  throw std::logic_error("internal error");
}

int64_t generate_server_id() {
  // Setup uniform random generation of integers between [1, 4294967295] using
  // Mersenne Twister algorithm and a non-determinist seed.
  std::random_device rd_seed;
  std::mt19937 rnd_gen(rd_seed());
  std::uniform_int_distribution<int64_t> distribution(1, 4294967295);
  return distribution(rnd_gen);
}

bool is_async_replication_running(const mysqlshdk::mysql::IInstance &instance) {
  auto session = instance.get_session();
  std::string receiver_channel_state, applier_channel_state;

  assert(session);

  try {
    // Get the state of the receiver and applier channels
    // NOTE:
    // - Values of receiver channel can be: ON, OFF, or CONNECTING
    // - Values of applier channel can be: ON, OFF
    std::string query(
        "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
        "AS APPLIER FROM "
        "performance_schema.replication_connection_status a, "
        "performance_schema.replication_applier_status b WHERE "
        "a.CHANNEL_NAME != 'group_replication_applier' AND "
        "a.CHANNEL_NAME != 'group_replication_recovery' AND "
        "b.CHANNEL_NAME != 'group_replication_applier' AND "
        "b.CHANNEL_NAME != 'group_replication_recovery'");

    log_debug("Executing query '%s'.", query.c_str());

    auto resultset = session->query(query);
    auto row = resultset->fetch_one();

    // If the query returned no values it means async replication channels are
    // not set
    if (!row) {
      log_debug("Query returned no results.");
      return false;
    }

    receiver_channel_state = row->get_string(0);
    applier_channel_state = row->get_string(1);

    // If any of the channels is running, we can consider async replication
    // is running
    if ((receiver_channel_state != "OFF") || (applier_channel_state != "OFF")) {
      return true;
    } else {
      return false;
    }
  } catch (const std::exception &e) {
    log_error("Error checking asynchronous replication status: %s", e.what());
    throw;
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
