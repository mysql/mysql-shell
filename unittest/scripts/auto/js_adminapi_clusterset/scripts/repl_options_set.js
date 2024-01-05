//@ {VER(>=8.0.27)}

//value must match on the MD and on the repl channel
function check_repl_option(session_primary, target_cluster, session_replica, option, value) {

    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = ?)`, [target_cluster]).fetchOne()[0], "Unexpected metadata value.");

    if (option == "opt_replConnectRetry") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replRetryCount") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_count FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replHeartbeatPeriod") {
        EXPECT_EQ(value, session_replica.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replCompressionAlgorithms") {
        EXPECT_EQ(value, session_replica.runSql("SELECT compression_algorithm FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replZstdCompressionLevel") {
        EXPECT_EQ(value, session_replica.runSql("SELECT zstd_compression_level FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
}

//value must match on the MD
function check_repl_option_metadata(session_primary, target_cluster, option, value) {

    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = ?)`, [target_cluster]).fetchOne()[0], "Unexpected metadata value.");
}

//value must not exist on the MD
function check_repl_option_metadata_not_present(session_primary, target_cluster, option) {

    EXPECT_EQ(null, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = ?)`, [target_cluster]).fetchOne()[0], "Unexpected metadata value.");
}

//value must match only on the repl channel
function check_repl_option_only(session_replica, option, value) {

    if (option == "opt_replConnectRetry") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replRetryCount") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_count FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replHeartbeatPeriod") {
        EXPECT_EQ(value, session_replica.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replCompressionAlgorithms") {
        EXPECT_EQ(value, session_replica.runSql("SELECT compression_algorithm FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replZstdCompressionLevel") {
        EXPECT_EQ(value, session_replica.runSql("SELECT zstd_compression_level FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
}

//value must match on the MD and mismatch the repl channel
function check_repl_option_metadata_only(session_primary, target_cluster, session_replica, option, value) {

    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.clusters WHERE (cluster_name = ?)`, [target_cluster]).fetchOne()[0], "Unexpected metadata value.");

    if (option == "opt_replConnectRetry") {
        EXPECT_NE(value, session_replica.runSql("SELECT connection_retry_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replRetryCount") {
        EXPECT_NE(value, session_replica.runSql("SELECT connection_retry_count FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replHeartbeatPeriod") {
        EXPECT_NE(value, session_replica.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replCompressionAlgorithms") {
        EXPECT_NE(value, session_replica.runSql("SELECT compression_algorithm FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replZstdCompressionLevel") {
        EXPECT_NE(value, session_replica.runSql("SELECT zstd_compression_level FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')").fetchOne()[0], "Unexpected replication channel value.");
    }
}

function check_option(options_cluster, options_instance, name, value_cluster, value_instance) {

    let option = undefined;
    for (var i in options_cluster) {
        if (options_cluster[i].option === name) {
            option = options_cluster[i];
            break;
        }
    }

    EXPECT_NE(undefined, option, `Option '${name}' not present in cluster options.`);
    EXPECT_EQ(value_cluster, option.value, `Value mismatch for cluster option '${name}'.`);

    option = undefined;
    for (var i in options_instance) {
        if (options_instance[i].option === name) {
            option = options_instance[i];
            break;
        }
    }

    EXPECT_NE(undefined, option, `Option '${name}' not present in instance options.`);
    EXPECT_EQ(value_instance, option.value, `Value mismatch for instance option '${name}'.`);
}

//@<> INCLUDE clusterset_utils.inc

//@<> init
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var c1 = dba.createCluster("c1", {gtidSetIsComplete:true});
var c2;
var cset = c1.createClusterSet("cluster_set");

//@<> FR1 check addInstance options (fail)
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationConnectRetry: ""});
}, "Option 'clusterSetReplicationConnectRetry' Integer expected, but value is String");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationConnectRetry: -1});
}, "Invalid value for 'clusterSetReplicationConnectRetry' option. Value cannot be negative.");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationRetryCount: ""});
}, "Option 'clusterSetReplicationRetryCount' Integer expected, but value is String");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationRetryCount: -1});
}, "Invalid value for 'clusterSetReplicationRetryCount' option. Value cannot be negative.");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationHeartbeatPeriod: ""});
}, "Option 'clusterSetReplicationHeartbeatPeriod' Float expected, but value is String");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationHeartbeatPeriod: -17});
}, "Invalid value for 'clusterSetReplicationHeartbeatPeriod' option. Value cannot be negative.");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationHeartbeatPeriod: -0.1});
}, "Invalid value for 'clusterSetReplicationHeartbeatPeriod' option. Value cannot be negative.");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationCompressionAlgorithms: 11});
}, "Argument #3: Option 'clusterSetReplicationCompressionAlgorithms' is expected to be of type String, but is Integer");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationZstdCompressionLevel: ""});
}, "Option 'clusterSetReplicationZstdCompressionLevel' Integer expected, but value is String");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationZstdCompressionLevel: -4});
}, "Invalid value for 'clusterSetReplicationZstdCompressionLevel' option. Value cannot be negative.");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationBind: 22});
}, "Argument #3: Option 'clusterSetReplicationBind' is expected to be of type String, but is Integer");
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationNetworkNamespace: 33});
}, "Argument #3: Option 'clusterSetReplicationNetworkNamespace' is expected to be of type String, but is Integer");

