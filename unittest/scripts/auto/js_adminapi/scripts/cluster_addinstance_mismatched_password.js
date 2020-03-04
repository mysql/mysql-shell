///BUG#30728744 MISLEADING ERROR MESSAGE WHEN DIFFERENT CREDENTIALS ARE USED IN CLUSTER ADMIN OP
//@<> Deploy
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@<> Set password
dba.configureInstance(__sandbox_uri1, { clusterAdmin: "cluster_admin", clusterAdminPassword: "foo" });
testutil.restartSandbox(__mysql_sandbox_port1)
dba.configureInstance(__sandbox_uri2, { clusterAdmin: "cluster_admin2", clusterAdminPassword: "bar" });
testutil.restartSandbox(__mysql_sandbox_port2)

//@<> Create cluster
shell.connect({ user: 'cluster_admin', password: 'foo', host: 'localhost', port: __mysql_sandbox_port1 });
var cluster = dba.createCluster('Europe');

//@<> Check instance state with credentials which mismatch seed instance
EXPECT_THROWS(function () {
    cluster.checkInstanceState({ user: 'cluster_admin2', password: 'bar', host: 'localhost', port: __mysql_sandbox_port2 });
}, "Cluster.checkInstanceState: Invalid administrative account credentials.");
EXPECT_OUTPUT_CONTAINS("ERROR: The administrative account credentials provided do not match the cluster's administrative account. The cluster administrative account user name and password must be the same on all instances that belong to an InnoDB cluster.");

//@<> Add instance with credentials which mismatch seed instance
EXPECT_THROWS(function () {
    cluster.addInstance({ user: 'cluster_admin2', password: 'bar', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "clone" });
}, "Cluster.addInstance: Invalid administrative account credentials.");
EXPECT_OUTPUT_CONTAINS("ERROR: The administrative account credentials provided do not match the cluster's administrative account. The cluster administrative account user name and password must be the same on all instances that belong to an InnoDB cluster.");

//@<> Cleanup
session.close()
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
