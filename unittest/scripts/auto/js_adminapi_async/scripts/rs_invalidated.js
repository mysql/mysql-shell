//@ {VER(>=8.0.11)}

// Various operations while an instance is invalidated

//@ INCLUDE async_utils.inc

//@<> Setup

// Set report_host to a valid value, in case hostname is bogus
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"report_host": hostname_ip});


rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.waitForReplConnectionError(__mysql_sandbox_port2, "");
testutil.waitForReplConnectionError(__mysql_sandbox_port3, "");

// faster connection timeouts
shell.options["connectTimeout"] = 1.0;
shell.options["dba.connectTimeout"] = 1.0;

//@ Check status from a member
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();

rs.status();

//@ Try addInstance (should fail)
rs.addInstance(__sandbox_uri2);

//@ Try removeInstance (should fail)
rs.removeInstance(__sandbox_uri2);

//@ Try setPrimary (should fail)
rs.setPrimaryInstance(__sandbox_uri2);

//@ force failover
shell.connect(__sandbox_uri2);
rs2 = dba.getReplicaSet();

rs2.forcePrimaryInstance(__sandbox_uri2);
rs2.status();

//@ Restart invalidated member and check status from the current primary
testutil.startSandbox(__mysql_sandbox_port1);

rs2.status();

//@ same from the invalidated primary
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();
rs.status();

//@ do stuff in the old primary to simulate split-brain
session.runSql("create schema newdb");

status = rs.status();

// status from the invalidated member
EXPECT_EQ(status.replicaSet.primary, rs2.status().replicaSet.primary);
rs2.status();

// errant GTID set should be included in the status
// covers Bug#30440392 REPLICASET: INVALIDATED MEMBERS SHOULD REPORT ERRANT GTIDSET
EXPECT_EQ("INCONSISTENT", status.replicaSet.topology[hostname_ip+":"+__mysql_sandbox_port1].transactionSetConsistencyStatus);
EXPECT_NE("", status.replicaSet.topology[hostname_ip+":"+__mysql_sandbox_port1].transactionSetErrantGtidSet);

//@ remove the invalidated member while it's up (should fail)
rs2.removeInstance(__sandbox_uri1);

rs2.status();

//@ remove the invalidated member while it's up with force
rs2.removeInstance(__sandbox_uri1, {force:true});

rs2.status();

//@ add back the split-brained instance (should fail)
rs2.addInstance(__sandbox_uri1);

//@ add back the split-brained instance after re-building it
shell.connect(__sandbox_uri1);

// Note: reset_instance() won't work because it would preserve the server_id
// If server_id doesn't change, the instance won't get back the transactions
// that originated at it, because of some protection against duplicate
// transactions in circular replication.
//reset_instance(session);
testutil.destroySandbox(__mysql_sandbox_port1);
// we need a unique server_id (default is the port#)
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_id:123});
rs2.addInstance(__sandbox_uri1);

//@<> re-prepare but invalidated sb3 now
// This covers bug#30735124, where rejoining an invalidated primary would fail if it's the last member
rs = rebuild_rs();
rs.setPrimaryInstance(__sandbox_uri3);
testutil.stopSandbox(__mysql_sandbox_port3);
shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();
rs.forcePrimaryInstance(__sandbox_uri1);
testutil.startSandbox(__mysql_sandbox_port3);

//@ status should show sb1 as PRIMARY
// before bugfix, this would show sb3 as the PRIMARY
rs.status();

//@ connect to the invalidated PRIMARY and check status from there
shell.connect(__sandbox_uri3);
rs3= dba.getReplicaSet();
rs3.status();

//@ rejoinInstance() the invalidated PRIMARY
rs.rejoinInstance(__sandbox_uri3);
rs.status();

//@<> re-prepare
rs = rebuild_rs();
testutil.stopSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
rs.forcePrimaryInstance(__sandbox_uri2);

//@ remove the invalidated instance while it's down (should fail)
rs.removeInstance(__sandbox_uri1);

rs.status();

//@ add back the removed instance while it's down (should fail)
rs.addInstance(__sandbox_uri1);

//@ add back the removed instance after bringing it back up (should fail)
// the instance still belongs to the replicaset
testutil.startSandbox(__mysql_sandbox_port1);
rs.addInstance(__sandbox_uri1);

rs.status();

//====== another secondary also invalidated

//@<> re-prepare again
rs = rebuild_rs();
// stop the primary
testutil.stopSandbox(__mysql_sandbox_port1);
// stop a secondary
testutil.stopSandbox(__mysql_sandbox_port2);

//@ promote remaining secondary (should fail)
shell.connect(__sandbox_uri3);
rs = dba.getReplicaSet();
rs.status();

rs.forcePrimaryInstance(__sandbox_uri3);

//@ promote remaining secondary with invalidateErrorInstances
rs.forcePrimaryInstance(__sandbox_uri3, {invalidateErrorInstances:1});

rs.status();

//@ connect to invalidated instances and try to removeInstance (should fail)
testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
rs.removeInstance(__sandbox_uri3);

//@ setPrimary (should fail)
rs.setPrimaryInstance(__sandbox_uri2);

//@ setPrimary with new rs (should fail)
// test with an old and new rs because the obj will auto-reconnected to the primary
rs = dba.getReplicaSet();
rs.setPrimaryInstance(__sandbox_uri2);

//@ forcePrimary while there's already a primary (should fail)
rs.forcePrimaryInstance(__sandbox_uri2);

//@ forcePrimary with new rs while there's already a primary (should fail)
rs = dba.getReplicaSet();
rs.forcePrimaryInstance(__sandbox_uri2);

//@ status
rs.status()

//@ status with new object
rs = dba.getReplicaSet();
rs.status()

//@ remove invalidated secondary
shell.connect(__sandbox_uri3);
rs = dba.getReplicaSet();
rs.removeInstance(__sandbox_uri2);

rs.status();

//@ Try to re-add invalidated instance (should fail)
rs.addInstance(__sandbox_uri1);

//@ Promote invalidated (should fail)
rs.setPrimaryInstance(__sandbox_uri1);

//@ remove invalidated primary
rs.removeInstance(__sandbox_uri1, {force:1});

//@ add back invalidated primary
rs.addInstance(__sandbox_uri1);

//@# add back other invalidated primary
rs.addInstance(__sandbox_uri2);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port3);
testutil.waitMemberTransactions(__mysql_sandbox_port1, __mysql_sandbox_port3);

//@ final status
rs.status();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
