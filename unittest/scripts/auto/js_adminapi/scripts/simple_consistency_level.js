//@ {VER(>=8.0.14)}

// Full cluster test using group_replication_consistency=BEFORE

//@<> Setup

testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.options.useWizards = false;

//@<> configureLocalInstance
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1);

//@<> configureInstance
dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

//@<> createCluster
shell.connect(__sandbox_uri1);
cluster = dba.createCluster("mycluster");

// Change the consistency level to a value != EVENTUAL of BEFORE_ON_PRIMARY_FAILOVER
cluster.setOption("consistency", "BEFORE");

//@<> status
status = cluster.status();

//@<> checkInstanceState
cluster.checkInstanceState(__sandbox_uri2);

//@<> describe
cluster.describe();

//@<> disconnect
cluster.disconnect();

//@<> getCluster
cluster = dba.getCluster();

//@<> addInstance
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

// Test also with consistency level AFTER
cluster.setOption("consistency", "AFTER");

//@<> addInstance (clone) {VER(>=8.0.17)}
cluster.removeInstance(__sandbox_uri2);

cluster.addInstance(__sandbox_uri2, {recoveryMethod:'clone'});

session2 = mysql.getSession(__sandbox_uri2);

//@<> removeInstance
cluster.removeInstance(__sandbox_uri2);

cluster.addInstance(__sandbox_uri2, {recoveryMethod:'auto'});

//@<> setPrimaryInstance {VER(>=8.0.0)}
cluster.setPrimaryInstance(__sandbox_uri2);

cluster.setPrimaryInstance(__sandbox_uri1);

// Test also with consistency level BEFORE_AND_AFTER
cluster.setOption("consistency", "BEFORE_AND_AFTER");

//@<> rejoinInstance
session2.runSql("STOP group_replication");

cluster.rejoinInstance(__sandbox_uri2);

//@<> rejoinInstance (clone) {VER(>=8.0.17)}
// NOTE: not supported yet
//session2.runSql("STOP group_replication");

//cluster.rejoinInstance(__sandbox_uri2, {recoveryMethod:'clone'});

//session2 = mysql.getSession(__sandbox_uri2);

//@<> forceQuorumUsingPartitionOf
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");

cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("set autocommit=1");

cluster.rejoinInstance(__sandbox_uri2);

//@<> rebootClusterFromCompleteOutage
session1.runSql("STOP group_replication");
session2.runSql("STOP group_replication");

cluster = dba.rebootClusterFromCompleteOutage();

// TODO(alfredo) - the reboot should auto-rejoin all members
cluster.rejoinInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> setOption
cluster.setOption("clusterName", "clooster");
custom_weigth=50;

//@<> setInstanceOption {VER(>=8.0.0)}
custom_weigth=20;
cluster.setInstanceOption(__sandbox_uri1, "memberWeight", custom_weigth);

//@<> options
cluster.options();

//@<> switchToMultiPrimaryMode {VER(>=8.0.0)}
cluster.switchToMultiPrimaryMode();

//@<> switchToSinglePrimaryMode {VER(>=8.0.0)}
cluster.switchToSinglePrimaryMode();

cluster.setPrimaryInstance(__sandbox_uri1);

shell.connect(__sandbox_uri1);
cluster = dba.getCluster(); // shouldn't be necessary

//@<> rescan
// delete sb2 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port2]);

cluster.rescan();

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

cluster.listRouters();

//@<> removeRouterMetadata
cluster.removeRouterMetadata("routerhost1::system");

cluster.listRouters();

//@<> createCluster(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
cluster = dba.createCluster("adopted", {adoptFromGR:true});

cluster.status();

//@<> dissolve
cluster.dissolve();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
