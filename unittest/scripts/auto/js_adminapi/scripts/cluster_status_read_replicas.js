//@ {VER(>=8.0.23)}

function check_default_status(extended, rr_status, current_source, replication_sources, auto_sources) {
  if (extended >= 4 || extended < 0) {
    throw new Error("Unexpected value for 'extended'");
  }

  EXPECT_EQ(current_source, rr_status["address"], "address");
  EXPECT_EQ("ONLINE", rr_status["status"], "status");
  EXPECT_EQ(__version, rr_status["version"], "version");
  EXPECT_EQ("READ_REPLICA", rr_status["role"], "role");
  EXPECT_EQ(undefined, rr_status["instanceErrors"], "instanceErrors");

  if (extended >= 1) {
    if (extended == 1) {
      if (type(replication_sources) == "Array") {
        index = 0;
        for (source of replication_sources) {
          EXPECT_EQ(source, rr_status["replicationSources"][index], "replicationSources");
          index++;
        }
      } else {
        EXPECT_EQ(replication_sources, rr_status["replicationSources"][0], "replicationSources");
      }
    } else if (extended >= 2) {
      if (auto_sources == undefined) {
        index = 0;
        weight = 100;
        for (source of replication_sources) {
          EXPECT_EQ(source, rr_status["replicationSources"][index]["address"], "replicationSources address");
          EXPECT_EQ(weight, rr_status["replicationSources"][index]["weight"], "replicationSources weight");
          index++;
          weight--;
        }
      } else if (auto_sources == "PRIMARY" || auto_sources == "SECONDARY") {
        index = 0;
        weight = 80;
        for (source of replication_sources) {
          EXPECT_EQ(source, rr_status["replicationSources"][index]["address"], "replicationSources address");
          EXPECT_EQ(weight, rr_status["replicationSources"][index]["weight"], "replicationSources weight");
          index++;
          weight = 60;
        }
      }

      EXPECT_EQ("APPLIED_ALL", rr_status["applierStatus"]);
      EXPECT_EQ("Waiting for an event from Coordinator", rr_status["applierThreadState"]);
      EXPECT_EQ(4, rr_status["applierWorkerThreads"]);
      EXPECT_EQ("ON", rr_status["receiverStatus"]);
      EXPECT_EQ("Waiting for source to send event", rr_status["receiverThreadState"]);
      EXPECT_EQ(null, rr_status["replicationLag"]);
    }

    if (extended >= 2) {
      EXPECT_EQ(0, rr_status["applierQueuedTransactionSetSize"]);
      EXPECT_EQ("", rr_status["applierQueuedTransactionSet"]);

      var regexp = /\d{2}:\d{2}:\d{2}\.\d{3}/;
      EXPECT_TRUE(rr_status["receiverTimeSinceLastMessage"].match(regexp))

      EXPECT_EQ("ON", rr_status["coordinatorState"]);
      EXPECT_EQ("Replica has read all relay log; waiting for more updates", rr_status["coordinatorThreadState"]);
    }

    if (extended >= 3) {
      EXPECT_EQ(0, rr_status["options"]["delay"]);
      EXPECT_EQ(30, rr_status["options"]["heartbeatPeriod"]);
      EXPECT_EQ(10, rr_status["options"]["retryCount"]);
      EXPECT_EQ(3, rr_status["options"]["connectRetry"]);
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

CHECK_READ_REPLICA(__sandbox_uri4, cluster, "primary", __endpoint1);

//@<> Add Read-Replica (2) with specific replicationSources
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint5, {replicationSources: [__endpoint2, __endpoint3]}); });

CHECK_READ_REPLICA(__sandbox_uri5, cluster, [__endpoint2, __endpoint3], [__endpoint2]);

// FR14.1: Each Read Replica must be added to the array readReplicas of the member acting as the active source in that moment (or the primary if the replicationSources is empty or the Read Replica is unreachable), identified by its label and as an array containing the following attributes, by default:
//   - address
//   - status
//   - instanceErrors

// FR14.2: If the extended option is 1, the readReplicas array must also include the following information:
//   - replicationSources
//   - applierStatus
//   - applierThreadState
//   - applierWorkerThreads
//   - receiverStatus
//   - receiverThreadState
//   - replicationLag

// FR14.3: If the extended option is 2, the readReplicas array must also include the following information:
//   - The replicationSources list must list all sources ordered by weight and include the configured weight for each one.
//   - applierQueuedTransactionSetSize
//   - applierQueuedTransactionSet
//   - receiverTimeSinceLastMessage
//   - coordinatorState
//   - coordinatorThreadState

// FR14.4: If the extended option is 3, the readReplicas array must also include the following information, under and array named options:
//   - delay
//   - heartbeatPeriod
//   - retryCount
//   - connectRetry

