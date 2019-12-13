//@ {__os_type != 'windows'}
// Plain cluster test, using non-TCP connections

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
sockpath1 = get_socket_path(session1);
session2 = mysql.getSession(__sandbox_uri2);
sockpath2 = get_socket_path(session2);

sockuri1 = "mysql://root:root@("+sockpath1+")";
sockuri2 = "mysql://root:root@("+sockpath2+")";

//@<> configureLocalInstance
dba.configureLocalInstance(sockuri1, {mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});
testutil.restartSandbox(__mysql_sandbox_port1);

session1 = mysql.getSession(sockuri1);

//@<> configureInstance
dba.configureInstance(sockuri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

session2 = mysql.getSession(sockuri2);

//@<> createCluster
shell.connect(sockuri1);

cluster = dba.createCluster("mycluster");

//@<> status
status = cluster.status();

//@<> checkInstanceState
cluster.checkInstanceState(sockuri2);

//@<> describe
cluster.describe();

//@<> disconnect
cluster.disconnect();

//@<> getCluster
cluster = dba.getCluster();

//@<> addInstance
cluster.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@<> removeInstance
//TODO(alfredo) - not supported yet
//cluster.removeInstance(sockuri2);

//cluster.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@<> setPrimaryInstance {VER(>=8.0.0)}
cluster.setPrimaryInstance(sockuri2);

cluster.setPrimaryInstance(sockuri1);

//@<> rejoinInstance
session2.runSql("STOP group_replication");

cluster.rejoinInstance(sockuri2);

//@<> forceQuorumUsingPartitionOf
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");

cluster.forceQuorumUsingPartitionOf(sockuri1);

testutil.startSandbox(__mysql_sandbox_port2);
session2 = mysql.getSession(sockuri2);

cluster.rejoinInstance(sockuri2);

//@<> rebootClusterFromCompleteOutage
session1.runSql("STOP group_replication");
session2.runSql("STOP group_replication");

cluster = dba.rebootClusterFromCompleteOutage();

// TODO(alfredo) - the reboot should auto-rejoin all members
cluster.rejoinInstance(sockuri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> setOption
cluster.setOption("clusterName", "clooster");

//@<> setInstanceOption {VER(>=8.0.0)}
cluster.setInstanceOption(sockuri1, "memberWeight", 20);

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
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

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
