// Tests for Group Replication's paxos single leader option management.
// The tests cover the main features and changes for InnoDB Cluster:
//
//  - New option 'paxosSingleLeader' in dba.createCluster()
//  - Automatic setting management in addInstance()/rejoinInstance()
//  - Information about the option in cluster.options()
//  - Information about the option in cluster.status() with extended >= 1
//  - Migrate a Cluster to enable/disable the option with
//    rebootClusterFromCompleteOutage(), with a new option 'paxosSingleLeader'

//@<> INCLUDE gr_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@<> Bad options in createCluster() (should fail)
if (__version_num < 80031) {
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {paxosSingleLeader: true}) }, "Option 'paxosSingleLeader' not supported on target server version: '" + __version + "'", "RuntimeError");
} else {
  EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {paxosSingleLeader: "yes"}); }, "Argument #2: Option 'paxosSingleLeader' Bool expected, but value is String", "TypeError");
}

//@<> createCluster() - defaults
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true}); });

var default_value = undefined;

// The default value is define by GR itself
if (__version_num >= 80031) {
  default_value = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";
}

//@<> paxosSingleLeader must not be allowed when adopting from a GR group {VER(>=8.0.31)}
dba.dropMetadataSchema({force: true});

EXPECT_THROWS_TYPE(function() { dba.createCluster("test", {paxosSingleLeader: false, adoptFromGR: true}) }, "Cannot use the 'paxosSingleLeader' option if 'adoptFromGR' is set to true", "ArgumentError");

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {adoptFromGR: true, gtidSetIsComplete: true}); });

//@<> Option must be included in cluster.options() - if version >= 8.0.27
var global_options = cluster.options()["defaultReplicaSet"]["globalOptions"];
print(global_options);
var found = false;

for (option of global_options) {
  if (option["option"] == "paxosSingleLeader") {
    EXPECT_EQ(default_value, option["value"]);
    found = true;
  }
}

if (__version_num < 80031) {
  EXPECT_FALSE(found, "paxosSingleLeader option found");
} else {
  EXPECT_TRUE(found, "paxosSingleLeader option not found");
}

//@<> cluster.status() must display the option use when extended is enabled - if version >= 8.0.31
var status_extended = cluster.status({extended: 1});
print(status_extended);

EXPECT_EQ(default_value, status_extended["defaultReplicaSet"]["paxosSingleLeader"]);

//@<> The option set in the Cluster must be automatically set in addInstance() {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, default_value);

//@<> The option set in the Cluster must be automatically set in rejoinInstance() {VER(>=8.0.31)}
session2.runSql("SET GLOBAL group_replication_paxos_single_leader = 1");
session2.runSql("STOP group_replication");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, default_value);

//@<> createCluster() - enable paxos single leader {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { cluster.dissolve({force: true}); });

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true, paxosSingleLeader: true}); });

// Confirm the values
var paxos_single_leader_set= session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader_set, "ON");

var global_options = cluster.options()["defaultReplicaSet"]["globalOptions"];
print(global_options);
var found = false;

for (option of global_options) {
  if (option["option"] == "paxosSingleLeader") {
    EXPECT_EQ(option["value"], paxos_single_leader_set);
    found = true;
  }
}
EXPECT_TRUE(found, "paxosSingleLeader option not found");

var status_extended = cluster.status({extended: 1});
print(status_extended);

EXPECT_EQ(status_extended["defaultReplicaSet"]["paxosSingleLeader"], paxos_single_leader_set);

//@<> The option set in the Cluster must be automatically set in addInstance() - enabled {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, paxos_single_leader_set);

//@<> The option set in the Cluster must be automatically set in rejoiInstance() - enabled {VER(>=8.0.31)}
session2.runSql("SET GLOBAL group_replication_paxos_single_leader = 0");
session2.runSql("STOP group_replication");

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, paxos_single_leader_set);

