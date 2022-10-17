//@<> INCLUDE gr_utils.inc

//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {"manualStartOnBoot": true});

//@<> FR1 if a single member is RECOVERING and the rest is OFFLINE, the command must succeed
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP group_replication");

shell.connect(__sandbox_uri1);
session.runSql("CREATE SCHEMA test");
session.runSql("CREATE TABLE test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 50; i++) {
    session.runSql("INSERT INTO test.data VALUES(default, repeat('x', 4*1024*1024))");
}

session2.runSql("START group_replication");
session2.close();
testutil.waitMemberState(__mysql_sandbox_port2, "RECOVERING");

testutil.killSandbox(__mysql_sandbox_port3);
testutil.killSandbox(__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port3);
testutil.startSandbox(__mysql_sandbox_port1);

EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage(); });
EXPECT_OUTPUT_CONTAINS(`Stopping Group Replication on '${hostname}:${__mysql_sandbox_port2}'...`);

status = cluster.status();
EXPECT_EQ_ONEOF(["ONLINE", "RECOVERING"], status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"]);
EXPECT_EQ_ONEOF(["ONLINE", "RECOVERING"], status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"]);

//reset cluster / instances (to delete all the previously created data)
session.close();

shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

if (testutil.versionCheck(__version, ">=", "8.0.27"))
    scene.cluster = dba.createCluster("cluster", {gtidSetIsComplete: true, communicationStack: "XCOM"});
else
    scene.cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

scene.cluster.addInstance(__sandbox_uri2);
scene.cluster.addInstance(__sandbox_uri3);

//@<> FR2
dba.configureLocalInstance(__sandbox_uri1);
dba.configureLocalInstance(__sandbox_uri2);
dba.configureLocalInstance(__sandbox_uri3);

disable_auto_rejoin(__mysql_sandbox_port3);

testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2]);
testutil.killSandbox(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

EXPECT_THROWS(function(){ dba.rebootClusterFromCompleteOutage("cluster", {removeInstances:[`${hostname}:${__mysql_sandbox_port3}`], rejoinInstances:[`${hostname}:${__mysql_sandbox_port3}`]}); }, "Dba.rebootClusterFromCompleteOutage: Argument #2: Invalid options: rejoinInstances, removeInstances");
EXPECT_THROWS(function(){ dba.rebootClusterFromCompleteOutage("cluster", {removeInstances:[`${hostname}:${__mysql_sandbox_port3}`]}); }, "Dba.rebootClusterFromCompleteOutage: Argument #2: Invalid options: removeInstances");

EXPECT_THROWS(function(){
    dba.rebootClusterFromCompleteOutage();
}, "Could not determine if Cluster is completely OFFLINE");
EXPECT_OUTPUT_CONTAINS(`WARNING: One or more instances of the Cluster could not be reached and cannot be rejoined nor ensured to be OFFLINE: '${hostname}:${__mysql_sandbox_port3}'. Cluster may diverge and become inconsistent unless all instances are either reachable or certain to be OFFLINE and not accepting new transactions. You may use the 'force' option to bypass this check and proceed anyway.`);

testutil.startSandbox(__mysql_sandbox_port3);

EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("cluster", {force: true}); });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
status = cluster.status();
EXPECT_EQ(3, Object.keys(status["defaultReplicaSet"]["topology"]).length);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"]);

//@<> FR4 Check if fails due to divergent GTIDs
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("SET GLOBAL super_read_only=0");
session2.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
session2.runSql("SET GLOBAL super_read_only=1");
session2.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL super_read_only=0");
session.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
session.runSql("SET GLOBAL super_read_only=1");
session.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

EXPECT_THROWS(function(){
    dba.rebootClusterFromCompleteOutage();
}, "To reboot a Cluster with GTID conflits, both the 'force' and 'primary' options must be used to proceed with the command and to explicitly pick a new seed instance.");
EXPECT_OUTPUT_CONTAINS(`WARNING: Detected GTID conflits between instances: '${hostname}:${__mysql_sandbox_port1}', '${hostname}:${__mysql_sandbox_port2}'`);

testutil.wipeAllOutput();
EXPECT_THROWS(function(){
    dba.rebootClusterFromCompleteOutage("cluster", {force: true});
}, "To reboot a Cluster with GTID conflits, both the 'force' and 'primary' options must be used to proceed with the command and to explicitly pick a new seed instance.");

