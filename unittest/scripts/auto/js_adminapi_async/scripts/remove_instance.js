//@ {VER(>=8.0.11)}
// Tests removeInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@ INCLUDE async_utils.inc


//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
var uuid3 = "5ef81566-9395-11e9-87e9-333333333333";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid: uuid1});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid: uuid2});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip, server_uuid: uuid3});

shell.connect(__sandbox_uri1);

var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox2);

var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

var report_host2 = session2.runSql("SELECT coalesce(@@report_host, @@hostname)").fetchOne()[0];

// enable interactive by default
shell.options['useWizards'] = true;

// Negative tests based on environment and params
//--------------------------------

//@ bad parameters (should fail)
rs.removeInstance();
rs.removeInstance(null);
rs.removeInstance(null, null);
rs.removeInstance(1, null);
rs.removeInstance(__sandbox1, 1);
rs.removeInstance(__sandbox1, 1, 3);
rs.removeInstance(null, {});
rs.removeInstance({}, {});
rs.removeInstance(__sandbox1, {badOption:123});
rs.removeInstance([__sandbox_uri1]);

//@ disconnected rs object (should fail)
rs.disconnect();
rs.removeInstance(__sandbox2);
rs = dba.getReplicaSet();

//@ remove invalid host (should fail)
rs.removeInstance("localhost:"+__mysql_sandbox_port3);

rs.removeInstance("bogushost");

//@ remove PRIMARY (should fail)
rs.removeInstance(__sandbox1);

rs.removeInstance("localhost:"+__mysql_sandbox_port1);

//@ remove when not in metadata (should fail)
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();

setup_slave(session3, __mysql_sandbox_port1);

rs.removeInstance(__sandbox3);

reset_instance(session3);

//@ remove while the instance we got the rs from is down (should fail)
// NOTE: The error for removeInstance might not be deterministic, do something
//       to fix it if the happens. The following could also be observed if we
//       wait more time after the server is stopped:
//       ReplicaSet.removeInstance: The Metadata is inaccessible (MetadataError)
rs.addInstance(__sandbox3);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

shell.connect(__sandbox_uri3);
var rs = dba.getReplicaSet();

testutil.stopSandbox(__mysql_sandbox_port3, {wait:true});
// should fail while updating MD on instance 3
rs.removeInstance(__sandbox2);

testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
var rs = dba.getReplicaSet();
// ok
rs.removeInstance(__sandbox2);

//@ bad URI with a different user (should fail)
rs.removeInstance("admin@"+__sandbox2);

//@ bad URI with a different password (should fail)
rs.removeInstance("root:bla@"+__sandbox2);

// Positive tests for specific issues
//--------------------------------

//@ removeInstance on a URI
rs.addInstance(__sandbox2);
rs.removeInstance(__sandbox_uri2);

//@ check state after removal
rs.addInstance(__sandbox2);
rs.removeInstance(__sandbox2);



// removed instance should be:
// SRO
EXPECT_TRUE(get_sysvar(session2, "super_read_only"));
// metadata exists, but instance is not there
shell.dumpRows(session2.runSql("SELECT * FROM mysql_innodb_cluster_metadata.v2_ar_members ORDER BY instance_id"), "tabbed");
// repl should be stopped
shell.dumpRows(session2.runSql("SHOW SLAVE STATUS"));
// slave_master_info should be empty
shell.dumpRows(session2.runSql("SELECT * FROM mysql.slave_master_info"));

//@ from same target as main session
rs.addInstance(__sandbox2);
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();

rs.removeInstance(__sandbox2);

//@ rs is connected to an invalidated member now, but the rs should handle that
rs.addInstance(__sandbox2);

//@ while some other secondary (sb3) is down
testutil.stopSandbox(__mysql_sandbox_port3);
rs.removeInstance(__sandbox2);

testutil.startSandbox(__mysql_sandbox_port3);

//@ target is not super-read-only
rs.addInstance(__sandbox2);

session2.runSql("SET GLOBAL super_read_only=0");
rs.removeInstance(__sandbox2);
EXPECT_TRUE(get_sysvar(session2, "super_read_only"));

//@ target is not read-only
rs.addInstance(__sandbox2);

session2.runSql("SET GLOBAL read_only=0");
rs.removeInstance(__sandbox2);
EXPECT_TRUE(get_sysvar(session2, "read_only"));

// Options
//--------------------------------
// force is tested elsewhere

//@ timeout (should fail)
var rs = rebuild_rs();
shell.connect(__sandbox_uri1);
session2.runSql("FLUSH TABLES WITH READ LOCK");
// sandbox2 slave will be stuck trying to apply this because of the lock
session.runSql("CREATE SCHEMA testdb");

// sandbox2 slave will be stuck trying to apply this because of the lock
rs.status();
rs.removeInstance(__sandbox2, {timeout: 1});

session2.runSql("UNLOCK TABLES");
rs.status();

// Runtime problems
//--------------------------------

//@ remove while target down - no force (should fail)
var rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port2, {wait:true});
rs.removeInstance(__sandbox2);

//@ remove while target down (localhost) - force (should fail)
// This one fails because __sandbox2 is not how the instance is known in the MD
rs.removeInstance(__sandbox2, {force:true});

//@ remove while target down (hostname) - force
rs.removeInstance(report_host2+":"+__mysql_sandbox_port2, {force:true});

testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@ remove instance not in replicaset (metadata), no force (fail).
rs.removeInstance(__sandbox2);

//@ remove instance not in replicaset (metadata), with force true (fail).
rs.removeInstance(__sandbox2, {force:true});

//@ remove while target down (using extra elements in URL) - force
// Covers Bug #30574305 REPLICASET - UNREACHABLE WAS NOT FOUND IN METADATA

rs.addInstance(__sandbox2);
testutil.stopSandbox(__mysql_sandbox_port2, {wait:true});

rs.removeInstance("root@"+report_host2+":"+__mysql_sandbox_port2, {force:true});

testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@ error in replication channel - no force (should fail)
rs.addInstance(__sandbox2);

// force an applier error
session2.runSql("SET global super_read_only=0");
run_nolog(session2, "CREATE SCHEMA testdb");
session2.runSql("SET global super_read_only=1");

session.runSql("CREATE SCHEMA testdb");

rs.removeInstance(__sandbox2);

//@ error in replication channel - force
// FIXME: The use of the force option is not working, still failing.
rs.removeInstance(__sandbox2, {force:true});

// rebuild
reset_instance(session2);
rs.addInstance(__sandbox2);

//@ remove with repl already stopped - no force (should fail)
session2.runSql("STOP SLAVE");

rs.removeInstance(__sandbox2);

//@ remove with repl already stopped - force
rs.removeInstance(__sandbox2, {force:true});

//@ impossible sync - no force (should fail)
// purge transactions from the master before removing instance, so that
// sync done during removal will never catch up
reset_instance(session2);
rs.addInstance(__sandbox2);
session2.runSql("STOP SLAVE");
session.runSql("CREATE SCHEMA testdb2");
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");
session2.runSql("START SLAVE");

rs.removeInstance(__sandbox2);

//@ impossible sync - force
rs.removeInstance(__sandbox2, {force:true});

//@ remove while PRIMARY down (should fail)
testutil.stopSandbox(__mysql_sandbox_port1, {wait:true});
shell.connect(__sandbox_uri2);

rs = dba.getReplicaSet();

rs.removeInstance(__sandbox2);

testutil.startSandbox(__mysql_sandbox_port1);

// XXX Rollback
//--------------------------------
// Ensure clean rollback after failures


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
