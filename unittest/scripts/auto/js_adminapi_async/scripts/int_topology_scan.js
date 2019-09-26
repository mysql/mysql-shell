//@ {VER(>=8.0.11)}
// These tests specifically test the topology scan for async replicasets
// It's used when adopting replicasets, but the tests are only for the
// topology detection itself.

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
testutil.deploySandbox(__mysql_sandbox_port4, "root");

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);
session4 = mysql.getSession(__sandbox_uri4);

//@ INCLUDE async_utils.inc

//@ master-slave from master
setup_slave(session2, __mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-slave from slave
shell.connect(__sandbox_uri2);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave1,slave2) from master
setup_slave(session3, __mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave1,slave2) from slave2
shell.connect(__sandbox_uri3);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave1-slave11,slave2) from master
setup_slave(session4, __mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave1-slave11,slave2) from slave2
shell.connect(__sandbox_uri3);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave1-slave11,slave2) from slave11
shell.connect(__sandbox_uri4);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master1-master2 from master1
reset_instance(session2);
reset_instance(session3);
reset_instance(session4);
setup_slave(session2, __mysql_sandbox_port1);
setup_slave(session1, __mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-master-master
reset_instance(session1);
reset_instance(session2);
setup_slave(session3, __mysql_sandbox_port2);
setup_slave(session2, __mysql_sandbox_port1);
setup_slave(session1, __mysql_sandbox_port3);

shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

// The following tests use non-default channels
//@ masterA-slaveA from master
reset_instance(session1);
reset_instance(session2);
reset_instance(session3);
setup_slave(session2, __mysql_sandbox_port1, "chan");

shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ masterA-slaveA from slave
shell.connect(__sandbox_uri2);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

// The following tests mix non-default channels with default

//@ master-(slave,slaveB) from master
setup_slave(session3, __mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave,slaveB) from slave
shell.connect(__sandbox_uri3);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ master-(slave,slaveB) from slaveB
shell.connect(__sandbox_uri2);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ masterA-slaveB-slave from slaveB
reset_instance(session1);
reset_instance(session2);
reset_instance(session3);
reset_instance(session4);
setup_slave(session2, __mysql_sandbox_port1, "chan");
setup_slave(session3, __mysql_sandbox_port2);

shell.connect(__sandbox_uri2);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@ masterA-slaveB-slave from slave
shell.connect(__sandbox_uri3);
dba.createReplicaSet("x", {adoptFromAR:1, dryRun:1});

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
