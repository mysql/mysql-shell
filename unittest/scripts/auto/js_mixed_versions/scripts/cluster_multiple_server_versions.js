//@ {DEF(MYSQLD57_PATH)}

//@<> Deploy sandbox using base server
EXPECT_NO_THROWS(function () {
    testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: hostname });
    testutil.snapshotSandboxConf(__mysql_sandbox_port1);
}, "Deploying 8.0 Sandbox");

//@<> Deploy sandbox using 5.7 server
EXPECT_NO_THROWS(function () {
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: hostname }, {mysqldPath: MYSQLD57_PATH});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
var mycnf_path2 = testutil.getSandboxConfPath(__mysql_sandbox_port2);
}, "Deploying 5.7 Sandbox");

//@<> Create the cluster on the 5.7 instance
var cluster;
EXPECT_NO_THROWS(function () {
    shell.connect(__sandbox_uri2);
    cluster = dba.createCluster("testCluster")
}, "Creating the 5.7 Cluster");

//@<> Add an 8.0 instance to the cluster
EXPECT_NO_THROWS(function () {
    cluster.addInstance(__sandbox_uri1, {recoveryMethod:'incremental'});
}, "Adding an 8.0 Instance");

//@ get cluster status
cluster.describe();

//@<> Get version for the 5.7 Server
session.runSql("select @@version");
EXPECT_OUTPUT_CONTAINS("5.7");


//@<> Get Version for the 8.0 Server
shell.connect(__sandbox_uri1);
session.runSql("select @@version");
EXPECT_OUTPUT_CONTAINS("8.0");

//@<> Finalize
EXPECT_NO_THROWS(function () {
    cluster.disconnect()
    session.close()
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
}, "Cleaning Up Test Case");
