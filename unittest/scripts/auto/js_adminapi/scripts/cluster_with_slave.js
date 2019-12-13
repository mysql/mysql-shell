// Test for a cluster with an async slave instance hanging out from it.
// Most of the time, that seems to be intended for a hand-made DR setup.
// Not an officially supported setup, but some customers have it and several
// different bugs have been filed in the past.

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host:hostname_ip});
testutil.deployRawSandbox(__mysql_sandbox_port2, "root", {report_host:hostname_ip});
testutil.deployRawSandbox(__mysql_sandbox_port3, "root", {report_host:hostname_ip});

testutil.deployRawSandbox(__mysql_sandbox_port4, "root", {report_host:hostname_ip});

//@<> config the sandboxes with clusterAdmin accounts
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
testutil.snapshotSandboxConf(__mysql_sandbox_port4);

dba.configureInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"pwd", mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port1)});
dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"pwd", mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port2)});
dba.configureInstance(__sandbox_uri3, {clusterAdmin:"admin", clusterAdminPassword:"pwd", mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port3)});
dba.configureInstance(__sandbox_uri4, {clusterAdmin:"admin", clusterAdminPassword:"pwd", mycnfPath:testutil.getSandboxConfPath(__mysql_sandbox_port4)});

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
testutil.restartSandbox(__mysql_sandbox_port3);
testutil.restartSandbox(__mysql_sandbox_port4);

testutil.waitSandboxAlive(__mysql_sandbox_port1);
testutil.waitSandboxAlive(__mysql_sandbox_port2);
testutil.waitSandboxAlive(__mysql_sandbox_port3);
testutil.waitSandboxAlive(__mysql_sandbox_port4);

session1 = mysql.getSession("admin:pwd@localhost:"+__mysql_sandbox_port1);
session4 = mysql.getSession("admin:pwd@localhost:"+__mysql_sandbox_port4);


//@<> setup sb4 as a replica of sb1
session1.runSql("create user 'async'@'localhost' identified by 'test'");
session1.runSql("grant replication slave on *.* to 'async'@'localhost'");

session4.runSql("change master to master_host='localhost', master_port=/*(*/ ? /*)*/, master_user='async', master_password='test', master_ssl=1", [__mysql_sandbox_port1]);

// filter out MD schema
session4.runSql("CHANGE REPLICATION FILTER Replicate_Ignore_DB = (mysql_innodb_cluster_metadata)");
session4.runSql("START SLAVE");

//@<> create cluster
shell.connect("admin:pwd@localhost:"+__mysql_sandbox_port1);
cluster = dba.createCluster("cluster");

//@<> addInstance using clone {VER(>=8.0.17)}
cluster.addInstance("admin:pwd@localhost:"+__mysql_sandbox_port2, {recoveryMethod:"clone", waitRecovery:1});

//@<> addInstance without clone {VER(<8.0.17)}
cluster.addInstance("admin:pwd@localhost:"+__mysql_sandbox_port2, {recoveryMethod:"incremental"});

//@<> check replication status (should be ok)
// Note for Bug#30609075
// Here, the slave should still be OK, because we added the instance using the same session
// as createCluster(), which still has mysql_innodb_cluster_metadata as the default DB.
// So any grants executed here will be filtered out.
shell.dumpRows(session4.runSql("SHOW SLAVE STATUS"), "vertical");

//@<> addInstance another one using clone {VER(>=8.0.17)}
shell.connect("admin:pwd@localhost:"+__mysql_sandbox_port1);
cluster = dba.getCluster();
cluster.addInstance("admin:pwd@localhost:"+__mysql_sandbox_port3, {recoveryMethod:"clone", waitRecovery:1});

//@<> addInstance another one without clone {VER(<8.0.17)}

// Note for Bug#30609075
// We add this instance on a brand new session, to force any SQL executed as part of the addInstance() to
// NOT be logged with the MD schema as default DB. This causes these statements to not be filtered.
shell.connect("admin:pwd@localhost:"+__mysql_sandbox_port1);
cluster = dba.getCluster();
cluster.addInstance("admin:pwd@localhost:"+__mysql_sandbox_port3, {recoveryMethod:"incremental"});

//@<> check replication status 
// check GTID executed at each sandbox
session.runSql("SELECT @@global.gtid_executed").fetchOne();
session4.runSql("SELECT @@global.gtid_executed").fetchOne();

// check schemas
// slave should have same schemas except for MD schema
session.runSql("SHOW SCHEMAS").fetchAll();
session4.runSql("SHOW SCHEMAS").fetchAll();

// check users
// slave supposedly filters out accounts too
shell.dumpRows(session.runSql("select user,host from mysql.user"), "tabbed");
shell.dumpRows(session4.runSql("select user,host from mysql.user"), "tabbed");

//@<> With bug #30609075 replication would be broken now
shell.dumpRows(session4.runSql("SHOW SLAVE STATUS"), "vertical");

// With the bug, we'd get:
// Last_Errno: 1410
// Last_Error: Error 'You are not allowed to create a user with GRANT' on query. Default database: ''. Query: 'GRANT BACKUP_ADMIN ON *.* TO 'mysql_innodb_cluster_233387353'@'%''
EXPECT_EQ(0, session4.runSql("SHOW SLAVE STATUS").fetchOne().Last_Errno);




//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
