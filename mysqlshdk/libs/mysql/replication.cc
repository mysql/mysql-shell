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

#include "mysqlshdk/libs/mysql/replication.h"

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/errors.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/structured_text.h"
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
      return "ON";
    case Replication_channel::Receiver::OFF:
      return "OFF";
    case Replication_channel::Receiver::CONNECTING:
      return "CONNECTING";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Receiver::Status status) {
  switch (status) {
    case Replication_channel::Receiver::Status::ON:
      return "ON";
    case Replication_channel::Receiver::Status::OFF:
      return "OFF";
    case Replication_channel::Receiver::Status::CONNECTING:
      return "CONNECTING";
    case Replication_channel::Receiver::Status::ERROR:
      return "ERROR";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Coordinator::State state) {
  switch (state) {
    case Replication_channel::Coordinator::ON:
      return "ON";
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
      return "ON";
    case Replication_channel::Applier::OFF:
      return "OFF";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Replication_channel::Applier::Status status) {
  switch (status) {
    case Replication_channel::Applier::Status::APPLYING:
      return "APPLYING";
    case Replication_channel::Applier::Status::APPLIED_ALL:
      return "APPLIED_ALL";
    case Replication_channel::Applier::Status::ON:
      return "ON";
    case Replication_channel::Applier::Status::ERROR:
      return "ERROR";
    case Replication_channel::Applier::Status::OFF:
      return "OFF";
  }
  throw std::logic_error("internal error");
}

std::string to_string(const Replication_channel::Error &error) {
  if (error.code == 0) return {};
  return shcore::str_format("%s (%i) at %s", error.message.c_str(), error.code,
                            error.timestamp.c_str());
}

std::string format_status(const Replication_channel &channel, bool verbose) {
  // This follows the KVP format

  std::string msg;
  msg.reserve(128);  // better than nothing

  msg += shcore::make_kvp(
      "source", shcore::str_format("%s:%d", channel.host.c_str(), channel.port),
      '"');
  msg.append(" ").append(shcore::make_kvp("channel", channel.channel_name));
  msg.append(" ").append(
      shcore::make_kvp("status", to_string(channel.status())));

  if (!verbose && channel.status() == Replication_channel::OFF) return msg;

  msg += " " + shcore::make_kvp("receiver", to_string(channel.receiver.state));
  if (channel.receiver.last_error.code != 0)
    msg += " " + shcore::make_kvp("last_error",
                                  to_string(channel.receiver.last_error), '"');

  if (channel.coordinator.state != Replication_channel::Coordinator::NONE) {
    msg += " " + shcore::make_kvp("coordinator",
                                  to_string(channel.coordinator.state));
    if (channel.coordinator.last_error.code != 0)
      msg += " " + shcore::make_kvp("last_error",
                                    to_string(channel.coordinator.last_error),
                                    '"');
  }

  if (channel.appliers.size() == 1) {
    msg += " " + shcore::make_kvp("applier",
                                  to_string(channel.appliers.front().state));
    if (channel.appliers.front().last_error.code != 0)
      msg += " " + shcore::make_kvp(
                       "last_error",
                       to_string(channel.appliers.front().last_error), '"');
  } else {
    int i = 0;
    for (const auto &a : channel.appliers) {
      msg += " " + shcore::make_kvp(("applier" + std::to_string(i)).c_str(),
                                    to_string(a.state));
      if (a.last_error.code != 0)
        msg += " " + shcore::make_kvp("last_error", to_string(a.last_error));
      ++i;
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

  channel->time_since_last_message =
      row.has_field("time_since_last_message")
          ? row.get_string("time_since_last_message", "")
          : "";
  channel->repl_lag_from_original =
      row.has_field("lag_from_original")
          ? row.get_string("lag_from_original", "")
          : "";
  channel->repl_lag_from_immediate =
      row.has_field("lag_from_immediate")
          ? row.get_string("lag_from_immediate", "")
          : "";
  channel->queued_gtid_set_to_apply =
      row.get_string("queued_gtid_set_to_apply", "");

  if (!row.is_null("io_state")) {
    std::string state = row.get_string("io_state");
    if (state == "ON") {
      channel->receiver.state = Replication_channel::Receiver::ON;
      channel->receiver.status = Replication_channel::Receiver::Status::ON;
    } else if (state == "OFF") {
      channel->receiver.state = Replication_channel::Receiver::OFF;
      channel->receiver.status = Replication_channel::Receiver::Status::OFF;
    } else if (state == "CONNECTING") {
      channel->receiver.state = Replication_channel::Receiver::CONNECTING;
      channel->receiver.status =
          Replication_channel::Receiver::Status::CONNECTING;
    } else {
      assert(!"Unknown repl channel service state");
    }

    channel->receiver.thread_state = row.get_string("io_thread_state", "");

    extract_error(&channel->receiver.last_error, row, "io_");
    if (channel->receiver.last_error.code != 0) {
      channel->receiver.status = Replication_channel::Receiver::Status::ERROR;
    }
  }

  if (!row.is_null("co_state")) {
    std::string state = row.get_string("co_state");
    if (state == "ON")
      channel->coordinator.state = Replication_channel::Coordinator::ON;
    else if (state == "OFF")
      channel->coordinator.state = Replication_channel::Coordinator::OFF;
    else
      assert(!"Unknown repl channel coordinator state");

    channel->coordinator.thread_state = row.get_string("co_thread_state", "");

    extract_error(&channel->coordinator.last_error, row, "co_");
  }
}

void unserialize_channel_applier_info(const mysqlshdk::db::Row_ref_by_name &row,
                                      Replication_channel *channel) {
  if (row.is_null("w_state")) return;

  Replication_channel::Applier applier;
  if (auto state = row.get_string("w_state"); state == "ON")
    applier.state = Replication_channel::Applier::ON;
  else if (state == "OFF")
    applier.state = Replication_channel::Applier::OFF;
  else
    assert(!"Unknown applier state");

  if (applier.state == Replication_channel::Applier::ON) {
    if (row.has_field("applier_busy_state")) {  // 8.0 only
      if (row.get_string("applier_busy_state") == "APPLYING")
        applier.status = Replication_channel::Applier::Status::APPLYING;
      else
        applier.status = Replication_channel::Applier::Status::APPLIED_ALL;
    } else {
      applier.status = Replication_channel::Applier::Status::ON;
    }
  } else {
    if (applier.last_error.code != 0)
      applier.status = Replication_channel::Applier::Status::ERROR;
    else
      applier.status = Replication_channel::Applier::Status::OFF;
  }

  applier.thread_state = row.get_string("w_thread_state", "");

  extract_error(&applier.last_error, row, "w_");
  channel->appliers.push_back(applier);
}

static const char *k_base_channel_query = R"*(SELECT
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
    w.last_error_timestamp w_errtime,
    /*!80011 TIMEDIFF(NOW(6),
      IF(TIMEDIFF(s.LAST_QUEUED_TRANSACTION_START_QUEUE_TIMESTAMP,
          s.LAST_HEARTBEAT_TIMESTAMP) >= 0,
        s.LAST_QUEUED_TRANSACTION_START_QUEUE_TIMESTAMP,
        s.LAST_HEARTBEAT_TIMESTAMP
      )) as time_since_last_message,
    IF(s.LAST_QUEUED_TRANSACTION='' OR s.LAST_QUEUED_TRANSACTION=latest_w.LAST_APPLIED_TRANSACTION,
      'IDLE',
      'APPLYING') as applier_busy_state,
    IF(s.LAST_QUEUED_TRANSACTION='' OR s.LAST_QUEUED_TRANSACTION=latest_w.LAST_APPLIED_TRANSACTION,
      NULL,
      TIMEDIFF(latest_w.LAST_APPLIED_TRANSACTION_END_APPLY_TIMESTAMP,
        latest_w.LAST_APPLIED_TRANSACTION_ORIGINAL_COMMIT_TIMESTAMP)
      ) as lag_from_original,
    IF(s.LAST_QUEUED_TRANSACTION='' OR s.LAST_QUEUED_TRANSACTION=latest_w.LAST_APPLIED_TRANSACTION,
      NULL,
      TIMEDIFF(latest_w.LAST_APPLIED_TRANSACTION_END_APPLY_TIMESTAMP,
        latest_w.LAST_APPLIED_TRANSACTION_IMMEDIATE_COMMIT_TIMESTAMP)
      ) as lag_from_immediate,
    */
    GTID_SUBTRACT(s.RECEIVED_TRANSACTION_SET, @@global.gtid_executed)
      as queued_gtid_set_to_apply
  FROM performance_schema.replication_connection_configuration c
  JOIN performance_schema.replication_connection_status s
    ON c.channel_name = s.channel_name
  LEFT JOIN performance_schema.replication_applier_status_by_coordinator co
    ON c.channel_name = co.channel_name
  JOIN performance_schema.replication_applier_status a
    ON c.channel_name = a.channel_name
  JOIN performance_schema.replication_applier_status_by_worker w
    ON c.channel_name = w.channel_name
  LEFT JOIN
  /* if parallel replication, fetch owner of most recently applied tx */
    (SELECT *
      FROM performance_schema.replication_applier_status_by_worker
      /*!80011 ORDER BY LAST_APPLIED_TRANSACTION_END_APPLY_TIMESTAMP DESC */
      LIMIT 1) latest_w
    ON c.channel_name = latest_w.channel_name
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

std::vector<std::string> get_incoming_channel_names(
    const mysqlshdk::mysql::IInstance &instance, bool include_stopped) {
  const char *q_all =
      "SELECT channel_name"
      " FROM performance_schema.replication_connection_status";
  const char *q =
      "SELECT channel_name"
      " FROM performance_schema.replication_connection_status"
      " WHERE service_state <> 'OFF'";

  std::vector<std::string> names;

  auto res = instance.query(include_stopped ? q_all : q);
  while (auto row = res->fetch_one()) {
    names.emplace_back(row->get_string(0));
  }

  return names;
}

bool get_channel_status(const mysqlshdk::mysql::IInstance &instance,
                        std::string_view channel_name,
                        Replication_channel *out_channel) {
  auto result = instance.queryf(
      std::string(k_base_channel_query) + "WHERE c.channel_name = ?",
      channel_name);

  auto row = result->fetch_one_named();
  if (!row) return false;

  if (out_channel) {
    unserialize_channel_info(row, out_channel);
    unserialize_channel_applier_info(row, out_channel);

    while ((row = result->fetch_one_named())) {
      unserialize_channel_applier_info(row, out_channel);
    }
  }

  return true;
}

bool get_channel_state(const mysqlshdk::mysql::IInstance &instance,
                       std::string_view channel_name,
                       Replication_channel::Receiver::State *out_io_state,
                       Replication_channel::Applier::State *out_sql_state) {
  auto result =
      instance.queryf("SHOW " + get_replica_keyword(instance.get_version()) +
                          " STATUS FOR CHANNEL ?",
                      channel_name);
  if (auto row = result->fetch_one()) {
    auto io_state = row->get_string(10);
    if (io_state == "Yes") {
      *out_io_state = Replication_channel::Receiver::ON;
    } else if (io_state == "No") {
      *out_io_state = Replication_channel::Receiver::OFF;
    } else if (io_state == "Connecting") {
      *out_io_state = Replication_channel::Receiver::CONNECTING;
    } else {
      throw std::logic_error("Unexpected value for Replica_IO_Running: " +
                             io_state);
    }

    auto sql_state = row->get_string(11);
    if (sql_state == "Yes") {
      *out_sql_state = Replication_channel::Applier::ON;
    } else if (sql_state == "No") {
      *out_sql_state = Replication_channel::Applier::OFF;
    } else {
      throw std::logic_error("Unexpected value for Replica_SQL_Running: " +
                             sql_state);
    }

    return true;
  }
  return false;
}

bool get_channel_state(const mysqlshdk::mysql::IInstance &instance,
                       std::string_view channel_name,
                       Replication_channel::Receiver::State *out_io_state,
                       Replication_channel::Error *out_io_error,
                       Replication_channel::Applier::State *out_sql_state,
                       Replication_channel::Error *out_sql_error) {
  auto result =
      instance.queryf("SHOW " + get_replica_keyword(instance.get_version()) +
                          " STATUS FOR CHANNEL ?",
                      channel_name);

  std::string source_column_prefix =
      shcore::str_lower(get_replica_keyword(instance.get_version()));
  source_column_prefix[0] = ::toupper(source_column_prefix[0]);

  if (auto row = result->fetch_one_named()) {
    auto io_state = row.get_string(source_column_prefix + "_IO_Running");
    auto sql_state = row.get_string(source_column_prefix + "_SQL_Running");

    out_io_error->code = row.get_int("Last_IO_Errno");
    out_io_error->message = row.get_string("Last_IO_Error");
    out_io_error->timestamp = row.get_string("Last_IO_Error_Timestamp");

    out_sql_error->code = row.get_int("Last_SQL_Errno");
    out_sql_error->message = row.get_string("Last_SQL_Error");
    out_sql_error->timestamp = row.get_string("Last_SQL_Error_Timestamp");

    if (io_state == "Yes") {
      *out_io_state = Replication_channel::Receiver::ON;
    } else if (io_state == "No") {
      *out_io_state = Replication_channel::Receiver::OFF;
    } else if (io_state == "Connecting") {
      *out_io_state = Replication_channel::Receiver::CONNECTING;
    } else {
      throw std::logic_error("Unexpected value for Replica_IO_Running: " +
                             io_state);
    }

    if (sql_state == "Yes") {
      *out_sql_state = Replication_channel::Applier::ON;
    } else if (sql_state == "No") {
      *out_sql_state = Replication_channel::Applier::OFF;
    } else {
      throw std::logic_error("Unexpected value for Replica_SQL_Running: " +
                             sql_state);
    }
    return true;
  }
  return false;
}

namespace {

void unserialize_channel_master_info(
    const mysqlshdk::db::Row_ref_by_name &row,
    Replication_channel_master_info *out_master_info) {
  out_master_info->master_log_name = row.get_string("Master_log_name");
  out_master_info->master_log_pos = row.get_uint("Master_log_pos");
  out_master_info->host = row.get_string("Host");
  out_master_info->user_name = row.get_string("User_name");
  out_master_info->port = row.get_int("Port");
  out_master_info->connect_retry = row.get_uint("Connect_retry");
  out_master_info->enabled_ssl = row.get_int("Enabled_ssl");
  out_master_info->ssl_ca = row.get_string("Ssl_ca");
  out_master_info->ssl_capath = row.get_string("Ssl_capath");
  out_master_info->ssl_cert = row.get_string("Ssl_cert");
  out_master_info->ssl_cipher = row.get_string("Ssl_cipher");
  out_master_info->ssl_key = row.get_string("Ssl_key");
  out_master_info->ssl_verify_server_cert =
      row.get_int("Ssl_verify_server_cert");
  out_master_info->heartbeat_period = row.get_double("Heartbeat");
  out_master_info->bind = row.get_string("Bind");
  out_master_info->ignored_server_ids = row.get_string("Ignored_server_ids");
  out_master_info->uuid = row.get_string("Uuid");
  out_master_info->retry_count = row.get_uint("Retry_count");
  out_master_info->ssl_crl = row.get_string("Ssl_crl");
  out_master_info->ssl_crlpath = row.get_string("Ssl_crlpath");
  out_master_info->enabled_auto_position = row.get_int("Enabled_auto_position");
  out_master_info->channel_name = row.get_string("Channel_name");
  out_master_info->tls_version = row.get_string("Tls_version");

  if (row.has_field("Public_key_path") && !row.is_null("Public_key_path"))
    out_master_info->public_key_path = row.get_string("Public_key_path");
  if (row.has_field("Get_public_key") && !row.is_null("Get_public_key"))
    out_master_info->get_public_key = row.get_int("Get_public_key");
  if (row.has_field("Network_namespace") && !row.is_null("Network_namespace"))
    out_master_info->network_namespace = row.get_string("Network_namespace");
  if (row.has_field("Master_compression_algorithm") &&
      !row.is_null("Master_compression_algorithm"))
    out_master_info->compression_algorithm =
        row.get_string("Master_compression_algorithm");
  if (row.has_field("Master_zstd_compression_level") &&
      !row.is_null("Master_zstd_compression_level"))
    out_master_info->zstd_compression_level =
        row.get_uint("Master_zstd_compression_level");
  if (row.has_field("Tls_ciphersuites") && !row.is_null("Tls_ciphersuites"))
    out_master_info->tls_ciphersuites = row.get_string("Tls_ciphersuites");
}

void unserialize_channel_relay_log_info(
    const mysqlshdk::db::Row_ref_by_name &row,
    Replication_channel_relay_log_info *out_relay_log_info) {
  out_relay_log_info->relay_log_name = row.get_string("Relay_log_name", "");
  out_relay_log_info->relay_log_pos = row.get_uint("Relay_log_pos", 0);
  out_relay_log_info->master_log_name = row.get_string("Master_log_name");
  out_relay_log_info->master_log_pos = row.get_uint("Master_log_pos");
  out_relay_log_info->sql_delay = row.get_uint("Sql_delay");
  out_relay_log_info->number_of_workers = row.get_uint("Number_of_workers");
  out_relay_log_info->id = row.get_uint("Id");
  out_relay_log_info->channel_name = row.get_string("Channel_name");
  if (row.has_field("Privilege_checks_username") &&
      !row.is_null("Privilege_checks_username"))
    out_relay_log_info->privilege_checks_username =
        row.get_string("Privilege_checks_username");
  if (row.has_field("Privilege_checks_hostname") &&
      !row.is_null("Privilege_checks_hostname"))
    out_relay_log_info->privilege_checks_hostname =
        row.get_string("Privilege_checks_hostname");
  if (row.has_field("Require_row_format") && !row.is_null("Require_row_format"))
    out_relay_log_info->require_row_format = row.get_int("Require_row_format");
}

}  // namespace

bool get_channel_info(const mysqlshdk::mysql::IInstance &instance,
                      std::string_view channel_name,
                      Replication_channel_master_info *out_master_info,
                      Replication_channel_relay_log_info *out_relay_log_info) {
  if (!out_master_info && !out_relay_log_info) return false;

  if (out_master_info) {
    auto result = instance.queryf(
        "SELECT * FROM mysql.slave_master_info"
        " WHERE channel_name = ?",
        channel_name);
    if (auto row = result->fetch_one_named()) {
      unserialize_channel_master_info(row, out_master_info);
    } else {
      return false;
    }
  }

  if (out_relay_log_info) {
    auto result = instance.queryf(
        "SELECT * FROM mysql.slave_relay_log_info"
        " WHERE channel_name = ?",
        channel_name);
    if (auto row = result->fetch_one_named()) {
      unserialize_channel_relay_log_info(row, out_relay_log_info);
    }
  }

  return true;
}

std::vector<Slave_host> get_slaves(
    const mysqlshdk::mysql::IInstance &instance) {
  std::vector<Slave_host> slaves;
  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (instance.get_version() < utils::Version(8, 2, 0)) {
    result = instance.query("SHOW SLAVE HOSTS");
  } else {
    result = instance.query("SHOW REPLICAS");
  }
  while (auto row = result->fetch_one_named()) {
    Slave_host host;
    host.host = row.get_string("Host");
    host.port = row.get_uint("Port");
    if (instance.get_version() < utils::Version(8, 2, 0)) {
      host.uuid = row.get_string("Slave_UUID");
    } else {
      host.uuid = row.get_string("Replica_UUID");
    }
    slaves.push_back(std::move(host));
  }

  // Sort by host/port to ensure deterministic output
  std::sort(slaves.begin(), slaves.end(),
            [](const Slave_host &a, const Slave_host &b) {
              return a.host < b.host || (a.host == b.host && a.port < b.port);
            });

  return slaves;
}

Replication_channel::Status Replication_channel::status() const {
  bool applier_off = false;

  size_t num_applier_off = 0;
  for (const auto &applier : appliers) {
    if (applier.state != Applier::OFF) continue;

    // error is the strongest status
    if (applier.last_error.code != 0) return APPLIER_ERROR;

    num_applier_off++;
  }

  if (num_applier_off == appliers.size())
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

  switch (receiver.state) {
    case Receiver::OFF:
      if (receiver.last_error.code != 0) return CONNECTION_ERROR;
      return applier_off ? OFF : RECEIVER_OFF;

    case Receiver::CONNECTING:
      return applier_off ? APPLIER_OFF : CONNECTING;

    default:
      if (receiver.last_error.code != 0) return CONNECTION_ERROR;
      if (applier_off) return APPLIER_OFF;
      break;
  }

  return ON;
}

Replication_channel::Status Replication_channel::applier_status() const {
  bool applier_off = false;

  size_t num_applier_off = 0;
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
  if (num_applier_off == appliers.size())
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

  if (applier_off) {
    return APPLIER_OFF;
  }

  return ON;
}

std::string get_replication_user(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &channel_name) {
  return instance.queryf_one_string(
      0, "",
      "SELECT User_name FROM mysql.slave_master_info WHERE Channel_name = ?",
      channel_name);
}

Replication_channel wait_replication_done_connecting(
    const mysqlshdk::mysql::IInstance &slave, std::string_view channel_name) {
  // Check that the IO thread for the channel switches to ON/OFF.
  // NOTE: It is assumed that the IO thread will not stay forever with the
  //       state CONNECTING and will eventually switch to ON or OFF (stopped).
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
  } while (true);
}

bool test_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const Gtid_set &gtid_set) {
  auto result = instance.queryf("SELECT GTID_SUBSET(?, @@GLOBAL.gtid_executed)",
                                gtid_set.str());
  return result->fetch_one()->get_int(0);
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
      [&count](std::string_view s) {
        size_t p = s.find(':');
        if (p == std::string::npos) return true;

        std::string range{s};
        size_t begin, end;
        switch (sscanf(&range[p + 1], "%zu-%zu", &begin, &end)) {
          case 2:
            count += end - begin + 1;
            break;
          case 1:
            count++;
            break;
        }

        return true;
      },
      ",");
  return count;
}

std::string get_executed_gtid_set(const mysqlshdk::mysql::IInstance &server) {
  // session GTID_EXECUTED is different from the global one in 5.7
  return server.queryf_one_string(0, "", "SELECT @@GLOBAL.GTID_EXECUTED");
}

std::string get_purged_gtid_set(const mysqlshdk::mysql::IInstance &server) {
  return server.queryf_one_string(0, "", "SELECT @@GLOBAL.GTID_PURGED");
}

std::string get_received_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  std::string_view channel_name) {
  // In the query, GTID_SUBTRACT is used, not to subtract GTIDs from the set,
  // but to actually clean the set, by removing duplicates, etc.
  return server.queryf_one_string(
      0, "",
      "SELECT received_transaction_set FROM "
      "performance_schema.replication_connection_status WHERE channel_name = ?",
      channel_name);
}