//@<> The option set in the Cluster must be automatically set in addInstance() - effective value differs {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2); });

// Switch the cluster to multi-primary mode so the effective value is OFF instead of ON
EXPECT_NO_THROWS(function() { cluster.switchToMultiPrimaryMode(); });

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, paxos_single_leader_set);

// Remove instance 2 and change the option to OFF in the primary
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2); });

session.runSql("SET GLOBAL group_replication_paxos_single_leader = 0");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, paxos_single_leader_set);

// Switch back to single-primary mode
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2); });
EXPECT_NO_THROWS(function() { cluster.switchToSinglePrimaryMode(); });

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader, paxos_single_leader_set);

// Dissolve the cluster to be able to downgrade the GR protocol version
EXPECT_NO_THROWS(function() { cluster.dissolve({force: true}); });

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true}); });

var paxos_single_leader_set = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

session.runSql("SELECT group_replication_set_communication_protocol('8.0.16');");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

// Remove instance 2 and change the option to OFF in the primary
EXPECT_NO_THROWS(function() { cluster.removeInstance(__sandbox_uri2); });

session.runSql("SET GLOBAL group_replication_paxos_single_leader = 0");

EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

var session2 = mysql.getSession(__sandbox_uri2);
// The option should be set in the target instance
var paxos_single_leader = session2.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

//@<> createCluster() - enable paxos single leader and multi-primary-mode {VER(>=8.0.31)}
EXPECT_NO_THROWS(function() { cluster.dissolve({force: true}); });

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("test", {gtidSetIsComplete: true, paxosSingleLeader: true, multiPrimary: true, force: true}); });

// Confirm the values
var paxos_single_leader_set= session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader_set, "ON");


//@<> cluster.status() must display a note informing paxosSingleLeader is ineffective since the Cluster is running in multi-primary mode {VER(>=8.0.31)}
var status_extended = cluster.status({extended: 1});
print(status_extended);

EXPECT_EQ(["NOTE: The Cluster is configured to use a Single Consensus Leader, however, this setting is ineffective since the Cluster is operating in multi-primary mode."], status_extended["defaultReplicaSet"]["clusterErrors"]);

//@<> Shutdown the Cluster to change the option in rebootClusterFromCompleteOutage()
shutdown_cluster(cluster);

//@<> Bad options in dba.rebootClusterFromCompleteOutage() (should fail)
if (__version_num < 80031) {
  EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {paxosSingleLeader: true}) }, "Option 'paxosSingleLeader' not supported on target server version: '" + __version + "'", "RuntimeError");
} else {
  EXPECT_THROWS_TYPE(function() { dba.rebootClusterFromCompleteOutage("test", {paxosSingleLeader: "yes"}); }, "Argument #2: Option 'paxosSingleLeader' Bool expected, but value is String", "TypeError");
}

//@<> Enable paxos single leader while rebooting a cluster from complete outage (switch from OFF to ON) {VER(>=8.0.31)}
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {paxosSingleLeader: true}) });

// Confirm the values
var paxos_single_leader_set = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader_set, "ON");

// Shutdown the Cluster to verify the value is kept when not used in the command
shutdown_cluster(cluster);

//@<> dba.rebootClusterFromCompleteOutage() must keep the original value of paxosSingleLeader {VER(>=8.0.31)}
shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test") });

// Confirm the values
var paxos_single_leader_set = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader_set, "ON");

//@<> Disable paxos single leader while rebooting a cluster from complete outage (switch from ON to OFF) {VER(>=8.0.31)}
shutdown_cluster(cluster);

shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() { cluster = dba.rebootClusterFromCompleteOutage("test", {paxosSingleLeader: false}) });

// Confirm the values
var paxos_single_leader_set = session.runSql("select @@group_replication_paxos_single_leader").fetchOne()[0] == 0 ? "OFF" : "ON";

EXPECT_EQ(paxos_single_leader_set, "OFF");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
