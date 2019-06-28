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

#include <string>

#include "mysqlshdk/libs/mysql/replication.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_test_wrapper.h"

namespace mysqlshdk {
namespace mysql {
using mysqlshdk::db::Type;
using testing::Mock_session;

class Replication_test : public tests::Shell_base_test {};

TEST_F(Replication_test, compare_gtid_sets) {
  auto session = create_mysql_session(_mysql_uri);
  Instance instance(session);

  std::string gtidset1 =
      "a75881c0-6ae5-11e9-bef7-24bb3d014d7f:1-124,\n"
      "b75881c0-6ae5-11e9-bef7-24bb3d014d7f:1";
  std::string gtidset1r =
      "b75881c0-6ae5-11e9-bef7-24bb3d014d7f:1,\n"
      "a75881c0-6ae5-11e9-bef7-24bb3d014d7f:1-124";
  std::string gtidset2 = "c75881c0-6ae5-11e9-bef7-24bb3d014d7f:5-32";
  std::string gtidset3 =
      "d75881c0-6ae5-11e9-bef7-24bb3d014d7f:1,\n"
      "e75881c0-6ae5-11e9-bef7-24bb3d014d7f:1";

  std::string diff_a;
  std::string diff_b;

  EXPECT_EQ(Gtid_set_relation::EQUAL,
            compare_gtid_sets(instance, gtidset1, gtidset1, &diff_a, &diff_b));
  EXPECT_EQ("", diff_a);
  EXPECT_EQ("", diff_b);

  EXPECT_EQ(Gtid_set_relation::EQUAL,
            compare_gtid_sets(instance, gtidset1, gtidset1r, &diff_a, &diff_b));
  EXPECT_EQ("", diff_a);
  EXPECT_EQ("", diff_b);

  EXPECT_EQ(Gtid_set_relation::EQUAL,
            compare_gtid_sets(instance, "", "", &diff_a, &diff_b));
  EXPECT_EQ("", diff_a);
  EXPECT_EQ("", diff_b);

  EXPECT_EQ(Gtid_set_relation::DISJOINT,
            compare_gtid_sets(instance, gtidset1, gtidset2, &diff_a, &diff_b));
  EXPECT_EQ(gtidset2, diff_a);
  EXPECT_EQ(gtidset1, diff_b);

  EXPECT_EQ(Gtid_set_relation::INTERSECTS,
            compare_gtid_sets(instance, gtidset1 + "," + gtidset2,
                              gtidset3 + "," + gtidset1, &diff_a, &diff_b));
  EXPECT_EQ(gtidset3, diff_a);
  EXPECT_EQ(gtidset2, diff_b);

  EXPECT_EQ(Gtid_set_relation::CONTAINED,
            compare_gtid_sets(instance, gtidset1, gtidset3 + "," + gtidset1,
                              &diff_a, &diff_b));
  EXPECT_EQ(gtidset3, diff_a);
  EXPECT_EQ("", diff_b);

  EXPECT_EQ(Gtid_set_relation::CONTAINED,
            compare_gtid_sets(instance, "", gtidset1, &diff_a, &diff_b));
  EXPECT_EQ(gtidset1, diff_a);
  EXPECT_EQ("", diff_b);

  EXPECT_EQ(Gtid_set_relation::CONTAINS,
            compare_gtid_sets(instance, gtidset3 + "," + gtidset1, gtidset1,
                              &diff_a, &diff_b));
  EXPECT_EQ("", diff_a);
  EXPECT_EQ(gtidset3, diff_b);

  EXPECT_EQ(Gtid_set_relation::CONTAINS,
            compare_gtid_sets(instance, gtidset1, "", &diff_a, &diff_b));
  EXPECT_EQ("", diff_a);
  EXPECT_EQ(gtidset1, diff_b);
}

TEST_F(Replication_test, check_replica_gtid_state) {
  auto session = create_mysql_session(_mysql_uri);
  Instance instance(session);

  const std::string gtidset1 =
      "a75881c0-6ae5-11e9-bef7-24bb3d014d7f:1-124,\n"
      "b75881c0-6ae5-11e9-bef7-24bb3d014d7f:1";
  const std::string gtidset1r =
      "b75881c0-6ae5-11e9-bef7-24bb3d014d7f:1,\n"
      "a75881c0-6ae5-11e9-bef7-24bb3d014d7f:1-124";
  const std::string gtidset2 = "c75881c0-6ae5-11e9-bef7-24bb3d014d7f:5-32";
  const std::string gtidset3 =
      "d75881c0-6ae5-11e9-bef7-24bb3d014d7f:1,\n"
      "e75881c0-6ae5-11e9-bef7-24bb3d014d7f:1";

  std::string missing;
  std::string errant;

  EXPECT_EQ(
      Replica_gtid_state::NEW,
      check_replica_gtid_state(instance, gtidset1, "", "", &missing, &errant));
  EXPECT_EQ(gtidset1, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::IDENTICAL,
            check_replica_gtid_state(instance, gtidset1, "", gtidset1, &missing,
                                     &errant));
  EXPECT_EQ("", missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::IDENTICAL,
            check_replica_gtid_state(instance, gtidset1, gtidset1, gtidset1r,
                                     &missing, &errant));
  EXPECT_EQ("", missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(
      Replica_gtid_state::IDENTICAL,
      check_replica_gtid_state(instance, gtidset1 + "," + gtidset2, gtidset1,
                               gtidset1r + "," + gtidset2, &missing, &errant));
  EXPECT_EQ("", missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::IRRECOVERABLE,
            check_replica_gtid_state(instance, gtidset1, gtidset1, "", &missing,
                                     &errant));
  EXPECT_EQ(gtidset1, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::IRRECOVERABLE,
            check_replica_gtid_state(instance, gtidset1 + "," + gtidset2,
                                     gtidset1, "", &missing, &errant));
  EXPECT_EQ(gtidset1 + "," + gtidset2, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::IRRECOVERABLE,
            check_replica_gtid_state(instance, gtidset1 + "," + gtidset2,
                                     gtidset1, gtidset2, &missing, &errant));
  EXPECT_EQ(gtidset1, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::RECOVERABLE,
            check_replica_gtid_state(instance, gtidset1 + "," + gtidset2,
                                     gtidset1, gtidset1, &missing, &errant));
  EXPECT_EQ(gtidset2, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(Replica_gtid_state::RECOVERABLE,
            check_replica_gtid_state(instance, gtidset1 + "," + gtidset2, "",
                                     gtidset2, &missing, &errant));
  EXPECT_EQ(gtidset1, missing);
  EXPECT_EQ("", errant);

  EXPECT_EQ(
      Replica_gtid_state::DIVERGED,
      check_replica_gtid_state(instance, gtidset1 + "," + gtidset2, "",
                               gtidset1 + "," + gtidset3, &missing, &errant));
  EXPECT_EQ(gtidset2, missing);
  EXPECT_EQ(gtidset3, errant);

  EXPECT_EQ(Replica_gtid_state::DIVERGED,
            check_replica_gtid_state(instance, gtidset1, "", gtidset3, &missing,
                                     &errant));
  EXPECT_EQ(gtidset1, missing);
  EXPECT_EQ(gtidset3, errant);
}

TEST_F(Replication_test, is_async_replication_running) {
  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // All of the following combinations shall return true
  // RECEIVER: ON, APPLIER: ON
  // RECEIVER: ON, APPLIER: OFF
  // RECEIVER: CONNECTING, APPLIER: ON
  // RECEIVER: CONNECTING, APPLIER: OFF
  // RECEIVER: OFF, APPLIER: ON

  // True if RECEIVER: ON, APPLIER: ON
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"ON", "ON"}}}});
  EXPECT_TRUE(mysqlshdk::mysql::is_async_replication_running(instance));

  // True if RECEIVER: ON, APPLIER: OFF
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"ON", "OFF"}}}});
  EXPECT_TRUE(mysqlshdk::mysql::is_async_replication_running(instance));

  // True if RECEIVER: CONNECTING, APPLIER: ON
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"CONNECTING", "ON"}}}});
  EXPECT_TRUE(mysqlshdk::mysql::is_async_replication_running(instance));

  // True if RECEIVER: CONNECTING, APPLIER: OFF
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"CONNECTING", "OFF"}}}});
  EXPECT_TRUE(mysqlshdk::mysql::is_async_replication_running(instance));

  // True if RECEIVER: CONNECTING, APPLIER: OFF
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"OFF", "ON"}}}});
  EXPECT_TRUE(mysqlshdk::mysql::is_async_replication_running(instance));

  // False if RECEIVER: OFF, APPLIER: OFF
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{"",
                     {"RECEIVER", "APPLIER"},
                     {Type::String, Type::String},
                     {{"OFF", "OFF"}}}});
  EXPECT_FALSE(mysqlshdk::mysql::is_async_replication_running(instance));

  // False if there are no results
  mock_session
      ->expect_query(
          "SELECT a.SERVICE_STATE AS RECEIVER, b.SERVICE_STATE "
          "AS APPLIER FROM "
          "performance_schema.replication_connection_status a, "
          "performance_schema.replication_applier_status b WHERE "
          "a.CHANNEL_NAME != 'group_replication_applier' AND "
          "a.CHANNEL_NAME != 'group_replication_recovery' AND "
          "b.CHANNEL_NAME != 'group_replication_applier' AND "
          "b.CHANNEL_NAME != 'group_replication_recovery'")
      .then_return({{""}});
  EXPECT_FALSE(mysqlshdk::mysql::is_async_replication_running(instance));
}

