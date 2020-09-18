//@ {VER(>=8.0.11)}

// Plain replicaset setup test with IPv6

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {"report_host":"::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host":"::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host":"::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards = false;

__sandbox_uri1 = "mysql://root:root@[::1]:"+__mysql_sandbox_port1;
__sandbox_uri2 = "mysql://root:root@[::1]:"+__mysql_sandbox_port2;
__sandbox_uri3 = "mysql://root:root@[::1]:"+__mysql_sandbox_port3;

sandbox1 = "[::1]:"+__mysql_sandbox_port1;
sandbox2 = "[::1]:"+__mysql_sandbox_port2;
sandbox3 = "[::1]:"+__mysql_sandbox_port3;

//@ configureReplicaSetInstance + create admin user
dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
testutil.restartSandbox(__mysql_sandbox_port1);

//@ configureReplicaSetInstance
dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

//@ createReplicaSet
shell.connect(__sandbox_uri1);

rs = dba.createReplicaSet("myrs");

//@<> disconnect
rs.disconnect();

//@<> getReplicaSet
rs = dba.getReplicaSet();

//@ addInstance (incremental)
rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

//@ addInstance (clone) (should fail) {VER(>=8.0.17)}
rs.addInstance(__sandbox_uri3, {recoveryMethod:'clone', cloneDonor:"[::1]:"+__mysql_sandbox_port1});

rs.addInstance(__sandbox_uri3, {recoveryMethod:'clone'});

//@<> addInstance sb3
rs.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

//@ status
rs.status();

//@ check IPv6 addresses in MD
shell.dumpRows(session.runSql("SELECT * FROM mysql_innodb_cluster_metadata.instances"), "tabbed");

//@<> removeInstance
rs.removeInstance(sandbox2);

// add back
rs.addInstance(sandbox2, {recoveryMethod:'incremental'});

//@<> setPrimaryInstance
rs.setPrimaryInstance(sandbox3);

//@<> forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3);
rs = dba.getReplicaSet();

rs.status();

//@<> forcePrimaryInstance
rs.forcePrimaryInstance(sandbox1);

//@<> rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port3);

rs.rejoinInstance(sandbox3);

//@ rejoinInstance (clone) (should fail) {VER(>=8.0.17)}
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP SLAVE");

rs.rejoinInstance(sandbox3, {recoveryMethod:"clone"});

//@<> listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

rs.listRouters();

//@<> removeRouterMetadata
rs.removeRouterMetadata("routerhost1::system");

rs.listRouters();

//@ createReplicaSet(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
