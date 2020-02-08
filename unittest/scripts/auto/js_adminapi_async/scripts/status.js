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

// ==== Various Config Options

//@<> Replication delay, extended:0
session2.runSql("STOP SLAVE");
session2.runSql("CHANGE MASTER TO master_delay=42");
session2.runSql("START SLAVE");

s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.delay, 42);

session2.runSql("STOP SLAVE");
session2.runSql("CHANGE MASTER TO master_delay=0");
session2.runSql("START SLAVE");

//@<> Heartbeat interval, extended:2
session2.runSql("STOP SLAVE");
session2.runSql("CHANGE MASTER TO MASTER_HEARTBEAT_PERIOD=123");
session2.runSql("START SLAVE");

s = rs.status({extended:2});
EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.heartbeatPeriod, 123);

session2.runSql("STOP SLAVE");
session2.runSql("CHANGE MASTER TO MASTER_HEARTBEAT_PERIOD=30");
session2.runSql("START SLAVE");

//@<> SSL, extended:2 - not implemented yet
// session2.runSql("STOP SLAVE");
// session2.runSql("CHANGE MASTER TO MASTER_SSL=1");
// session2.runSql("START SLAVE");

// s = rs.status({extended:2});
// EXPECT_EQ(s.replicaSet.topology[sb2].replication.options.sslEnabled, true);

// session2.runSql("STOP SLAVE");
// session2.runSql("CHANGE MASTER TO MASTER_SSL=0");
// session2.runSql("START SLAVE");

//@<> Worker threads, extended:0
session2.runSql("STOP SLAVE");
session2.runSql("SET GLOBAL slave_parallel_workers=3");
session2.runSql("START SLAVE");

s = rs.status();
EXPECT_EQ(s.replicaSet.topology[sb2].replication.applierWorkerThreads, 3);

session2.runSql("STOP SLAVE");
session2.runSql("SET GLOBAL slave_parallel_workers=0");
session2.runSql("START SLAVE");

// ==== Error conditions

//@ Primary is RO, should show as error
session1.runSql("SET GLOBAL super_read_only=1");
s = rs.status();
session1.runSql("SET GLOBAL super_read_only=0");

//@ Replication stopped in a secondary
session2.runSql("STOP SLAVE");
rs.status();

session2.runSql("START SLAVE");

//@ Replication running in a primary
session1.runSql("CHANGE MASTER TO MASTER_HOST='localhost', MASTER_PORT=/*(*/?/*)*/, MASTER_USER='root', MASTER_PASSWORD='root'", [__mysql_sandbox_port2]);
session1.runSql("START SLAVE");
// Ignore SQL and IO Thread state output, because they can still be in an
// intermediary state (slower machines running tests), ensuring determinism:
// - SQL Thread: "Reading event from the relay log" instead of
//               "Slave has read all relay log; waiting for more updates";
// - IO Thread: "Queueing master event to the relay log" instead of
//              "Waiting for master to send event";
rs.status();

session1.runSql("STOP SLAVE");

//@ A secondary is down
testutil.stopSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();

rs.status();

testutil.startSandbox(__mysql_sandbox_port2);

//@ Primary is down
testutil.stopSandbox(__mysql_sandbox_port1, {wait:1});

shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
rs.status();
testutil.startSandbox(__mysql_sandbox_port1);

// @<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
