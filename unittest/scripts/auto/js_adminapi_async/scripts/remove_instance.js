//@ {VER(>=8.0.11)}
// Tests removeInstance() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@<> INCLUDE async_utils.inc


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

//@<> bad parameters (should fail)
EXPECT_THROWS(function () { rs.removeInstance(); }, "ReplicaSet.removeInstance: Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function () { rs.removeInstance(null); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");
EXPECT_THROWS(function () { rs.removeInstance(null, null); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");
EXPECT_THROWS(function () { rs.removeInstance(1, null); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");
EXPECT_THROWS(function () { rs.removeInstance(__sandbox1, 1); }, "ReplicaSet.removeInstance: Argument #2 is expected to be a map");
EXPECT_THROWS(function () { rs.removeInstance(__sandbox1, 1, 3); }, "ReplicaSet.removeInstance: Invalid number of arguments, expected 1 to 2 but got 3");
EXPECT_THROWS(function () { rs.removeInstance(null, {}); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");
EXPECT_THROWS(function () { rs.removeInstance({}, {}); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");
EXPECT_THROWS(function () { rs.removeInstance(__sandbox1, {badOption:123}); }, "ReplicaSet.removeInstance: Invalid options: badOption");
EXPECT_THROWS(function () { rs.removeInstance([__sandbox_uri1]); }, "ReplicaSet.removeInstance: Argument #1 is expected to be a string");

//@<> disconnected rs object (should fail)
rs.disconnect();
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: The replicaset object is disconnected. Please use dba.\<\<\<getReplicaSet\>\>\>() to obtain a new object.`);

rs = dba.getReplicaSet();

//@<> remove invalid host (should fail)
EXPECT_THROWS(function () { rs.removeInstance("localhost:"+__mysql_sandbox_port3); }, `ReplicaSet.removeInstance: ${hostname_ip}:${__mysql_sandbox_port3} does not belong to the replicaset`);
EXPECT_STDOUT_CONTAINS(`ERROR: Instance ${hostname_ip}:${__mysql_sandbox_port3} cannot be removed because it does not belong to the replicaset (not found in the metadata). If you really want to remove this instance because it is still using replication then it must be stopped manually.`);

EXPECT_THROWS(function () { rs.removeInstance("bogushost"); }, `ReplicaSet.removeInstance: Could not open connection to 'bogushost': Unknown MySQL server host 'bogushost'`);

//@<> remove PRIMARY (should fail)
EXPECT_THROWS(function () { rs.removeInstance(__sandbox1); }, `ReplicaSet.removeInstance: PRIMARY instance cannot be removed from the replicaset.`);

EXPECT_THROWS(function () { rs.removeInstance("localhost:"+__mysql_sandbox_port1); }, `ReplicaSet.removeInstance: PRIMARY instance cannot be removed from the replicaset.`);

//@<> remove when not in metadata (should fail)
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();

setup_slave(session3, __mysql_sandbox_port1);

EXPECT_THROWS(function () { rs.removeInstance(__sandbox3); }, `ReplicaSet.removeInstance: ${hostname_ip}:${__mysql_sandbox_port3} does not belong to the replicaset`);

reset_instance(session3);

//@<> remove while the instance we got the rs from is down (should fail)
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
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: Failed to execute query on Metadata server ${hostname_ip}:${__mysql_sandbox_port3}: Lost connection to MySQL server during query`);

testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
var rs = dba.getReplicaSet();
// ok
rs.removeInstance(__sandbox2);

//@<> bad URI with a different user (should fail)
EXPECT_THROWS(function () { rs.removeInstance("admin@"+__sandbox2); }, `ReplicaSet.removeInstance: Invalid target instance specification`);
EXPECT_STDOUT_CONTAINS(`ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them`);

//@<> bad URI with a different password (should fail)
EXPECT_THROWS(function () { rs.removeInstance("root:bla@"+__sandbox2); }, `ReplicaSet.removeInstance: Invalid target instance specification`);
EXPECT_STDOUT_CONTAINS(`ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them`);

// Positive tests for specific issues
//--------------------------------

//@<> removeInstance on a URI
rs.addInstance(__sandbox2);
rs.removeInstance(__sandbox_uri2);

EXPECT_STDOUT_CONTAINS(`The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.`);

//@<> check state after removal
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

EXPECT_STDOUT_CONTAINS_MULTILINE(`
view_id	cluster_id	instance_id	label	member_id	member_role	master_instance_id	master_member_id
9	[[*]]	1	${hostname_ip}:${__mysql_sandbox_port1}	5ef81566-9395-11e9-87e9-111111111111	PRIMARY	NULL	NULL
9	[[*]]	3	${hostname_ip}:${__mysql_sandbox_port3}	5ef81566-9395-11e9-87e9-333333333333	SECONDARY	1	5ef81566-9395-11e9-87e9-111111111111
2
0
0
`);

//@<> from same target as main session
rs.addInstance(__sandbox2);
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();

rs.removeInstance(__sandbox2);

EXPECT_STDOUT_CONTAINS(`The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.`);

//@<> rs is connected to an invalidated member now, but the rs should handle that
rs.addInstance(__sandbox2);

//@<> while some other secondary (sb3) is down
testutil.stopSandbox(__mysql_sandbox_port3);
rs.removeInstance(__sandbox2);

EXPECT_STDOUT_CONTAINS(`The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.`);

testutil.startSandbox(__mysql_sandbox_port3);

//@<> target is not super-read-only
rs.addInstance(__sandbox2);

session2.runSql("SET GLOBAL super_read_only=0");
rs.removeInstance(__sandbox2);
EXPECT_STDOUT_CONTAINS(`The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.`);
EXPECT_TRUE(get_sysvar(session2, "super_read_only"));

//@<> target is not read-only
rs.addInstance(__sandbox2);

session2.runSql("SET GLOBAL read_only=0");
rs.removeInstance(__sandbox2);

EXPECT_STDOUT_CONTAINS(`The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.`);
EXPECT_TRUE(get_sysvar(session2, "read_only"));

// Options
//--------------------------------
// force is tested elsewhere

//@<> timeout (should fail)
var rs = rebuild_rs();
shell.connect(__sandbox_uri1);
session2.runSql("FLUSH TABLES WITH READ LOCK");
// sandbox2 slave will be stuck trying to apply this because of the lock
session.runSql("CREATE SCHEMA testdb");

// sandbox2 slave will be stuck trying to apply this because of the lock
rs.status();
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2, {timeout: 1}); }, `ReplicaSet.removeInstance: Timeout reached waiting for transactions from ${hostname_ip}:${__mysql_sandbox_port1} to be applied on instance '${hostname_ip}:${__mysql_sandbox_port2}'`);

session2.runSql("UNLOCK TABLES");
rs.status();

// Runtime problems
//--------------------------------

//@<> remove while target down - no force (should fail)
var rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port2, {wait:true});

EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: Could not open connection to 'localhost:${__mysql_sandbox_port2}': Can't connect to MySQL server on 'localhost' `);

EXPECT_STDOUT_NOT_CONTAINS(`ERROR: Unable to connect to the target instance 'localhost:${__mysql_sandbox_port2}'. Please verify the connection settings, make sure the instance is available and try again.`);
EXPECT_STDOUT_CONTAINS(`ERROR: Unable to connect to the target instance localhost:${__mysql_sandbox_port2}. Please make sure the instance is available and try again. If the instance is permanently not reachable, use the 'force' option to remove it from the replicaset metadata and skip reconfiguration of that instance.`);

//@<> remove while target down (localhost) - force (should fail)
// This one fails because __sandbox2 is not how the instance is known in the MD
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2, {force:true}); }, `ReplicaSet.removeInstance: localhost:${__mysql_sandbox_port2} does not belong to the replicaset`);

