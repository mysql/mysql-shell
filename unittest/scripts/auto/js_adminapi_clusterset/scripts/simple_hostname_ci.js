//@ {VER(>=8.0.27)}
// Tests to check if hostnames are compared correctly

//@<> Setup sandboxes and initial cluster
hostname_caps = hostname.slice(0, hostname.length / 2).toUpperCase() + hostname.slice(hostname.length / 2).toLowerCase();

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname_caps});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname_caps});

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

//@<> clusterset.createReplicaCluster
cs = cluster.createClusterSet("cset");
EXPECT_NO_THROWS(function() { replicacluster = cs.createReplicaCluster(hostname + ":" + __mysql_sandbox_port2, "replica"); });

shell.connect(__sandbox_uri2);
EXPECT_NO_THROWS(function() { replicacluster.addInstance(__sandbox_uri3); });
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> clusterset.removeRouterMetadata
shell.connect(__sandbox_uri1);
var clusterset_id = session.runSql("SELECT clusterset_id FROM mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'RoutERHosT1', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\", \"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', NULL, NULL, ?)", [clusterset_id]);

EXPECT_EQ(cs.routingOptions()["routers"]["RoutERHosT1::system"].hasOwnProperty("target_cluster"), false);
EXPECT_NO_THROWS(function() { cs.setRoutingOption("routerhost1::system", "target_cluster", "primary"); });
EXPECT_EQ(cs.routingOptions()["routers"]["RoutERHosT1::system"].hasOwnProperty("target_cluster"), true);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);