testutil.wipeAllOutput();
EXPECT_NO_THROWS(function(){ dba.rebootClusterFromCompleteOutage("cluster", {force: true, primary:`${hostname}:${__mysql_sandbox_port1}`, dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_NOT_CONTAINS("Restoring the Cluster 'cluster' from complete outage...");

testutil.wipeAllOutput();
EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster", {force: true, primary:`${hostname}:${__mysql_sandbox_port1}`}); });
EXPECT_OUTPUT_CONTAINS(`Not rejoining instance '${hostname}:${__mysql_sandbox_port2}' because its GTID set isn't compatible with '${hostname}:${__mysql_sandbox_port1}'`);

//restore cluster
cluster.removeInstance(__sandbox_uri2, {force: true});
session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);
session2.close();
cluster.addInstance(__sandbox_uri2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

status = cluster.status();
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["memberRole"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"]);

//@<> FR5 pick a new primary
testutil.stopGroup([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3]);

shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() {
    cluster = dba.rebootClusterFromCompleteOutage("cluster", {primary:`${hostname}:${__mysql_sandbox_port3}`});
});
EXPECT_OUTPUT_CONTAINS(`Switching over to instance '${hostname}:${__mysql_sandbox_port3}' to be used as seed.`);

status = cluster.status();
EXPECT_EQ("OK", status["defaultReplicaSet"]["status"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port3}`, status["defaultReplicaSet"]["primary"]);

shell.connect(__sandbox_uri3);

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> FR5.1 Check if new seed (with highest GTID) is choosen
shutdown_cluster(cluster);

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("SET GLOBAL super_read_only=0");
session2.runSql("CREATE SCHEMA errant_trx_db");
session2.runSql("SET GLOBAL super_read_only=1");
session2.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

shell.connect(__sandbox_uri1);
session.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster", {dryRun: true}); });
EXPECT_OUTPUT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_NOT_CONTAINS("Restoring the Cluster 'cluster' from complete outage...");
EXPECT_OUTPUT_CONTAINS(`Switching over to instance '${hostname}:${__mysql_sandbox_port2}' (which has the highest GTID set), to be used as seed.`);
EXPECT_OUTPUT_NOT_CONTAINS(`${hostname}:${__mysql_sandbox_port2} was restored.`);
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port1}' was successfully rejoined to the cluster.`);
EXPECT_OUTPUT_CONTAINS("dryRun finished.");

testutil.wipeAllOutput();
EXPECT_NO_THROWS(function(){ cluster = dba.rebootClusterFromCompleteOutage("cluster", {dryRun: false}); });
EXPECT_OUTPUT_NOT_CONTAINS("NOTE: dryRun option was specified. Validations will be executed, but no changes will be applied.");
EXPECT_OUTPUT_CONTAINS(`Switching over to instance '${hostname}:${__mysql_sandbox_port2}' (which has the highest GTID set), to be used as seed.`);
EXPECT_OUTPUT_NOT_CONTAINS("dryRun finished.");

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
status = cluster.status();
EXPECT_EQ("PRIMARY", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberRole"]);

//@<> FR5.3 Must fail if we choose an instance with a lower GTID
shutdown_cluster(cluster);

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("SET GLOBAL super_read_only=0");
session3.runSql("DROP SCHEMA IF EXISTS errant_trx_db");
session3.runSql("SET GLOBAL super_read_only=1");
session3.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

shell.connect(__sandbox_uri1);
session.runSql("SELECT @@GTID_EXECUTED").fetchOne()[0];

EXPECT_THROWS(function(){
    dba.rebootClusterFromCompleteOutage("cluster", {primary:`${hostname}:${__mysql_sandbox_port1}`});
}, `The requested instance '${hostname}:${__mysql_sandbox_port1}' can't be used as the new seed because it has a lower GTID when compared with the other members: '${hostname}:${__mysql_sandbox_port3}'. Use option 'force' if this is indeed the desired behaviour.`);

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function(){
    cluster = dba.rebootClusterFromCompleteOutage("cluster", {force: true, primary:`${hostname}:${__mysql_sandbox_port1}`});
});
EXPECT_OUTPUT_CONTAINS(`Not rejoining instance '${hostname}:${__mysql_sandbox_port3}' because its GTID set isn't compatible with '${hostname}:${__mysql_sandbox_port1}'`);
EXPECT_OUTPUT_CONTAINS("The Cluster was successfully rebooted.");

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

status = cluster.status();
EXPECT_EQ("OK_NO_TOLERANCE_PARTIAL", status["defaultReplicaSet"]["status"]);
EXPECT_EQ("Cluster is NOT tolerant to any failures. 1 member is not active.", status["defaultReplicaSet"]["statusText"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port1}`, status["defaultReplicaSet"]["primary"]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
