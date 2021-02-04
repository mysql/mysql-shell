//@<> Deploy 2 sandboxes, used for the following tests {VER(>= 5.7) && VER(< 8.1)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

//@<> BUG#29255212 addInstance fails with nice error if group_replication_gtid_assignment_block_size of instance different from the value off the cluster
testutil.changeSandboxConf(__mysql_sandbox_port1, "loose_group_replication_gtid_assignment_block_size", "2000");
testutil.changeSandboxConf(__mysql_sandbox_port2, "loose_group_replication_gtid_assignment_block_size", "1000");
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

// Create a cluster
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

// Verify that addInstance fails with the expected error
EXPECT_THROWS_TYPE(function(){cluster.addInstance(__sandbox_uri2);}, "The 'group_replication_gtid_assignment_block_size' value '1000' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster '2000'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'group_replication_gtid_assignment_block_size' value.`);
session.close();

// Remove the configuration from the sandboxes
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "loose_group_replication_gtid_assignment_block_size");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "loose_group_replication_gtid_assignment_block_size");

//@<> BUG#29255212 addInstance fails with nice error if default_table_encryption of instance different from the value off the cluster {VER(>= 8.0.16)}
testutil.changeSandboxConf(__mysql_sandbox_port1, "default_table_encryption", "OFF");
testutil.changeSandboxConf(__mysql_sandbox_port2, "default_table_encryption", "1");
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

// Get the cluster handle, using reboot since the instances were restarted
shell.connect(__sandbox_uri1);
var cluster = dba.rebootClusterFromCompleteOutage();

// Verify that addInstance fails with the expected error
EXPECT_THROWS_TYPE(function(){cluster.addInstance(__sandbox_uri2);}, "The 'default_table_encryption' value 'ON' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster 'OFF'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'default_table_encryption' value.`);
session.close();

// Remove the configuration from the sandboxes
testutil.removeFromSandboxConf(__mysql_sandbox_port1, "default_table_encryption");
testutil.removeFromSandboxConf(__mysql_sandbox_port2, "default_table_encryption");

//@<> Destroy sandboxes {VER(>= 5.7) && VER(< 8.1)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