EXPECT_STDOUT_CONTAINS(`ERROR: Instance localhost:${__mysql_sandbox_port2} is unreachable and was not found in the replicaset metadata. The exact address of the instance as recorded in the metadata must be used in cases where the target is unreachable.`);

//@<> remove while target down (hostname) - force
rs.removeInstance(report_host2+":"+__mysql_sandbox_port2, {force:true});

EXPECT_STDOUT_NOT_CONTAINS(`ERROR: Unable to connect to the target instance 'localhost:${__mysql_sandbox_port2}'. Please verify the connection settings, make sure the instance is available and try again.`);
EXPECT_STDOUT_CONTAINS_MULTILINE(`
NOTE: Unable to connect to the target instance ${hostname_ip}:${__mysql_sandbox_port2}. The instance will only be removed from the metadata, but its replication configuration cannot be updated. Please, take any necessary actions to make sure that the instance will not replicate from the replicaset if brought back online.
Metadata for instance '${hostname_ip}:${__mysql_sandbox_port2}' was deleted, but instance configuration could not be updated.
`)

testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@<> remove instance not in replicaset (metadata), no force (fail).
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: ${hostname_ip}:${__mysql_sandbox_port2} does not belong to the replicaset`);

EXPECT_STDOUT_CONTAINS(`ERROR: Instance ${hostname_ip}:${__mysql_sandbox_port2} cannot be removed because it does not belong to the replicaset (not found in the metadata). If you really want to remove this instance because it is still using replication then it must be stopped manually.`);

//@<> remove instance not in replicaset (metadata), with force true (fail).
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2, {force:true}); }, `ReplicaSet.removeInstance: ${hostname_ip}:${__mysql_sandbox_port2} does not belong to the replicaset`);

EXPECT_STDOUT_CONTAINS(`ERROR: Instance ${hostname_ip}:${__mysql_sandbox_port2} cannot be removed because it does not belong to the replicaset (not found in the metadata). If you really want to remove this instance because it is still using replication then it must be stopped manually.`);

//@<> remove while target down (using extra elements in URL) - force
// Covers Bug #30574305 REPLICASET - UNREACHABLE WAS NOT FOUND IN METADATA

rs.addInstance(__sandbox2);
testutil.stopSandbox(__mysql_sandbox_port2, {wait:true});

rs.removeInstance("root@"+report_host2+":"+__mysql_sandbox_port2, {force:true});

EXPECT_STDOUT_CONTAINS_MULTILINE(`
NOTE: Unable to connect to the target instance ${hostname_ip}:${__mysql_sandbox_port2}. The instance will only be removed from the metadata, but its replication configuration cannot be updated. Please, take any necessary actions to make sure that the instance will not replicate from the replicaset if brought back online.
Metadata for instance '${hostname_ip}:${__mysql_sandbox_port2}' was deleted, but instance configuration could not be updated.
`);

testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

//@<> error in replication channel - no force (should fail)
rs.addInstance(__sandbox2);

// force an applier error
session2.runSql("SET global super_read_only=0");
run_nolog(session2, "CREATE SCHEMA testdb");
session2.runSql("SET global super_read_only=1");

session.runSql("CREATE SCHEMA testdb");

EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: Replication applier error in ${hostname_ip}:${__mysql_sandbox_port2}`);