//@<> FR1 check createReplicaCluster options

// fails because of network related options
WIPE_SHELL_LOG();

begin_dba_log_sql();
EXPECT_THROWS(function() {
 cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationConnectRetry: 10, clusterSetReplicationRetryCount: 11, clusterSetReplicationHeartbeatPeriod: 33.3, clusterSetReplicationCompressionAlgorithms: "zlib", clusterSetReplicationZstdCompressionLevel: "4", clusterSetReplicationBind: "foo", clusterSetReplicationNetworkNamespace: "bar"});
}, (__version_num >= 80400) ? "Error found in replication receiver thread" : "Replication thread not in expected state");
end_dba_log_sql();

EXPECT_SHELL_LOG_CONTAINS("SOURCE_CONNECT_RETRY=10");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_RETRY_COUNT=11");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_HEARTBEAT_PERIOD=33.3");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_COMPRESSION_ALGORITHMS='zlib'");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_ZSTD_COMPRESSION_LEVEL=4");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_BIND='foo'");
EXPECT_SHELL_LOG_CONTAINS("NETWORK_NAMESPACE='bar'");

// must succeed
EXPECT_NO_THROWS(function() { cset.createReplicaCluster(__sandbox_uri2, "c2", {clusterSetReplicationConnectRetry: 10, clusterSetReplicationRetryCount: 11, clusterSetReplicationHeartbeatPeriod: 33.3, clusterSetReplicationCompressionAlgorithms: "zlib", clusterSetReplicationZstdCompressionLevel: "4"}); });

session2 = mysql.getSession(__sandbox_uri2);
check_repl_option(session, "c2", session2, "opt_replConnectRetry", 10);
check_repl_option(session, "c2", session2, "opt_replRetryCount", 11);
check_repl_option(session, "c2", session2, "opt_replHeartbeatPeriod", 33.3);
check_repl_option(session, "c2", session2, "opt_replCompressionAlgorithms", "zlib");
check_repl_option(session, "c2", session2, "opt_replZstdCompressionLevel", 4);

//cleanup
cset.removeCluster("c2");
reset_instance(session2);

//@<> FR1.2 check default values for options

EXPECT_NO_THROWS(function() { c2 = cset.createReplicaCluster(__sandbox_uri2, "c2"); });

session2 = mysql.getSession(__sandbox_uri2);
check_repl_option_only(session2, "opt_replConnectRetry", 3);
check_repl_option_metadata_not_present(session, "c2", "opt_replConnectRetry");
check_repl_option_only(session2, "opt_replRetryCount", 10);
check_repl_option_metadata_not_present(session, "c2", "opt_replRetryCount");

