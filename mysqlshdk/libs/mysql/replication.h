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

#ifndef MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"
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
    enum class Status { ON, CONNECTING, ERROR, OFF };
    Status status = Status::OFF;
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
    enum class Status { APPLIED_ALL, APPLYING, ON, ERROR, OFF };
    Status status = Status::OFF;
    std::string thread_state;
    Error last_error;
  };

  std::string channel_name;
  std::string user;
  std::string host;
  int port = 0;
  std::string group_name;

  std::string source_uuid;

  std::string time_since_last_message;

  std::string repl_lag_from_original;
  std::string repl_lag_from_immediate;

  std::string queued_gtid_set_to_apply;

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

  /**
   * Get the applier status, ignoring the receiver status (I/O thread).
   *
   * NOTE: Used to determine only the status of the applier when the primary is
   *       down, for example during a failover operation where a
   *       CONNECTION_ERROR is expected (having priority for status() over
   *       other applier status).
   *
   * @return status of the applier, ignoring the receiver status. Possible
   *         returned values: APPLIER_ERROR, APPLIER_OFF, ON.
   */
  Status applier_status() const;
};

struct Replication_channel_master_info {
  std::string master_log_name;
  uint64_t master_log_pos;
  std::string host;
  std::string user_name;
  // std::string user_password; will be deprecated/removed
  uint64_t port;
  uint64_t connect_retry;
  int enabled_ssl;
  std::string ssl_ca;
  std::string ssl_capath;
  std::string ssl_cert;
  std::string ssl_cipher;
  std::string ssl_key;
  int ssl_verify_server_cert;
  double heartbeat_period;
  std::string bind;
  std::string ignored_server_ids;
  std::string uuid;
  uint64_t retry_count;
  std::string ssl_crl;
  std::string ssl_crlpath;
  int enabled_auto_position;
  std::string channel_name;
  std::string tls_version;

  // 8.0+
  std::optional<std::string> public_key_path;
  std::optional<int> get_public_key;
  std::optional<std::string> network_namespace;
  std::optional<std::string> compression_algorithm;
  std::optional<uint64_t> zstd_compression_level;
  std::optional<std::string> tls_ciphersuites;
};

struct Replication_channel_relay_log_info {
  std::string relay_log_name;
  uint64_t relay_log_pos;
  std::string master_log_name;
  uint64_t master_log_pos;
  int sql_delay;
  uint64_t number_of_workers;
  uint64_t id;
  std::string channel_name;
  std::optional<std::string> privilege_checks_username;
  std::optional<std::string> privilege_checks_hostname;
  std::optional<int> require_row_format;
};

struct Replication_channel_failover_source {
  std::string host;
  int port = 0;
  uint32_t weight = 1;
  std::string network_namespace;
  std::string managed_name;
};

struct Slave_host {
  std::string host;
  int port = 0;
  std::string uuid;
};

std::string to_string(Replication_channel::Status status);
std::string to_string(Replication_channel::Receiver::State state);
std::string to_string(Replication_channel::Receiver::Status status);
std::string to_string(Replication_channel::Coordinator::State state);
std::string to_string(Replication_channel::Applier::State state);
std::string to_string(Replication_channel::Applier::Status status);
std::string to_string(const Replication_channel::Error &error);

std::string format_status(const Replication_channel &channel,
                          bool verbose = false);

/**
 * Gets status information for a replication channel.
 *
 * @return false if the channel does not exist.
 */
bool get_channel_status(const mysqlshdk::mysql::IInstance &instance,
                        std::string_view channel_name,
                        Replication_channel *out_channel);

/**
 * Gets state information for a replication channel
 *
 * @param  instance      [TODO]
 * @param  channel_name  [TODO]
 * @param  out_io_state  [TODO]
 * @param  out_sql_state [TODO]
 * @return               [TODO]
 */
bool get_channel_state(const mysqlshdk::mysql::IInstance &instance,
                       std::string_view channel_name,
                       Replication_channel::Receiver::State *out_io_state,
                       Replication_channel::Applier::State *out_sql_state);

bool get_channel_state(const mysqlshdk::mysql::IInstance &instance,
                       std::string_view channel_name,
                       Replication_channel::Receiver::State *out_io_state,
                       Replication_channel::Error *out_io_error,
                       Replication_channel::Applier::State *out_sql_state,
                       Replication_channel::Error *out_sql_error);

