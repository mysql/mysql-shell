//@ {VER(>=8.0.27)}

// Tests various scenarios where a cluster is removed and added back
//@<> Setup

var scene = new ClusterScenario([__mysql_sandbox_port1]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

rc = cs.createReplicaCluster(__sandbox_uri4, "replica");

//@<> Remove cluster and then add it back with createReplicaCluster with same name
cs.removeCluster("replica");
shell.connect(__sandbox_uri4);
//session.runSql("set global super_read_only=0");
EXPECT_THROWS(function(){dba.getClusterSet();}, "This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata, and GR is not active)");
EXPECT_THROWS(function(){dba.getCluster("cluster")}, "This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata, and GR is not active)");

rc = cs.createReplicaCluster(__sandbox_uri4, "replica");

c = dba.getCluster();
c.status();

EXPECT_EQ("replica", c.name);
EXPECT_EQ(__endpoint4, c.status()["defaultReplicaSet"]["primary"]);

cs.removeCluster("replica");

// regression test to ensure members view only includes the latest view
session.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members");
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0]);

//@<> Create a new clusterset in removed cluster reusing name

// this should wipeout data from the clusterset from the past life
c = dba.createCluster("newcluster", {gtidSetIsComplete: true});
cs2 = c.createClusterSet("domain");
session.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members");
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances").fetchOne()[0]);
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0]);

//@<> Create a replica cluster at the new clusterset
rc2 = cs2.createReplicaCluster(__sandbox_uri5, "cluster");

session.runSql("select * from mysql_innodb_cluster_metadata.v2_cs_members");
EXPECT_EQ(2, session.runSql("select count(*) from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_EQ(2, session.runSql("select count(*) from mysql_innodb_cluster_metadata.instances").fetchOne()[0]);
EXPECT_EQ(2, session.runSql("select count(*) from mysql_innodb_cluster_metadata.v2_cs_members").fetchOne()[0]);

//@<> Create a new clusterset in removed cluster and add a replica reusing instance (should fail)
cs2.removeCluster("cluster");
shell.connect(__sandbox_uri5);
c = dba.createCluster("newcluster", {gtidSetIsComplete: true});
cs3 = c.createClusterSet("newdomain");

EXPECT_THROWS(function(){cs3.createReplicaCluster(__sandbox_uri1, "cluster2");}, "Target instance already part of an InnoDB Cluster", "MYSQLSH");

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
