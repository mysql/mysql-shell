
// NOTE: no replication filters are allowed at all

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id:32});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id:33});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id:34});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Restart sandbox instances with specific binlog filtering option.
testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "binlog-do-db", "db1,db2");
testutil.startSandbox(__mysql_sandbox_port2);

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "binlog-ignore-db", "db1,mysql_innodb_cluster_metadata,db2");
testutil.startSandbox(__mysql_sandbox_port3);


shell.connect({scheme:'mysql', 'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port2});

//@## Dba: create_cluster fails with binlog-do-db
dba.createCluster("testCluster");

session.close();

shell.connect({scheme:'mysql', 'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port3});

//@# Dba: create_cluster fails with binlog-ignore-db
dba.createCluster("testCluster");

session.close();

shell.connect({scheme:'mysql', 'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

//@# Dba: create_cluster fails with global repl filter {VER(>=8.0.11)}
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo)");
dba.createCluster("testCluster");

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo), REPLICATE_DO_DB = (foo)");
dba.createCluster("testCluster");

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (), REPLICATE_DO_DB = ()");

//@# Dba: create_cluster fails with repl filter {VER(>=8.0.11)}
session.runSql("CHANGE MASTER TO master_host='localhost', master_port=3306 FOR CHANNEL 'x'");
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo) FOR CHANNEL 'x'");
dba.createCluster("testCluster");

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo), REPLICATE_DO_DB = (foo) FOR CHANNEL 'x'");
dba.createCluster("testCluster");

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (), REPLICATE_DO_DB = () FOR CHANNEL 'x'");
session.runSql("RESET SLAVE ALL");

//@# Dba: create_cluster succeed without binlog-do-db nor repl filter
cluster = dba.createCluster("testCluster", {gtidSetIsComplete: true});
cluster

//@# Dba: add_instance fails with binlog-do-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port2});

//@# Dba: add_instance fails with binlog-ignore-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port3});

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.changeSandboxConf(__mysql_sandbox_port3, "binlog-ignore-db", "");
testutil.startSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri3);
session.runSql("CHANGE MASTER TO master_host='localhost', master_port=3306 FOR CHANNEL 'x'");

//@# Dba: add_instance fails with global repl filter {VER(>=8.0.11)}
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo)");
cluster.addInstance(__sandbox_uri3);

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo), REPLICATE_DO_DB = (foo)");
cluster.addInstance(__sandbox_uri3);

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (), REPLICATE_DO_DB = ()");

//@# Dba: add_instance fails with repl filter {VER(>=8.0.11)}
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo) FOR CHANNEL 'x'");
cluster.addInstance(__sandbox_uri3);

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo), REPLICATE_DO_DB = (foo) FOR CHANNEL 'x'");
cluster.addInstance(__sandbox_uri3);

session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (), REPLICATE_DO_DB = () FOR CHANNEL 'x'");

//@# Dba: add_instance succeeds without repl filter
session.runSql("RESET SLAVE ALL");
cluster.addInstance(__sandbox_uri3);

//@ Finalization
session.close();
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
