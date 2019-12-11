// Full cluster test using autocommit=0 and sql_mode with weird non-default values

//@<> Setup

var mycnf = {};
mycnf["autocommit"] = 0;
mycnf["sql_mode"] = "ANSI_QUOTES,NO_AUTO_VALUE_ON_ZERO,NO_BACKSLASH_ESCAPES";
// BUG: setting sql_mode to PAD_CHAR_TO_FULL_LENGTH doesn't work because 
// precondition checks use the main shell session, which we don't change.
// The checks have to be changed to use their own session.

testutil.deployRawSandbox(__mysql_sandbox_port1, "root", mycnf);
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", mycnf);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.options.useWizards = false;

//@<> configureLocalInstance
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(__sandbox_uri1);
// all our own sessions (not AdminAPI internal ones) need to have autocommit=1, otherwise
// setPrimaryInstance() will freeze waiting for open transactions to be closed
session1.runSql("set autocommit=1");

//@<> configureInstance
dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("set autocommit=1");

//@<> createCluster
shell.connect(__sandbox_uri1);
session.runSql("set autocommit=1");

cluster = dba.createCluster("mycluster");

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
session.runSql("set autocommit=1");
cluster = dba.getCluster(); // shouldn't be necessary

//@<> rescan
// delete sb2 from the metadata so that rescan picks it up
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE instance_name LIKE ?", ["%:"+__mysql_sandbox_port2]);

cluster.rescan();

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

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
