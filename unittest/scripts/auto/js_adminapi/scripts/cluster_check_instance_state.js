function purgeLogs(sess) {
  sess.runSql("FLUSH BINARY LOGS");
  // Flush and purge the binary log
  sess.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), interval 1 day)");
}

//@ Create single-primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var cluster = scene.cluster

// Exceptions in checkInstanceState():
//
//  ArgumentError in the following scenarios:
//     - If the 'instance' parameter is empty.
//     - If the 'instance' parameter is invalid.
//     - If the 'instance' definition is a connection dictionary but empty.
//
//   RuntimeError in the following scenarios:
//     - If the 'instance' is unreachable/offline.
//     - If the 'instance' is a cluster member.
//     - If the 'instance' belongs GR group that is not managed as an InnoDB cluster.
//     - If the 'instance' is a standalone instance but is part of a different InnoDB Cluster.
//     - If the 'instance' has an unknown state.

//@ checkInstanceState: ArgumentErrors
cluster.checkInstanceState();
cluster.checkInstanceState("");
cluster.checkInstanceState(12345);
cluster.checkInstanceState({});

//@ checkInstanceState: unreachable instance
cluster.checkInstanceState("localhost:11111");

// Regression test for BUG#24942875
//@<ERR> checkInstanceState: cluster member
cluster.checkInstanceState(__sandbox_uri1);

//@ Create a single-primary cluster on instance 2
var scene2 = new ClusterScenario([__mysql_sandbox_port2]);

//@ Drop metadatata schema from instance 2
dba.dropMetadataSchema({force: true});

//@<ERR> checkInstanceState: instance belongs to unmanaged GR group
cluster.checkInstanceState(__sandbox_uri2);

//@ Re-create cluster
scene2.destroy();
var scene2 = new ClusterScenario([__mysql_sandbox_port2]);

//@ Stop GR on instance 2
var session = scene2.session
session.runSql("stop group_replication;")
session.close();

//@<ERR> checkInstanceState: is a standalone instance but is part of a different InnoDB Cluster
cluster.checkInstanceState(__sandbox_uri2);

//@ Drop metadatata schema from instance 2 to get diverged GTID
shell.connect(__sandbox_uri2);
dba.dropMetadataSchema({force: true, clearReadOnly: true});
session.close();

//@<OUT> checkInstanceState: state: error, reason: diverged
cluster.checkInstanceState(__sandbox_uri2);

//@ Destroy cluster2
scene2.destroy();

//@ Deploy instance 2
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<OUT> checkInstanceState: state: ok, reason: new
cluster.checkInstanceState(__sandbox_uri2);

// Create a test dataset in instance 1
shell.connect(__sandbox_uri1);
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");

//@ Add instance 2 to the cluster
cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Remove instance 2 from the cluster manually
shell.connect(__sandbox_uri2);
session.runSql("stop group_replication;")

testutil.waitMemberState(__mysql_sandbox_port2, "OFFLINE");

//@ Drop metadatata schema from instance 2 with binlog disabled
session.runSql("set sql_log_bin=0");
session.runSql("set global super_read_only = 0");
session.runSql("drop schema mysql_innodb_cluster_metadata");
session.runSql("set sql_log_bin=1");
session.close();

//@ Remove instance 2 from cluster metadata
shell.connect(__sandbox_uri1);
cluster.removeInstance(__sandbox_uri2, {force: true});

// Insert data on instance 1
for (i = 0; i < 10; i++) {
  session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

//@<OUT> checkInstanceState: state: ok, reason: recoverable
cluster.checkInstanceState(__sandbox_uri2);

// Flush the binary logs
// binlogs will sometimes be blocked from purging so keep repeating until it looks done
wait(80, 1, function(){ purgeLogs(session); return session.runSql("show binary logs").fetchAll().length <= 1;});
session.runSql("show binary logs");
session.close();
//@<OUT> checkInstanceState: state: error, reason: all_purged
cluster.checkInstanceState(__sandbox_uri2);

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port2);
cluster.disconnect();
scene.destroy();

//@<> WL#13208: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

//@ WL#13208: TS_FR5_1 create cluster with clone enabled. (disableClone: false) {VER(>=8.0.17)}
var c = dba.createCluster('test', {disableClone: false});

//@<> WL#13208: TS_FR5_1 clean (purge) binary logs on cluster {VER(>=8.0.17)}
// Flush the binary logs
purgeLogs(session);

//@<> WL#13208: TS_FR5_1 connect to instance 2 to clean all transactions {VER(>=8.0.17)}
session.close();
var s2 = shell.connect(__sandbox_uri2);
s2.runSql("RESET MASTER");

//@ WL#13208: TS_FR5_1 No errors for instance 2 using checkInstanceState() {VER(>=8.0.17)}
c.checkInstanceState(__sandbox_uri2);

//@<> WL#13208: TS_FR5_2 adding some transactions to instance 2 {VER(>=8.0.17)}
s2.runSql("CREATE DATABASE test_wl13208");
s2.runSql("CREATE TABLE test_wl13208.t1 (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, name VARCHAR(100))");
s2.runSql("INSERT INTO test_wl13208.t1 (name) VALUES ('row1')");
s2.runSql("INSERT INTO test_wl13208.t1 (name) VALUES ('row2')");

//@ WL#13208: TS_FR5_2 checkInstanceState() indicate instance 2 can be added with clone enabled {VER(>=8.0.17)}
c.checkInstanceState(__sandbox_uri2);

//@<> WL#13208: Re-create the sandboxes {VER(>=8.0.17)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

//@<> WL#13208: Re-create the cluster {VER(>=8.0.17)}
shell.connect(__sandbox_uri1);
var c = dba.createCluster('test', {disableClone: false});

//@<> WL#13208: Add some transactions to the cluster {VER(>=8.0.17)}
session.runSql("CREATE DATABASE test_wl13208");
session.runSql("CREATE TABLE test_wl13208.t1 (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, name VARCHAR(100))");
session.runSql("INSERT INTO test_wl13208.t1 (name) VALUES ('row1')");
session.runSql("INSERT INTO test_wl13208.t1 (name) VALUES ('row2')");

//@<> WL#13208: Add instance 2 to the cluster {VER(>=8.0.17)}
// NOTE: Use incremental to ensure the binary logs are available in both instances
c.addInstance(__sandbox_uri2, {recoveryMethod: "incremental"});

//@ WL#13208: TS_FR5_3 checkInstanceState() indicate instance 3 can be added {VER(>=8.0.17)}
c.checkInstanceState(__sandbox_uri3);

//@<> WL#13208: TS_FR5_5 clean (purge) binary logs on instance 1 only {VER(>=8.0.17)}
// Flush the binary logs
session.close();
shell.connect(__sandbox_uri1);
purgeLogs(session);

//@ WL#13208: TS_FR5_3 checkInstanceState() indicate instance 3 can be added since instance 2 still has the binlogs {VER(>=8.0.17)}
c.checkInstanceState(__sandbox_uri3);

//@<> WL#13208: TS_FR5_5 clean (purge) binary logs on instance 2 {VER(>=8.0.17)}
// Flush the binary logs
session.close();
shell.connect(__sandbox_uri2);
purgeLogs(session);

//@ WL#13208: TS_FR5_3 checkInstanceState() indicate instance 3 can be added only if clone is used because binlogs were purged from all members {VER(>=8.0.17)}
c.checkInstanceState(__sandbox_uri3);

//@<> WL#13208: clean-up
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
