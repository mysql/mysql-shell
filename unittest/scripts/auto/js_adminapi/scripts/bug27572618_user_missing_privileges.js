// Deploy Instances
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@ Configure instance 1 creating an Admin User.
var cnfPath1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: cnfPath1, clusterAdmin: "gr_user", clusterAdminPassword: "gr_pass"});

//@ Configure instance 2 creating an Admin User.
var cnfPath2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
dba.configureLocalInstance(__sandbox_uri2, {mycnfPath: cnfPath2, clusterAdmin: "gr_user", clusterAdminPassword: "gr_pass"});

//@ Create cluster.
shell.connect("gr_user:gr_pass@localhost:"+__mysql_sandbox_port1);
var cluster = dba.createCluster("test_cluster");

cluster.addInstance("gr_user:gr_pass@localhost:"+__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ Remove one instance using Admin user (cannot fail with lack of privileges).
cluster.removeInstance("gr_user:gr_pass@localhost:"+__mysql_sandbox_port2);

// Clean-up deployed instances.
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
