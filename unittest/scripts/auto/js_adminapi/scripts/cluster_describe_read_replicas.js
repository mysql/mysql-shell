//@ {VER(>=8.0.23)}

function check_describe() {
  var describe = cluster.describe();

  var topology = describe["defaultReplicaSet"]["topology"];

  for (instance in topology) {
    if (topology[instance]["address"] == __endpoint4) {
      EXPECT_EQ(__endpoint4, topology[instance]["label"], "label");
      EXPECT_EQ("READ_REPLICA", topology[instance]["role"], "role");
      EXPECT_EQ("PRIMARY", topology[instance]["replicationSources"][0], "replicationSources");
    } else if (topology[instance]["address"] == __endpoint5) {
      EXPECT_EQ(__endpoint5, topology[instance]["label"], "label");
      EXPECT_EQ("READ_REPLICA", topology[instance]["role"], "role");
      EXPECT_EQ(__endpoint2, topology[instance]["replicationSources"][0], "replicationSources");
      EXPECT_EQ(__endpoint3, topology[instance]["replicationSources"][1], "replicationSources");
    }
  }
}

//@<> INCLUDE read_replicas_utils.inc

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster

testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port5, "root", {report_host: hostname});

//@<> Add Read-Replica (1) with the defaults
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint4); });

//@<> Add Read-Replica (2) with specific replicationSources
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint5, {replicationSources: [__endpoint2, __endpoint3]}); });

// FR13: <Cluster>.describe() must be extended to include Read Replicas in its output.
//
//  - FR13.1: Each Read Replica must be added to the topology array as a dictionary containing the following attributes:
//
//    - FR13.1.1: address: Canonical address
//    - FR13.1.2: label: Label
//    - FR13.1.3: role: the exact string "READ_REPLICA"
//    - FR13.1.4: replicationSources: an array of the configured sources

//@<> Describe with read-replicas
check_describe();

//@<> Describe with read-replicas: shutdown one of the sources
testutil.killSandbox(__mysql_sandbox_port2);

// Describe should remain unchanged
check_describe();

//@<> Describe with read-replicas: change the replicationSources but don't rejoin
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", "secondary"); });

// Describe should reflect the configuration in the Metadata
var describe = cluster.describe();
var topology = describe["defaultReplicaSet"]["topology"];

for (instance in topology) {
  if (topology[instance]["address"] == __endpoint4) {
      EXPECT_EQ(__endpoint4, topology[instance]["label"], "label");
      EXPECT_EQ("READ_REPLICA", topology[instance]["role"], "role");
      EXPECT_EQ("SECONDARY", topology[instance]["replicationSources"][0], "replicationSources");
  }
}

EXPECT_NO_THROWS(function() { cluster.setInstanceOption(__endpoint4, "replicationSources", [__endpoint1]); });

var describe = cluster.describe();
var topology = describe["defaultReplicaSet"]["topology"];

for (instance in topology) {
  if (topology[instance]["address"] == __endpoint4) {
      EXPECT_EQ(__endpoint4, topology[instance]["label"], "label");
      EXPECT_EQ("READ_REPLICA", topology[instance]["role"], "role");
      EXPECT_EQ(__endpoint1, topology[instance]["replicationSources"][0], "replicationSources");
  }
}

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