//@<> FR2 check setOption() (fail)
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationConnectRetry", "");
}, "Invalid value for 'clusterSetReplicationConnectRetry': Argument #2 is expected to be a integer");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationConnectRetry", -4);
}, "Invalid value for 'clusterSetReplicationConnectRetry': Argument #2 cannot have a negative value");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationRetryCount", "");
}, "Invalid value for 'clusterSetReplicationRetryCount': Argument #2 is expected to be a integer");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationRetryCount", -44);
}, "Invalid value for 'clusterSetReplicationRetryCount': Argument #2 cannot have a negative value");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationHeartbeatPeriod", "");
}, "Invalid value for 'clusterSetReplicationHeartbeatPeriod': Argument #2 is expected to be a double");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationHeartbeatPeriod", -33);
}, "Invalid value for 'clusterSetReplicationHeartbeatPeriod': Argument #2 cannot have a negative value");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationHeartbeatPeriod", -66.9);
}, "Invalid value for 'clusterSetReplicationHeartbeatPeriod': Argument #2 cannot have a negative value");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationCompressionAlgorithms", 11);
}, "Invalid value for 'clusterSetReplicationCompressionAlgorithms': Argument #2 is expected to be a string");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationZstdCompressionLevel", "");
}, "Invalid value for 'clusterSetReplicationZstdCompressionLevel': Argument #2 is expected to be a integer");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationZstdCompressionLevel", -8);
}, "Invalid value for 'clusterSetReplicationZstdCompressionLevel': Argument #2 cannot have a negative value");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationBind", 22);
}, "Invalid value for 'clusterSetReplicationBind': Argument #2 is expected to be a string");
EXPECT_THROWS(function() {
 c1.setOption("clusterSetReplicationNetworkNamespace", 33);
}, "Invalid value for 'clusterSetReplicationNetworkNamespace': Argument #2 is expected to be a string");

//@<> FR2.1 / FR2.5 check if value is only stored in the metadata
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationConnectRetry", 14); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationConnectRetry' to '14' for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");
check_repl_option_metadata(session, "c1", "opt_replConnectRetry", 14);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationHeartbeatPeriod", 38.9); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationHeartbeatPeriod' to '38.90' for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");
check_repl_option_metadata(session, "c1", "opt_replHeartbeatPeriod", 38.9);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationCompressionAlgorithms", "zlib"); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationCompressionAlgorithms' to 'zlib' for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");
check_repl_option_metadata(session, "c1", "opt_replCompressionAlgorithms", "zlib");

//@<> FR2.2 on a primary cluster, there's no need to store null values on the MD
WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationCompressionAlgorithms", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationCompressionAlgorithms' to NULL for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");
check_repl_option_metadata_not_present(session, "c1", "opt_replCompressionAlgorithms");

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationCompressionAlgorithms", "zlib"); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationCompressionAlgorithms' to 'zlib' for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");
check_repl_option_metadata(session, "c1", "opt_replCompressionAlgorithms", "zlib");

//@<> FR2.1 / FR2.2 / FR2.6 check if value is only stored in the metadata (including null) to a secondary instance
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationRetryCount", 16); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationRetryCount' to '16' for the 'c2' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until ClusterSet.rejoinCluster() is called on the cluster.");
check_repl_option_metadata_only(session, "c2", session2, "opt_replRetryCount", 16);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationZstdCompressionLevel' to NULL for the 'c2' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until ClusterSet.rejoinCluster() is called on the cluster.");
check_repl_option_metadata_only(session, "c2", session2, "opt_replZstdCompressionLevel", null);

//@<> FR2.3 should work even with an offline cluster
session2.runSql("STOP group_replication;");
session2.runSql("STOP REPLICA FOR CHANNEL 'clusterset_replication';");

EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationCompressionAlgorithms", "zstd"); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationCompressionAlgorithms' to 'zstd' for the 'c2' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until ClusterSet.rejoinCluster() is called on the cluster.");
check_repl_option_metadata_only(session, "c2", session2, "opt_replCompressionAlgorithms", "zstd");

