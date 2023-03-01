//@ {VER(>=8.0.27)}

// Tests for Group Replication's paxos single leader option management.
// The tests cover the main features and changes for InnoDB ClusterSet:
//
//  - New option 'paxosSingleLeader' in dba.createReplicaCluster()
//  - Automatic setting management in addInstance()/rejoinInstance()
//  - Information about the option in clusterset.status() with extended >= 2

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
cluster = dba.createCluster("test", {gtidSetIsComplete: 1});
cs = cluster.createClusterSet("cs");

//@<> Bad options in createReplicaCluster() (should fail)
if (__version_num < 80031) {
  EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica1", {paxosSingleLeader: true}); }, "Option 'paxosSingleLeader' not supported on target server version: '" + __version + "'", "RuntimeError");
} else {
  EXPECT_THROWS_TYPE(function() { cs.createReplicaCluster(__sandbox_uri2, "replica1", {paxosSingleLeader: "yes"}); }, "Argument #3: Option 'paxosSingleLeader' Bool expected, but value is String", "TypeError");
}

//@<> createReplicaCluster() - defaults (paxosSingleLeader OFF) {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { replica = cs.createReplicaCluster(__sandbox_uri2, "replica1") });

var default_value = undefined;
var session2 = mysql.getSession(__sandbox_uri2);

// The default value is define by GR itself
if (__version_num >= 80027) {
  default_value = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";
}

// Verify the value
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, default_value);

//@<> The option set in the Cluster must be automatically set in addInstance() - OFF {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { replica.addInstance(__sandbox_uri4); });

var session4 = mysql.getSession(__sandbox_uri4);
// The option should be set in the target instance
var paxos_single_leader = session4.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, default_value);

// Clean-up
EXPECT_NO_THROWS(function() { replica.removeInstance(__sandbox_uri4); });


//@<> createReplicaCluster() - enabling paxosSingleLeader {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { replica2 = cs.createReplicaCluster(__sandbox_uri3, "replica2", {paxosSingleLeader: true}) });

var session3 = mysql.getSession(__sandbox_uri3);

// Verify the value
var paxos_single_leader = session3.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, "ON");

//@<> The option set in the Cluster must be automatically set in addInstance() - ON {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { replica2.addInstance(__sandbox_uri4); });

var session4 = mysql.getSession(__sandbox_uri4);
// The option should be set in the target instance
var paxos_single_leader = session4.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, "ON");

//@<> clusterset.status() must display the option use when extended is > 2 - if version >= 8.0.31 {VER(>=8.0.31)}
var status_extended = cs.status({extended: 2});
print(status_extended);

EXPECT_EQ(default_value, status_extended["clusters"]["test"]["paxosSingleLeader"]);
EXPECT_EQ(default_value, status_extended["clusters"]["replica1"]["paxosSingleLeader"]);
EXPECT_EQ("ON", status_extended["clusters"]["replica2"]["paxosSingleLeader"]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
