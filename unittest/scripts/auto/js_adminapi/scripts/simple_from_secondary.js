// Ensure that all cluster operations work even if the shell is connected to a SECONDARY
// Bug#31757737

// The basic test pattern is:
// positive case:
// getCluster() while connected to a secondary perform operation
//   - shell should automatically find the primary and connect to it
//
// negative case:
// make primary unconnectable (change root password) while connected to a
// secondary perform operation
//   - shell should fallback to a secondary member

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

//@<> createCluster
shell.connect(__sandbox_uri1);

cluster = dba.createCluster("mycluster");

cluster.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

// POSITIVE CASES

//@<> checkInstanceState
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.checkInstanceState(__sandbox_uri3);

//@<> addInstance using clone recovery {VER(>=8.0.17)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.addInstance(__sandbox_uri3, {recoveryMethod:'clone'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

session3 = mysql.getSession(__sandbox_uri3);
cluster.removeInstance(__sandbox_uri3);

//@<> addInstance using incremental recovery
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

//@<> removeInstance
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.removeInstance(__sandbox_uri3);

//@<> setPrimaryInstance {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.setPrimaryInstance(__sandbox_uri2);

cluster.setPrimaryInstance(__sandbox_uri1);

//@<> rejoinInstance
cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});
session3.runSql("STOP group_replication");

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.rejoinInstance(__sandbox_uri3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

cluster.status();

//@<> setOption
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.setOption("clusterName", "clooster");

custom_weigth=50;

//@<> setInstanceOption {VER(>=8.0.0)}
custom_weigth=20;

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth);

//@<> options
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.options();

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.switchToMultiPrimaryMode();

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.switchToSinglePrimaryMode();

cluster.setPrimaryInstance(__sandbox_uri1);

//@<> rescan
// delete sb3 from the metadata so that rescan picks it up
session1.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.rescan({addInstances:"auto"});

//@<> listRouters
cluster_id = session1.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.listRouters();

//@<> removeRouterMetadata
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.removeRouterMetadata("routerhost1::system");

cluster.listRouters();

//@<> dissolve
shell.connect(__sandbox_uri2);
cluster = dba.getCluster();

cluster.dissolve();

EXPECT_EQ("OFFLINE", session1.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);
EXPECT_EQ("OFFLINE", session2.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);
EXPECT_EQ("OFFLINE", session3.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

// NEGATIVE CASES
// @<> prepare and make primary unreachable from shell

session1.runSql("set global super_read_only=0");
session1.runSql("reset master");
session2.runSql("set global super_read_only=0");
session2.runSql("reset master");
session3.runSql("set global super_read_only=0");
session3.runSql("reset master");

shell.connect(__sandbox_uri1);

cluster = dba.createCluster("mycluster");

run_nolog(session1, "create user root2@'%'");
run_nolog(session1, "grant all on *.* to root2@'%' with grant option");
run_nolog(session2, "create user root2@'%'");
run_nolog(session2, "grant all on *.* to root2@'%' with grant option");
run_nolog(session3, "create user root2@'%'");
run_nolog(session3, "grant all on *.* to root2@'%' with grant option");

cluster.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});

cluster_id = session1.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

run_nolog(session1, "alter user root@'%' account lock");
run_nolog(session1, "alter user root@'localhost' account lock");

// this cluster object will connect to the primary, although the other one wont
shell.connect("root2:@localhost:"+__mysql_sandbox_port1);
cluster2= dba.getCluster();

//@<> checkInstanceState (no primary, should fail)
EXPECT_THROWS(function(){cluster.checkInstanceState(__sandbox_uri3)}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> addInstance using clone recovery (no primary, should fail) {VER(>=8.0.17)}
EXPECT_THROWS(function(){cluster.addInstance(__sandbox_uri3, {recoveryMethod:'clone'})}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> addInstance using incremental recovery (no primary, should fail)
EXPECT_THROWS(function(){cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'})}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> removeInstance (no primary, should fail)
EXPECT_THROWS(function(){cluster.removeInstance(__sandbox_uri2)}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: Unable to connect to instance '127.0.0.1:"+__mysql_sandbox_port1+"'. Please verify connection credentials and make sure the instance is available.");
EXPECT_OUTPUT_CONTAINS("ERROR: Could not update remaining cluster members after removing '"+__sandbox2+"': Access denied for user 'root'@'localhost'. Account is locked.");

//@<> setPrimaryInstance (no primary, should fail) {VER(>=8.0.0)}
EXPECT_THROWS(function(){cluster.setPrimaryInstance(__sandbox_uri2)}, "This operation requires all the cluster members to be ONLINE");

//@<> rejoinInstance (no primary, should fail)
cluster2.addInstance("localhost:"+__mysql_sandbox_port3, {recoveryMethod:'incremental'});
session3.runSql("STOP group_replication");

EXPECT_THROWS(function(){cluster.rejoinInstance(__sandbox_uri3)}, "Access denied for user 'root'@'localhost'. Account is locked.");

cluster2.rejoinInstance("localhost:"+__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.status();

//@<> setOption (no primary, should fail)
EXPECT_THROWS(function(){cluster.setOption("clusterName", "clooster")}, "This operation requires all the cluster members to be ONLINE");

custom_weigth=50;

//@<> setInstanceOption (no primary, should fail) {VER(>=8.0.0)}
custom_weigth=20;

EXPECT_THROWS(function(){cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth)}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: Unable to connect to the target instance '"+__sandbox1+"'. Please verify the connection settings, make sure the instance is available and try again.");

//@<> options (no primary, should pass)
cluster.options();

//@<> switchToMultiPrimaryMode (no primary, should fail) {VER(>=8.0.0)}
EXPECT_THROWS(function(){cluster.switchToMultiPrimaryMode()}, "This operation requires all the cluster members to be ONLINE");

//@<> switchToSinglePrimaryMode (no primary, should fail) {VER(>=8.0.0)}
EXPECT_THROWS(function(){cluster.switchToSinglePrimaryMode()}, "This operation requires all the cluster members to be ONLINE");

//@<> rescan (no primary, should fail)
// delete sb3 from the metadata so that rescan picks it up
session1.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

EXPECT_THROWS(function(){cluster.rescan({addInstances:"auto"})}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> listRouters (no primary, should pass)
cluster.listRouters();

//@<> removeRouterMetadata (no primary, should pass)
EXPECT_NO_THROWS(function(){cluster.removeRouterMetadata("routerhost1::system")});

cluster.listRouters();

//@<> dissolve (no primary, should fail)
EXPECT_THROWS(function(){cluster.dissolve()}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: Unable to connect to instance '127.0.0.1:"+__mysql_sandbox_port1+"'. Please verify connection credentials and make sure the instance is available.");

//@<> Cleanup
run_nolog(session1, "alter user root@'%' account unlock");
run_nolog(session1, "alter user root@'localhost' account unlock");

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
