//@ {VER(>=8.0.11)}

function check_repl_option(session_primary, session_replica, option, value) {

    let reg_match = session_replica.uri.match(/.+:\/\/.+:(\d+)/);
    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${reg_match[1]}')`).fetchOne()[0], "Unexpected metadata value.");

    if (option == "opt_replConnectRetry") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replRetryCount") {
        EXPECT_EQ(value, session_replica.runSql("SELECT connection_retry_count FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replHeartbeatPeriod") {
        EXPECT_EQ(value, session_replica.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replCompressionAlgorithms") {
        EXPECT_EQ(value, session_replica.runSql("SELECT compression_algorithm FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replZstdCompressionLevel") {
        EXPECT_EQ(value, session_replica.runSql("SELECT zstd_compression_level FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
}

function check_repl_option_metadata(session_primary, session_replica, option, value) {

    let reg_match = session_replica.uri.match(/.+:\/\/.+:(\d+)/);
    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${reg_match[1]}')`).fetchOne()[0], "Unexpected metadata value.");
}

function check_repl_option_metadata_not_present(session_primary, session_replica, option) {

    let reg_match = session_replica.uri.match(/.+:\/\/.+:(\d+)/);
    EXPECT_EQ(null, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${reg_match[1]}')`).fetchOne()[0], "Unexpected metadata value.");
}

function check_repl_option_metadata_only(session_primary, session_replica, option, value) {

    let reg_match = session_replica.uri.match(/.+:\/\/.+:(\d+)/);
    EXPECT_EQ(`${value}`, session_primary.runSql(`SELECT attributes->>'$.${option}' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${reg_match[1]}')`).fetchOne()[0], "Unexpected metadata value.");

    if (option == "opt_replConnectRetry") {
        EXPECT_NE(value, session_replica.runSql("SELECT connection_retry_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replRetryCount") {
        EXPECT_NE(value, session_replica.runSql("SELECT connection_retry_count FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replHeartbeatPeriod") {
        EXPECT_NE(value, session_replica.runSql("SELECT heartbeat_interval FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replCompressionAlgorithms") {
        EXPECT_NE(value, session_replica.runSql("SELECT compression_algorithm FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
    else if (option == "opt_replZstdCompressionLevel") {
        EXPECT_NE(value, session_replica.runSql("SELECT zstd_compression_level FROM performance_schema.replication_connection_configuration WHERE (channel_name = '')").fetchOne()[0], "Unexpected replication channel value.");
    }
}

function check_option(options, name, value, variable) {

    let option = undefined;
    for (var i in options) {
        if (options[i].option === name) {
            option = options[i];
            break;
        }
    }

    EXPECT_NE(undefined, option, `Option '${name}' not present in array.`);

    EXPECT_EQ(value, option.value, `Value mismatch for option '${name}'.`);
    EXPECT_EQ(variable, option.variable, `Value mismatch for option '${name}'.`);
}

//@<> INCLUDE async_utils.inc

//@<> init
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("rset", {gtidSetIsComplete:true});

var session2 = mysql.getSession(__sandbox_uri2);

//@<> FR9 check addInstance options (fail)
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationConnectRetry: ""});
}, "Option 'replicationConnectRetry' Integer expected, but value is String");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationConnectRetry: -1});
}, "Invalid value for 'replicationConnectRetry' option. Value cannot be negative.");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationRetryCount: ""});
}, "Option 'replicationRetryCount' Integer expected, but value is String");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationRetryCount: -1});
}, "Invalid value for 'replicationRetryCount' option. Value cannot be negative.");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationHeartbeatPeriod: ""});
}, "Option 'replicationHeartbeatPeriod' Float expected, but value is String");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationHeartbeatPeriod: -1});
}, "Invalid value for 'replicationHeartbeatPeriod' option. Value cannot be negative.");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationHeartbeatPeriod: -0.1});
}, "Invalid value for 'replicationHeartbeatPeriod' option. Value cannot be negative.");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationCompressionAlgorithms: 11});
}, "Argument #2: Option 'replicationCompressionAlgorithms' is expected to be of type String, but is Integer");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationZstdCompressionLevel: ""});
}, "Option 'replicationZstdCompressionLevel' Integer expected, but value is String");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationZstdCompressionLevel: -23});
}, "Invalid value for 'replicationZstdCompressionLevel' option. Value cannot be negative.");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationBind: 22});
}, "Argument #2: Option 'replicationBind' is expected to be of type String, but is Integer");
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationNetworkNamespace: 33});
}, "Argument #2: Option 'replicationNetworkNamespace' is expected to be of type String, but is Integer");

