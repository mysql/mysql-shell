// Assumptions: smart deployment routines available
//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, server_id:111});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname, server_id:222});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname, server_id:333});
var add_instance_options = {host:localhost, port: 0000, password:'root', scheme:'mysql'};

//@<> connect to instance
shell.connect(__sandbox_uri1);
var singleSession = session;

//@<> create first cluster
var single;
EXPECT_NO_THROWS(function(){ single = dba.createCluster('single', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true}); });
EXPECT_OUTPUT_NOT_CONTAINS("WARNING: Option 'memberSslMode' is deprecated");

//@<> Success adding instance
EXPECT_NO_THROWS(function(){ single.addInstance(__sandbox_uri2); });

// Wait for the second added instance to fetch all the replication data
testutil.waitMemberTransactions(__mysql_sandbox_port2);

//@ Check that recovery account looks OK and that it was stored in the metadata and that it was configured
shell.dumpRows(session.runSql("SELECT user,host FROM mysql.user WHERE user like 'mysql_inno%'"), "tabbed");
shell.dumpRows(session.runSql("SELECT instance_name,attributes FROM mysql_innodb_cluster_metadata.instances ORDER BY instance_id"), "tabbed");
shell.dumpRows(session.runSql("SELECT user_name FROM mysql.slave_master_info WHERE channel_name='group_replication_recovery'"), "tabbed");

//@<> clear out recoveryAccount data {VER(>=8.0.17)}
// BUG#32157182
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.recoveryAccountUser', '$.recoveryAccountHost')");

//@<> add new instance to the cluster {VER(>=8.0.17)}
//BUG#32157182

// check status during clone
var pid = testutil.callMysqlshAsync(["--js", __sandbox_uri1, "--cluster", "-e", `os.sleep(5); s=cluster.status()['defaultReplicaSet']['topology']['${hostname}:${__mysql_sandbox_port3}']; print(s.address, '=', s.mode, '/', s.role, '/', s.status, '\\n');`]);

single.addInstance(__sandbox_uri3, {label: '2node2', recoveryMethod: "clone"});
EXPECT_STDOUT_CONTAINS("Unable to enable clone on the instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>': MetadataError: The replication recovery account in use by '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not stored in the metadata. Use cluster.rescan() to update the metadata.")
EXPECT_STDOUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' was successfully added to the cluster.`);

testutil.waitMysqlshAsync(pid);

WIPE_STDOUT();

//@<> cleanup {VER(>=8.0.17)}
//BUG#32157182
single.removeInstance(__sandbox_uri3);

//@<> create cluster with unsupported recovery user
//BUG#32157182
single.dissolve();
var single;
if (__version_num < 80027) {
  single = dba.createCluster("c", {gtidSetIsComplete: true});
} else {
    single = dba.createCluster("c", {gtidSetIsComplete: true, communicationStack: "xcom"});
}
single.addInstance(__sandbox_uri2);

shell.connect(__sandbox_uri1);
session.runSql("CREATE USER rpl_user@'%' IDENTIFIED BY 'rpl_user'");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO rpl_user@'%'");
session.runSql("FLUSH PRIVILEGES");
shell.connect(__sandbox_uri2);
session.runSql("STOP GROUP_REPLICATION");
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("SET GLOBAL super_read_only = off");
session.runSql("change " + get_replication_source_keyword() + " TO " + get_replication_option_keyword() + "_USER='rpl_user', " + get_replication_option_keyword() + "_PASSWORD='rpl_user' FOR CHANNEL 'group_replication_recovery'");
session.runSql("START GROUP_REPLICATION");
EXPECT_STDERR_EMPTY();
shell.connect(__sandbox_uri1);
print(__sandbox_uri2);
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = json_remove(attributes, '$.recoveryAccountUser', '$.recoveryAccountHost') WHERE address='"+hostname+":"+__mysql_sandbox_port2+"'");
var status = single.status();
EXPECT_EQ("WARNING: Unsupported recovery account 'rpl_user' has been found for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Operations such as Cluster.resetRecoveryAccountsPassword() and Cluster.addInstance() may fail. Please remove and add the instance back to the Cluster to ensure a supported recovery account is used.", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0])
//@<> try to add instance to the cluster with unsupported recovery user using clone method {VER(>=8.0.17)}
single.addInstance(__sandbox_uri3, {recoveryMethod: "clone"});
EXPECT_STDOUT_CONTAINS("ERROR: Unsupported recovery account has been found for instance <<<hostname>>>:<<<__mysql_sandbox_port2>>>. Operations such as <Cluster>.resetRecoveryAccountsPassword() and <Cluster>.addInstance() may fail. Please remove and add the instance back to the Cluster to ensure a supported recovery account is used.");