TEST_F(Replication_test, estimate_gtid_set_size) {
  EXPECT_EQ(0, estimate_gtid_set_size(""));
  EXPECT_EQ(1,
            estimate_gtid_set_size("d089d788-3ed0-11e9-bb3d-3b943755051b:1"));
  EXPECT_EQ(2,
            estimate_gtid_set_size("d089d788-3ed0-11e9-bb3d-3b943755051b:1-2"));
  EXPECT_EQ(6,
            estimate_gtid_set_size("d089d788-3ed0-11e9-bb3d-3b943755051b:1-5,"
                                   "d089d788-3ed0-11e9-bb3d-3b943755051c:3"));
  EXPECT_EQ(
      53, estimate_gtid_set_size("d089d788-3ed0-11e9-bb3d-3b943755051b:1-52,\n"
                                 "d089d788-3ed0-11e9-bb3d-3b943755051c:3123"));
}

std::vector<std::string> k_repl_channel_column_names = {
    "channel_name",
    "host",
    "port",
    "user",
    "source_uuid",
    "group_name",
    "last_heartbeat_timestamp",
    "io_state",
    "io_thread_state",
    "io_errno",
    "io_errmsg",
    "io_errtime",
    "co_state",
    "co_thread_state",
    "co_errno",
    "co_errmsg",
    "co_errtime",
    "w_state",
    "w_thread_state",
    "w_errno",
    "w_errmsg",
    "w_errtime",
    "w_last_trx_delay"};