std::string get_total_gtid_set(const mysqlshdk::mysql::IInstance &server,
                               std::string_view channel_name) {
  // In the query, GTID_SUBTRACT is used, not to subtract GTIDs from the set,
  // but to actually clean the set, by removing duplicates, etc.
  return server.queryf_one_string(
      0, "",
      "SELECT GTID_SUBTRACT(CONCAT(@@GLOBAL.GTID_EXECUTED, ',', (SELECT "
      "received_transaction_set FROM "
      "performance_schema.replication_connection_status WHERE channel_name = "
      "?)), '')",
      channel_name);
}

std::string get_total_gtid_set(const mysqlshdk::mysql::IInstance &server,
                               const std::vector<std::string> &channel_names) {
  if (channel_names.empty()) return {};
  if (channel_names.size() == 1)
    return get_total_gtid_set(server, channel_names.front());

  // In the query, GTID_SUBTRACT is used, not to subtract GTIDs from the set,
  // but to actually clean the set, by removing duplicates, etc.
  return server.queryf_one_string(
      0, "",
      shcore::str_format(
          "SELECT GTID_SUBTRACT(CONCAT(@@GLOBAL.GTID_EXECUTED, ',', (SELECT "
          "GROUP_CONCAT(received_transaction_set) FROM "
          "performance_schema.replication_connection_status WHERE channel_name "
          "IN (%s))), '')",
          shcore::str_join(channel_names, ",", shcore::quote_sql_string)
              .c_str()));
}

