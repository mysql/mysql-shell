//@ {VER(>=8.0.11)}

// Tests setPrimaryInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-222222222222"});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

sb2 = hostname_ip+":"+__mysql_sandbox_port2;

//@ Errors (should fail)
rs.status(1);
rs.status({extended:"a"});
rs.status({extended:-1});
rs.status({extended:3});
rs.status({bla:1});

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@ Regular, extended:0
rs.status();

//@ Regular, extended:1
rs.status({extended:1});

//@ Regular, extended:2
rs.status({extended:2});

//@<> BUG#34530914 make sure that the session collation is the intended ('utf8mb4_0900_ai_ci')

testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "character-set-server", "utf8mb4");
testutil.changeSandboxConf(__mysql_sandbox_port1, "collation-server", "utf8mb4_unicode_ci");
if (__version_num < 80300) {
    testutil.changeSandboxConf(__mysql_sandbox_port1, "character-set-client-handshake", "OFF");
}

testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

rs = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { rs.status({extended:2}); });
EXPECT_SHELL_LOG_NOT_CONTAINS("Illegal mix of collations");

testutil.stopSandbox(__mysql_sandbox_port1);
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "character-set-server");
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "collation-server");
if (__version_num < 80300) {
    testutil.removeFromSandboxConf(__mysql_sandbox_port1, "character-set-client-handshake");
}
testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
session1 = mysql.getSession(__sandbox_uri1);

rs = dba.getReplicaSet();

// ==== Various Config Options

//@<> Replication delay, extended:0
session2.runSql("STOP " + get_replica_keyword());
session2.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_delay=42");
session2.runSql("START " + get_replica_keyword());

s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.delay, 42);

session2.runSql("STOP " + get_replica_keyword());
session2.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_delay=0");
session2.runSql("START " + get_replica_keyword());

//@<> Heartbeat interval, extended:2
session2.runSql("STOP " + get_replica_keyword());
session2.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HEARTBEAT_PERIOD=123");
session2.runSql("START " + get_replica_keyword());

s = rs.status({extended:2});
EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.heartbeatPeriod, 123);

session2.runSql("STOP " + get_replica_keyword());
session2.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HEARTBEAT_PERIOD=30");
session2.runSql("START " + get_replica_keyword());

//@<> SSL, extended:2 - not implemented yet
// session2.runSql("STOP " + get_replica_keyword());
// session2.runSql("change " + get_replication_source_keyword() + " MASTER_SSL=1");
// session2.runSql("START " + get_replica_keyword());

// s = rs.status({extended:2});
// EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.sslEnabled, true);

// session2.runSql("STOP " + get_replica_keyword());
// session2.runSql("change " + get_replication_source_keyword() + " MASTER_SSL=0");
// session2.runSql("START " + get_replica_keyword());

//@<> Worker threads, extended:0
session2.runSql("STOP " + get_replica_keyword());
session2.runSql("SET GLOBAL slave_parallel_workers=3");
session2.runSql("START " + get_replica_keyword());

s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].replication.applierWorkerThreads, 3);

if (__version_num < 80300) {
    session2.runSql("STOP " + get_replica_keyword());
    session2.runSql("SET GLOBAL slave_parallel_workers=0");
    session2.runSql("START " + get_replica_keyword());
}

// ==== Error conditions

// BUG#32015164: MISSING INFORMATION ABOUT REQUIRED PARALLEL-APPLIERS SETTINGS ON UPGRADE SCENARIO
// If a cluster member with a version >= 8.0.23 doesn't have parallel-appliers enabled, that information
// must be included in 'instanceErrors'

//@<> BUG#32015164: preparation {VER(>=8.0.23)}

// Disable parallel-appliers
testutil.changeSandboxConf(__mysql_sandbox_port2, "binlog_transaction_dependency_tracking", "COMMIT_ORDER");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "replica_parallel_type");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "replica_preserve_commit_order");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "transaction_write_set_extraction");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "replica_parallel_workers");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port2);
// Instance must be back online
sb2 = hostname_ip+":"+__mysql_sandbox_port2;
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].status, "ONLINE");

//@ BUG#32015164: instanceErrors must report missing parallel-appliers {VER(>=8.0.23)}
println(s);

//@<> BUG#32015164: fix with dba.configureInstance() {VER(>=8.0.23)}
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP " + get_replica_keyword());
dba.configureReplicaSetInstance(__sandbox_uri2);

//@<> BUG#32015164: rejoin instance after fix {VER(>=8.0.23)}
testutil.restartSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);
s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].status, "ONLINE");

//@<OUT> BUG#32015164: status should be fine now {VER(>=8.0.23)}
println(s);

//@<> BUG#32015164: Finalize (restore value of slave_parallel_workers)
session2.runSql("STOP " + get_replica_keyword());
if (__version_num < 80300) {
    session2.runSql("SET GLOBAL slave_parallel_workers=0");
}
session2.runSql("START " + get_replica_keyword());

//@ Primary is RO, should show as error
session1.runSql("SET GLOBAL super_read_only=1");
s = rs.status();
session1.runSql("SET GLOBAL super_read_only=0");

//@ Replication stopped in a secondary
session2.runSql("STOP " + get_replica_keyword());
rs.status();

session2.runSql("START " + get_replica_keyword());

//@ Replication running in a primary
session1.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HOST='localhost', " + get_replication_option_keyword() + "_PORT=/*(*/?/*)*/, " + get_replication_option_keyword() + "_USER='root', " + get_replication_option_keyword() + "_PASSWORD='root'", [__mysql_sandbox_port2]);
session1.runSql("START " + get_replica_keyword());
// Ignore SQL and IO Thread state output, because they can still be in an
// intermediary state (slower machines running tests), ensuring determinism:
// - SQL Thread: "Reading event from the relay log" instead of
//               "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates";
// - IO Thread: "Queueing master event to the relay log" instead of
//              "Waiting for <<<__source_keyword>>> to send event";
rs.status();

session1.runSql("STOP " + get_replica_keyword());

//@ A secondary is down
testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});

shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();

rs.status();

testutil.startSandbox(__mysql_sandbox_port2);

//@ Primary is down
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

testutil.waitForReplConnectionError(__mysql_sandbox_port2, "");

shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
testutil.waitForReplConnectionError(__mysql_sandbox_port2, "");
rs.status();
testutil.startSandbox(__mysql_sandbox_port1);

// @<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
