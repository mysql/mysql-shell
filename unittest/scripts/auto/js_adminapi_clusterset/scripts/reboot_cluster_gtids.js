//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup + Create primary cluster + add Replica Cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session
var cluster = scene.cluster
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port6, "root", {report_host: hostname});

cs = cluster.createClusterSet("domain");

replica = cs.createReplicaCluster(__sandbox_uri4, "replica");
replica.addInstance(__sandbox_uri5);
replica.addInstance(__sandbox_uri6);

CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4, __sandbox_uri5, __sandbox_uri6], cluster, replica);
CHECK_CLUSTER_SET(session);

//<> FR6.1.3: The Cluster must not be holding any Metadata or InnoDB transaction lock

//<> FR6.1.4: The Cluster must not contain errant transactions that are not identified as VCLEs
shell.connect(__sandbox_uri4);

session.runSql("stop replica for channel 'clusterset_replication'");
session.runSql("STOP group_replication"); //view-change-events
session.runSql("START group_replication");

testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS("* Reconciling ");
EXPECT_OUTPUT_CONTAINS(" internally generated GTIDs")
EXPECT_OUTPUT_CONTAINS("Rejoining Cluster into its original ClusterSet...");

//create errant transactions
session.runSql("set global super_read_only=0");
session.runSql("set @gtid_before=@@gtid_executed");
session.runSql("create schema diverger");
session.runSql("set global super_read_only=1");

testutil.waitMemberTransactions(__mysql_sandbox_port5, __mysql_sandbox_port4);
testutil.waitMemberTransactions(__mysql_sandbox_port6, __mysql_sandbox_port4);

testutil.stopGroup([__mysql_sandbox_port4, __mysql_sandbox_port5, __mysql_sandbox_port6]);

testutil.wipeAllOutput();

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });
EXPECT_OUTPUT_CONTAINS("Cluster 'replica' cannot be rejoined because it contains transactions that did not originate from the primary of the clusterset.");
EXPECT_OUTPUT_CONTAINS("WARNING: Unable to rejoin Cluster to the ClusterSet because this Cluster has errant transactions that did not originate from the primary Cluster of the ClusterSet.");

testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");
wait_channel_ready(session, __mysql_sandbox_port4, "clusterset_replication");
testutil.waitMemberTransactions(__mysql_sandbox_port4, __mysql_sandbox_port1);

//reset cluster set

shell.connect(__sandbox_uri1);
cset = dba.getClusterSet();
cset.removeCluster("replica");

var session4 = mysql.getSession(__sandbox_uri4);
var session5 = mysql.getSession(__sandbox_uri5);
var session6 = mysql.getSession(__sandbox_uri6);

reset_instance(session4);
reset_instance(session5);
reset_instance(session6);

EXPECT_NO_THROWS(function(){ replica = cset.createReplicaCluster(__sandbox_uri4, "replica", {recoveryMethod:"clone"}); });
EXPECT_NO_THROWS(function(){ replica.addInstance(__sandbox_uri5, {recoveryMethod:"clone"}); });
EXPECT_NO_THROWS(function(){ replica.addInstance(__sandbox_uri6, {recoveryMethod:"clone"}); });

shell.connect(__sandbox_uri4);
testutil.waitMemberState(__mysql_sandbox_port5, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port6, "ONLINE");

//<> FR6.1.5: The Cluster's executed transaction set (GTID_EXECUTED) must not be empty
// The replica cluster has more than one instance so rebootClusterFromCompleteOutage should pick the best one (in terms of GTID)

testutil.stopGroup([__mysql_sandbox_port5,__mysql_sandbox_port6]);

shell.connect(__sandbox_uri4);
session.runSql("STOP replica FOR CHANNEL 'clusterset_replication'");

session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("STOP group_replication;")
session.runSql("RESET MASTER");
session.runSql("SET GLOBAL super_read_only = 1");

EXPECT_THROWS(function(){
    replica = dba.rebootClusterFromCompleteOutage("replica");
}, `The instance '${hostname}:${__mysql_sandbox_port4}' has an incompatible GTID set with the seed instance '${hostname}:${__mysql_sandbox_port5}' (former has missing transactions). If you wish to proceed, the 'force' option must be explicitly set.`);

testutil.wipeAllOutput();

EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica", {force:true}); });
EXPECT_OUTPUT_CONTAINS(`NOTE: Not rejoining instance '${hostname}:${__mysql_sandbox_port4}' because its GTID set isn't compatible with '${hostname}:${__mysql_sandbox_port5}'.`);

status = replica.status();
EXPECT_EQ("Cluster is NOT tolerant to any failures. 1 member is not active.", status["defaultReplicaSet"]["statusText"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port5}`]["status"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port6}`]["status"]);
EXPECT_EQ("(MISSING)", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["status"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port5}`, status["defaultReplicaSet"]["primary"]);

replica.removeInstance(__sandbox_uri4, {force: true});

testutil.waitMemberState(__mysql_sandbox_port4, "OFFLINE (MISSING)");

shell.connect(__sandbox_uri4);
reset_instance(session);

shell.connect(__sandbox_uri5);
replica = dba.getCluster();
replica.addInstance(__sandbox_uri4, {recoveryMethod: "clone"});
replica.setPrimaryInstance(__sandbox_uri4);

shell.connect(__sandbox_uri4);

replica = dba.getCluster();
status = replica.status();
EXPECT_EQ("OK", status["defaultReplicaSet"]["status"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port5}`]["status"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port6}`]["status"]);
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port4}`]["status"]);
EXPECT_EQ(`${hostname}:${__mysql_sandbox_port4}`, status["defaultReplicaSet"]["primary"]);

//<> FR6.1.5: The Cluster's executed transaction set (GTID_EXECUTED) must not be empty
// The replica cluster has only one instance so rebootClusterFromCompleteOutage should fail

shell.connect(__sandbox_uri4);
replica = dba.getCluster();
replica.removeInstance(__sandbox_uri5);
replica.removeInstance(__sandbox_uri6);

testutil.waitMemberState(__mysql_sandbox_port5, "OFFLINE (MISSING)");
testutil.waitMemberState(__mysql_sandbox_port6, "OFFLINE (MISSING)");

session.runSql("STOP replica FOR CHANNEL 'clusterset_replication'");

session.runSql("SET GLOBAL super_read_only = 0");
session.runSql("STOP group_replication;")
session.runSql("RESET MASTER");
session.runSql("SET GLOBAL super_read_only = 1");

session.runSql("SELECT * FROM mysql.gtid_executed;");

EXPECT_THROWS(function(){
    replica = dba.rebootClusterFromCompleteOutage("replica");
}, `The instance '${hostname}:${__mysql_sandbox_port4}' has an empty GTID set.`);
EXPECT_OUTPUT_CONTAINS(`NOTE: The target instance '${hostname}:${__mysql_sandbox_port4}' has not been pre-provisioned (GTID set is empty). The Shell is unable to determine whether the instance has pre-existing data that would be overwritten.`);

//reset cluster (with just 1 instances on 'replica' cluster)
shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
cs.removeCluster("replica", {force: true});

var session6 = mysql.getSession(__sandbox_uri6);
var session5 = mysql.getSession(__sandbox_uri5);
var session4 = mysql.getSession(__sandbox_uri4);

reset_instance(session6);
reset_instance(session5);
reset_instance(session4);

replica = cs.createReplicaCluster(__sandbox_uri4, "replica", {recoveryMethod: "clone"});

//@<> Must not throw if there's a new cluster in the clusterset (BUG#34408687)
testutil.killSandbox(__mysql_sandbox_port4);

shell.connect(__sandbox_uri1);
cs = dba.getClusterSet();
replica2 = cs.createReplicaCluster(__sandbox_uri6, "replica2", {recoveryMethod: "clone"});

testutil.startSandbox(__mysql_sandbox_port4);

shell.connect(__sandbox_uri4);
EXPECT_NO_THROWS(function(){ replica = dba.rebootClusterFromCompleteOutage("replica"); });

shell.connect(__sandbox_uri1);
CHECK_PRIMARY_CLUSTER([__sandbox_uri1, __sandbox_uri2, __sandbox_uri3], cluster);
CHECK_REPLICA_CLUSTER([__sandbox_uri4], cluster, replica);
CHECK_REPLICA_CLUSTER([__sandbox_uri6], cluster, replica2);
CHECK_CLUSTER_SET(session);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
testutil.destroySandbox(__mysql_sandbox_port6);
