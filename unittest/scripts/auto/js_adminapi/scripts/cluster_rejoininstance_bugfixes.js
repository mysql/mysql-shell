//@<> BUG#29255212 rejoinInstance fails with nice error if group_replication_gtid_assignment_block_size of instance different from the value off the cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {loose_group_replication_gtid_assignment_block_size: "2000"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {loose_group_replication_gtid_assignment_block_size: "2000"});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {loose_group_replication_gtid_assignment_block_size: "2000"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");
// Change replication_gtid_assignment_block_size before trying to rejoin instance to cluster
testutil.changeSandboxConf(__mysql_sandbox_port2, 'loose_group_replication_gtid_assignment_block_size', '1000');
testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("stop group_replication");

EXPECT_THROWS_TYPE(function(){cluster.rejoinInstance(__sandbox_uri2);}, "Cluster.rejoinInstance: The 'group_replication_gtid_assignment_block_size' value '1000' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster '2000'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'group_replication_gtid_assignment_block_size' value.`);

//@<> BUG#29255212 Test that rejoinInstance fails with nice error if default_table_encryption of instance different from the value off the cluster  {VER(>= 8.0.16)}
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, 'loose_group_replication_gtid_assignment_block_size', '2000');
testutil.changeSandboxConf(__mysql_sandbox_port2, 'default_table_encryption', 'ON');
testutil.startSandbox(__mysql_sandbox_port2);

EXPECT_THROWS_TYPE(function(){cluster.rejoinInstance(__sandbox_uri2);}, "Cluster.rejoinInstance: The 'default_table_encryption' value 'ON' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster 'OFF'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'default_table_encryption' value.`);

//@<> rejoinInstance without credentials
// Bug #31632554   CLUSTER.REJOININSTANCE() USES WRONG CREDENTIALS
shell.connect(__sandbox_uri3);
session.runSql("stop group_replication");

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"]["127.0.0.1:"+__mysql_sandbox_port3]["status"], "(MISSING)");

cluster.rejoinInstance("localhost:"+__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"]["127.0.0.1:"+__mysql_sandbox_port3]["status"], "ONLINE");

//@<> BUG#29255212 Cleanup
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
