// Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

//@ Configure instance 1 creating an Admin User.
var cnfPath1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: cnfPath1, clusterAdmin: "gr_user", clusterAdminPassword: "gr_pass"});

//@ Configure instance 2 creating an Admin User.
var cnfPath2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: cnfPath2, clusterAdmin: "gr_user", clusterAdminPassword: "gr_pass"});

//@ Configure instance 3 creating an Admin User.
var cnfPath3 = testutil.getSandboxConfPath(__mysql_sandbox_port3);
dba.configureLocalInstance(__sandbox_uri3, {mycnfPath: cnfPath3, clusterAdmin: "gr_user", clusterAdminPassword: "gr_pass"});

//@ Create a cluster with 3 members.
var admin_user_uri1 = "gr_user:gr_pass@localhost:"+__mysql_sandbox_port1;
var admin_user_uri2 = "gr_user:gr_pass@localhost:"+__mysql_sandbox_port2;
var admin_user_uri3 = "gr_user:gr_pass@localhost:"+__mysql_sandbox_port3;
shell.connect(admin_user_uri1);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

cluster.addInstance(admin_user_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.addInstance(admin_user_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Show cluster status, all online.
var status = cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])

//@ Kill instance 2
testutil.killSandbox(__mysql_sandbox_port2);
// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING)");

//@ Kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);
// Waiting for the third added instance to become unreachable
// Will remain unreachable since there's no quorum to kick it off
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

//@ Cluster.forceQuorumUsingPartitionOf success
cluster.forceQuorumUsingPartitionOf(admin_user_uri1);

//@<> Show cluster status.
var status = cluster.status();
EXPECT_EQ("OK_NO_TOLERANCE", status["defaultReplicaSet"]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])

// Clean-up deployed instances.
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2, true);
testutil.destroySandbox(__mysql_sandbox_port3, true);
