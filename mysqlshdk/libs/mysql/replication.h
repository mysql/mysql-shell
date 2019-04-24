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

#ifndef MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

struct Replication_channel {
  struct Error {
    int code = 0;
    std::string message;
    std::string timestamp;
  };

  struct Receiver {
    enum State { ON, OFF, CONNECTING };
    State state = OFF;
    std::string thread_state;
    Error last_error;
  };

  struct Coordinator {
    enum State { NONE, ON, OFF };
    State state = NONE;
    std::string thread_state;
    Error last_error;
  };

  struct Applier {
    enum State { ON, OFF };
    State state = OFF;
    std::string thread_state;
    Error last_error;
    uint64_t last_applied_trx_delay =
        0;  // time between original_commit and apply_end, in ns
  };

  std::string channel_name;
  std::string user;
  std::string host;
  int port = 0;
  std::string group_name;

  std::string source_uuid;
  std::string last_heartbeat_timestamp;

  // IO thread status
  Receiver receiver;
  // Coordinator thread status (only if parallel applier used)
  Coordinator coordinator;
  // Applier thread status (only 1 unless parallel applier used)
  std::vector<Applier> appliers;

  enum Status {
    OFF,               // all threads are OFF (and with no error)
    ON,                // all threads are ON
    RECEIVER_OFF,      // receiver thread OFF
    APPLIER_OFF,       // applier threads OFF
    CONNECTING,        // connection thread still connecting
    CONNECTION_ERROR,  // connection thread has error or is stopped
    APPLIER_ERROR      // applier thread(s) have error or is stopped
  };

  Status status() const;
};

struct Slave_host {
  std::string host;
  int port = 0;
  std::string uuid;
};

std::string to_string(Replication_channel::Receiver::State state);
std::string to_string(Replication_channel::Coordinator::State state);
std::string to_string(Replication_channel::Applier::State state);
std::string to_string(const Replication_channel::Error &error);

std::string format_status(const Replication_channel &channel,
                          bool verbose = false);

/**
 * Gets status information for a replication channel.
 *
 * @return false if the channel does not exist.
 */
bool get_channel_status(const mysqlshdk::mysql::IInstance &instance,
                        const std::string &channel_name,
                        Replication_channel *out_channel);

/**
 * Return status list of all replication channels (masters) at the given
 * instance.
 */
std::vector<Replication_channel> get_incoming_channels(
    const mysqlshdk::mysql::IInstance &instance);

/**
 * Returns list of all replication channels (slaves) from the given instance.
 */
std::vector<Slave_host> get_slaves(const mysqlshdk::mysql::IInstance &instance);

/**
 * Wait for the named replication channel to either switch from the CONNECTING
 * state, an error appears or timeout.
 *
 * @param slave - the instance to monitor
 * @param channel_name - the name of the replication channel to monitor
 * @param timeout - timeout in seconds
 */
Replication_channel wait_replication_done_connecting(
    const mysqlshdk::mysql::IInstance &slave, const std::string &channel_name,
    int timeout);

/**
 * Returns GTID_EXECUTED set from the server.
 *
 * @param include_received - if true, will include GTID sets that were received
 * but not yet applied, across all channels.
 */
std::string get_executed_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  bool include_received = false);

/**
 * Returns GTID_PURGED set from the server.
 */
std::string get_purged_gtid_set(const mysqlshdk::mysql::IInstance &server);

/**
 * Returns GTID set received by a specific channel.
 */
std::string get_received_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  const std::string &channel_name);

/**
 * Returns an upper bound of the number of transactions in the given GTID set.
 *
 * Can be used to give a rough estimate of the number of transactions, which
 * should be accurate if there are no gaps in GTID ranges.
 *
 * Note that this function will not detect duplicates in the set.
 */
size_t estimate_gtid_set_size(const std::string &gtid_set);

