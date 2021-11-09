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

//BUG#33492812 Member auto-rejoin with clone fails due to plugin being uninstalled everywhere
// The AdminAPI uninstalls the CLONE plugin from all Cluster members when
// incremental recovery is used to add a new instance, even though CLONE is
// available. This causes any subsequent auto-rejoin that requires CLONE to fail
// when it could have succeeded.

//<> Ensure clone plugin is not uninstalled from all members when incremental recovery is used {VER(>=8.0.17)}
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);

c = dba.getCluster();
c.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});
c.addInstance(__sandbox_uri3, {recoveryMethod: "incremental"});

testutil.stopSandbox(__mysql_sandbox_port2);

// Create a new transaction and flush binlogs from all members
session.runSql("CREATE schema missingtrx");
session.runSql("FLUSH BINARY LOGS");
session.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("FLUSH BINARY LOGS");
session3.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

EXPECT_TRUE(clone_installed(session));
EXPECT_TRUE(clone_installed(session3));

// Restart the instance, GR will trigger clone since incremental recovery
// cannot be used
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Destroy sandboxes {VER(>= 5.7) && VER(< 8.1)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
