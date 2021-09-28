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
}, "Invalid target instance specification");
EXPECT_OUTPUT_CONTAINS("ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them (localhost:"+__mysql_sandbox_port2+")");

//@<> credentials that match seed instance, but does not exist at instance
EXPECT_THROWS(function () {
    cluster.addInstance({ user: 'cluster_admin', password: 'foo', host: 'localhost', port: __mysql_sandbox_port2 }, { recoveryMethod: "incremental" });
}, "Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin'@'localhost' (using password: YES)");
EXPECT_OUTPUT_CONTAINS("ERROR: MySQL Error 1045 (28000): Access denied for user 'cluster_admin'@'localhost' (using password: YES): Credentials for user cluster_admin at localhost:"+__mysql_sandbox_port2+" must be the same as in the rest of the cluster.");

//@<> inherit credentials from seed instance, but does not exist at instance
EXPECT_THROWS(function () {
    cluster.addInstance('localhost:'+__mysql_sandbox_port2, { recoveryMethod: "incremental" });
}, "Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin'@'localhost' (using password: YES)");
EXPECT_OUTPUT_CONTAINS("ERROR: MySQL Error 1045 (28000): Access denied for user 'cluster_admin'@'localhost' (using password: YES): Credentials for user cluster_admin at localhost:"+__mysql_sandbox_port2+" must be the same as in the rest of the cluster.");

//@<> Add instance using account that exists in the seed but the password does not match
dba.configureInstance(__sandbox_uri2, { clusterAdmin: "cluster_admin", clusterAdminPassword: "bar" });

EXPECT_THROWS(function () {
    cluster.addInstance('localhost:'+__mysql_sandbox_port2, { recoveryMethod: "incremental" });
}, "Could not open connection to 'localhost:"+__mysql_sandbox_port2+"': Access denied for user 'cluster_admin'@'localhost' (using password: YES)");
EXPECT_OUTPUT_CONTAINS("ERROR: MySQL Error 1045 (28000): Access denied for user 'cluster_admin'@'localhost' (using password: YES): Credentials for user cluster_admin at localhost:"+__mysql_sandbox_port2+" must be the same as in the rest of the cluster.");

//@<> Cleanup
session.close()
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
