
//@<> BUG#29255212 addInstance fails with nice error if group_replication_gtid_assignment_block_size of instance different from the value off the cluster
testutil.deploySandbox(__mysql_sandbox_port1, "root", {loose_group_replication_gtid_assignment_block_size: "2000"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {loose_group_replication_gtid_assignment_block_size: "1000"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
EXPECT_THROWS_TYPE(function(){cluster.addInstance(__sandbox_uri2);}, "Cluster.addInstance: The 'group_replication_gtid_assignment_block_size' value '1000' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster '2000'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'group_replication_gtid_assignment_block_size' value.`);
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> BUG#29255212 addInstance fails with nice error if default_table_encryption of instance different from the value off the cluster {VER(>= 8.0.16)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {default_table_encryption: "OFF"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {default_table_encryption: "1"});
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
EXPECT_THROWS_TYPE(function(){cluster.addInstance(__sandbox_uri2);}, "Cluster.addInstance: The 'default_table_encryption' value 'ON' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster 'OFF'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'default_table_encryption' value.`);
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