/**
 * Gets configuration for a replication channel (as obtained from
 * mysql.slave_master_info and slave_relay_log_info)
 *
 * @return false if the channel does not exist.
 */
bool get_channel_info(const mysqlshdk::mysql::IInstance &instance,
                      std::string_view channel_name,
                      Replication_channel_master_info *out_master_info,
                      Replication_channel_relay_log_info *out_relay_log_info);

/**
 * Return status list of all replication channels (masters) at the given
 * instance.
 */
std::vector<Replication_channel> get_incoming_channels(
    const mysqlshdk::mysql::IInstance &instance);

std::vector<std::string> get_incoming_channel_names(
    const mysqlshdk::mysql::IInstance &instance, bool include_stopped);

/**
 * Returns list of all replication slaves from the given instance.
 */
std::vector<Slave_host> get_slaves(const mysqlshdk::mysql::IInstance &instance);

/**
 * Wait for the named replication channel to either switch from the CONNECTING
 * state, an error appears.
 *
 * @param slave - the instance to monitor
 * @param channel_name - the name of the replication channel to monitor
 */
Replication_channel wait_replication_done_connecting(
    const mysqlshdk::mysql::IInstance &slave, std::string_view channel_name);

/**
 * Get the replication user used for the given channel.
 *
 * This function returns the replication user used in the (last)
 * CHANGE MASTER TO statement FOR CHANNEL channel_name.
 *
 * NOTE: The correct execution of this function requires the variable
 *       master_info_repository=TABLE to be set.
 *
 * @param instance instance object of target member to obtain the
 *                 replication user.
 * @param channel_name name of the channel to query
 * @return a string with the replication user set for the specified
 *         instance. Note: If no replication user was specified an empty string
 *         is returned.
 */
std::string get_replication_user(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &channel_name);

/**
 * Returns GTID_EXECUTED set from the server.
 */
std::string get_executed_gtid_set(const mysqlshdk::mysql::IInstance &server);

/**
 * Returns GTID_PURGED set from the server.
 */
std::string get_purged_gtid_set(const mysqlshdk::mysql::IInstance &server);

/**
 * Returns GTID set received by a specific channel.
 */
std::string get_received_gtid_set(const mysqlshdk::mysql::IInstance &server,
                                  std::string_view channel_name);

/**
 * Returns total GTID set from the server, including received but not yet
 * applied.
 *
 * channel_name the channel to consider when checking for
 * received_transaction_set (usually group_replication_applier)
 */
std::string get_total_gtid_set(const mysqlshdk::mysql::IInstance &server,
                               std::string_view channel_name);

/**
 * Returns total GTID set from the server, including received but not yet
 * applied.
 *
 * channel_names must contain the list of all channels to consider when
 * checking for received_transaction_set (usually just
 * group_replication_applier)
 */
std::string get_total_gtid_set(const mysqlshdk::mysql::IInstance &server,
                               const std::vector<std::string> &channel_names);

/**
 * Returns an upper bound of the number of transactions in the given GTID set.
 *
 * Can be used to give a rough estimate of the number of transactions, which
 * should be accurate if there are no gaps in GTID ranges.
 *
 * Note that this function will not detect duplicates in the set.
 */
size_t estimate_gtid_set_size(const std::string &gtid_set);

void compute_joining_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::mysql::Gtid_set &primary_gtids,
    const std::vector<mysqlshdk::mysql::Gtid_set> &purged_gtids,
    const mysqlshdk::mysql::Gtid_set &joiner_gtids,
    const std::vector<std::string> &allowed_errant_uuids,
    mysqlshdk::mysql::Gtid_set *out_missing_gtids,
    mysqlshdk::mysql::Gtid_set *out_unrecoverable_gtids,
    mysqlshdk::mysql::Gtid_set *out_errant_gtids,
    mysqlshdk::mysql::Gtid_set *out_allowed_errant_gtids);

enum class Gtid_set_relation {
  EQUAL,       // a = b
  CONTAINS,    // a contains b (and a <> b)
  CONTAINED,   // b contains a (and a <> b)
  INTERSECTS,  // some GTIDs in common
  DISJOINT     // nothing in common
};

