//@ {VER(>=8.0.11)}

// Tests forcePrimaryInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested elsewhere.

//@ INCLUDE async_utils.inc

//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-222222222222"});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip, server_uuid:"5ef81566-9395-11e9-87e9-333333333333"});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:1});
rs.addInstance(__sandbox_uri2);
rs.addInstance(__sandbox_uri3);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__hostname_uri3);

var userhost=session3.runSql("select user()").fetchOne()[0];

// faster connection timeouts
shell.options["connectTimeout"] = 1.0;
shell.options["dba.connectTimeout"] = 1.0;

// Runtime problems
//--------------------------------

//@ promoted has stopped replication, should fail
testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rs= dba.getReplicaSet();
session.runSql("STOP SLAVE");

testutil.waitForReplConnectionError(__mysql_sandbox_port3, "");

rs.forcePrimaryInstance(__sandbox2);

//@ promoted has stopped replication, still fail with invalidateErrorInstances
testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rs= dba.getReplicaSet();
session.runSql("STOP SLAVE");

testutil.waitForReplConnectionError(__mysql_sandbox_port3, "");

rs.forcePrimaryInstance(__sandbox2, {invalidateErrorInstances: true});

//@ a secondary has stopped replication, should fail
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP SLAVE");

testutil.waitForReplConnectionError(__mysql_sandbox_port3, "");
rs.forcePrimaryInstance(__sandbox3);

//@ a secondary has stopped replication, pass with invalidateErrorInstances
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP SLAVE");

testutil.waitForReplConnectionError(__mysql_sandbox_port3, "");
rs.forcePrimaryInstance(__sandbox3, {invalidateErrorInstances: true});

strip_status(rs.status());

//@ promoted is down (should fail)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.stopSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

rs.forcePrimaryInstance(__sandbox3);

//@ a secondary is down (should fail and suggest invalidateErrorInstances)
rs.forcePrimaryInstance(__sandbox2);

//@ a different slave is more up-to-date (should fail)
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
// Make sb2 behind by making it not retrieve all trx from primary.
session2.runSql("STOP SLAVE IO_THREAD");
session.runSql("CREATE SCHEMA mydb");
testutil.stopSandbox(__mysql_sandbox_port1);
session2.runSql("START SLAVE IO_THREAD");
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

// trying to promote sb2, should fail now
rs.forcePrimaryInstance(__sandbox2);

//@ but promoting sb3 should be fine
rs.forcePrimaryInstance(__sandbox3);

//@ retry the same but let the primary be picked automatically
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();

// exec transaction at the primary while one of the instances is down
testutil.stopSandbox(__mysql_sandbox_port2);
session.runSql("CREATE SCHEMA mydb");
// now we stop the primary and restart sb2, which won't get the update
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

// sb3 should be auto-selected, since it's the most up-to-date
rs.forcePrimaryInstance();

//@ a secondary has errant GTIDs (should fail)
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs=dba.getReplicaSet();

inject_errant_gtid(mysql.getSession(__sandbox_uri3));

// There's no way to tell between an errant transaction and a legitimate
// transaction, since we don't know what the primary looks like
rs.forcePrimaryInstance(__sandbox2);

//@ Replication conflict error (should fail)
// Create a DB at the slave and then create the same one in the master
// NOTE: In this case the operation will fail immediately when trying to apply
//       the relay logs on all available instances.
testutil.startSandbox(__mysql_sandbox_port1);
rs = rebuild_rs();
inject_applier_error(session, mysql.getSession(__sandbox_uri2));

testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
// An error should occur while trying apply received GTIDs at the target instance.
rs.forcePrimaryInstance(__sandbox_uri3);

//@ Replication conflict error (pass with invalidateErrorInstances)
// Create a DB at the slave and then create the same one in the master
// NOTE: In this case the operation will fail immediately when trying to apply
//       the relay logs on all available instances.
testutil.startSandbox(__mysql_sandbox_port1);
rs = rebuild_rs();
inject_applier_error(session, mysql.getSession(__sandbox_uri2));

testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
// An error should occur while trying apply received GTIDs at the target instance.
rs.forcePrimaryInstance(__sandbox_uri3, {invalidateErrorInstances: true});

strip_status(rs.status());

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
