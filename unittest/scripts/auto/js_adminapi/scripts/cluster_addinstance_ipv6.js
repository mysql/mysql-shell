//@ {VER(>=8.0.17)}

//@<> Setup for addInstance scenario with errant gtids and IPv6 instances
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "127.0.0.1"});

// create admin accounts
dba.configureLocalInstance(__sandbox_uri1, {clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});
dba.configureLocalInstance(__sandbox_uri2, {clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});

// Create transaction on instance1
shell.connect(__sandbox_uri1);
session.runSql("create schema test123");
session.close();

shell.connect(__sandbox_admin_uri1);
var cluster = dba.createCluster("C");

// create some errant transaction on instance 2 so that clone has to be used
shell.connect(__sandbox_uri2);
session.runSql("create schema test123");
session.close();
shell.connect(__sandbox_admin_uri1);

//@<> Add instance2 should fail since clone was selected and all cluster members are IPv6.
EXPECT_THROWS(function () { cluster.addInstance(__sandbox_admin_uri2, {recoveryMethod: "clone"}) }, "The Cluster has no compatible clone donors.");

//@<> Add instance 3, which uses IPv4 to the cluster
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: "127.0.0.1"});
dba.configureLocalInstance(__sandbox_uri3, {clusterAdmin:CLUSTER_ADMIN, clusterAdminPassword:CLUSTER_ADMIN_PWD});
cluster.addInstance(__sandbox_admin_uri3, {recoveryMethod: "incremental"});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Add instance2 should succeed since clone was selected and there is 1 online IPv4 instance.
try {
    cluster.addInstance(__sandbox_admin_uri2, {recoveryMethod: "clone"});
    EXPECT_STDOUT_CONTAINS("Adding instance to the cluster...");
} catch (err) {
    // if it failed it is because of GR BUG#31002759 where the in a group with ipv4 instances, sometimes clone selects the IPv6 one
    EXPECT_STDOUT_CONTAINS("Adding instance to the cluster...");
    EXPECT_EQ(err.message, "Cluster.addInstance: Distributed recovery has failed");
}
//@<> Cleanup sandboxes
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