EXPECT_STDOUT_CONTAINS(`ERROR: Replication applier error in ${hostname_ip}:${__mysql_sandbox_port2}:`);

//@<> error in replication channel - force
// FIXME: The use of the force option is not working, still failing.
EXPECT_THROWS(function () { rs.removeInstance(__sandbox2, {force:true}); }, `ReplicaSet.removeInstance: Replication applier error in ${hostname_ip}:${__mysql_sandbox_port2}`);

// rebuild
reset_instance(session2);
EXPECT_THROWS(function () { rs.addInstance(__sandbox2); }, `ReplicaSet.addInstance: ${hostname_ip}:${__mysql_sandbox_port2} is already a member of this replicaset.`);

//@<> remove with repl already stopped - no force (should fail)
session2.runSql("STOP SLAVE");

EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: Replication is not active in target instance`);

EXPECT_STDOUT_CONTAINS_MULTILINE(`
WARNING: Replication is not active in instance ${hostname_ip}:${__mysql_sandbox_port2}.
ERROR: Instance cannot be safely removed while in this state.
Use the 'force' option to remove this instance anyway. The instance may be left in an inconsistent state after removed.
`);

//@<> remove with repl already stopped - force
rs.removeInstance(__sandbox2, {force:true});

EXPECT_STDOUT_CONTAINS_MULTILINE(`
WARNING: Replication is not active in instance ${hostname_ip}:${__mysql_sandbox_port2}.
The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.
`);

//@<> impossible sync - no force (should fail)
// purge transactions from the master before removing instance, so that
// sync done during removal will never catch up
reset_instance(session2);
rs.addInstance(__sandbox2);
session2.runSql("STOP SLAVE");
session.runSql("CREATE SCHEMA testdb2");
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(), INTERVAL 1 DAY)");
session2.runSql("START SLAVE");

EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: Missing purged transactions at ${hostname_ip}:${__mysql_sandbox_port2}`);

//@<> impossible sync - force
rs.removeInstance(__sandbox2, {force:true});

EXPECT_STDOUT_CONTAINS_MULTILINE(`
WARNING: ${hostname_ip}:${__mysql_sandbox_port2} is missing [[*]] transactions that have been purged from the current PRIMARY (${hostname_ip}:${__mysql_sandbox_port1})
The instance '${hostname_ip}:${__mysql_sandbox_port2}' was removed from the replicaset.
`);

//@<> remove while PRIMARY down (should fail)
testutil.stopSandbox(__mysql_sandbox_port1, {wait:true});
shell.connect(__sandbox_uri2);

rs = dba.getReplicaSet();

EXPECT_THROWS(function () { rs.removeInstance(__sandbox2); }, `ReplicaSet.removeInstance: PRIMARY instance is unavailable`);

testutil.startSandbox(__mysql_sandbox_port1);

// XXX Rollback
//--------------------------------
// Ensure clean rollback after failures


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