//@<> cleanup cluster and recreate
//BUG#32157182
single.dissolve();
single = dba.createCluster('single', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});
single.addInstance(__sandbox_uri2);
// deploy another

//@<> Connect to the future new seed node
shell.connect(__sandbox_uri2);
var singleSession2 = session;

//@ Check auto_increment values for single-primary
// TODO(alfredo) this test currently fails (never worked?) because of Bug #27084767
//var row = singleSession.runSql("select @@auto_increment_increment, @@auto_increment_offset").fetchOne();
//testutil.expectEq(1, row[0]);
//testutil.expectEq(2, row[1]);
//var row = singleSession2.runSql("select @@auto_increment_increment, @@auto_increment_offset").fetchOne();
//testutil.expectEq(1, row[0]);
//testutil.expectEq(2, row[1]);

single.disconnect();

// Kill the seed instance
testutil.killSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING),UNREACHABLE");

//@ Get the cluster back
var single = dba.getCluster();

//@ Restore the quorum
single.forceQuorumUsingPartitionOf({host: localhost, port: __mysql_sandbox_port2, user: 'root', password:'root'});

//@ Success adding instance to the single cluster
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

// wait until SRO is cleared
session2 = mysql.getSession(__sandbox_uri2);
while (1) {
  if (!session2.runSql("SELECT @@super_read_only").fetchOne()[0]) {
    break;
  }
  os.sleep(1);
}

single.addInstance(__sandbox_uri3);
//@ Remove the instance from the cluster
single.removeInstance({host: localhost, port: __mysql_sandbox_port3});

//@ create second cluster
shell.connect(__sandbox_uri3);
var multiSession = session;

// We must use clearReadOnly because the instance 3 was removed from the cluster before
// (BUG#26422638)
var multi = dba.createCluster('multi', {memberSslMode:'REQUIRED', multiPrimary:true, force:true, gtidSetIsComplete: true});

//@ Failure adding instance from multi cluster into single
add_instance_options['port'] = __mysql_sandbox_port3;
add_instance_options['user'] = 'root';
single.addInstance(add_instance_options);

// Drops the metadata on the multi cluster letting a non managed replication group
dba.dropMetadataSchema({force:true});

//@ Failure adding instance from an unmanaged replication group into single
add_instance_options['port'] = __mysql_sandbox_port3;
single.addInstance(add_instance_options);

//@ Failure adding instance already in the InnoDB cluster
add_instance_options['port'] = __mysql_sandbox_port2;
single.addInstance(add_instance_options);

//@<> Failure adding instance already in the InnoDB cluster but (MISSING)
testutil.startSandbox(__mysql_sandbox_port1);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");

EXPECT_THROWS_TYPE(function() { single.addInstance(__sandbox_uri1)}, "The instance '" + __endpoint1 + "' is already part of this InnoDB Cluster", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${__endpoint1}' is already part of this InnoDB Cluster but is (MISSING). Please use <Cluster>.rejoinInstance() to rejoin it.`);

//@<> don't check for tables with PK BUG#32992693 {VER(>=8.0.17)}
reset_instance(session); //reset instance 3
session.runSql("CREATE SCHEMA test;");
session.runSql("CREATE TABLE test.t1 (id int);");
session.runSql("INSERT INTO test.t1 VALUES (42);");

EXPECT_NO_THROWS(function() { single.addInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
EXPECT_STDOUT_NOT_CONTAINS("The following tables do not have a Primary Key or equivalent column:");

//@<> cleanup old clusters and sessions
multiSession.close();
multi.disconnect();
singleSession.close();
singleSession2.close();
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