shell.connect(__sandbox_uri2);
c2 = dba.rebootClusterFromCompleteOutage();

shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

//@<> FR3 / FR3.3 rejoinCluster should update the replication channel

// these 3 options are stored in the metadata, but not on the channel
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationCompressionAlgorithms", null); });
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", 10); });
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationBind", null); });
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationNetworkNamespace", null); });

check_repl_option_metadata_only(session, "c2", session2, "opt_replCompressionAlgorithms", null);
check_repl_option_metadata_only(session, "c2", session2, "opt_replZstdCompressionLevel", 10);
check_repl_option_metadata_only(session, "c2", session2, "opt_replBind", null);
check_repl_option_metadata_only(session, "c2", session2, "opt_replNetworkNamespace", null);

WIPE_SHELL_LOG();

begin_dba_log_sql();
EXPECT_NO_THROWS(function() { cset.rejoinCluster("c2"); });
end_dba_log_sql();

// because we add a NULL option, the channel should have been reset
EXPECT_SHELL_LOG_CONTAINS("RESET REPLICA ALL FOR CHANNEL 'clusterset_replication'");

// they should now be on both and the null gone
check_repl_option(session, "c2", session2, "opt_replZstdCompressionLevel", 10);
check_repl_option_only(session2, "opt_replCompressionAlgorithms", "uncompressed");
check_repl_option_metadata_not_present(session, "c2", "opt_replCompressionAlgorithms");
check_repl_option_metadata_not_present(session, "c2", "opt_replBind");
check_repl_option_metadata_not_present(session, "c2", "opt_replNetworkNamespace");

//@<> FR3.1 unspecified options should keep the defaults values
check_repl_option_only(session2, "opt_replConnectRetry", 3);
check_repl_option_only(session2, "opt_replBind", "");

check_repl_option(session, "c2", session2, "opt_replRetryCount", 16);

//@<> FR3.2 simple change, no reset of the channel needed
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationCompressionAlgorithms", "zlib"); });

WIPE_SHELL_LOG();

begin_dba_log_sql();
EXPECT_NO_THROWS(function() { cset.rejoinCluster("c2"); });
end_dba_log_sql();

EXPECT_SHELL_LOG_NOT_CONTAINS("RESET REPLICA ALL FOR CHANNEL 'clusterset_replication'");

//@<> FR4 check if the repl channel are updated in clusterset.forcePrimaryCluster()

// clusterset currently has 2 clusters, so confirm the metadata values and the repl channel on the replica cluster
check_repl_option_metadata(session, "c1", "opt_replConnectRetry", 14);
check_repl_option_metadata(session, "c1", "opt_replHeartbeatPeriod", 38.9);
check_repl_option_metadata(session, "c1", "opt_replCompressionAlgorithms", "zlib");

check_repl_option_metadata(session, "c2", "opt_replRetryCount", 16);
check_repl_option_metadata(session, "c2", "opt_replZstdCompressionLevel", 10);
check_repl_option_metadata(session, "c2", "opt_replCompressionAlgorithms", "zlib");

check_repl_option_only(session2, "opt_replRetryCount", 16);
check_repl_option_only(session2, "opt_replZstdCompressionLevel", 10);
check_repl_option_only(session2, "opt_replCompressionAlgorithms", "zlib");

// add a second replica cluster, check the values and update the options (but don't call rejoin)
var c3;
EXPECT_NO_THROWS(function() { c3 = cset.createReplicaCluster(__sandbox_uri3, "c3", {clusterSetReplicationHeartbeatPeriod: 17.89}); });

session3 = mysql.getSession(__sandbox_uri3);

check_repl_option(session, "c3", session3, "opt_replHeartbeatPeriod", 17.89);

