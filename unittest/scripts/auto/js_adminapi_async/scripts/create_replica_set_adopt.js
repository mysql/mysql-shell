//@ {VER(>=8.0.11)}

// Tests createReplicaSet() for adoption specifically

//@ INCLUDE async_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");

// enable interactive by default
shell.options.useWizards = true;

var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

// Negative tests based on environment and params
//--------------------------------

shell.connect(__sandbox_uri1);

//@ on a standalone server (should fail)
dba.createReplicaSet("aa", {adoptFromAR:true});

//@ Bad options (should fail)
dba.createReplicaSet(0, {adoptFromAR:true});
dba.createReplicaSet("myrs", {adoptFromAR:true, instanceLabel:"label"});

// on a disconnected session
session.close();
dba.createReplicaSet("aa", {adoptFromAR:true});

//@ adopt with InnoDB cluster (should fail)
shell.connect(__sandbox_uri1);
c = dba.createCluster("bla");
c.disconnect();
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ indirect adopt with InnoDB cluster (should fail)
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});

reset_instance(session2);
shell.connect(__sandbox_uri1);

//@ adopt with unmanaged GR (should fail)
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ indirect adopt with unmanaged GR (should fail)
setup_slave(session2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});
reset_instance(session2);
shell.connect(__sandbox_uri1);

//@ adopt with existing replicaset (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
// should work
rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:1});
rs.addInstance(__sandbox_uri2);
rs.disconnect();
// should fail
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ indirect adopt with existing replicaset (should fail)
shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});
reset_instance(session2);
shell.connect(__sandbox_uri1);

session.runSql("drop schema mysql_innodb_cluster_metadata");

//@ adopt with insufficient privs (should fail)
var rootsess = mysql.getSession("mysql://root:root@localhost:"+__mysql_sandbox_port1);

reset_instance(session);
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);

rootsess.runSql("CREATE USER admin@'%'");
shell.connect("mysql://admin:@localhost:"+__mysql_sandbox_port1);

dba.createReplicaSet("myrs", {adoptFromAR:true});

// grant a few more and try again
for (var i in kGrantsForPerformanceSchema) {
    var grant = kGrantsForPerformanceSchema[i];
    rootsess.runSql(grant);
}

dba.createReplicaSet("myrs", {adoptFromAR:true});

// grant privs on MD
rootsess.runSql("GRANT ALL ON mysql_innodb_cluster_metadata.* TO admin@'%'");

// while connected by hostname
shell.connect("mysql://admin:@"+hostname_ip+":"+__mysql_sandbox_port1);

dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ adopt with existing old metadata, belongs to other (should fail)
shell.connect(__sandbox_uri1);
session.runSql("create schema mysql_innodb_cluster_metadata");
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-1.0.1-cluster_1member.sql", "mysql_innodb_cluster_metadata");

dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ indirect adopt with existing old metadata, belongs to other (should fail)
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});
// revert
shell.connect(__sandbox_uri1);
reset_instance(session);
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);

//@ replication filters (should fail)
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo)");
dba.createReplicaSet("myrs", {adoptFromAR:true});

session.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/ FOR CHANNEL 'bla'", [__mysql_port]);
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo) FOR CHANNEL 'bla'");
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ binlog filters (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_ignore_db", "igndata");
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
reset_instance(session);
reset_instance(session2);
setup_slave(session2, __mysql_sandbox_port1);

dba.createReplicaSet("myrs", {adoptFromAR:true});

// cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

//@ broken repl (should fail)
shell.connect(__sandbox_uri2);
session.runSql("STOP SLAVE");
session.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='badpass', MASTER_AUTO_POSITION=1, MASTER_SSL=1", [__mysql_sandbox_port1]);
session.runSql("START SLAVE");
dba.createReplicaSet("myrs", {adoptFromAR:true});

reset_instance(session);

//@ bad repl channels - bad name (should fail)
shell.connect(__sandbox_uri1);
reset_instance(session);
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root' FOR CHANNEL 'bla'", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE FOR CHANNEL 'bla'");

// this should fail because of the channel name
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ bad repl channels - 2 channels (should fail)
setup_slave(session2, __mysql_sandbox_port1);
dba.createReplicaSet("myrs", {adoptFromAR:true});

session2.runSql("STOP SLAVE FOR CHANNEL 'bla'");
reset_instance(session2);

//@ bad repl channels - master has a bogus channel
setup_slave(session, __mysql_sandbox_port3, "foob");
setup_slave(session2, __mysql_sandbox_port1);
dba.createReplicaSet("myrs", {adoptFromAR:true});

session.runSql("STOP SLAVE FOR CHANNEL 'foob'");
reset_instance(session);
reset_instance(session2);

//@ admin account has mismatched passwords (should fail)
session.runSql("SET sql_log_bin=0");
session.runSql("CREATE USER rooty@localhost IDENTIFIED BY 'p1'");
session.runSql("CREATE USER rootx@'%' IDENTIFIED BY 'p1'");
session.runSql("GRANT ALL ON *.* to rooty@localhost WITH GRANT OPTION");
session.runSql("SET sql_log_bin=1");

session2.runSql("SET sql_log_bin=0");
session2.runSql("CREATE USER rooty@localhost IDENTIFIED BY 'p2'");
session2.runSql("CREATE USER rootx@localhost IDENTIFIED BY 'p1'");
session2.runSql("GRANT ALL ON *.* to rooty@localhost WITH GRANT OPTION");
session2.runSql("SET sql_log_bin=1");

setup_slave(session2, __mysql_sandbox_port1);
shell.connect("rooty:p1@localhost:"+__mysql_sandbox_port1);
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ admin account passwords match, but they don't allow connection from the shell (should fail)
shell.connect("rootx:p1@"+hostname+":"+__mysql_sandbox_port1);
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ invalid topology: master-master (should fail)
shell.connect(__sandbox_uri1);
setup_slave(session, __mysql_sandbox_port2);

dba.createReplicaSet("myrs", {adoptFromAR:true});

reset_instance(session2);
reset_instance(session);

//@ invalid topology: master-master-master (should fail)
setup_slave(session2, __mysql_sandbox_port1);
setup_slave(session3, __mysql_sandbox_port2);
setup_slave(session, __mysql_sandbox_port3);

dba.createReplicaSet("myrs", {adoptFromAR:true});

reset_instance(session2);
reset_instance(session3);
reset_instance(session);

//@ invalid topology: master-master-slave (should fail)
setup_slave(session2, __mysql_sandbox_port1);
setup_slave(session3, __mysql_sandbox_port2);
setup_slave(session, __mysql_sandbox_port2);

dba.createReplicaSet("myrs", {adoptFromAR:true});

reset_instance(session2);
reset_instance(session3);
reset_instance(session);
// restart needed to clear SHOW SLAVE HOSTS
testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

//@ bad configs: SBR (should fail)
session.runSql("SET GLOBAL binlog_format='STATEMENT'");
session2.runSql("SET GLOBAL binlog_format='STATEMENT'");
setup_slave(session2, __mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
dba.createReplicaSet("myrs", {adoptFromAR:true});

shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});

