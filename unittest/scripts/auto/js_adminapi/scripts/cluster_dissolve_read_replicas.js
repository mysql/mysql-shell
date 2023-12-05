//@ {VER(>=8.0.23)}

//@<> INCLUDE dba_utils.inc

//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {"manualStartOnBoot": true});
var session = scene.session
var cluster = scene.cluster

testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri4);

// Add a Read-Replica with defaults
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });

testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

// Add a Read-Replica with replicationSources: __endpoint2, __endpoint1
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint1]}); });

testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

//@<> Dissolve cluster - All members online
EXPECT_NO_THROWS(function() { cluster.dissolve(); });

CHECK_DISSOLVED_CLUSTER(session, session);
CHECK_DISSOLVED_CLUSTER(session, session2);
CHECK_DISSOLVED_CLUSTER(session, session3);
CHECK_DISSOLVED_CLUSTER(session, session4);

//@<> Dissolve cluster - Some read-replicas unreachable - interactive disabled

// Re-create Cluster
cluster = dba.createCluster("cluster");
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3); });
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint1]}); });
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// Make read-replica 3 unreachable
testutil.killSandbox(__mysql_sandbox_port3);

EXPECT_THROWS_TYPE(function() { cluster.dissolve()}, "Can't connect to MySQL server on '" + __endpoint3 + "'", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`ERROR: Unable to connect to instance '${__endpoint3}'. Please verify connection credentials and make sure the instance is available.`);

//@<> Dissolve cluster - Some read-replicas unreachable - interactive enabled - cancel
shell.options.useWizards=1;

testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "n");
EXPECT_THROWS_TYPE(function() { cluster.dissolve(); }, "Operation canceled by user.", "RuntimeError");

shell.options.useWizards=0;

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
The cluster still has the following registered instances:
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${__endpoint1}",
                "label": "${__endpoint1}",
                "role": "HA"
            },
            {
                "address": "${__endpoint2}",
                "label": "${__endpoint2}",
                "role": "HA"
            },
            {
                "address": "${__endpoint3}",
                "label": "${__endpoint3}",
                "replicationSources": [
                    "PRIMARY"
                ],
                "role": "READ_REPLICA"
            },
            {
                "address": "${__endpoint4}",
                "label": "${__endpoint4}",
                "replicationSources": [
                    "${__endpoint2}",
                    "${__endpoint1}"
                ],
                "role": "READ_REPLICA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}
WARNING: You are about to dissolve the whole cluster and lose the high availability features provided by it. This operation cannot be reverted. All members will be removed from the cluster and replication will be stopped, internal recovery user accounts and the cluster metadata will be dropped. User data will be maintained intact in all instances.
`);

//@<> Dissolve cluster - Some read-replicas unreachable - force
EXPECT_NO_THROWS(function() { cluster.dissolve({force: true}); });

var session2 = mysql.getSession(__sandbox_uri2);
var session4 = mysql.getSession(__sandbox_uri4);

CHECK_DISSOLVED_CLUSTER(session, session);
CHECK_DISSOLVED_CLUSTER(session, session2);
CHECK_DISSOLVED_CLUSTER(session, session4);

//@<> Dissolve cluster - Some read-replicas OFFLINE

// Re-create Cluster
testutil.startSandbox(__mysql_sandbox_port3);
var session3 = mysql.getSession(__sandbox_uri3);

session3.runSql("set global super_read_only=0");
session3.runSql("drop schema mysql_innodb_cluster_metadata");

cluster = dba.createCluster("cluster");
EXPECT_NO_THROWS(function() { cluster.addInstance(__sandbox_uri2); });

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri3, {recoveryMethod: "clone"}); });
testutil.waitReplicationChannelState(__mysql_sandbox_port3, "read_replica_replication", "ON");

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__sandbox_uri4, {replicationSources: [__endpoint2, __endpoint1]}); });
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// Make read-replica 3 unreachable
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);
var session4 = mysql.getSession(__sandbox_uri4);

session3.runSql("STOP REPLICA");

EXPECT_THROWS_TYPE(function() { cluster.dissolve()}, "The instance '" + __endpoint3 + "' is 'OFFLINE'", "RuntimeError");

EXPECT_OUTPUT_CONTAINS(`The instance '${__endpoint3}' cannot be removed because it is on a 'OFFLINE' state. Please bring the instance back ONLINE and try to dissolve the cluster again. If the instance is permanently not reachable, then please use <Cluster>.dissolve() with the force option set to true to proceed with the operation and only remove the instance from the Cluster Metadata.`);

//@<> Dissolve cluster - Some read-replicas OFFLINE - force
EXPECT_NO_THROWS(function() { cluster.dissolve({force: true })});

CHECK_DISSOLVED_CLUSTER(session, session);
CHECK_DISSOLVED_CLUSTER(session, session2);
CHECK_DISSOLVED_CLUSTER(session, session4);

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
testutil.destroySandbox(__mysql_sandbox_port4);
