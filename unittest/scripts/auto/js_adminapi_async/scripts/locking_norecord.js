//@ {VER(>=8.0.11)}

// Tests locking of replica set operations to manage concurrent executions.

//@ INCLUDE async_utils.inc

//@<> Initialization.
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
var uuid3 = "5ef81566-9395-11e9-87e9-333333333333";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": localhost, server_uuid: uuid1, server_id: 11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": localhost, server_uuid: uuid2, server_id: 22});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": localhost, server_uuid: uuid3, server_id: 33});
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

var lock_instance = "AdminAPI_instance";
var lock_metadata = "AdminAPI_metadata";
var lock_name = "AdminAPI_lock";

//@<> Installing locking UDFs should succeed even if instance is read-only (no warning).
session1.runSql("SET GLOBAL super_read_only=1");
shell.connect(__sandbox_uri1);
dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Concurrent execution");
session1.runSql("SET GLOBAL read_only=0");
session1.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");

//@ Create ReplicaSet another operation in progress (fail)
// Create ReplicaSet when another operation holds an exclusive lock (fail).
// Simulate exclusive lock hold by another operation on instance 1.
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_1
shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Create ReplicaSet when another operation holds a shared lock (fail). [USE: Create ReplicaSet another operation in progress (fail)]
// Simulate shared lock hold by another operation on instance 1.
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_1
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Create Async replica set (succeed).
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});

//@ Configure instance when another operation in progress on the target (fail)
// Configure instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
// WL#13540: TSFR1_4
dba.configureReplicaSetInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Configure instance when another operation holds a shared lock on the target instance (fail). [USE: Configure instance when another operation in progress on the target (fail)]
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

dba.configureReplicaSetInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Add instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_3, TSFR2_3
rs.addInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Add instance when another operation in progress on the target (fail)
// Add instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
rs.addInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Add instance when another operation holds a shared lock on the target instance (fail). [USE: Add instance when another operation in progress on the target (fail)]
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

rs.addInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@<> Add instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.addInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Remove instance when another operation in progress on the target (fail)
// Remove instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
rs.removeInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Remove instance when another operation holds a shared lock on the target instance (fail). [USE: Remove instance when another operation in progress on the target (fail)]
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

rs.removeInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Remove instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_3, TSFR2_3
rs.removeInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Remove instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.removeInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Add instance 2 back again.
rs.addInstance(__sandbox_uri2);

//@<> Stop replication on instance 2 to rejoin it.
session2.runSql("STOP SLAVE");

//@ Rejoin instance when another operation in progress on the target (fail)
// Rejoin instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
// WL#13540: TSFR3_1, TSFR2_3, TSFR3_1
rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Rejoin instance when another operation holds a shared lock on the target instance (fail). [USE: Rejoin instance when another operation in progress on the target (fail)]
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Rejoin instance when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_3, TSFR2_3
rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> Rejoin instance when another operation holds an shared lock on the primary (succeed).
// Simulate shared lock hold by another operation on instance 1 (primary).
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR3_2
rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using localhost).
rs.rejoinInstance(__sandbox1);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname).
rs.rejoinInstance(__endpoint1);

//@<> (BUG#30673719) rejoinInstance on primary should not fail due to double self lock (using hostname_ip).
var address_using_ip = hostname_ip + ":" + __mysql_sandbox_port1;
rs.rejoinInstance(address_using_ip);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using localhost).
rs.addInstance(__sandbox1);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname).
rs.addInstance(__endpoint1);

//@<> (BUG#30673719) addInstance on primary should not fail due to double self lock (using hostname_ip).
rs.addInstance(address_using_ip);

//@<> Add instance 3.
rs.addInstance(__sandbox_uri3);

//@ Set primary instance when another operation in progress on the replica set (fail)
// Set primary instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port2;
// WL#13540: TSFR2_1
rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Set primary instance when another operation holds a shared lock on the target instance (fail). [USE: Set primary instance when another operation in progress on the replica set (fail)]
// Simulate shared lock hold by another operation on instance 2 (target).
testutil.getSharedLock(session2, lock_instance, lock_name);

rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Set primary instance when another operation holds an exclusive lock on the primary (fail). [USE: Set primary instance when another operation in progress on the replica set (fail)]
// Simulate exclusive lock hold by another operation on instance 1 (target).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port1;
// WL#13540: TSFR2_1
rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Set primary instance when another operation holds a shared lock on the primary (fail). [USE: Set primary instance when another operation in progress on the replica set (fail)]
// Simulate shared lock hold by another operation on instance 1 (target).
testutil.getSharedLock(session1, lock_instance, lock_name);

rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Set primary instance when another operation holds an exclusive lock on another rs instance (fail). [USE: Set primary instance when another operation in progress on the replica set (fail)]
// Simulate exclusive lock hold by another operation on instance 3 (target).
testutil.getExclusiveLock(session3, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port3;
// WL#13540: TSFR2_1
rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@ Set primary instance when another operation holds a shared lock on another rs instance (fail). [USE: Set primary instance when another operation in progress on the replica set (fail)]
// Simulate shared lock hold by another operation on instance 3 (target).
testutil.getSharedLock(session3, lock_instance, lock_name);

rs.setPrimaryInstance(__sandbox2);
// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@ Force primary instance when another operation in progress on the replica set (fail)
// Force primary instance when another operation holds an exclusive lock on the target instance (fail).
// Simulate exclusive lock hold by another operation on instance 2 (target).
testutil.getExclusiveLock(session2, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port2;
// WL#13540: TSFR2_1
rs.forcePrimaryInstance(__sandbox2);
// Release locks on instance 2.
testutil.releaseLocks(session2, lock_instance);

//@ Force primary instance when another operation holds an exclusive lock on another rs instance (fail). [USE: Force primary instance when another operation in progress on the replica set (fail)]
// Simulate exclusive lock hold by another operation on instance 3 (target).
testutil.getExclusiveLock(session3, lock_instance, lock_name);
__locked_port = __mysql_sandbox_port3;
// WL#13540: TSFR2_1
rs.forcePrimaryInstance(__sandbox2);
// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@ Force primary instance when another operation holds a shared lock on another rs instance (fail). [USE: Force primary instance when another operation in progress on the replica set (fail)]
// Simulate shared lock hold by another operation on instance 3 (target).
testutil.getSharedLock(session3, lock_instance, lock_name);

rs.forcePrimaryInstance(__sandbox2);
// Release locks on instance 3.
testutil.releaseLocks(session3, lock_instance);

//@ Remove router metadata when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_3
rs.removeRouterMetadata("routerhost");
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Upgrade Metadata another operation in progress (fail)
// Upgrade Metadata when another operation holds an exclusive lock on the primary (fail).
// Simulate exclusive lock hold by another operation on instance 1 (primary).
testutil.getExclusiveLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_2
shell.connect(__sandbox_uri1);
dba.upgradeMetadata();
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

//@ Upgrade Metadata when another operation holds a shared lock on the primary (fail). [USE: Upgrade Metadata another operation in progress (fail)]
// Simulate shared lock hold by another operation on instance 1.
testutil.getSharedLock(session1, lock_instance, lock_name);
// WL#13540: TSFR1_3
dba.upgradeMetadata();
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_instance);

// Override timeout value used for locking to always be 1 (to avoid waiting 60 sec for metadata locking error).
testutil.dbugSet("+d,dba_locking_timeout_one");

//@ Metadata lock error for remove instance (fail) {__dbug_off == 0}
// Simulate lock hold by another operation on metadata.
testutil.getExclusiveLock(session1, lock_metadata, lock_name);
// WL#13540: NFR1
rs.removeInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_metadata);

//@<> remove instance (succeed) {__dbug_off == 0}
rs.removeInstance(__sandbox_uri2);

//@ Metadata lock error for add instance (fail) {__dbug_off == 0}
// Simulate lock hold by another operation on metadata.
testutil.getExclusiveLock(session1, lock_metadata, lock_name);
// WL#13540: NFR1
rs.addInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_metadata);

//@<> add instance (succeed) {__dbug_off == 0}
rs.addInstance(__sandbox_uri2);

//@ Metadata lock error for rejoin instance (fail) {__dbug_off == 0}
// Simulate lock hold by another operation on metadata.
testutil.getExclusiveLock(session1, lock_metadata, lock_name);
// WL#13540: NFR1
session2.runSql("STOP SLAVE");
rs.rejoinInstance(__sandbox_uri2);
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_metadata);

//@<> rejoin instance (succeed) {__dbug_off == 0}
session2.runSql("STOP SLAVE");
rs.rejoinInstance(__sandbox_uri2);

//@ Metadata lock error for remove router metadata (fail) {__dbug_off == 0}
// Simulate lock hold by another operation on metadata.
testutil.getExclusiveLock(session1, lock_metadata, lock_name);
// WL#13540: NFR1
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost', '8.0.19', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
rs.removeRouterMetadata("routerhost");
// Release locks on instance 1.
testutil.releaseLocks(session1, lock_metadata);

//@<> remove router metadata (succeed) {__dbug_off == 0}
rs.removeRouterMetadata("routerhost");

// Remove previously added dbug modifier
testutil.dbugSet("-d,dba_locking_timeout_one");

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