shell.connect(__sandbox_uri1);
reset_instance(session2);
session.runSql("SET GLOBAL binlog_format='ROW'");
session2.runSql("SET GLOBAL binlog_format='ROW'");

// bad configs: duplicate server_id (should fail) -- server will reject this itself

//@ bad configs: filepos replication (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {gtid_mode:'OFF', server_id:1});
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deployRawSandbox(__mysql_sandbox_port2, "root", {gtid_mode:'OFF', server_id:2});
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root'", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:true});


testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
session1 = mysql.getSession(__sandbox_uri1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
testutil.restartSandbox(__mysql_sandbox_port3);
session3 = mysql.getSession(__sandbox_uri3);

// unsupported replication options

//@ unsupported option: SSL (should fail)
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_ssl=1", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: SSL=0
session2.runSql("STOP SLAVE");
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_ssl=0", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

reset_instance(session1);
reset_instance(session2);

//@ unsupported option: delay (should fail)
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_delay=5", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: connect_retry (should fail)
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_connect_retry=4", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: retry_count (should fail)
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_retry_count=3", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: heartbeat (should fail)
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, master_heartbeat_period=32", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: no auto_position (should fail)
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root'", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ unsupported option: MASTER_COMPRESSION_ALGORITHMS (should fail) {VER(>=8.0.18)}
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, MASTER_COMPRESSION_ALGORITHMS='zstd'", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

//@ supported option: GET_MASTER_PUBLIC_KEY
reset_instance(session2);
session2.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/, master_user='root', master_password='root', master_auto_position=1, GET_MASTER_PUBLIC_KEY=1", [__mysql_sandbox_port1]);
session2.runSql("START SLAVE");

dba.createReplicaSet("myrs", {adoptFromAR:1});

reset_instance(session1);
reset_instance(session2);

// Positive tests
//--------------------------------

//@ adopt master-slave requires connection to the master (should fail)
setup_slave(session2, __mysql_sandbox_port1);

var slaves1 = session.runSql("SELECT * FROM performance_schema.replication_connection_configuration").fetchAll();
var slaves2 = session2.runSql("SELECT * FROM performance_schema.replication_connection_configuration").fetchAll();

shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs", {adoptFromAR:true});

//@ adopt master-slave
shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {adoptFromAR:true});

// check that repl configs didn't change
EXPECT_EQ(slaves1, session.runSql("SELECT * FROM performance_schema.replication_connection_configuration").fetchAll());
EXPECT_EQ(slaves2, session2.runSql("SELECT * FROM performance_schema.replication_connection_configuration").fetchAll());

rs.status();

reset_instance(session);
reset_instance(session2);

//@ adopt master-slave,slave
setup_slave(session2, __mysql_sandbox_port1);
setup_slave(session3, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
var rs = dba.createReplicaSet("myrs", {adoptFromAR:true});
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);
rs.status();

reset_instance(session);
reset_instance(session2);
reset_instance(session3);

// Options
//--------------------------------

//@ dryRun - should work, without changing the metadata
testutil.restartSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);
setup_slave(session2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

// save state of replication settings
var snap1 = repl_snapshot(session);
var snap2 = repl_snapshot(session2);

dba.createReplicaSet("myrs", {adoptFromAR:true, dryRun:true});

function clean(snap) {
    copy = JSON.parse(JSON.stringify(snap));
    copy["slave_master_info"][0]["Master_log_name"] = null;
    return copy;
}

// ensure nothing changed
EXPECT_JSON_EQ(snap1, repl_snapshot(session));

EXPECT_JSON_EQ(clean(snap2), clean(repl_snapshot(session2)));


reset_instance(session);
reset_instance(session2);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