//@<> FR9 check addInstance options

// fails because of network related options
WIPE_SHELL_LOG();

shell.options["logSql"] = "unfiltered"
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationConnectRetry: 10, replicationRetryCount: 11, replicationHeartbeatPeriod: 33.3, replicationCompressionAlgorithms: "zlib", replicationZstdCompressionLevel: "4", replicationBind: "foo", replicationNetworkNamespace: "bar"});
}, (__version_num >= 80400) ? "Error found in replication receiver thread" : "Replication thread not in expected state");
shell.options["logSql"] = "off"

EXPECT_SHELL_LOG_CONTAINS("SOURCE_CONNECT_RETRY=10");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_RETRY_COUNT=11");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_HEARTBEAT_PERIOD=33.3");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_COMPRESSION_ALGORITHMS='zlib'");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_ZSTD_COMPRESSION_LEVEL=4");
EXPECT_SHELL_LOG_CONTAINS("SOURCE_BIND='foo'");
EXPECT_SHELL_LOG_CONTAINS("NETWORK_NAMESPACE='bar'");

WIPE_SHELL_LOG();

shell.options["logSql"] = "on"
EXPECT_THROWS(function() {
    rs.addInstance(__sandbox_uri2, {replicationConnectRetry: 10, replicationRetryCount: 11, replicationHeartbeatPeriod: 33.3, replicationCompressionAlgorithms: "zlib", replicationZstdCompressionLevel: "4", replicationBind: "foo", replicationNetworkNamespace: "bar"});
}, (__version_num >= 80400) ? "Error found in replication receiver thread" : "Replication thread not in expected state");
shell.options["logSql"] = "off"

EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_CONNECT_RETRY=10");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_RETRY_COUNT=11");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_HEARTBEAT_PERIOD=33.3");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_COMPRESSION_ALGORITHMS='zlib'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_ZSTD_COMPRESSION_LEVEL=4");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_BIND='foo'");
EXPECT_SHELL_LOG_NOT_CONTAINS("NETWORK_NAMESPACE='bar'");

// must succeed
EXPECT_NO_THROWS(function() { rs.addInstance(__sandbox_uri2, {replicationConnectRetry: 20, replicationRetryCount: 22, replicationHeartbeatPeriod: 44.4, replicationCompressionAlgorithms: "zstd", replicationZstdCompressionLevel: "7"}); });

check_repl_option(session, session2, "opt_replConnectRetry", 20);
check_repl_option(session, session2, "opt_replRetryCount", 22);
check_repl_option(session, session2, "opt_replHeartbeatPeriod", 44.4);
check_repl_option(session, session2, "opt_replCompressionAlgorithms", "zstd");
check_repl_option(session, session2, "opt_replZstdCompressionLevel", 7);

//remove instance
rs.removeInstance(__sandbox_uri2);
reset_instance(session2);

//@<> FR9.1 / FR9.2 check addInstance options (don't use hard-coded values: if the server updates the defaults the test stops working)
WIPE_SHELL_LOG();

\option logSql = on
EXPECT_NO_THROWS(function() { rs.addInstance(__sandbox_uri2); });
\option --unset logSql

// "source_connect_retry" and "source_retry_count" are always present because they're used in the connectivity tests
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_HEARTBEAT_PERIOD");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_COMPRESSION_ALGORITHMS");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_ZSTD_COMPRESSION_LEVEL");
EXPECT_SHELL_LOG_NOT_CONTAINS("SOURCE_BIND");
EXPECT_SHELL_LOG_NOT_CONTAINS("NETWORK_NAMESPACE");

EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replConnectRetry' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replRetryCount' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replHeartbeatPeriod' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replCompressionAlgorithms' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replZstdCompressionLevel' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replBind' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);
EXPECT_EQ(null, session.runSql(`SELECT attributes->>'$.opt_replNetworkNamespace' FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

//remove instance
rs.removeInstance(__sandbox_uri2);
reset_instance(session2);

//@<> FR10 check setInstanceOption options (fail)
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationConnectRetry", "");
}, "Option 'replicationConnectRetry' only accepts an integer value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationConnectRetry", -1);
}, "Option 'replicationConnectRetry' doesn't support negative values.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationRetryCount", "");
}, "Option 'replicationRetryCount' only accepts an integer value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationRetryCount", -1);
}, "Option 'replicationRetryCount' doesn't support negative values.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationHeartbeatPeriod", "");
}, "Option 'replicationHeartbeatPeriod' only accepts a numeric value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationHeartbeatPeriod", -2);
}, "Option 'replicationHeartbeatPeriod' doesn't support negative values.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationHeartbeatPeriod", -0.1);
}, "Option 'replicationHeartbeatPeriod' doesn't support negative values.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationCompressionAlgorithms", 11);
}, "Option 'replicationCompressionAlgorithms' only accepts a string value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationZstdCompressionLevel", "");
}, "Option 'replicationZstdCompressionLevel' only accepts an integer value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationZstdCompressionLevel", -22);
}, "Option 'replicationZstdCompressionLevel' doesn't support negative values.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationBind", 22);
}, "Option 'replicationBind' only accepts a string value.");
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationNetworkNamespace", 33);
}, "Option 'replicationNetworkNamespace' only accepts a string value.");

//@<> FR10.1 / FR10.4 check if value is only stored in the metadata
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri1, "replicationConnectRetry", 14); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationConnectRetry' to '14' for the cluster member: '${hostname}:${__mysql_sandbox_port1}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the instance remains the primary instance of the ReplicaSet.");
check_repl_option_metadata(session, session, "opt_replConnectRetry", 14);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri1, "replicationHeartbeatPeriod", 38.9); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationHeartbeatPeriod' to '38.9' for the cluster member: '${hostname}:${__mysql_sandbox_port1}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the instance remains the primary instance of the ReplicaSet.");
check_repl_option_metadata(session, session, "opt_replHeartbeatPeriod", 38.9);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri1, "replicationCompressionAlgorithms", "zlib"); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationCompressionAlgorithms' to 'zlib' for the cluster member: '${hostname}:${__mysql_sandbox_port1}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect while the instance remains the primary instance of the ReplicaSet.");
check_repl_option_metadata(session, session, "opt_replCompressionAlgorithms", "zlib");

//@<> FR10.2 check if value is stored in the metadata as null (fails because it's the primary)
EXPECT_THROWS(function() {
    rs.setInstanceOption(__sandbox_uri1, "replicationBind", null);
}, "Option 'replicationBind' only accepts a string value.");

//@<> FR10.1 / FR10.2 / FR10.5 check if value is only stored in the metadata (including null) to a secondary instance
EXPECT_NO_THROWS(function() { rs.addInstance(__sandbox_uri2, {replicationZstdCompressionLevel: 5}); });
check_repl_option(session, session2, "opt_replZstdCompressionLevel", 5);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationRetryCount", 16); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationRetryCount' to '16' for the cluster member: '${hostname}:${__mysql_sandbox_port2}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until rejoinInstance() is called on the instance.");
check_repl_option_metadata_only(session, session2, "opt_replRetryCount", 16);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationZstdCompressionLevel", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationZstdCompressionLevel' to NULL for the cluster member: '${hostname}:${__mysql_sandbox_port2}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until rejoinInstance() is called on the instance.");
check_repl_option_metadata_only(session, session2, "opt_replZstdCompressionLevel", null);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationBind", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationBind' to NULL for the cluster member: '${hostname}:${__mysql_sandbox_port2}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until rejoinInstance() is called on the instance.");
check_repl_option_metadata_only(session, session2, "opt_replBind", null);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationNetworkNamespace", null); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationNetworkNamespace' to NULL for the cluster member: '${hostname}:${__mysql_sandbox_port2}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until rejoinInstance() is called on the instance.");
check_repl_option_metadata_only(session, session2, "opt_replNetworkNamespace", null);

//@<> FR10.3 should work even with an offline instance
session2.runSql("STOP REPLICA FOR CHANNEL '';");

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationCompressionAlgorithms", "zstd"); });
EXPECT_OUTPUT_CONTAINS(`Successfully set the value of 'replicationCompressionAlgorithms' to 'zstd' for the cluster member: '${hostname}:${__mysql_sandbox_port2}'.`)
EXPECT_OUTPUT_CONTAINS("WARNING: The new value won't take effect until rejoinInstance() is called on the instance.");
check_repl_option_metadata_only(session, session2, "opt_replCompressionAlgorithms", "zstd");

//@<> FR11 / FR11.2 rejoin instance should update the replication channel

//these 3 options are stored in the metadata, but not on the channel
check_repl_option_metadata_only(session, session2, "opt_replRetryCount", 16);
check_repl_option_metadata_only(session, session2, "opt_replCompressionAlgorithms", "zstd");
check_repl_option_metadata_only(session, session2, "opt_replZstdCompressionLevel", null);

WIPE_SHELL_LOG();

\option logSql = on
EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });
\option --unset logSql

//because we added a NULL option, the channel should have been reset
EXPECT_SHELL_LOG_CONTAINS("RESET REPLICA ALL FOR CHANNEL ''");

//they should now be on both and the null gone
check_repl_option(session, session2, "opt_replRetryCount", 16);
check_repl_option(session, session2, "opt_replCompressionAlgorithms", "zstd");
check_repl_option_metadata_not_present(session, session2, "opt_replZstdCompressionLevel");
check_repl_option_metadata_not_present(session, session2, "opt_replBind");
check_repl_option_metadata_not_present(session, session2, "opt_replNetworkNamespace");

//@<> FR11.3 no changes are needed, so the command should simply exit
EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' is ONLINE and replicating from '${hostname}:${__mysql_sandbox_port1}'.`)

//@<> FR11.4 simple change, no reset of the channel needed
shell.options["logSql"] = "on";
WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationRetryCount", 17); });
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationConnectRetry", 33); });
EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });
EXPECT_OUTPUT_CONTAINS(`The replication channel of instance '${hostname}:${__mysql_sandbox_port2}' was updated.`);
EXPECT_OUTPUT_CONTAINS(`The instance is replicating from '${hostname}:${__mysql_sandbox_port1}'.`);