Gtid_set_relation compare_gtid_sets(const mysqlshdk::mysql::IInstance &server,
                                    std::string_view gtidset_a,
                                    std::string_view gtidset_b,
                                    std::string *out_missing_from_a,
                                    std::string *out_missing_from_b) {
  if (gtidset_a.empty() || gtidset_b.empty()) {
    if (out_missing_from_a) *out_missing_from_a = std::string{gtidset_b};
    if (out_missing_from_b) *out_missing_from_b = std::string{gtidset_a};

    if (gtidset_a.empty()) {
      return gtidset_b.empty() ? Gtid_set_relation::EQUAL
                               : Gtid_set_relation::CONTAINED;
    }

    assert(gtidset_b.empty());
    return Gtid_set_relation::CONTAINS;
  }

  auto set_a = mysqlshdk::mysql::Gtid_set::from_normalized_string(
      std::string{gtidset_a});
  auto set_b = mysqlshdk::mysql::Gtid_set::from_normalized_string(
      std::string{gtidset_b});

  if (!mysqlsh::dba::supports_gtid_tags(server.get_version()) &&
      (set_a.has_tags() || set_b.has_tags())) {
    throw shcore::Exception(
        shcore::str_format("Instance '%s' doesn't support GTID tags.",
                           server.descr().c_str()),
        SHERR_UNSUPPORTED_GTID_TAG);
  }

  auto a_sub_b = mysqlshdk::mysql::Gtid_set{set_a}.subtract(set_b, server);
  auto b_sub_a = mysqlshdk::mysql::Gtid_set{set_b}.subtract(set_a, server);

  if (out_missing_from_a) *out_missing_from_a = b_sub_a.str();
  if (out_missing_from_b) *out_missing_from_b = a_sub_b.str();

  if (a_sub_b.empty() && b_sub_a.empty()) return Gtid_set_relation::EQUAL;
  if (a_sub_b.empty() && !b_sub_a.empty()) return Gtid_set_relation::CONTAINED;
  if (!a_sub_b.empty() && b_sub_a.empty()) return Gtid_set_relation::CONTAINS;

  set_b.intersect(set_a, server);
  return set_b.empty() ? Gtid_set_relation::DISJOINT
                       : Gtid_set_relation::INTERSECTS;
}

