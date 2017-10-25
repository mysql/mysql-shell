// Assumptions: smart deployment functions available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

// Restart sandbox instances with specific binlog filtering option.
testutil.stopSandbox(__mysql_sandbox_port1, 'root');
testutil.addToSandboxConf(__mysql_sandbox_port1, "[mysqld]\nbinlog-do-db=db1,mysql_innodb_cluster_metadata,db2");
testutil.startSandbox(__mysql_sandbox_port1);

testutil.stopSandbox(__mysql_sandbox_port2, 'root');
testutil.addToSandboxConf(__mysql_sandbox_port2, "[mysqld]\nbinlog-do-db=db1,db2");
testutil.startSandbox(__mysql_sandbox_port2);

testutil.stopSandbox(__mysql_sandbox_port3, 'root');
testutil.addToSandboxConf(__mysql_sandbox_port3, "[mysqld]\nbinlog-ignore-db=db1,mysql_innodb_cluster_metadata,db2");
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

//@# Dba: create_cluster succeed with binlog-do-db
cluster = dba.createCluster("testCluster");
cluster

//@# Dba: add_instance fails with binlog-do-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port2});

//@# Dba: add_instance fails with binlog-ignore-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port3});

session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