EXPECT_NO_THROWS(function() { c3.setOption("clusterSetReplicationHeartbeatPeriod", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationHeartbeatPeriod' to NULL for the 'c3' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until ClusterSet.rejoinCluster() is called on the cluster.");

check_repl_option_metadata_only(session, "c3", session3, "opt_replHeartbeatPeriod", null);

// update the options of the current primary cluster

EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationHeartbeatPeriod", 23.45); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'clusterSetReplicationHeartbeatPeriod' to '23.45' for the 'c1' cluster.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the cluster remains the primary Cluster of the ClusterSet.");

check_repl_option_metadata(session, "c1", "opt_replConnectRetry", 14);
check_repl_option_metadata(session, "c1", "opt_replHeartbeatPeriod", 23.45);
check_repl_option_metadata(session, "c1", "opt_replCompressionAlgorithms", "zlib");

session2.close();
session3.close();

// force the primary cluster to c2 and then:
// - check if c3 updated its repl channel
// - reboot the former primary, rejoin it to the clusterset and check its repl channel

testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
cset = dba.getClusterSet();

EXPECT_NO_THROWS(function() { cset.forcePrimaryCluster("c2"); });

testutil.startSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);
session3 = mysql.getSession(__sandbox_uri3);

c1 = dba.rebootClusterFromCompleteOutage();
cset.rejoinCluster("c1");

check_repl_option_metadata_not_present(session2, "c3", "opt_replHeartbeatPeriod");
EXPECT_NE(17.89, session3.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = 'clusterset_replication')")[0]);

check_repl_option(session2, "c1", session, "opt_replHeartbeatPeriod", 23.45);

//@<> FR4 check if the repl channels are updated in clusterset.setPrimaryCluster()

// clusterset currently has 3 clusters, so confirm the metadata values and the repl channel on the replica clusters

shell.connect(__sandbox_uri3);
c3 = dba.getCluster();
shell.connect(__sandbox_uri2);
c2 = dba.getCluster();
shell.connect(__sandbox_uri1);
c1 = dba.getCluster();

check_repl_option(session2, "c1", session, "opt_replConnectRetry", 14);
check_repl_option(session2, "c1", session, "opt_replHeartbeatPeriod", 23.45);
check_repl_option(session2, "c1", session, "opt_replCompressionAlgorithms", "zlib");

check_repl_option_metadata(session, "c2", "opt_replRetryCount", 16);
check_repl_option_metadata(session, "c2", "opt_replZstdCompressionLevel", 10);
check_repl_option_metadata(session, "c2", "opt_replCompressionAlgorithms", "zlib");

// change some options on the clusters and switch the primary

EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationHeartbeatPeriod", 37.3); });
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationZstdCompressionLevel", null); });
EXPECT_NO_THROWS(function() { c1.setOption("clusterSetReplicationCompressionAlgorithms", null); });

EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", 22); });

EXPECT_NO_THROWS(function() { c3.setOption("clusterSetReplicationRetryCount", 17); });
EXPECT_NO_THROWS(function() { c3.setOption("clusterSetReplicationConnectRetry", null); });

cset.status({extended: 1});
EXPECT_NO_THROWS(function() { cset.setPrimaryCluster("c1"); });
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

// for the new primary, the null options should have been removed (don't make sense in a primary)
check_repl_option_metadata(session, "c1", "opt_replHeartbeatPeriod", 37.3);
check_repl_option_metadata_not_present(session, "c1", "opt_replZstdCompressionLevel");
check_repl_option_metadata_not_present(session, "c1", "opt_replCompressionAlgorithms");

// check the previous primary cluster
check_repl_option(session, "c2", session2, "opt_replRetryCount", 16);
check_repl_option(session, "c2", session2, "opt_replZstdCompressionLevel", 22);
check_repl_option(session, "c2", session2, "opt_replCompressionAlgorithms", "zlib");

// check cluster3
check_repl_option(session, "c3", session3, "opt_replRetryCount", 17);
check_repl_option_only(session3, "opt_replConnectRetry", 3); //default value for clusterset

//check status of the cluster
status = cset.status();
EXPECT_EQ("HEALTHY", status["status"]);
EXPECT_EQ("c1", status["primaryCluster"]);

// cleanup
session3.close();
cset.removeCluster("c3");

//@<> FR5 verify that all replication options are present in options()

let options = c2.options();
optionsC = options.defaultReplicaSet.globalOptions;
optionsI = options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(optionsC, optionsI, "clusterSetReplicationConnectRetry", null, 3);
check_option(optionsC, optionsI, "clusterSetReplicationRetryCount", 16, 16);
check_option(optionsC, optionsI, "clusterSetReplicationHeartbeatPeriod", null, 30);
check_option(optionsC, optionsI, "clusterSetReplicationCompressionAlgorithms", "zlib", "zlib");
check_option(optionsC, optionsI, "clusterSetReplicationZstdCompressionLevel", 22, 22);
check_option(optionsC, optionsI, "clusterSetReplicationBind", null, "");
check_option(optionsC, optionsI, "clusterSetReplicationNetworkNamespace", null, "");

EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", 4); });

options = c2.options();
optionsC = options.defaultReplicaSet.globalOptions;
optionsI = options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(optionsC, optionsI, "clusterSetReplicationConnectRetry", null, 3);
check_option(optionsC, optionsI, "clusterSetReplicationRetryCount", 16, 16);
check_option(optionsC, optionsI, "clusterSetReplicationHeartbeatPeriod", null, 30);
check_option(optionsC, optionsI, "clusterSetReplicationCompressionAlgorithms", "zlib", "zlib");
check_option(optionsC, optionsI, "clusterSetReplicationZstdCompressionLevel", 4, 22);
check_option(optionsC, optionsI, "clusterSetReplicationBind", null, "");
check_option(optionsC, optionsI, "clusterSetReplicationNetworkNamespace", null, "");

EXPECT_NO_THROWS(function() { cset.rejoinCluster("c2"); });

options = c2.options();
optionsC = options.defaultReplicaSet.globalOptions;
optionsI = options.defaultReplicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(optionsC, optionsI, "clusterSetReplicationConnectRetry", null, 3);
check_option(optionsC, optionsI, "clusterSetReplicationRetryCount", 16, 16);
check_option(optionsC, optionsI, "clusterSetReplicationHeartbeatPeriod", null, 30);
check_option(optionsC, optionsI, "clusterSetReplicationCompressionAlgorithms", "zlib", "zlib");
check_option(optionsC, optionsI, "clusterSetReplicationZstdCompressionLevel", 4, 4);
check_option(optionsC, optionsI, "clusterSetReplicationBind", null, "");
check_option(optionsC, optionsI, "clusterSetReplicationNetworkNamespace", null, "");

//@<> FR6 / FR7 verify that status warns about option mismatch

// no error
let status = c1.status();
EXPECT_FALSE("clusterErrors" in status["defaultReplicaSet"]);
status = c2.status();
EXPECT_FALSE("clusterErrors" in status["defaultReplicaSet"]);

let status_set = cset.status();
status_set
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c1"]);
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c2"]);

// error should be present (because of null)
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", null); });

status = c2.status();
EXPECT_TRUE("clusterErrors" in status["defaultReplicaSet"]);
EXPECT_EQ("WARNING: The effective ClusterSet replication channel configurations do not match the configured ones (to see the affected options, use Cluster.options()). Please call ClusterSet.rejoinCluster() to update them.", status["defaultReplicaSet"]["clusterErrors"][0]);

status_set = cset.status();
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c1"]);
EXPECT_TRUE("clusterErrors" in status_set["clusters"]["c2"]);
EXPECT_EQ("WARNING: The effective ClusterSet replication channel configurations do not match the configured ones (to see the affected options, use Cluster.options()). Please call ClusterSet.rejoinCluster() to update them.", status_set["clusters"]["c2"]["clusterErrors"][0]);

// no error because the value coincides with the value on the channel
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", 4); });

status = c2.status();
EXPECT_FALSE("clusterErrors" in status["defaultReplicaSet"]);

status_set = cset.status();
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c1"]);
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c2"]);

// error should be present
EXPECT_NO_THROWS(function() { c2.setOption("clusterSetReplicationZstdCompressionLevel", 29); });

status = c2.status();
EXPECT_TRUE("clusterErrors" in status["defaultReplicaSet"]);
EXPECT_EQ("WARNING: The effective ClusterSet replication channel configurations do not match the configured ones (to see the affected options, use Cluster.options()). Please call ClusterSet.rejoinCluster() to update them.", status["defaultReplicaSet"]["clusterErrors"][0]);

status_set = cset.status();
EXPECT_FALSE("clusterErrors" in status_set["clusters"]["c1"]);
EXPECT_TRUE("clusterErrors" in status_set["clusters"]["c2"]);
EXPECT_EQ("WARNING: The effective ClusterSet replication channel configurations do not match the configured ones (to see the affected options, use Cluster.options()). Please call ClusterSet.rejoinCluster() to update them.", status_set["clusters"]["c2"]["clusterErrors"][0]);

//@<> FR2.4 use of any of the options on a cluster not belonging to a clusterset must fail

shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

var c = dba.createCluster("cluster", {gtidSetIsComplete:true});

EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationConnectRetry", null);
}, "Cannot update replication option 'clusterSetReplicationConnectRetry' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationRetryCount", null);
}, "Cannot update replication option 'clusterSetReplicationRetryCount' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationHeartbeatPeriod", null);
}, "Cannot update replication option 'clusterSetReplicationHeartbeatPeriod' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationCompressionAlgorithms", null);
}, "Cannot update replication option 'clusterSetReplicationCompressionAlgorithms' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationZstdCompressionLevel", null);
}, "Cannot update replication option 'clusterSetReplicationZstdCompressionLevel' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationBind", null);
}, "Cannot update replication option 'clusterSetReplicationBind' on a Cluster that doesn't belong to a ClusterSet.");
EXPECT_THROWS(function() {
    c.setOption("clusterSetReplicationNetworkNamespace", null);
}, "Cannot update replication option 'clusterSetReplicationNetworkNamespace' on a Cluster that doesn't belong to a ClusterSet.");