enum class Gtid_set_relation {
  EQUAL,       // a = b
  CONTAINS,    // a contains b (and a <> b)
  CONTAINED,   // b contains a (and a <> b)
  INTERSECTS,  // some GTIDs in common
  DISJOINT     // nothing in common
};

Gtid_set_relation compare_gtid_sets(const mysqlshdk::mysql::IInstance &server,
                                    const std::string &gtidset_a,
                                    const std::string &gtidset_b,
                                    std::string *out_missing_from_a = nullptr,
                                    std::string *out_missing_from_b = nullptr);

enum class Replica_gtid_state {
  NEW,            // Replica is new (gtid_executed is empty)
  IDENTICAL,      // GTID set identical to master
  RECOVERABLE,    // GTID set is missing transactions, but can be recovered
  IRRECOVERABLE,  // GTID set is missing purged transactions
  DIVERGED        // GTID sets have diverged
};

/**
 * Checks the transaction state of a replica in relation to another instance
 * (its master).
 *
 * If include_received is true, transactions that were received (relay_log)
 * are included in the calculations.
 */
Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &master,
    const mysqlshdk::mysql::IInstance &slave, bool include_received,
    std::string *out_missing_gtids = nullptr,
    std::string *out_errant_gtids = nullptr);

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &master_gtidset, const std::string &master_purged_gtidset,
    const std::string &slave_gtidset, std::string *out_missing_gtids = nullptr,
    std::string *out_errant_gtids = nullptr);

/**
 * Wait until the given GTID set is applied on the target instance.
 *
 * @param instance target instance to wait for GTIDs to be applied.
 * @param gtid_set string with the GTID set to wait to be applied.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occured when waiting for transaction to be
 *         applied.
 */
bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &gtid_set, int timeout);

/**
 * Wait until GTID_EXECUTED from the source instance is applied at the target.
 *
 * @param target target instance to wait for GTIDs to be applied.
 * @param source source instance to take GTID_EXECUTED from.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occured when waiting for transaction to be
 *         applied.
 *
 * Returns true immediately if GTID_EXECUTED from the source instance is
 * empty or NULL.
 */
bool wait_for_gtid_set_from(const mysqlshdk::mysql::IInstance &target,
                            const mysqlshdk::mysql::IInstance &source,
                            int timeout);

/**
 * Generate a random server ID.
 *
 * Generate a pseudo-random server_id, with a value between 1 and 4294967295.
 * Two generated values have a low probability of being the same (independently
 * of the time they are generated). Minimum value generated is 1 and maximum
 * 4294967295.
 *
 * For the random generation (assuming random generation is uniformly) the
 * probability of the same value being generated is given by:
 *      P(n, t) = 1 - t!/(t^n * (t-n)!)
 * where t is the total number of different values that can be generated, and
 * n is the number of values that are generated.
 *
 * In this case, t = 4294967295 (max number of values that can be generated),
 * and for example the probability of generating the same id for 15, 100, and
 * 1000 servers (n=15, n=100, and n=1000) is approximately:
 *      P(15, 4294967295)   = 2.44 * 10^-8    (0.00000244 %)
 *      P(100, 4294967295)  = 1.15 * 10^-6    (0.000115 %)
 *      P(1000, 4294967295) = 1.16 * 10^-4    (0.0116 %)
 *
 *  Note: Zero is not a valid sever_id.
 *
 * @return an integer with a random between 1 and 4294967295.
 */
int64_t generate_server_id();

/**
 * Checks if the asynchronous (master-slave) replication channels and threads
 * are active on the target instance:
 *   - Verifies if the replication I/O thread exists (receiver channel) and is
 * active or connecting to the master
 *   - Verifies if replication applier thread is active (applier channel)
 *
 * @param instance Instance to use to perform the check.
 *
 * @return a boolean value indicating if the instance has the asynchronous
 * (master-slave) replication active
 */
bool is_async_replication_running(const mysqlshdk::mysql::IInstance &instance);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_
