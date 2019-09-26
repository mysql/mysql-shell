//@ {VER(>=8.0.11)}

// Plain replicaset setup test, use as a template for other tests that check
// a specific feature/aspect across the whole API

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards = false;

//@ configureReplicaSetInstance + create admin user
dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
testutil.restartSandbox(__mysql_sandbox_port1);

//@ configureReplicaSetInstance
dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

//@ createReplicaSet
shell.connect(__sandbox_uri1);

rs = dba.createReplicaSet("myrs");

//@ status
rs.status();

//@ disconnect
rs.disconnect();

//@ getReplicaSet
rs = dba.getReplicaSet();

//@ addInstance
rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

rs.addInstance(__sandbox_uri3, {recoveryMethod:'incremental'});

//@ removeInstance
rs.removeInstance(__sandbox_uri2);

rs.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});

//@ setPrimaryInstance
rs.setPrimaryInstance(__sandbox_uri3);

//@ forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3);
rs = dba.getReplicaSet();

rs.status();

//@ forcePrimaryInstance
rs.forcePrimaryInstance(__sandbox_uri1);

//@ rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

rs.rejoinInstance(__sandbox_uri3);

//@ listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

rs.listRouters();

//@ removeRouterMetadata
rs.removeRouterMetadata("routerhost1::system");

rs.listRouters();

//@ createReplicaSet(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
//BREAK
rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
