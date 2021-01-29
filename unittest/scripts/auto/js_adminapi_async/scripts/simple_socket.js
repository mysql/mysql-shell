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

function socket_uri(session, uri = undefined) {
  if (undefined === uri) {
    uri = session.uri;
  }
  const path = get_socket_path(session, uri);
  return shell.parseUri(uri).scheme + "://root:root@(" + path + ")";
}

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

sockuri1 = socket_uri(session1);
sockuri2 = socket_uri(session2);
sockuri3 = socket_uri(session3);

xsockuri1 = socket_uri(session1, get_mysqlx_uris(__sandbox_uri1)[0]);
xsockuri2 = socket_uri(session2, get_mysqlx_uris(__sandbox_uri2)[0]);
xsockuri3 = socket_uri(session3, get_mysqlx_uris(__sandbox_uri3)[0]);

// redefine the functions to handle sockets
function get_mysqlx_uris(uri) {
  const u = sockuri1 === uri ? xsockuri1 : sockuri2 === uri ? xsockuri2 : xsockuri3;
  return [u, u.substring("mysqlx://".length)];
}

function get_mysqlx_endpoint(uri) {
  const u = shell.parseUri(sockuri1 === uri ? xsockuri1 : sockuri2 === uri ? xsockuri2 : xsockuri3);
  return shell.unparseUri({ 'socket': u.socket });
}

//@ configureReplicaSetInstance + create admin user
EXPECT_DBA_THROWS_PROTOCOL_ERROR("Dba.configureReplicaSetInstance", dba.configureReplicaSetInstance, sockuri1, {clusterAdmin:"admin", clusterAdminPassword:"bla", restart:1});

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
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.addInstance", rs.addInstance, sockuri2, {recoveryMethod:'incremental'});

rs.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@ addInstance (clone) {VER(>=8.0.17)}
rs.addInstance(sockuri3, {recoveryMethod:'clone'});

//@ addInstance (no clone) {VER(<8.0.17)}
rs.addInstance(sockuri3, {recoveryMethod:'incremental'});

//@ removeInstance
rs.removeInstance(sockuri2);

rs.addInstance(sockuri2, {recoveryMethod:'incremental'});

//@ setPrimaryInstance
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.setPrimaryInstance", rs.setPrimaryInstance, sockuri3);

rs.setPrimaryInstance(sockuri3);

//@ forcePrimaryInstance (prepare)
testutil.stopSandbox(__mysql_sandbox_port3, {wait:1});
rs = dba.getReplicaSet();

rs.status();

//@ forcePrimaryInstance
EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.forcePrimaryInstance", rs.forcePrimaryInstance, sockuri1);

rs.forcePrimaryInstance(sockuri1);

//@ rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_CLUSTER_THROWS_PROTOCOL_ERROR("ReplicaSet.rejoinInstance", rs.rejoinInstance, sockuri3);

rs.rejoinInstance(sockuri3);

//@ rejoinInstance (clone) {VER(>=8.0.17)}
session3 = mysql.getSession(sockuri3);
session3.runSql("STOP SLAVE");

rs.rejoinInstance(sockuri3, {recoveryMethod:"clone"});

//@ listRouters
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

rs.listRouters();

//@ removeRouterMetadata
rs.removeRouterMetadata("routerhost1::system");

rs.listRouters();

//@ createReplicaSet(adopt)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

rs = dba.createReplicaSet("adopted", {adoptFromAR:true});

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
