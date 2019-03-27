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

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "mysqlshdk/libs/mysql/replication.h"

#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &gtid_set, int timeout) {
  // NOTE: According to the GR team the max supported timeout value for
  //       WAIT_FOR_EXECUTED_GTID_SET() is 18446744073.7096 seconds
  //       (2^64/1000000000). Therefore, a type with a max value that include
  //       that range should be used.
  auto result = instance.get_session()->queryf(
      "SELECT WAIT_FOR_EXECUTED_GTID_SET(?, ?)", gtid_set, timeout);
  auto row = result->fetch_one();
  // WAIT_FOR_EXECUTED_GTID_SET() returns 0 for success, 1 for timeout,
  // otherwise an error is generated.
  return row->get_int(0) == 0;
}

bool wait_for_gtid_set_from(const mysqlshdk::mysql::IInstance &target,
                            const mysqlshdk::mysql::IInstance &source,
                            int timeout) {
  auto result = source.get_session()->query("SELECT @@GTID_EXECUTED");
  auto row = result->fetch_one();
  if (!row || row->is_null(0) || row->get_string(0).empty()) return true;

  return wait_for_gtid_set(target, row->get_string(0), timeout);
}

int64_t generate_server_id() {
  // Setup uniform random generation of integers between [1, 4294967295] using
  // Mersenne Twister algorithm and a non-determinist seed.
  std::random_device rd_seed;
  std::mt19937 rnd_gen(rd_seed());
  std::uniform_int_distribution<int64_t> distribution(1, 4294967295);
  return distribution(rnd_gen);
}

std::string get_report_host(const mysqlshdk::mysql::IInstance &instance,
                            bool *out_is_report_host_set) {
  auto result = instance.get_session()->query("SELECT @@REPORT_HOST");
  auto row = result->fetch_one();
  if (!row->is_null(0)) {
    if (out_is_report_host_set != nullptr) {
      *out_is_report_host_set = true;
    }
    std::string res = row->get_string(0);
    if (!res.empty()) {
      return res;
    } else {
      // NOTE: The value for report_host can be set to an empty string which is
      // invalid. If defined the report_host value should not be an empty
      // string, otherwise it is used by replication as an empty string "".
      throw std::runtime_error(
          "The value for variable 'report_host' cannot be empty.");
    }
  } else {
    if (out_is_report_host_set != nullptr) {
      *out_is_report_host_set = false;
    }
    result = instance.get_session()->query("SELECT @@HOSTNAME");
    return result->fetch_one()->get_string(0);
  }
}

bool is_async_replication_running(const mysqlshdk::mysql::IInstance &instance) {
  auto session = instance.get_session();
  std::string receiver_channel_state, applier_channel_state;

  std::vector<std::string> valid_receiver_states = {"ON", "OFF", "CONNECTING"};
  std::vector<std::string> valid_applier_states = {"ON", "OFF"};

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