std::string to_string(Replica_gtid_state state) {
  switch (state) {
    case Replica_gtid_state::NEW:
      return "NEW";
    case Replica_gtid_state::IDENTICAL:
      return "IDENTICAL";
    case Replica_gtid_state::RECOVERABLE:
      return "RECOVERABLE";
    case Replica_gtid_state::IRRECOVERABLE:
      return "IRRECOVERABLE";
    case Replica_gtid_state::DIVERGED:
      return "DIVERGED";
  }
  throw std::logic_error("internal error");
}

void compute_joining_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::mysql::Gtid_set &primary_gtids,
    const std::vector<mysqlshdk::mysql::Gtid_set> &purged_gtids,
    const mysqlshdk::mysql::Gtid_set &joiner_gtids,
    const std::vector<std::string> &allowed_errant_uuids,
    mysqlshdk::mysql::Gtid_set *out_missing_gtids,
    mysqlshdk::mysql::Gtid_set *out_unrecoverable_gtids,
    mysqlshdk::mysql::Gtid_set *out_errant_gtids,
    mysqlshdk::mysql::Gtid_set *out_allowed_errant_gtids) {
  assert(out_missing_gtids && out_unrecoverable_gtids && out_errant_gtids &&
         out_allowed_errant_gtids);

  if (!mysqlsh::dba::supports_gtid_tags(instance.get_version()) &&
      joiner_gtids.has_tags()) {
    throw shcore::Exception(
        shcore::str_format("Instance '%s' doesn't support GTID tags.",
                           instance.descr().c_str()),
        SHERR_UNSUPPORTED_GTID_TAG);
  }

  using mysqlshdk::mysql::Gtid_set;

  Gtid_set completely_purged_gtids;
  // find out GTIDs purged from everywhere
  if (!purged_gtids.empty()) {
    auto gtids = purged_gtids.begin();
    completely_purged_gtids = *gtids;
    for (++gtids; gtids != purged_gtids.end(); ++gtids) {
      completely_purged_gtids.intersect(*gtids, instance);
    }
  }

  // compute missing and errant trxs
  *out_missing_gtids = primary_gtids;
  out_missing_gtids->subtract(joiner_gtids, instance);

  *out_errant_gtids = joiner_gtids;
  out_errant_gtids->subtract(primary_gtids, instance);

  // from the missing trxs, check what's non-recoverable
  *out_unrecoverable_gtids = *out_missing_gtids;
  out_unrecoverable_gtids->intersect(completely_purged_gtids, instance);

  // missing gtids that are recoverable
  out_missing_gtids->subtract(*out_unrecoverable_gtids, instance);

  // from the errant trxs, check what's allowed (e.g. VCLEs)
  *out_allowed_errant_gtids = Gtid_set();
  for (const auto &uuid : allowed_errant_uuids) {
    out_allowed_errant_gtids->add(out_errant_gtids->get_gtids_from(uuid));
  }
  out_allowed_errant_gtids->normalize(instance);

  out_errant_gtids->subtract(*out_allowed_errant_gtids, instance);
}

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &master,
    const mysqlshdk::mysql::IInstance &slave, std::string *out_missing_gtids,
    std::string *out_errant_gtids) {
  auto slave_gtid = get_executed_gtid_set(slave);
  auto master_gtid = get_executed_gtid_set(master);
  auto master_purged_gtid = get_purged_gtid_set(master);

  return check_replica_gtid_state(master, master_gtid, master_purged_gtid,
                                  slave_gtid, out_missing_gtids,
                                  out_errant_gtids);
}

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &master_gtidset, const std::string &master_purged_gtidset,
    const std::string &slave_gtidset, std::string *out_missing_gtids,
    std::string *out_errant_gtids) {
  if (slave_gtidset.empty() && master_purged_gtidset.empty() &&
      !master_gtidset.empty()) {
    if (out_missing_gtids) {
      *out_missing_gtids = master_gtidset;
    }
    if (out_errant_gtids) {
      *out_errant_gtids = "";
    }
    return Replica_gtid_state::NEW;
  }

  Gtid_set_relation rel =
      compare_gtid_sets(instance, master_gtidset, slave_gtidset,
                        out_errant_gtids, out_missing_gtids);

  switch (rel) {
    case Gtid_set_relation::INTERSECTS:
    case Gtid_set_relation::DISJOINT:
    case Gtid_set_relation::CONTAINED:
      return Replica_gtid_state::DIVERGED;

    case Gtid_set_relation::EQUAL:
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

std::string get_replica_keyword(const mysqlshdk::utils::Version &version) {
  if (version < mysqlshdk::utils::Version(8, 0, 23)) {
    return "SLAVE";
  } else {
    return "REPLICA";
  }
}

std::string get_replication_source_keyword(
    const mysqlshdk::utils::Version &version, bool command) {
  if (version < mysqlshdk::utils::Version(8, 0, 23)) {
    return "MASTER";
  } else {
    return (command == true ? "REPLICATION SOURCE" : "SOURCE");
  }
}

std::string get_binary_logs_keyword(const mysqlshdk::utils::Version &version,
                                    bool status) {
  if (version < mysqlshdk::utils::Version(8, 2, 0)) {
    return "MASTER";
  } else {
    return (status == true ? "BINARY LOG" : "BINARY LOGS AND GTIDS");
  }
}

std::tuple<std::string, std::string> get_replication_source_keywords(
    const mysqlshdk::utils::Version &version) {
  if (version < mysqlshdk::utils::Version(8, 0, 23))
    return {"MASTER", "MASTER"};
  return {"SOURCE", "REPLICATION SOURCE"};
}

std::string get_replication_option_keyword(
    const mysqlshdk::utils::Version &version, std::string_view option) {
  using namespace std::literals;

  // assume current keywords (>= 8.0.26)
  std::array<std::tuple<std::string_view, std::string_view>, 2> replacements{
      std::make_tuple("slave"sv, "replica"sv),
      std::make_tuple("master"sv, "source"sv)};

  // switch old<>new keywords if target is an older version
  if (version < mysqlshdk::utils::Version(8, 0, 26)) {
    for (auto &rep : replacements)
      std::swap(std::get<0>(rep), std::get<1>(rep));
  }

  // Cannot blindly replace keywords, otherwise "aaa_replication_bbb" is
  // incorrectly replaced with "aaa_slavetion_bbb". Replacements are only made
  // if:
  //  - keyword is at the beginning and is followed with '_', '-' or ' '
  //  - keyword is at the end and is preceded with '_', '-' or ' '
  //  - keyword is followed and preceded with '_', '-' or ' '
  //
  // It's also assumed that the replacement only needs to be made once per
  // keyword.

  std::array<const char, 3> separators{'_', '-', ' '};

  std::string ret{option};

  for (const auto &[old_name, new_name] : replacements) {
    if (ret == old_name) return std::string{new_name};

    auto it = ret.find(old_name);
    if (it == std::string::npos) continue;

    if (it == 0) {  // replace at beginning
      assert(ret.size() > old_name.size());
      if (std::find(separators.begin(), separators.end(),
                    ret[old_name.size()]) != separators.end())
        ret.replace(it, old_name.size(), new_name);
      continue;
    }

    if (it == (ret.size() - old_name.size())) {  // replace at end
      assert(ret.size() > old_name.size());
      if (std::find(separators.begin(), separators.end(), ret[it - 1]) !=
          separators.end())
        ret.replace(it, old_name.size(), new_name);
      continue;
    }

    // replace in the middle
    assert((it + old_name.size()) < ret.size());
    if (std::find(separators.begin(), separators.end(), ret[it - 1]) ==
        separators.end())
      continue;
    if (std::find(separators.begin(), separators.end(),
                  ret[it + old_name.size()]) == separators.end())
      continue;

    ret.replace(it, old_name.size(), new_name);
  }

  return ret;
}

}  // namespace mysql
}  // namespace mysqlshdk
