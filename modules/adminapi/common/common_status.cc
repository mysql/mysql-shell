/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/common_status.h"

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {

void add_error(const mysqlshdk::mysql::Replication_channel::Error &error,
               const std::string &prefix, shcore::Dictionary_t status) {
  status->set(prefix + "Error", shcore::Value(error.message));
  status->set(prefix + "ErrorNumber", shcore::Value(error.code));
  status->set(prefix + "ErrorTimestamp", shcore::Value(error.timestamp));
}

shcore::Dictionary_t make_error(
    const mysqlshdk::mysql::Replication_channel::Error &error) {
  return shcore::make_dict("error", shcore::Value(error.message), "errorNumber",
                           shcore::Value(error.code), "errorTimestamp",
                           shcore::Value(error.timestamp));
}

void add_channel_errors(const mysqlshdk::mysql::Replication_channel &channel,
                        shcore::Dictionary_t status, int show_details) {
  status->set(
      "receiverStatus",
      shcore::Value(mysqlshdk::mysql::to_string(channel.receiver.status)));

  status->set("receiverThreadState",
              shcore::Value(channel.receiver.thread_state));

  if (channel.receiver.last_error.code != 0)
    add_error(channel.receiver.last_error, "receiverLast", status);

  if (show_details) {
    if (channel.coordinator.state !=
        mysqlshdk::mysql::Replication_channel::Coordinator::NONE) {
      status->set("coordinatorState", shcore::Value(mysqlshdk::mysql::to_string(
                                          channel.coordinator.state)));
      if (!channel.coordinator.thread_state.empty())
        status->set("coordinatorThreadState",
                    shcore::Value(channel.coordinator.thread_state));

      if (channel.coordinator.last_error.code != 0)
        add_error(channel.coordinator.last_error, "coordinatorLast", status);
    }
  }

  {
    const auto &applier = channel.appliers.front();
    status->set("applierThreadState", shcore::Value(applier.thread_state));

    if (show_details)
      status->set("applierState",
                  shcore::Value(mysqlshdk::mysql::to_string(applier.state)));

    mysqlshdk::mysql::Replication_channel::Applier::Status astatus =
        applier.status;

    shcore::Array_t applier_errors = shcore::make_array();
    for (const auto &channel_applier : channel.appliers) {
      if (channel_applier.last_error.code != 0) {
        applier_errors->push_back(
            shcore::Value(make_error(channel_applier.last_error)));
        astatus = mysqlshdk::mysql::Replication_channel::Applier::Status::ERROR;
      }
    }

    status->set("applierStatus",
                shcore::Value(mysqlshdk::mysql::to_string(astatus)));

    if (!applier_errors->empty()) {
      status->set("applierLastErrors", shcore::Value(applier_errors));
    }
  }
}
}  // namespace

shcore::Dictionary_t channel_status(
    const mysqlshdk::mysql::Replication_channel *channel_info,
    const mysqlshdk::mysql::Replication_channel_master_info *master_info,
    const mysqlshdk::mysql::Replication_channel_relay_log_info *relay_log_info,
    const std::string &expected_source, int show_details, bool show_source,
    bool show_lag) {
  shcore::Dictionary_t rstatus = shcore::make_dict();

  bool show_expected_master = false;
  if (channel_info) {
    add_channel_errors(*channel_info, rstatus, show_details);

    if (channel_info->status() !=
        mysqlshdk::mysql::Replication_channel::Status::ON)
      show_expected_master = true;

    std::string actual_source = mysqlshdk::utils::make_host_and_port(
        channel_info->host.c_str(), channel_info->port);

    if (!expected_source.empty() &&
        !mysqlshdk::utils::are_endpoints_equal(actual_source, expected_source))
      show_expected_master = true;

    if (show_expected_master || show_details || show_source) {
      rstatus->set("source", channel_info->port != 0
                                 ? shcore::Value(actual_source)
                                 : shcore::Value::Null());
    }

    if (show_details > 1 || relay_log_info->number_of_workers > 1) {
      rstatus->set("applierWorkerThreads",
                   shcore::Value(relay_log_info->number_of_workers == 0
                                     ? 1
                                     : relay_log_info->number_of_workers));
    }

    if (relay_log_info && (show_details > 1 || relay_log_info->sql_delay > 0)) {
      shcore::Dictionary_t conf = shcore::make_dict();

      conf->set("delay", shcore::Value(relay_log_info->sql_delay));

      if (show_details > 1 && master_info) {
        conf->set("heartbeatPeriod", shcore::Value(master_info->heartbeat));
        conf->set("connectRetry", shcore::Value(master_info->connect_retry));
        conf->set("retryCount", shcore::Value(master_info->retry_count));
      }

      rstatus->set("options", shcore::Value(conf));
    }

    if (show_details) {
      rstatus->set("receiverTimeSinceLastMessage",
                   shcore::Value(channel_info->time_since_last_message));

      rstatus->set("applierQueuedTransactionSet",
                   shcore::Value(channel_info->queued_gtid_set_to_apply));
    }
    if (!channel_info->queued_gtid_set_to_apply.empty() || show_details) {
      rstatus->set(
          "applierQueuedTransactionSetSize",
          shcore::Value(int64_t(mysqlshdk::mysql::estimate_gtid_set_size(
              channel_info->queued_gtid_set_to_apply))));
    }

    if (show_lag) {
      if (channel_info->status() ==
          mysqlshdk::mysql::Replication_channel::Status::ON) {
        if (channel_info->repl_lag_from_original ==
            channel_info->repl_lag_from_immediate) {
          rstatus->set(
              "replicationLag",
              channel_info->repl_lag_from_original.empty()
                  ? shcore::Value::Null()
                  : shcore::Value(channel_info->repl_lag_from_original));
        } else {
          rstatus->set("replicationLagFromOriginalSource",
                       shcore::Value(channel_info->repl_lag_from_original));

          rstatus->set("replicationLagFromImmediateSource",
                       shcore::Value(channel_info->repl_lag_from_immediate));
        }
      } else {
        rstatus->set("replicationLag", shcore::Value::Null());
      }
    }
  } else {
    rstatus->set("receiverStatus", shcore::Value::Null());
    rstatus->set("applierStatus", shcore::Value::Null());
    show_expected_master = true;
  }
  if (show_expected_master && !expected_source.empty())
    rstatus->set("expectedSource", shcore::Value(expected_source));

  return rstatus;
}

}  // namespace dba
}  // namespace mysqlsh
