// Plain positive test case for everything related to a cluster
// Use as a template for positive test case in other scenarios

//@<> Setup

testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.deployRawSandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@ configureInstance {VER(>=8.0.0)}
shell.connect(__sandbox_uri1);
dba.configureInstance();

dba.configureInstance(__sandbox_uri2);

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@ configureLocalInstance {VER(<8.0.0)}
shell.connect(__sandbox_uri1);

mycnf_path1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);

dba.configureLocalInstance(null, {mycnfPath: mycnf_path1});

dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: mycnf_path2});

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@ createCluster
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("clus", {gtidSetIsComplete:1});

//@ addInstance
cluster.addInstance(__sandbox_uri2);
cluster.addInstance(__sandbox_uri3);

//@ rejoinInstance
session1.runSql("stop group_replication");

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.rejoinInstance(__sandbox_uri1);

//@ status
cluster.status();

//@ describe
cluster.describe();

//@ listRouters
cluster.listRouters();

//@ removeInstance
cluster.removeInstance(__sandbox_uri3);

//@ setPrimaryInstance {VER(>=8.0.0)}
cluster.setPrimaryInstance(__sandbox_uri1);
cluster.setPrimaryInstance(__sandbox_uri2);

//@ options
cluster.options();

//@ setOption {VER(>=8.0.0)}
cluster.setOption("expelTimeout", 5);

//@ setInstanceOption
cluster.setInstanceOption(__sandbox_uri1, "memberWeight", 42);

cluster.options();

//@ forceQuorum
// TODO these 2 lines shouldn't be needed, but without them the addInstance
// fails with Cluster.addInstance: This function is not available through a session to a read only instance (RuntimeError)
// everything should be fixed to always work through the PRIMARY
shell.connect(__sandbox_uri1); 
cluster = dba.getCluster();
// <--

cluster.addInstance(__sandbox_uri3);
cluster.status();
session1.runSql("stop group_replication");

testutil.killSandbox(__mysql_sandbox_port3);
wait_member_state_from(session2, __mysql_sandbox_port3, "UNREACHABLE");
testutil.startSandbox(__mysql_sandbox_port3);
session3 = mysql.getSession(__sandbox_uri3);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();
cluster.forceQuorumUsingPartitionOf(__sandbox_uri2);
cluster.rejoinInstance(__sandbox_uri1);

//@<> forceQuorum (rejoin restarted sandbox without persisted start_on_boot) {VER(<8.0.0)}
cluster.rejoinInstance(__sandbox_uri3);

//@ rebootCluster
session3.runSql("stop group_replication");
session2.runSql("stop group_replication");
session1.runSql("stop group_replication");

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage();

cluster.rejoinInstance(__sandbox_uri2);
cluster.rejoinInstance(__sandbox_uri3);

cluster.status();

//@ rescan
// remove an instance from the MD
session.runSql("delete from mysql_innodb_cluster_metadata.instances where instance_name=?", ["127.0.0.1:"+__mysql_sandbox_port2]);

cluster.rescan();

//@ dissolve
cluster.dissolve();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