Gtid_set_relation compare_gtid_sets(const mysqlshdk::mysql::IInstance &server,
                                    std::string_view gtidset_a,
                                    std::string_view gtidset_b,
                                    std::string *out_missing_from_a = nullptr,
                                    std::string *out_missing_from_b = nullptr);

enum class Replica_gtid_state {
  NEW,            // Replica is new (gtid_executed is empty)
  IDENTICAL,      // GTID set identical to master
  RECOVERABLE,    // GTID set is missing transactions, but can be recovered
  IRRECOVERABLE,  // GTID set is missing purged transactions
  DIVERGED        // GTID sets have diverged
};

std::string to_string(Replica_gtid_state state);

/**
 * Checks the transaction state of a replica in relation to another instance
 * (its master).
 */
Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &master,
    const mysqlshdk::mysql::IInstance &slave,
    std::string *out_missing_gtids = nullptr,
    std::string *out_errant_gtids = nullptr);

Replica_gtid_state check_replica_gtid_state(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &master_gtidset, const std::string &master_purged_gtidset,
    const std::string &slave_gtidset, std::string *out_missing_gtids = nullptr,
    std::string *out_errant_gtids = nullptr);

/**
 * Checks if the given GTID set is applied on the target instance.
 *
 * @param instance target instance to check for GTIDs to be applied.
 * @param gtid_set string with the GTID set to check to be applied.
 * @return Return true if the GTID set has been applied, false otherwise
 * @throws an error if some issue occurred when checking for transactions to be
 *         applied.
 */
bool test_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const Gtid_set &gtid_set);

/**
 * Wait until the given GTID set is applied on the target instance.
 *
 * @param instance target instance to wait for GTIDs to be applied.
 * @param gtid_set string with the GTID set to wait to be applied.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occurred when waiting for transaction to be
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
 * @throws an error if some issue occurred when waiting for transaction to be
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
 * Get the correct keyword in use for replica/slave regarding the target
 * instance version.
 *
 * Useful for the construction of queries or output/error messages.
 *
 * @param version Version of the target server.
 *
 * @return a string with the right keyword to be used for replica/slave.
 */
std::string get_replica_keyword(const mysqlshdk::utils::Version &version);

/**
 * Get the correct keyword in use for the replication configuration command:
 * 'change replication source/master' regarding the target instance version.
 *
 * Useful for the construction of queries or output/error messages.
 *
 * @param version Version of the target server.
 * @param command Boolean value to indicate if the keyword is for the
 * replication configuration command itself or if it is for an optional
 * parameter of it (e.g. SOURCE_HOST)
 *
 * @return a string with the right keyword to be used for the replication
 * configuration command
 */
std::string get_replication_source_keyword(
    const mysqlshdk::utils::Version &version, bool command = false);

/**
 * Get the correct keyword in use for the replication configuration commands:
 * 'SHOW MASTER LOGS' / 'SHOW MASTER STATUS' / 'PURGE MASTER LOGS'
 *  regarding the target instance version.
 *
 * Useful for the construction of queries or output/error messages.
 *
 * @param version Version of the target server.
 * @param command Boolean value to indicate if the keyword is for status
 * command: 'SHOW MASTER STATUS'
 *
 * @return a string with the right keyword to be used for the replication
 * configuration command
 */
std::string get_binary_logs_keyword(const mysqlshdk::utils::Version &version,
                                    bool status = false);

/**
 * Get the correct keywords in use for the replication configuration command:
 * 'change replication source/master' regarding the target instance version.
 *
 * Useful for the construction of queries or output/error messages.
 *
 * This command is equivalent to calling get_replication_source_keyword() twice
 * with 'command' to false (first returned string) and then to true (second
 * returned string).
 *
 * @param version Version of the target server.
 *
 * @return a pair of strings with the keywords: the first is for an optional
 * parameter and the scond one is the replication configuration command itself
 */
std::tuple<std::string, std::string> get_replication_source_keywords(
    const mysqlshdk::utils::Version &version);

/**
 * Get the correct replication option name to use regarding the target instance
 * version.
 *
 * Terminology changed throughout the versions in regards to the terms "master"
 * and "slave", in favor of "source" and "replica"
 *
 * @param version Version of the target server.
 * @param options Replication Option
 *
 * @return a string with the right value of the sysvars to be used as
 * Replication option
 */
std::string get_replication_option_keyword(
    const mysqlshdk::utils::Version &version, std::string_view option);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_
