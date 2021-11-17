//@ {VER(>=8.0.11)}

// Tests createReplicaSet() specifically
// Only tests corner cases and negative cases since the positive ones will
// be tested everywhere else.

//@ INCLUDE async_utils.inc

//@ Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});

// enable interactive by default
shell.options['useWizards'] = true;

// Negative tests based on environment and params
//--------------------------------

//@ Bad options (should fail)
dba.createReplicaSet();
dba.createReplicaSet("bla");

shell.connect(__sandbox_uri1);
dba.createReplicaSet();
dba.createReplicaSet(null);
dba.createReplicaSet(123);
dba.createReplicaSet("my cluster");
dba.createReplicaSet("my:::cluster");
dba.createReplicaSet("my:::clus/ter");
// BUG#30466874: dba.create_cluster - cluster name can have 82 characters
dba.createReplicaSet("_1234567890::_1234567890123456789012345678901234567890123456789012345678901234");
// BUG#30466809 : dba.create_cluster("::") - cluster name cannot be empty
dba.createReplicaSet("::");

// on a disconnected session
session.close();
dba.createReplicaSet("aa");

//@<> check if innodb is forced for metadata schema BUG#32110085
shell.connect(__sandbox_uri1)
session.runSql("SET default_storage_engine = MyISAM");
EXPECT_NO_THROWS(function() { dba.createReplicaSet("myisamtest"); });

//@<> cleanup BUG#32110085
session.runSql("SET default_storage_engine = InnoDB");
reset_instance(session);

//@ create with unmanaged GR (should fail)
shell.connect(__sandbox_uri1);
start_standalone_gr(session, __mysql_sandbox_gr_port1);

dba.createReplicaSet("myrs");

stop_standalone_gr(session);

//@ create with unmanaged AR (should fail)
reset_instance(session);
setup_slave(session, __mysql_sandbox_port2);

dba.createReplicaSet("myrs");

//@ create with unmanaged AR from the master (should fail)
shell.connect(__sandbox_uri2);
dba.createReplicaSet("myrs");

//@ create with existing replicaset (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
// should work
rs = dba.createReplicaSet("myrs");
rs.disconnect();
// should fail
dba.createReplicaSet("myrs");

session.runSql("drop schema mysql_innodb_cluster_metadata");

//@ create with insufficient privs (should fail)
var rootsess = mysql.getSession(__sandbox_uri1);
rootsess.runSql("CREATE USER admin@'%'");
shell.connect("mysql://admin:@localhost:"+__mysql_sandbox_port1);
dba.createReplicaSet("myrs");

// grant a few more and try again
for (var i in kGrantsForPerformanceSchema) {
    var grant = kGrantsForPerformanceSchema[i];
    rootsess.runSql(grant);
}

dba.createReplicaSet("myrs");

//@ create with bad configs (should fail)
testutil.deployRawSandbox(__mysql_sandbox_port3, "root");

shell.connect(__sandbox_uri3);
dba.createReplicaSet("myrs");

testutil.destroySandbox(__mysql_sandbox_port3);

//@ replication filters (should fail)
shell.connect(__sandbox_uri1);
reset_instance(session);
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo)");
dba.createReplicaSet("myrs");

session.runSql("CHANGE MASTER TO master_host='localhost', master_port=/*(*/?/*)*/ FOR CHANNEL 'bla'", [__mysql_port]);
session.runSql("CHANGE REPLICATION FILTER REPLICATE_IGNORE_DB = (foo) FOR CHANNEL 'bla'");
dba.createReplicaSet("myrs");

//@ binlog filters (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_ignore_db", "igndata");
testutil.startSandbox(__mysql_sandbox_port1);

dba.createReplicaSet("myrs");

//@ Invalid loopback ip (should fail)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.1.1"});
dba.createReplicaSet("invalid_ip");
testutil.destroySandbox(__mysql_sandbox_port1);

// cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

// Positive tests for non-obvious cases
//--------------------------------

//@<> reprepare

// connect to the hostname
shell.connect(__hostname_uri1);

dba.createReplicaSet("myrs");
reset_instance(session);

//@ from a X protocol session
shell.connect("mysqlx://root:root@localhost:"+__mysql_sandbox_x_port1);
dba.createReplicaSet("myrs");

reset_instance(session);

//@ from a socket session {__os_type != 'windows' && !__replaying && !__recording}
shell.connect({
    "scheme": "mysql",
    "socket": get_sysvar(session, "datadir") + "/" + get_sysvar(session, "socket"),
    "user": "root",
    "password": "root"
});

dba.createReplicaSet("myrs");

reset_instance(session);

session.close();

// Options
//--------------------------------
// adopt is tested separately

//@<> re-deploy pre-configured instance
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);

//@ dryRun - should work, without changing the metadata
var snap = repl_snapshot(session);

dba.createReplicaSet("myrs", {dryRun:true});

EXPECT_EQ(snap, repl_snapshot(session));

//@ instanceLabel
rs = dba.createReplicaSet("myrs", {instanceLabel:"seed"});
session.runSql("SELECT instance_name FROM mysql_innodb_cluster_metadata.instances");

//@ gtidSetIsCompatible
session.runSql("SELECT attributes FROM mysql_innodb_cluster_metadata.clusters");

reset_instance(session);
dba.createReplicaSet("myrs", {gtidSetIsComplete:1});

session.runSql("SELECT attributes FROM mysql_innodb_cluster_metadata.clusters");

// XXX Rollback
//--------------------------------
// Ensure clean rollback after failures

// BUG#33574005: 1:1 mapping of Cluster:Metadata not enforced
// createReplicaSet() must always drop and re-created the MD schema to ensure older records are wiped out

//@<> createCluster() must drop and re-create the MD schema
reset_instance(session);

EXPECT_NO_THROWS(function() { rs = dba.createReplicaSet("first", {gtidSetIsComplete: true}); });

EXPECT_NO_THROWS(function() {rs.addInstance(__sandbox_uri2); });

EXPECT_NO_THROWS(function() {rs.removeInstance(__sandbox_uri2); });

shell.connect(__sandbox_uri2);
EXPECT_NO_THROWS(function() { rs = dba.createReplicaSet("second")});

EXPECT_EQ(1, session.runSql("select count(*) from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
