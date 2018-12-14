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
testutil.deploySandbox(__mysql_sandbox_port2, "root");

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
for (i = 0; i < 5; i++) {
  session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

//@<OUT> checkInstanceState: state: ok, reason: recoverable
cluster.checkInstanceState(__sandbox_uri2);

// Flush and get the current binary log
session.runSql("FLUSH BINARY LOGS");
var res = session.runSql("SHOW MASTER STATUS");
var row = res.fetchOne();
var file = row[0];

// Flush and purge the binary log
session.runSql("PURGE BINARY LOGS TO '" + file + "'");
session.close();

//@<OUT> checkInstanceState: state: error, reason: lost_transactions
cluster.checkInstanceState(__sandbox_uri2);

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port2);
cluster.disconnect();
scene.destroy();