//@<> BUG#35442622 Failed clusterset.createReplicaCluster() not reverting changes

var cset = c.createClusterSet("cluster_set");

EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "replica", {clusterSetReplicationZstdCompressionLevel: 999});
}, `Could not update replication source of ${hostname}:${__mysql_sandbox_port2}`);

EXPECT_NO_THROWS(function() { cset.createReplicaCluster(__sandbox_uri2, "replica", {clusterSetReplicationZstdCompressionLevel: 3}); });

session2 = mysql.getSession(__sandbox_uri2);
check_repl_option(session, "replica", session2, "opt_replZstdCompressionLevel", 3);

status = cset.status({extended: 1});
EXPECT_EQ(status["clusters"]["replica"]["transactionSetErrantGtidSet"], "");

//@<> BUG#35442622 Failed clusterset.createReplicaCluster() not reverting changes (default value)

shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

var c = dba.createCluster("cluster", {gtidSetIsComplete:true});
cset = c.createClusterSet("cluster_set");

EXPECT_THROWS(function() {
    cset.createReplicaCluster(__sandbox_uri2, "replica", {clusterSetReplicationZstdCompressionLevel: 999});
}, `Could not update replication source of ${hostname}:${__mysql_sandbox_port2}`);

EXPECT_NO_THROWS(function() { cset.createReplicaCluster(__sandbox_uri2, "replica"); });

session2 = mysql.getSession(__sandbox_uri2);
check_repl_option_metadata_not_present(session, "replica", "opt_replZstdCompressionLevel");
check_repl_option_only(session2, "opt_replZstdCompressionLevel", 3);

status = cset.status({extended: 1});
EXPECT_EQ(status["clusters"]["replica"]["transactionSetErrantGtidSet"], "");

//@<> cleanup
session.close();
session2.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