std::vector<db::Type> k_repl_channel_column_types = {
    Type::String, Type::String,   Type::Integer, Type::String,   Type::Enum,
    Type::String, Type::String,   Type::String,  Type::DateTime, Type::Integer,
    Type::String, Type::DateTime, Type::Enum,    Type::String,   Type::Integer,
    Type::String, Type::DateTime, Type::Enum,    Type::String,   Type::Integer,
    Type::String, Type::DateTime, Type::UInteger};

static std::string k_null = "___NULL___";

TEST_F(Replication_test, get_channel_status) {
  const char *k_repl_channel_query = R"*(
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
WHERE c.channel_name = '%s')*";

  // no replication running
  auto mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // channel doesn't exist
  {
    Replication_channel ch;
    mock_session->expect_query(shcore::str_format(k_repl_channel_query, ""))
        .then_return({{""}});
    EXPECT_FALSE(get_channel_status(instance, "", &ch));

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return({{""}});
    EXPECT_FALSE(get_channel_status(instance, "mychannel", &ch));
  }

  // channel exists but is stopped
  {
    Replication_channel ch;
    mock_session->expect_query(shcore::str_format(k_repl_channel_query, ""))
        .then_return({{"",
                       k_repl_channel_column_names,
                       k_repl_channel_column_types,
                       {{"",
                         "192.168.42.6",
                         "3000",
                         "mysql_innodb_asyncrs_3001",
                         "",
                         "",
                         "2019-04-29 09:59:05.741307",
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         "0"}}}});
    EXPECT_TRUE(get_channel_status(instance, "", &ch));

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return({{"",
                       k_repl_channel_column_names,
                       k_repl_channel_column_types,
                       {{"mychannel",
                         "192.168.42.6",
                         "3000",
                         "mysql_innodb_asyncrs_3001",
                         "",
                         "",
                         "2019-04-29 09:59:05.741307",
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         "0"}}}});
    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::OFF);
  }

  // channel is running
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "109017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("mychannel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_3001", ch.user);
    EXPECT_EQ("192.168.42.6", ch.host);
    EXPECT_EQ(3000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("109017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // channel IO is running, SQL stopped
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return({{"",
                       k_repl_channel_column_names,
                       k_repl_channel_column_types,
                       {{"mychannel",
                         "192.168.42.6",
                         "3000",
                         "mysql_innodb_asyncrs_3001",
                         "",
                         "",
                         "2019-04-29 09:59:05.741307",
                         "ON",
                         "Waiting for master to send event",
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::APPLIER_OFF);

    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::OFF, ch.appliers[0].state);
    EXPECT_EQ("", ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // channel IO is stopped, SQL running
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "OFF",
                k_null,
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::RECEIVER_OFF);

    EXPECT_EQ(Replication_channel::Receiver::OFF, ch.receiver.state);
    EXPECT_EQ("", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // channel IO is connecting, SQL running
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "CONNECTING",
                k_null,
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::CONNECTING);

    EXPECT_EQ(Replication_channel::Receiver::CONNECTING, ch.receiver.state);
    EXPECT_EQ("", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // channel IO is connecting + error, SQL running
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "CONNECTING",
                k_null,
                "1045",
                "error connecting to master 'roo@192.168.42.6:3000' - "
                "retry-time: 60  retries: 1",
                "2019-04-29 15:43:58.627143",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::CONNECTION_ERROR);

    EXPECT_EQ(Replication_channel::Receiver::CONNECTING, ch.receiver.state);
    EXPECT_EQ("", ch.receiver.thread_state);
    EXPECT_EQ(1045, ch.receiver.last_error.code);
    EXPECT_EQ(
        "error connecting to master 'roo@192.168.42.6:3000' - "
        "retry-time: 60  retries: 1",
        ch.receiver.last_error.message);
    EXPECT_EQ("2019-04-29 15:43:58.627143", ch.receiver.last_error.timestamp);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // channel IO is running, SQL has error
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "OFF",
                "",
                "1007",
                "Error 'Can't create database 'db'; database exists' on "
                "query. Default database: 'db'. Query: 'create schema db'",
                "2019-04-29 16:36:56.600064",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::APPLIER_ERROR);

    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::OFF, ch.appliers[0].state);
    EXPECT_EQ("", ch.appliers[0].thread_state);
    EXPECT_EQ(1007, ch.appliers[0].last_error.code);
    EXPECT_EQ(
        "Error 'Can't create database 'db'; database exists' on "
        "query. Default database: 'db'. Query: 'create schema db'",
        ch.appliers[0].last_error.message);
    EXPECT_EQ("2019-04-29 16:36:56.600064",
              ch.appliers[0].last_error.timestamp);
  }

  // 3 parallel appliers, all running
  // 3 parallel appliers, applier error
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Waiting for an event from Coordinator",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Waiting for an event from Coordinator",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Waiting for an event from Coordinator",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::ON);

    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::ON, ch.coordinator.state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(3, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Waiting for an event from Coordinator",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);

    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[1].state);
    EXPECT_EQ("Waiting for an event from Coordinator",
              ch.appliers[1].thread_state);
    EXPECT_EQ(0, ch.appliers[1].last_error.code);

    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[2].state);
    EXPECT_EQ("Waiting for an event from Coordinator",
              ch.appliers[2].thread_state);
    EXPECT_EQ(0, ch.appliers[2].last_error.code);
  }

  // 3 parallel appliers, applier error
  {
    Replication_channel ch;

    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "OFF",
                k_null,
                "1007",
                "Coordinator stopped because there were error(s) in the "
                "worker(s). The most recent failure being: Worker 3 failed "
                "executing transaction "
                "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
                "myhost.000004, end_log_pos 2630. See error log and/or "
                "performance_schema.replication_applier_status_by_worker table "
                "for more details about this failure or others, if any.",
                "2019-04-29 16:41:59.502796",
                "OFF",
                k_null,
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "OFF",
                k_null,
                "1007",
                "Coordinator stopped because there were error(s) in the "
                "worker(s). The most recent failure being: Worker 3 failed "
                "executing transaction "
                "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
                "myhost.000004, end_log_pos 2630. See error log and/or "
                "performance_schema.replication_applier_status_by_worker table "
                "for more details about this failure or others, if any.",
                "2019-04-29 16:41:59.502796",
                "OFF",
                k_null,
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "OFF",
                k_null,
                "1007",
                "Coordinator stopped because there were error(s) in the "
                "worker(s). The most recent failure being: Worker 3 failed "
                "executing transaction "
                "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
                "myhost.000004, end_log_pos 2630. See error log and/or "
                "performance_schema.replication_applier_status_by_worker table "
                "for more details about this failure or others, if any.",
                "2019-04-29 16:41:59.502796",
                "OFF",
                k_null,
                "1007",
                "Worker 3 failed executing transaction "
                "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
                "myhost.000004, end_log_pos 2630; Error 'Can't create database "
                "'db'; database exists' on query. Default database: 'db'."
                " Query: 'create schema db'",
                "2019-04-29 16:41:59.502796",
                "0"}}}});

    EXPECT_TRUE(get_channel_status(instance, "mychannel", &ch));
    EXPECT_EQ(ch.status(), Replication_channel::APPLIER_ERROR);

    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);

    EXPECT_EQ(Replication_channel::Coordinator::OFF, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(1007, ch.coordinator.last_error.code);
    EXPECT_EQ(
        "Coordinator stopped because there were error(s) in the "
        "worker(s). The most recent failure being: Worker 3 failed "
        "executing transaction "
        "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
        "myhost.000004, end_log_pos 2630. See error log and/or "
        "performance_schema.replication_applier_status_by_worker table "
        "for more details about this failure or others, if any.",
        ch.coordinator.last_error.message);
    EXPECT_EQ("2019-04-29 16:41:59.502796",
              ch.coordinator.last_error.timestamp);

    EXPECT_EQ(3, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::OFF, ch.appliers[0].state);
    EXPECT_EQ("", ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);

    EXPECT_EQ(Replication_channel::Applier::OFF, ch.appliers[1].state);
    EXPECT_EQ("", ch.appliers[1].thread_state);
    EXPECT_EQ(0, ch.appliers[1].last_error.code);

    EXPECT_EQ(Replication_channel::Applier::OFF, ch.appliers[2].state);
    EXPECT_EQ("", ch.appliers[2].thread_state);
    EXPECT_EQ(1007, ch.appliers[2].last_error.code);
    EXPECT_EQ(
        "Worker 3 failed executing transaction "
        "'109017c0-6aa0-11e9-8ab6-da28369859f2:47' at master log "
        "myhost.000004, end_log_pos 2630; Error 'Can't create database "
        "'db'; database exists' on query. Default database: 'db'."
        " Query: 'create schema db'",
        ch.appliers[2].last_error.message);
    EXPECT_EQ("2019-04-29 16:41:59.502796",
              ch.appliers[2].last_error.timestamp);
  }
}

