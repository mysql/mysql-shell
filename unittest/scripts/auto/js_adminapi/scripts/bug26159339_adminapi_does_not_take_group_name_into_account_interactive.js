testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Create a 3 member cluster
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("clus");

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Cluster state
cluster.status();

//@ Remove the persisted group_replication_start_on_boot and group_replication_group_name {VER(>=8.0.4)}
var s3 = mysql.getSession(__sandbox_uri3);
s3.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");
s3.runSql("RESET PERSIST IF EXISTS group_replication_group_name");
s3.close();

//@ Take third sandbox down, change group_name, start it back
testutil.stopSandbox(__mysql_sandbox_port3, "root");
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

testutil.removeFromSandboxConf(__mysql_sandbox_port3, "group_replication_start_on_boot");
testutil.changeSandboxConf(__mysql_sandbox_port3, "group_replication_group_name", "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");
testutil.startSandbox(__mysql_sandbox_port3);

//@<OUT> Should have 2 members ONLINE and one missing
cluster.status();

testutil.expectPrompt("Would you like to remove it from the cluster metadata? [Y|n]: ", "n");
//@ Rescan
cluster.rescan();

//@# Try to rejoin it (error)
cluster.rejoinInstance(__sandbox_uri3);

//@ getCluster() where the member we're connected to has a mismatched group_name vs metadata
session.close();
shell.connect(__sandbox_uri1);
// Change the group_name in the metadata
session.runSql("update mysql_innodb_cluster_metadata.replicasets set attributes = JSON_SET(attributes, '$.group_replication_group_name', 'fd4b70e8-5cb1-11e7-a68b-b86b230042b0') where replicaset_id = 1");

//@# check error
dba.getCluster("clus");

//@ Cleanup
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
