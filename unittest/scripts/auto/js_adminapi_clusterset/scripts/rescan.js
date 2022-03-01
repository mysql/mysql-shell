//@ {VER(>=8.0.27)}

//@<> INCLUDE gr_utils.inc

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {gtidSetIsComplete: true});
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

cluster = scene.cluster;
scene.session.close();

cset = cluster.createClusterSet("cset");

shell.connect(__sandbox_uri1);

rcluster = cset.createReplicaCluster(__sandbox_uri3, "rcluster", {recoveryMethod:"clone"});

//@<> BUG #33235502 Make sure that account errors aren't shown and/or fixed in replica clusters

shell.connect(__sandbox_uri3);

rcluster.addInstance(__sandbox_uri4, {recoveryMethod: "clone", waitRecovery:0});
testutil.waitMemberState(__mysql_sandbox_port4, "ONLINE");

cset.setPrimaryCluster("rcluster");

session4 = mysql.getSession(__sandbox_uri4);
var instance4_id = session4.runSql("SELECT @@server_id").fetchOne()[0];
session4.close();

// cluster should not show any erros, but replica should
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"][0], `WARNING: Incorrect recovery account (mysql_innodb_cluster_${instance4_id}) being used. Use Cluster.rescan() to repair.`);

// this must not change anything
WIPE_STDOUT();
cluster.rescan();
EXPECT_OUTPUT_NOT_CONTAINS("Dropping unused recovery account: ");
EXPECT_OUTPUT_NOT_CONTAINS("Fixing incorrect recovery account 'mysql_innodb_cluster_");

// confirm that everything is the same
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_TRUE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"][0], `WARNING: Incorrect recovery account (mysql_innodb_cluster_${instance4_id}) being used. Use Cluster.rescan() to repair.`);

// fix the problem
WIPE_STDOUT();
rcluster.rescan();
EXPECT_OUTPUT_CONTAINS(`Fixing incorrect recovery account 'mysql_innodb_cluster_${instance4_id}' in instance '${hostname}:${__mysql_sandbox_port3}'`)

// confirm that everything is correct
status = cluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);

status = rcluster.status();
EXPECT_FALSE("instanceErrors" in status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);

//@<> Cleanup
session.close()
scene.destroy();

testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
