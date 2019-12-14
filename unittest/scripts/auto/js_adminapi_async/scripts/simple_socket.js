//@ {VER(>=8.0.11) && __os_type != 'windows'}

// Plain replicaset setup using non-TCP connections

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deployRawSandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
sockpath1 = get_socket_path(session1);
session2 = mysql.getSession(__sandbox_uri2);
sockpath2 = get_socket_path(session2);
session3 = mysql.getSession(__sandbox_uri3);
sockpath3 = get_socket_path(session3);

sockuri1 = "mysql://root:root@("+sockpath1+")";
sockuri2 = "mysql://root:root@("+sockpath2+")";
sockuri3 = "mysql://root:root@("+sockpath3+")";


//@ configureReplicaSetInstance + create admin user
dba.configureReplicaSetInstance(sockuri1, {clusterAdmin:"admin", clusterAdminPassword:"bla", restart:1});

//@ configureReplicaSetInstance
shell.connect(sockuri2);
dba.configureReplicaSetInstance();
testutil.restartSandbox(__mysql_sandbox_port2);

dba.configureReplicaSetInstance(sockuri3);

//@ createReplicaSet
shell.connect(sockuri1);

rs = dba.createReplicaSet("myrs");

//@ status
rs.status();

//@ disconnect
rs.disconnect();

//@ getReplicaSet
rs = dba.getReplicaSet();

//@ addInstance (incremental)
rs.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@ addInstance (clone) {VER(>=8.0.17)}
rs.addInstance(sockuri3, {recoveryMethod:'clone'});

//@ addInstance (no clone) {VER(<8.0.17)}
rs.addInstance(sockuri3, {recoveryMethod:'incremental'});

//@ removeInstance
rs.removeInstance(sockuri2);

rs.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@ setPrimaryInstance
rs.setPrimaryInstance(sockuri3);

//@ forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3);
rs = dba.getReplicaSet();

rs.status();

//@ forcePrimaryInstance
rs.forcePrimaryInstance(sockuri1);

//@ rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

rs.rejoinInstance(sockuri3);

//@ rejoinInstance (clone) {VER(>=8.0.17)}
session3 = mysql.getSession(sockuri3);
session3.runSql("STOP SLAVE");

rs.rejoinInstance(sockuri3, {recoveryMethod:"clone"});

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