//@<> Status with ONLINE read-replicas (extended: 0)
var status = cluster.status();
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
var read_replica2 = status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint5];

// Check read-replica 1
check_default_status(0, read_replica1, __endpoint4);

// Check read-replica 2
check_default_status(0, read_replica2, __endpoint5);

//@<> Status with ONLINE read-replicas (extended: 1)

status = cluster.status({extended: 1});
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
read_replica2 = status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint5];

// Check read-replica 1
check_default_status(1, read_replica1, __endpoint4, "PRIMARY");

// Check read-replica 2
check_default_status(1, read_replica2, __endpoint5, [__endpoint2, __endpoint3]);

//@<> Status with ONLINE read-replicas (extended: 2)
status = cluster.status({extended: 2});
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
read_replica2 = status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint5];

// Check read-replica 1
check_default_status(2, read_replica1, __endpoint4, [__endpoint1, __endpoint3, __endpoint2], "PRIMARY");

// Check read-replica 2
check_default_status(2, read_replica2, __endpoint5, [__endpoint2, __endpoint3]);

//@<> Status with ONLINE read-replicas (extended: 3)
status = cluster.status({extended: 3});
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
read_replica2 = status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint5];

// Check read-replica 1
check_default_status(3, read_replica1, __endpoint4, [__endpoint1, __endpoint3, __endpoint2], "PRIMARY");

// Check read-replica 2
check_default_status(3, read_replica2, __endpoint5, [__endpoint2, __endpoint3]);

//@<> Status with OFFLINE read-replica

// Stop replication on read-replica 1
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("STOP replica");

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];

EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("OFFLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replication channel is stopped. Use Cluster.rejoinInstance() to restore it.", read_replica1["instanceErrors"][0]);

//@<> Status with UNREACHABLE read-replica
testutil.killSandbox(__mysql_sandbox_port4);

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("UNREACHABLE", read_replica1["status"]);
EXPECT_EQ("ERROR: Could not connect to the Read-Replica: The instance is unreachable.", read_replica1["instanceErrors"][0]);

//@<> Status with ERROR read-replica

// Start the instance back
testutil.startSandbox(__mysql_sandbox_port4);
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// Introduce an errant transaction to make replication fail
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("SET GLOBAL super_read_only = 0");
session4.runSql("SET GLOBAL read_only = 0");
session4.runSql("SET sql_log_bin = 0");
session4.runSql("CREATE DATABASE error_trx_db");
session4.runSql("SET sql_log_bin = 1");
session4.runSql("SET GLOBAL super_read_only = 1");

session.runSql("CREATE DATABASE error_trx_db");

session4.runSql("START replica");

testutil.waitForReplApplierError(__mysql_sandbox_port4, "read_replica_replication");

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ERROR", read_replica1["status"]);

var regexp = /WARNING: Read Replica's replication channel stopped with an error: 'Worker \d+ failed executing transaction '[0-9a-f-]+:\d+' at source log [a-z0-9\-\.]+, end_log_pos \d+; Error '.+' on query\. Default database: '.+'\..+\. Use Cluster\.rejoinInstance\(\) to restore it\./;

EXPECT_TRUE(read_replica1["instanceErrors"][0].match(regexp))

// FR14.1.3: instanceErrors: A list of diagnostic messages when at least one error if observed. It may include any of the following information:
//
//  - super_read_only is disabled
//  - Replication channel stopped. Include the error in case it stopped with an error. (covered above)
//  - Errant transaction set detected.
//  - Replication channel misconfiguration.
//  - replicationSources mismatch or empty, with an indication to use <Cluster>.rejoinInstance() with the replicationSources option
//  - Read Replica unreachable (covered above)

//@<> Status with super_read_only disable in read-replica

// Fix the replication error and rejoin the read-replica
session4.runSql("STOP replica");
session4.runSql("SET GLOBAL super_read_only = 0");
session4.runSql("SET GLOBAL read_only = 0");
session4.runSql("SET sql_log_bin = 0");
session4.runSql("DROP DATABASE error_trx_db");
session4.runSql("SET sql_log_bin = 1");
session4.runSql("SET GLOBAL super_read_only = 1");
session4.runSql("START replica");
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

// Disable super_read_only
session4.runSql("SET GLOBAL super_read_only = 0");

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Instance is a Read-Replica but super_read_only option is OFF. Use Cluster.rejoinInstance() to fix it.", read_replica1["instanceErrors"][0]);

// Enable back super_read_only
session4.runSql("SET GLOBAL super_read_only = 1");

//@<> Status with replication channel misconfiguration in read-replica

