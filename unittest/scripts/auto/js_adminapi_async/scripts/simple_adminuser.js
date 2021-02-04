//@ {VER(>=8.0.11)}

// Plain replicaset setup test using the admin account

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});

shell.options.useWizards = false;

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
const __sb1 = hostname_ip + ":" + __mysql_sandbox_port1;

// Enable the validate_password plugin on all instances
ensure_plugin_enabled("validate_password", session1, "validate_password");
ensure_plugin_enabled("validate_password", session2, "validate_password");
// configure the validate_password plugin for the lowest policy, for the sake of simplicity
session1.runSql('SET GLOBAL validate_password_policy=\'LOW\'');
session1.runSql('SET GLOBAL validate_password_length=1');
session2.runSql('SET GLOBAL validate_password_policy=\'LOW\'');
session2.runSql('SET GLOBAL validate_password_length=1');

//@<> configureReplicaSetInstance - validate_password error
EXPECT_THROWS_TYPE(function(){dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"})}, "" + __sb1 + ": Your password does not satisfy the current policy requirements", "MYSQLSH");

//@ configureReplicaSetInstance + create admin user
dba.configureReplicaSetInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"blaa"});
testutil.restartSandbox(__mysql_sandbox_port1);
session1 = mysql.getSession(__sandbox_uri1);

dba.configureReplicaSetInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"blaa"});
dba.configureReplicaSetInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"blaa"});

// Uninstall the validate_password plugin: negative and positive tests done
ensure_plugin_disabled("validate_password", session1, "validate_password");
ensure_plugin_disabled("validate_password", session2, "validate_password");

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
shell.connect("admin:blaa@"+__sandbox1);

rs = dba.createReplicaSet("myrs");

//@ status
rs.status();

//@ disconnect
rs.disconnect();

//@ getReplicaSet
rs = dba.getReplicaSet();

//@ addInstance (incremental)
rs.addInstance(__sandbox2, {recoveryMethod:'incremental'});

//@ addInstance (clone) {VER(>=8.0.17)}
rs.addInstance(__sandbox3, {recoveryMethod:'clone'});

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

//@ rejoinInstance
testutil.startSandbox(__mysql_sandbox_port3);

rs.rejoinInstance("admin:blaa@"+__sandbox3);

//@ rejoinInstance (clone) {VER(>=8.0.17)}
session3 = mysql.getSession("admin:blaa@"+__sandbox3);
session3.runSql("STOP SLAVE");

rs.rejoinInstance("admin:blaa@"+__sandbox3, {recoveryMethod:"clone"});

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
