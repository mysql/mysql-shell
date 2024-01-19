//@ {VER(>=8.0.11)}

// Tests execution of replicaset operations on adopted replicasets

//@<> INCLUDE async_utils.inc

//@<> Setup.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});

var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> Create a custom replicaset.
// BUG#30505897 : rejoin_instance fails when replicaset was adopted
session1.runSql("CREATE USER rpl@'%' IDENTIFIED BY 'rpl'");
session1.runSql("GRANT REPLICATION SLAVE ON *.* TO rpl@'%'");
session2.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_HOST='"+hostname_ip+"', " + get_replication_option_keyword() + "_PORT="+__mysql_sandbox_port1+", " + get_replication_option_keyword() + "_USER='rpl', " + get_replication_option_keyword() + "_PASSWORD='rpl', " + get_replication_option_keyword() + "_AUTO_POSITION=1, get_" + get_replication_option_keyword() + "_public_key=1");
session2.runSql("START " + get_replica_keyword());
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@<> Adopt custom replicaset.
shell.connect(__sandbox_uri1);
var r = dba.createReplicaSet("R", {"adoptFromAR": true});

//@<> Status
status = r.status();
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, status.replicaSet.primary);
EXPECT_EQ("AVAILABLE", status.replicaSet.status);

instance1 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port1}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, instance1.address);
EXPECT_EQ("PRIMARY", instance1.instanceRole);
EXPECT_EQ("ONLINE", instance1.status);

instance2 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port2}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port2}`, instance2.address);
EXPECT_EQ("SECONDARY", instance2.instanceRole);
EXPECT_EQ("ONLINE", instance2.status);

//@<> Stop replication on instance 2.
session2.runSql("STOP " + get_replica_keyword());

//@<> Status, instance 2 OFFLINE
status = r.status();
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, status.replicaSet.primary);
EXPECT_EQ("AVAILABLE_PARTIAL", status.replicaSet.status);

instance1 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port1}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, instance1.address);
EXPECT_EQ("PRIMARY", instance1.instanceRole);
EXPECT_EQ("ONLINE", instance1.status);

instance2 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port2}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port2}`, instance2.address);
EXPECT_EQ("SECONDARY", instance2.instanceRole);
EXPECT_EQ("OFFLINE", instance2.status);

//@<> Rejoin instance 2 (succeed).
EXPECT_NO_THROWS(function(){ r.rejoinInstance(__sandbox2); });
EXPECT_OUTPUT_CONTAINS_MULTILINE(`* Validating instance...

This instance reports its own address as ${hostname_ip}:${__mysql_sandbox_port2}
${hostname_ip}:${__mysql_sandbox_port2}: Instance configuration is suitable.
** Checking transaction state of the instance...
The safest and most convenient way to provision a new instance is through automatic clone provisioning, which will completely overwrite the state of '${hostname_ip}:${__mysql_sandbox_port2}' with a physical snapshot from an existing replicaset member. To use this method by default, set the 'recoveryMethod' option to 'clone'.

WARNING: It should be safe to rely on replication to incrementally recover the state of the new instance if you are sure all updates ever executed in the replicaset were done with GTIDs enabled, there are no purged transactions and the new instance contains the same GTID set as the replicaset or a subset of it. To use this method by default, set the 'recoveryMethod' option to 'incremental'.

Incremental state recovery was selected because it seems to be safely usable.

* Rejoining instance to replicaset...
** Changing replication source of ${hostname_ip}:${__mysql_sandbox_port2} to ${hostname_ip}:${__mysql_sandbox_port1}
** Checking replication channel status...
** Waiting for rejoined instance to synchronize with PRIMARY...


* Updating the Metadata...
The instance '${hostname_ip}:${__mysql_sandbox_port2}' rejoined the replicaset and is replicating from '${hostname_ip}:${__mysql_sandbox_port1}'.
`);

//@<> Status (after rejoin)
status = r.status();
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, status.replicaSet.primary);
EXPECT_EQ("AVAILABLE", status.replicaSet.status);

instance1 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port1}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port1}`, instance1.address);
EXPECT_EQ("PRIMARY", instance1.instanceRole);
EXPECT_EQ("ONLINE", instance1.status);

instance2 = status.replicaSet.topology[`${hostname_ip}:${__mysql_sandbox_port2}`]
EXPECT_EQ(`${hostname_ip}:${__mysql_sandbox_port2}`, instance2.address);
EXPECT_EQ("SECONDARY", instance2.instanceRole);
EXPECT_EQ("ONLINE", instance2.status);

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