// Disable source_connection_auto_failover
session4.runSql("STOP replica");
session4.runSql("change replication source to source_connection_auto_failover=0 for channel 'read_replica_replication'");
session4.runSql("START replica");

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replication channel is misconfigured: automatic failover is disabled. Use Cluster.rejoinInstance() to fix it.", read_replica1["instanceErrors"][0]);

// Enable back source_connection_auto_failover
session4.runSql("STOP replica");
session4.runSql("change replication source to source_connection_auto_failover=1 for channel 'read_replica_replication'");
session4.runSql("START replica");

//@<> Status with replication channel misconfiguration in read-replica: replicationSources empty

// Remove the replicationSources from the Metadata
var uuid = session4.runSql("SELECT @@server_uuid").fetchOne()[0];
var replication_sources = session.runSql("SELECT attributes->>'$.replicationSources' FROM mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = ?", [uuid]).fetchOne()[0]

session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_REMOVE(attributes, '$.replicationSources') WHERE mysql_server_uuid = ?", [uuid]);

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replicationSources is empty. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.", read_replica1["instanceErrors"][0]);

//@<> Status with replication channel misconfiguration in read-replica: replicationSources mistmatch

// Update the replication sources
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_SET(attributes, '$.replicationSources', CAST('[\"localhost:3316\"]' as JSON)) WHERE mysql_server_uuid = ?", [uuid]);

status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ONLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replication channel is misconfigured: the current sources don't match the configured ones. Use Cluster.rejoinInstance() to fix it.", read_replica1["instanceErrors"][0]);

// Restore the replicationSources
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_SET(attributes, '$.replicationSources', ?) WHERE mysql_server_uuid = ?", [replication_sources, uuid]);

//@<> Status with replicationSources configured for "secondary" and no secondary members - Should not include warnings
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint4); });
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint5); });

// Add a read-replica to follow secondaries
EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint4, {replicationSources: "secondary"}); });
CHECK_READ_REPLICA(__sandbox_uri4, cluster, "secondary", __endpoint2);

// Remove all secondaries from the cluster
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint2); });
EXPECT_NO_THROWS(function() { cluster.removeInstance(__endpoint3); });

// It takes ~5 seconds for the group membership to be fetched by the managed replication channel and the read_replica to connect to a new source
os.sleep(6);

var status = cluster.status({extended: 1});
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];

check_default_status(1, read_replica1, __endpoint4, "SECONDARY");

//@<> Read-Replicas with the replication channel broken and then stopped should be placed under the primary member of the cluster

// Introduce an errant transaction to make replication fail
var session4 = mysql.getSession(__sandbox_uri4);
session4.runSql("SET GLOBAL super_read_only = 0");
session4.runSql("SET GLOBAL read_only = 0");
session4.runSql("SET sql_log_bin = 0");
session4.runSql("CREATE DATABASE error_trx_db2");
session4.runSql("SET sql_log_bin = 1");
session4.runSql("SET GLOBAL super_read_only = 1");

session.runSql("CREATE DATABASE error_trx_db2");

session4.runSql("START replica");

testutil.waitForReplApplierError(__mysql_sandbox_port4, "read_replica_replication");

session4.runSql("STOP replica");

status = cluster.status({extended: 1});
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("ERROR", read_replica1["status"]);

// Fix the replication error and rejoin the read-replica
session4.runSql("SET GLOBAL super_read_only = 0");
session4.runSql("SET GLOBAL read_only = 0");
session4.runSql("SET sql_log_bin = 0");
session4.runSql("DROP DATABASE error_trx_db2");
session4.runSql("SET sql_log_bin = 1");
session4.runSql("SET GLOBAL super_read_only = 1");
session4.runSql("START replica");
testutil.waitReplicationChannelState(__mysql_sandbox_port4, "read_replica_replication", "ON");

//@<> Read-Replicas with the replication channel reset should be placed under the primary member of the cluster
session4.runSql("STOP REPLICA")
session4.runSql("RESET REPLICA ALL")

var status = cluster.status({extended: 1});
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];

EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("OFFLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replication channel is missing. Use Cluster.rejoinInstance() to restore it.", read_replica1["instanceErrors"][0]);

//@<> If the channel is missing but also empty, intanceErrors should include both information

// Remove the replicationSources from the Metadata
var uuid = session4.runSql("SELECT @@server_uuid").fetchOne()[0];
var replication_sources = session.runSql("SELECT attributes->>'$.replicationSources' FROM mysql_innodb_cluster_metadata.instances WHERE mysql_server_uuid = ?", [uuid]).fetchOne()[0]

session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_REMOVE(attributes, '$.replicationSources') WHERE mysql_server_uuid = ?", [uuid]);

var status = cluster.status({extended: 1});
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];

EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("OFFLINE", read_replica1["status"]);
EXPECT_EQ("WARNING: Read Replica's replicationSources is empty. Use Cluster.setInstanceOption() with the option 'replicationSources' to update the instance's sources.", read_replica1["instanceErrors"][0]);
EXPECT_EQ("WARNING: Read Replica's replication channel is missing. Use Cluster.rejoinInstance() to restore it.", read_replica1["instanceErrors"][1]);

// Restore the replication sources
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_SET(attributes, '$.replicationSources', ?) WHERE mysql_server_uuid = ?", [replication_sources, uuid]);

//@<> Switching from single-primary mode to multi-primary mode with read-replicas configured with replicationSources: "secondary" should not produce warnings
EXPECT_NO_THROWS(function() { cluster.rejoinInstance(__endpoint4); });
EXPECT_NO_THROWS(function() { cluster.switchToMultiPrimaryMode(); });

var status = cluster.status({extended: 1});
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
check_default_status(1, read_replica1, __endpoint4, "SECONDARY");

//@<> Unreachable read-replicas should be placed under any but only one member of the Cluster when in multi-primary mode
EXPECT_NO_THROWS(function() { cluster.addInstance(__endpoint2); });
testutil.killSandbox(__mysql_sandbox_port4);

var status = cluster.status();
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];

EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("UNREACHABLE", read_replica1["status"]);
EXPECT_EQ("ERROR: Could not connect to the Read-Replica: The instance is unreachable.", read_replica1["instanceErrors"][0]);

EXPECT_EQ({}, status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"]);

EXPECT_NO_THROWS(function() { cluster.addReplicaInstance(__endpoint5); });
testutil.killSandbox(__mysql_sandbox_port5);

var status = cluster.status();
print(status);
var read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint4];
var read_replica2 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint5];

EXPECT_EQ(__endpoint4, read_replica1["address"]);
EXPECT_EQ("UNREACHABLE", read_replica1["status"]);
EXPECT_EQ("ERROR: Could not connect to the Read-Replica: The instance is unreachable.", read_replica1["instanceErrors"][0]);

EXPECT_EQ(__endpoint5, read_replica2["address"]);
EXPECT_EQ("UNREACHABLE", read_replica2["status"]);
EXPECT_EQ("ERROR: Could not connect to the Read-Replica: The instance is unreachable.", read_replica2["instanceErrors"][0]);

EXPECT_EQ({}, status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"]);

cluster.switchToSinglePrimaryMode();
testutil.startSandbox(__mysql_sandbox_port5);
cluster.removeInstance(__endpoint4, {force: true});
CHECK_READ_REPLICA(__sandbox_uri5, cluster, "primary", __endpoint1);

session5 = mysql.getSession(__sandbox_uri5);

session5.runSql("stop replica");

session1 = mysql.getSession(__sandbox_uri1);

session1.runSql("CREATE schema foobar;")

var session2 = mysql.getSession(__sandbox_uri2);

session1.runSql("FLUSH BINARY LOGS");
session1.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");
session2.runSql("FLUSH BINARY LOGS");
session2.runSql("PURGE BINARY LOGS BEFORE DATE_ADD(NOW(6), INTERVAL 1 DAY)");

session5.runSql("start replica");

// Check error
status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint1]["readReplicas"][__endpoint5];

EXPECT_EQ(__endpoint5, read_replica1["address"]);
EXPECT_EQ("ERROR", read_replica1["status"]);

var regexp = /WARNING: Read Replica's replication channel stopped with a connection error: 'Got fatal error \d+ from source when reading data from binary log: 'Cannot replicate because the source purged required binary logs\. Replicate the missing transactions from elsewhere, or provision a new replica from backup\. Consider increasing the source's binary log expiration period\. The GTID set sent by the replica is '[0-9a-f-:,\s]+', and the missing transactions are '[0-9a-f-:,\s]+'''\. Use Cluster\.rejoinInstance\(\) to restore it\./;

EXPECT_TRUE(read_replica1["instanceErrors"][0].match(regexp));

cluster.setPrimaryInstance(__endpoint2);
cluster.removeInstance(__endpoint1);

// Confirm the instance is still placed under the current primary after the switch
status = cluster.status();
print(status);
read_replica1 = status["defaultReplicaSet"]["topology"][__endpoint2]["readReplicas"][__endpoint5];

EXPECT_EQ(__endpoint5, read_replica1["address"]);
EXPECT_EQ("ERROR", read_replica1["status"]);

EXPECT_TRUE(read_replica1["instanceErrors"][0].match(regexp));

//@<> Cleanup
scene.destroy();
testutil.destroySandbox(__mysql_sandbox_port4);
testutil.destroySandbox(__mysql_sandbox_port5);