TEST_F(Replication_test, get_incoming_channels) {
  const char *k_repl_channels_query = R"*(
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
ORDER BY channel_name)*";

  // no replication running
  auto mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // channel doesn't exist
  {
    mock_session->expect_query(k_repl_channels_query).then_return({{""}});
    EXPECT_EQ(0, get_incoming_channels(instance).size());
  }

  // 1 channel
  {
    mock_session->expect_query(k_repl_channels_query)
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "109017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});
    auto chlist = get_incoming_channels(instance);
    EXPECT_EQ(1, chlist.size());

    auto ch = chlist[0];

    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("mychannel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_3001", ch.user);
    EXPECT_EQ("192.168.42.6", ch.host);
    EXPECT_EQ(3000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("109017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // 2 channels
  {
    mock_session->expect_query(k_repl_channels_query)
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "109017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"nother_channel",
                "192.168.42.16",
                "4000",
                "mysql_innodb_asyncrs_4001",
                "209017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    auto chlist = get_incoming_channels(instance);
    EXPECT_EQ(2, chlist.size());

    auto ch = chlist[0];

    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("mychannel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_3001", ch.user);
    EXPECT_EQ("192.168.42.6", ch.host);
    EXPECT_EQ(3000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("109017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);

    ch = chlist[1];
    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("nother_channel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_4001", ch.user);
    EXPECT_EQ("192.168.42.16", ch.host);
    EXPECT_EQ(4000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("209017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);
  }

  // 2 channels + parallel
  {
    mock_session->expect_query(k_repl_channels_query)
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"another_channel",
                "192.168.42.16",
                "4000",
                "mysql_innodb_asyncrs_4001",
                "209017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "109017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Waiting for an event from Coordinator",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"},
               {"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "109017c0-6aa0-11e9-8ab6-da28369859f2",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "ON",
                "Waiting for an event from Coordinator",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});

    auto chlist = get_incoming_channels(instance);
    EXPECT_EQ(2, chlist.size());

    auto ch = chlist[0];

    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("another_channel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_4001", ch.user);
    EXPECT_EQ("192.168.42.16", ch.host);
    EXPECT_EQ(4000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("209017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::NONE, ch.coordinator.state);
    EXPECT_EQ("", ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(1, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);

    ch = chlist[1];
    EXPECT_EQ(ch.status(), Replication_channel::ON);
    EXPECT_EQ("mychannel", ch.channel_name);
    EXPECT_EQ("mysql_innodb_asyncrs_3001", ch.user);
    EXPECT_EQ("192.168.42.6", ch.host);
    EXPECT_EQ(3000, ch.port);
    EXPECT_EQ("", ch.group_name);
    EXPECT_EQ("109017c0-6aa0-11e9-8ab6-da28369859f2", ch.source_uuid);
    EXPECT_EQ(Replication_channel::Receiver::ON, ch.receiver.state);
    EXPECT_EQ("Waiting for master to send event", ch.receiver.thread_state);
    EXPECT_EQ(0, ch.receiver.last_error.code);

    EXPECT_EQ(Replication_channel::Coordinator::ON, ch.coordinator.state);
    EXPECT_EQ("Slave has read all relay log; waiting for more updates",
              ch.coordinator.thread_state);
    EXPECT_EQ(0, ch.coordinator.last_error.code);

    EXPECT_EQ(2, ch.appliers.size());
    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[0].state);
    EXPECT_EQ("Waiting for an event from Coordinator",
              ch.appliers[0].thread_state);
    EXPECT_EQ(0, ch.appliers[0].last_error.code);

    EXPECT_EQ(Replication_channel::Applier::ON, ch.appliers[1].state);
    EXPECT_EQ("Waiting for an event from Coordinator",
              ch.appliers[1].thread_state);
    EXPECT_EQ(0, ch.appliers[1].last_error.code);
  }
}

TEST_F(Replication_test, wait_replication_done_connecting) {
  const char *k_repl_channel_query = R"*(
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
WHERE c.channel_name = '%s')*";

  // no replication running
  auto mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // channel IO is connecting, SQL running
  auto add_connecting = [&]() {
    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "CONNECTING",
                k_null,
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});
  };

  // channel IO is running, SQL running
  auto add_running = [&]() {
    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return(
            {{"",
              k_repl_channel_column_names,
              k_repl_channel_column_types,
              {{"mychannel",
                "192.168.42.6",
                "3000",
                "mysql_innodb_asyncrs_3001",
                "",
                "",
                "2019-04-29 09:59:05.741307",
                "ON",
                "Waiting for master to send event",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                k_null,
                k_null,
                k_null,
                k_null,
                k_null,
                "ON",
                "Slave has read all relay log; waiting for more updates",
                "0",
                "",
                "0000-00-00 00:00:00.000000",
                "0"}}}});
  };

  // channel is stopped
  auto add_stopped = [&]() {
    mock_session
        ->expect_query(shcore::str_format(k_repl_channel_query, "mychannel"))
        .then_return({{"",
                       k_repl_channel_column_names,
                       k_repl_channel_column_types,
                       {{"mychannel",
                         "192.168.42.6",
                         "3000",
                         "mysql_innodb_asyncrs_3001",
                         "",
                         "",
                         "2019-04-29 09:59:05.741307",
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         k_null,
                         "OFF",
                         k_null,
                         "0",
                         "",
                         "0000-00-00 00:00:00.000000",
                         "0"}}}});
  };

  add_connecting();
  add_connecting();
  add_connecting();
  add_running();
  Replication_channel ch =
      wait_replication_done_connecting(instance, "mychannel", 10);
  EXPECT_EQ(Replication_channel::ON, ch.status());

  add_stopped();
  ch = wait_replication_done_connecting(instance, "mychannel", 10);
  EXPECT_EQ(Replication_channel::OFF, ch.status());
}

}  // namespace mysql
}  // namespace mysqlshdk