if (__version_num >= 80400) {
    EXPECT_SHELL_LOG_NOT_CONTAINS("mysql.slave_master_info");
    EXPECT_SHELL_LOG_NOT_CONTAINS("mysql.slave_relay_log_info");
}

//@<> FR12 when switching the primary, the new replica instance must be configured properly

//these 3 options are stored in the metadata, but not on the channel
check_repl_option_metadata(session, session, "opt_replConnectRetry", 14);
check_repl_option_metadata(session, session, "opt_replHeartbeatPeriod", 38.9);
check_repl_option_metadata(session, session, "opt_replCompressionAlgorithms", "zlib");

EXPECT_NO_THROWS(function() { rs.setPrimaryInstance(__sandbox_uri2); });

//now the options should be on both
check_repl_option(session2, session, "opt_replConnectRetry", 14);
check_repl_option(session2, session, "opt_replHeartbeatPeriod", 38.9);
check_repl_option(session2, session, "opt_replCompressionAlgorithms", "zlib");

//change back to instance 2 and the previous options
EXPECT_NO_THROWS(function() { rs.setPrimaryInstance(__sandbox_uri1); });

check_repl_option(session, session2, "opt_replRetryCount", 17);
check_repl_option(session, session2, "opt_replConnectRetry", 33);
check_repl_option(session, session2, "opt_replCompressionAlgorithms", "zstd");

//@<> FR12 when forcing the primary, the new replica instance must be configured properly

testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rs = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { rs.forcePrimaryInstance(__sandbox_uri2); });

testutil.startSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri1); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port1}' rejoined the replicaset and is replicating from '${hostname}:${__mysql_sandbox_port2}'.`)

check_repl_option(session2, session, "opt_replConnectRetry", 14);
check_repl_option(session2, session, "opt_replHeartbeatPeriod", 38.9);
check_repl_option(session2, session, "opt_replCompressionAlgorithms", "zlib");

//and switch back...

session2.close();
testutil.stopSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
rs = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { rs.forcePrimaryInstance(__sandbox_uri1); });

testutil.startSandbox(__mysql_sandbox_port2);
var session2 = mysql.getSession(__sandbox_uri2);

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' rejoined the replicaset and is replicating from '${hostname}:${__mysql_sandbox_port1}'.`)

