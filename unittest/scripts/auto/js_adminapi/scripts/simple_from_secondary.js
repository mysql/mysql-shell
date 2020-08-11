// Ensure that all cluster operations work even if the shell is connected to a SECONDARY
// Bug#31757737

// The basic test pattern is:
// positive case:
// getCluster({connectToPrimary:false}) while connected to a secondary
// perform operation
//
// negative case:
// make primary unconnectable (change root password)
// getCluster({connectToPrimary:false}) while connected to a secondary
// perform operation

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
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.checkInstanceState(__sandbox_uri3);

//@<> addInstance using clone recovery {VER(>=8.0.17)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.addInstance(__sandbox_uri3, {recoveryMethod:'clone'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

session3 = mysql.getSession(__sandbox_uri3);
cluster.removeInstance(__sandbox_uri3);

//@<> addInstance using incremental recovery
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

//@<> removeInstance
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.removeInstance(__sandbox_uri3);

//@<> setPrimaryInstance {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.setPrimaryInstance(__sandbox_uri2);

cluster.setPrimaryInstance(__sandbox_uri1);

//@<> rejoinInstance
cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});
session3.runSql("STOP group_replication");

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.rejoinInstance(__sandbox_uri3);

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

cluster.status();

//@<> setOption
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.setOption("clusterName", "clooster");

custom_weigth=50;

//@<> setInstanceOption {VER(>=8.0.0)}
custom_weigth=20;

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth);

//@<> options
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.options();

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.switchToMultiPrimaryMode();

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.switchToSinglePrimaryMode();

cluster.setPrimaryInstance(__sandbox_uri1);

//@<> rescan
// delete sb3 from the metadata so that rescan picks it up
session1.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.rescan({addInstances:"auto"});

//@<> listRouters
cluster_id = session1.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.listRouters();

//@<> removeRouterMetadata
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.removeRouterMetadata("routerhost1::system");

cluster.listRouters();

//@<> dissolve
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.dissolve();

EXPECT_EQ("OFFLINE", session1.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);
EXPECT_EQ("OFFLINE", session2.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);
EXPECT_EQ("OFFLINE", session3.runSql("select member_state from performance_schema.replication_group_members").fetchOne()[0]);

// NEGATIVE CASES
// @<> prepare and make primary unreachable from shell

session1.runSql("set global super_read_only=0");
session1.runSql("drop schema mysql_innodb_cluster_metadata");
session1.runSql("reset master");
session2.runSql("set global super_read_only=0");
session2.runSql("drop schema mysql_innodb_cluster_metadata");
session2.runSql("reset master");
session3.runSql("set global super_read_only=0");
session3.runSql("drop schema mysql_innodb_cluster_metadata");
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
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session1.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

run_nolog(session1, "alter user root@'%' account lock");
run_nolog(session1, "alter user root@'localhost' account lock");

// this cluster object will connect to the primary, although the other one wont
shell.connect("root2:@localhost:"+__mysql_sandbox_port1);
cluster2= dba.getCluster();


//@<> checkInstanceState (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.checkInstanceState(__sandbox_uri3)}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> addInstance using clone recovery (no primary, should fail) {VER(>=8.0.17)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.addInstance(__sandbox_uri3, {recoveryMethod:'clone'})}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> addInstance using incremental recovery (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'})}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> removeInstance (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.removeInstance(__sandbox_uri2)}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> setPrimaryInstance (no primary, should fail) {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.setPrimaryInstance(__sandbox_uri2)}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> rejoinInstance (no primary, should fail)
cluster2.addInstance("localhost:"+__mysql_sandbox_port3, {recoveryMethod:'incremental'});
session3.runSql("STOP group_replication");

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.rejoinInstance(__sandbox_uri3)}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

cluster2.rejoinInstance("localhost:"+__mysql_sandbox_port3);
cluster.status();

//@<> setOption (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.setOption("clusterName", "clooster")}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

custom_weigth=50;

//@<> setInstanceOption (no primary, should fail) {VER(>=8.0.0)}
custom_weigth=20;

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth)}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> options (no primary, should pass)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.options();

//@<> switchToMultiPrimaryMode (no primary, should fail) {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.switchToMultiPrimaryMode()}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> switchToSinglePrimaryMode (no primary, should fail) {VER(>=8.0.0)}
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.switchToSinglePrimaryMode()}, "Access denied for user 'root'@'localhost'. Account is locked.");

//@<> rescan (no primary, should fail)
// delete sb3 from the metadata so that rescan picks it up
session1.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port3]);

shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.rescan({addInstances:"auto"})}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> listRouters (no primary, should pass)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

cluster.listRouters();

//@<> removeRouterMetadata (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.removeRouterMetadata("routerhost1::system")}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

cluster.listRouters();

//@<> dissolve (no primary, should fail)
shell.connect(__sandbox_uri2);
cluster = dba.getCluster(null, {connectToPrimary:false});

EXPECT_THROWS(function(){cluster.dissolve()}, "Access denied for user 'root'@'localhost'. Account is locked.");
EXPECT_OUTPUT_CONTAINS("ERROR: A connection to the PRIMARY instance at 127.0.0.1:"+__mysql_sandbox_port1+" could not be established to perform this action.");

//@<> Cleanup
run_nolog(session1, "alter user root@'%' account unlock");
run_nolog(session1, "alter user root@'localhost' account unlock");

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
