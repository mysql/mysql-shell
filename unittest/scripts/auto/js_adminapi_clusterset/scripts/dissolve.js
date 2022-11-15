//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);
c = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
cs = c.createClusterSet("cs");
r = cs.createReplicaCluster(__sandbox_uri2, "replica");

//@<> dissolve on PC in a clusterset should fail
EXPECT_THROWS(function () { c.dissolve() }, "Cluster.dissolve: Cluster 'cluster' cannot be dissolved because it is part of a ClusterSet with other Clusters.");

//@<> dissolve on RC in a clusterset should fail
EXPECT_THROWS(function () { r.dissolve() }, "Cluster.dissolve: Cluster 'replica' cannot be dissolved because it is part of a ClusterSet with other Clusters.");

//@<> dissolve on last cluster in a clusterset should work
cs.removeCluster("replica");

c.dissolve();

//@<> dissolve on removed cluster in a clusterset is unnecessary because removeCluster already did it
EXPECT_THROWS(function () { r.dissolve(); }, "This function is not available through a session to a standalone instance");

//@<> dropMD on invalidated cluster should work
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

c = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
cs = c.createClusterSet("cs");
r = cs.createReplicaCluster(__sandbox_uri2, "replica");

invalidate_cluster(r, c);

s = cs.status();
EXPECT_EQ("INVALIDATED", s["clusters"]["replica"]["status"]);

// actual check
shell.connect(__sandbox_uri2);
dba.dropMetadataSchema({ force: 1 });

//@<> dissolve on invalidated cluster should work
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);

c = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
cs = c.createClusterSet("cs");

r = cs.createReplicaCluster(__sandbox_uri2, "replica");

invalidate_cluster(r, c);

s = cs.status();
EXPECT_EQ("INVALIDATED", s["clusters"]["replica"]["status"]);

// actual check
r.dissolve();

//@<> setup again for dropMD
shell.connect(__sandbox_uri2);
reset_instance(session);

shell.connect(__sandbox_uri1);
reset_instance(session);
c = dba.createCluster("cluster", { gtidSetIsComplete: 1 });
cs = c.createClusterSet("cs");
r = cs.createReplicaCluster(__sandbox_uri2, "replica");

//@<> dropMD on PC in a clusterset should fail
EXPECT_THROWS(function(){dba.dropMetadataSchema({force:1})}, "Cannot drop metadata for the Cluster because it is part of a ClusterSet with other Clusters.");

//@<> dropMD on RC in a clusterset should fail (via PC)
shell.connect(__sandbox_uri2);
EXPECT_THROWS(function(){dba.dropMetadataSchema({force:1})}, "Cannot drop metadata for the Cluster because it is part of a ClusterSet with other Clusters.");

//@<> dropMD on removed cluster in a clusterset should work
cs.removeCluster("replica");
shell.connect(__sandbox_uri2);
session.runSql("set global super_read_only=0"); // dropMD doesn't clear SRO
dba.dropMetadataSchema({ force: 1 });

//@<> dropMD on last cluster in a clusterset should work
shell.connect(__sandbox_uri1);
dba.dropMetadataSchema({ force: 1 });

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);