check_repl_option(session, session2, "opt_replRetryCount", 17);
check_repl_option(session, session2, "opt_replConnectRetry", 33);
check_repl_option(session, session2, "opt_replCompressionAlgorithms", "zstd");

//@<> FR13 verify that all replication options are present in options()

let options = rs.options();
let options1 = options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`];
let options2 = options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(options1, "replicationConnectRetry", null, 14);
check_option(options1, "replicationRetryCount", null, null);
check_option(options1, "replicationHeartbeatPeriod", null, 38.9);
check_option(options1, "replicationCompressionAlgorithms", null, "zlib");
check_option(options1, "replicationZstdCompressionLevel", null, null);
check_option(options1, "replicationBind", null, null);
check_option(options1, "replicationNetworkNamespace", null, null);

check_option(options2, "replicationConnectRetry", 33, 33);
check_option(options2, "replicationRetryCount", 17, 17);
check_option(options2, "replicationHeartbeatPeriod", 30, null);
check_option(options2, "replicationCompressionAlgorithms", "zstd", "zstd");
check_option(options2, "replicationZstdCompressionLevel", 3, null);
check_option(options2, "replicationBind", "", null);
check_option(options2, "replicationNetworkNamespace", "", null);

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri1, "replicationZstdCompressionLevel", 4); });
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationZstdCompressionLevel", 5); });

options = rs.options();
options1 = options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port1}`];
options2 = options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(options1, "replicationConnectRetry", null, 14);
check_option(options1, "replicationRetryCount", null, null);
check_option(options1, "replicationHeartbeatPeriod", null, 38.9);
check_option(options1, "replicationCompressionAlgorithms", null, "zlib");
check_option(options1, "replicationZstdCompressionLevel", null, 4);
check_option(options1, "replicationBind", null, null);
check_option(options1, "replicationNetworkNamespace", null, null);

check_option(options2, "replicationConnectRetry", 33, 33);
check_option(options2, "replicationRetryCount", 17, 17);
check_option(options2, "replicationHeartbeatPeriod", 30, null);
check_option(options2, "replicationCompressionAlgorithms", "zstd", "zstd");
check_option(options2, "replicationZstdCompressionLevel", 3, 5);
check_option(options2, "replicationBind", "", null);
check_option(options2, "replicationNetworkNamespace", "", null);

EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });

options = rs.options();
options2 = options.replicaSet.topology[`${hostname}:${__mysql_sandbox_port2}`];

check_option(options2, "replicationConnectRetry", 33, 33);
check_option(options2, "replicationRetryCount", 17, 17);
check_option(options2, "replicationHeartbeatPeriod", 30, null);
check_option(options2, "replicationCompressionAlgorithms", "zstd", "zstd");
check_option(options2, "replicationZstdCompressionLevel", 5, 5);
check_option(options2, "replicationBind", "", null);
check_option(options2, "replicationNetworkNamespace", "", null);

//@<> FR14 / BUG#35434803 check heartbeat period precision (which is 0.001)

var status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

// essentially, the "same" value as 30.0
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationHeartbeatPeriod", 30.0009); });

status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

check_repl_option_metadata(session, session2, "opt_replHeartbeatPeriod", 30.0009);

// not the "same" value as 30.0
EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationHeartbeatPeriod", 30.001); });

status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_EQ("WARNING: The instance replication channel is not configured as expected (to see the affected options, use options()). Please call rejoinInstance() to reconfigure the channel accordingly.", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0]);

check_repl_option_metadata(session, session2, "opt_replHeartbeatPeriod", 30.001);

EXPECT_NO_THROWS(function() { rs.rejoinInstance(__sandbox_uri2); });

check_repl_option(session, session2, "opt_replHeartbeatPeriod", 30.001);

//@<> FR14 verify that status warns about option mismatch

var status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationZstdCompressionLevel", null); });

status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_EQ("WARNING: The instance replication channel is not configured as expected (to see the affected options, use options()). Please call rejoinInstance() to reconfigure the channel accordingly.", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0]);

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationZstdCompressionLevel", 5); });

status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);

EXPECT_NO_THROWS(function() { rs.setInstanceOption(__sandbox_uri2, "replicationZstdCompressionLevel", 28); });

status = rs.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]);
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_EQ("WARNING: The instance replication channel is not configured as expected (to see the affected options, use options()). Please call rejoinInstance() to reconfigure the channel accordingly.", status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0]);

//@<> cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
