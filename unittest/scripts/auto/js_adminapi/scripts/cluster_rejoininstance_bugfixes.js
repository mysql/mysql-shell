//@<> Initialization
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

// Bug #26649039 - SHELL FAILS TO IDENTIFY REJOINING MEMBER WITH NEW UUID
// To be tested instance 2
//@<> Bug #26649039 - Current cluster state
var uuid = get_sandbox_uuid(__mysql_sandbox_port2);
var topology = cluster.status({extended:1})["defaultReplicaSet"]["topology"];

EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["memberId"], `${uuid}`);

//@<> Bug #26649039 - Stops the 2nd instance
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

var topology = cluster.status({extended:1})["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["status"], "(MISSING)");
EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["memberId"], "");

//@<> Bug #26649039 - Update the uuid of the instance, rejoin it and get the status
// Should be part of the cluster again but notes should be printed that the UUID was updated.
var new_uuid = uuid.substring(0, uuid.length - 4) + "1111";
testutil.changeSandboxConf(__mysql_sandbox_port2, 'server-uuid', new_uuid);
testutil.startSandbox(__mysql_sandbox_port2)
cluster.rejoinInstance(__sandbox_uri2)
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
var topology = cluster.status({extended:1})["defaultReplicaSet"]["topology"];
EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(topology[`127.0.0.1:${__mysql_sandbox_port2}`]["memberId"], `${new_uuid}`);
EXPECT_OUTPUT_CONTAINS("Updating instance metadata...")
EXPECT_OUTPUT_CONTAINS(`The instance metadata for '127.0.0.1:${__mysql_sandbox_port2}' was successfully updated.`)

//@<> BUG#29255212 rejoinInstance fails with nice error if group_replication_gtid_assignment_block_size of instance different from the value off the cluster
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
// Removes the server_id from the instance table for the instance to be rejoined
// on the test below, the rejoin operation puts it back
shell.connect(__sandbox_uri1);
var initial_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '127.0.0.1:${__mysql_sandbox_port3}'`).fetchOneObject();
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances set attributes=JSON_REMOVE(attributes, '$.server_id') WHERE address= '127.0.0.1:${__mysql_sandbox_port3}'`);

// Bug #31632554   CLUSTER.REJOININSTANCE() USES WRONG CREDENTIALS
shell.connect(__sandbox_uri3);

session.runSql("stop group_replication");

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"]["127.0.0.1:"+__mysql_sandbox_port3]["status"], "(MISSING)");

cluster.rejoinInstance("localhost:"+__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

EXPECT_EQ(cluster.status()["defaultReplicaSet"]["topology"]["127.0.0.1:"+__mysql_sandbox_port3]["status"], "ONLINE");

// Gets the updated server_id
var final_atts = session.runSql(`SELECT attributes->'$.server_id' AS server_id FROM mysql_innodb_cluster_metadata.instances WHERE address = '127.0.0.1:${__mysql_sandbox_port3}'`).fetchOneObject();
EXPECT_EQ(initial_atts.server_id, final_atts.server_id);


//@<> BUG#29255212 Cleanup
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
