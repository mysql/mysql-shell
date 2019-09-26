//@ {VER(<8.0.11)}

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");

//@ Unsupported server version (should fail)
dba.createReplicaSet("myrs");
dba.getReplicaSet();
dba.configureReplicaSetInstance();

shell.connect(__sandbox_uri1);
dba.createReplicaSet("myrs");
dba.getReplicaSet();
dba.configureReplicaSetInstance();

session.close();
dba.createReplicaSet("aa");
dba.getReplicaSet();
dba.configureReplicaSetInstance();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
