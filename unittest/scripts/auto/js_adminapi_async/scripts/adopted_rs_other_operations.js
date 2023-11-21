//@ {VER(>=8.0.11)}

// Tests execution of replicaset operations on adopted replicasets

//@ INCLUDE async_utils.inc

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

//@ Status.
strip_status(r.status());

//@<> Stop replication on instance 2.
session2.runSql("STOP " + get_replica_keyword());

//@ Status, instance 2 OFFLINE.
strip_status(r.status());

//@ Rejoin instance 2 (succeed).
r.rejoinInstance(__sandbox2);

//@ Status (after rejoin).
strip_status(r.status());

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
