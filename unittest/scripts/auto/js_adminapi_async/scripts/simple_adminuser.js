//@ {VER(>=8.0.11)}

// Plain replicaset setup test using the admin account

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

shell.options.useWizards = false;

//@ configureReplicaSetInstance + create admin user
dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
testutil.restartSandbox(__mysql_sandbox_port1);

dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"bla"});

// drop root account just in case
s = mysql.getSession(__sandbox_uri1);
s.runSql("SET SESSION sql_log_bin=0");
s.runSql("DROP USER root@'%'");
s.runSql("DROP USER root@localhost");
s = mysql.getSession(__sandbox_uri2);
s.runSql("SET SESSION sql_log_bin=0");
s.runSql("DROP USER root@'%'");
s.runSql("DROP USER root@localhost");
s = mysql.getSession(__sandbox_uri3);
s.runSql("SET SESSION sql_log_bin=0");
s.runSql("DROP USER root@'%'");
s.runSql("DROP USER root@localhost");

//@ createReplicaSet
shell.connect("admin:bla@"+__sandbox1);

rs = dba.createReplicaSet("myrs");

//@ status
rs.status();

//@ disconnect
rs.disconnect();

//@ getReplicaSet
rs = dba.getReplicaSet();

//@ addInstance
rs.addInstance(__sandbox2, {recoveryMethod:'incremental'});

rs.addInstance(__sandbox3, {recoveryMethod:'incremental'});

//@ removeInstance
rs.removeInstance(__sandbox2);

rs.addInstance(__sandbox2, {recoveryMethod:'incremental'});

//@ setPrimaryInstance
rs.setPrimaryInstance(__sandbox3);

//@ forcePrimaryInstance (prepare)
testutil.killSandbox(__mysql_sandbox_port3);
rs = dba.getReplicaSet();

rs.status();

//@ forcePrimaryInstance
rs.forcePrimaryInstance(__sandbox1);

// rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

// XXX rs.rejoinInstance("admin:bla@"+__sandbox3);

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

rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
