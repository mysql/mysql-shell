///BUG#30728744 MISLEADING ERROR MESSAGE WHEN DIFFERENT CREDENTIALS ARE USED IN CLUSTER ADMIN OP
//@<> Deploy
testutil.deploySandbox(__mysql_sandbox_port1, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, 'root');
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@<> Set password
dba.configureInstance(__sandbox_uri1, { clusterAdmin: "cluster_admin", clusterAdminPassword: "foo" });
dba.configureInstance(__sandbox_uri2, { clusterAdmin: "cluster_admin2", clusterAdminPassword: "bar" });

//@<> Create cluster
shell.connect({ user: 'cluster_admin', password: 'foo', host: 'localhost', port: __mysql_sandbox_port1 });
var cluster = dba.createCluster('Europe');

//@<> Check instance state with credentials which mismatch seed instance
EXPECT_THROWS(function () {
    cluster.checkInstanceState({ user: 'cluster_admin2', password: 'bar', host: 'localhost', port: __mysql_sandbox_port2 });
}, "Cluster.checkInstanceState: Invalid target instance specification");
EXPECT_OUTPUT_CONTAINS("ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them (localhost:"+__mysql_sandbox_port2+")");

//@<> credentials that match seed instance, but does not exist at instance
EXPECT_THROWS(function () {
    cluster.addInstance({ user: 'cluster_admin', password: 'foo', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "incremental" });
}, "Cluster.addInstance: Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin'@'localhost' (using password: YES)");
EXPECT_OUTPUT_CONTAINS("ERROR: MySQL Error 1045 (28000): Access denied for user 'cluster_admin'@'localhost' (using password: YES): Credentials for user cluster_admin at localhost:"+__mysql_sandbox_port2+" must be the same as in the rest of the cluster.");

//@<> inherit credentials from seed instance, but does not exist at instance
EXPECT_THROWS(function () {
    cluster.addInstance('localhost:'+__mysql_sandbox_port2, { recoveryMethod: "incremental" });
}, "Cluster.addInstance: Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin'@'localhost' (using password: YES)");
EXPECT_OUTPUT_CONTAINS("ERROR: MySQL Error 1045 (28000): Access denied for user 'cluster_admin'@'localhost' (using password: YES): Credentials for user cluster_admin at localhost:"+__mysql_sandbox_port2+" must be the same as in the rest of the cluster.");

// Added instance has custom credentials

//@<> Add instance with custom credentials (interactive password, wrong)
testutil.expectPassword("Please provide the password for 'cluster_admin2@localhost:"+__mysql_sandbox_port2+"': ", "bla");

EXPECT_THROWS(function () {
    cluster.addInstance({ user: 'cluster_admin2', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "incremental", interactive:1 });
}, "Cluster.addInstance: Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin2'@'localhost' (using password: YES)");

EXPECT_OUTPUT_CONTAINS("ERROR: Unable to connect to the target instance 'localhost:"+__mysql_sandbox_port2+"'. Please verify the connection settings, make sure the instance is available and try again.");

//@<> Add instance with custom credentials, incremental (interactive password, correct)
testutil.expectPassword("Please provide the password for 'cluster_admin2@localhost:"+__mysql_sandbox_port2+"': ", "bar");
cluster.addInstance({ user: 'cluster_admin2', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "incremental", interactive:1 });

cluster.removeInstance(__sandbox_uri2);

//@<> Add instance with custom credentials, good password, clone {VER(>=8.0)}
cluster.addInstance({ user: 'cluster_admin2', password: 'bar', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "clone" });

//@<> Cleanup
session.close()
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
