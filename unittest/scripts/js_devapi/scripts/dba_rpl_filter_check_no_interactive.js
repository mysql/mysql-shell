// Assumptions: smart deployment functions available

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port2});

//@## Dba: create_cluster fails with binlog-do-db
dba.createCluster("testCluster");

session.close();

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port3});

//@# Dba: create_cluster fails with binlog-ignore-db
dba.createCluster("testCluster");

session.close();

shell.connect({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port1});

//@# Dba: create_cluster succeed with binlog-do-db
cluster = dba.createCluster("testCluster");
cluster

//@# Dba: add_instance fails with binlog-do-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port2});

//@# Dba: add_instance fails with binlog-ignore-db
cluster.addInstance({'user':'root', 'password': 'root', 'host':'localhost', 'port':__mysql_sandbox_port3});

session.close();
