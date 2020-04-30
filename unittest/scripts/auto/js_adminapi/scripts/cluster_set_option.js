function print_metadata_clusters_cluster_name(session) {
    var res = session.runSql("select cluster_name from mysql_innodb_cluster_metadata.clusters")
    var row = res.fetchOne();
    print(row[0] + "\n");
}

function get_tags_for_instance(cluster, uri) {
    var opts = cluster.options();
    var res = {};
    if(typeof opts.defaultReplicaSet.tags[uri] == "undefined") {
        testutil.fail("Found no list of tags for '" + uri + "' on cluster.");
    }
    // Buid a dictionary of key, value pairs that we can use easily to retrieve all the keys and their values
    for (var i = 0; i < opts.defaultReplicaSet.tags[uri].length; ++i) {
        res[opts.defaultReplicaSet.tags[uri][i]["option"]] = opts.defaultReplicaSet.tags[uri][i]["value"];
    }
    return res;
}

function get_global_tags(cluster){
    return get_tags_for_instance(cluster,"global");
}

// WL#11465 AdminAPI: AdminAPI: change cluster member options
//
// Currently, it's not possible to change a previously configuration option
// of a running Cluster or Cluster active member in the AdminAPI.
// Such settings can be set during the calls to:
//
//     - dba.createCluster()
//     - <Cluster.>addInstance()
//     - <Cluster.>rejoinInstance()
//
// In order to support changing configuration options of cluster members
// individually or globally, the AdminAPI was extended with two new commands:
//
//     - <Cluster.>setOption(option, value)
//     - <Cluster.>setInstanceOption(instance, option, value)
//
// Each command has a defined set of accepted options supported by the AdminAPI.
//
// On top of that, in order to verify which are the cluster configuration options
// in place, a new command was added to the AdminAPI:
//
//     - <Cluster.>options([options])

//@<OUT> WL#11465: Create single-primary cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster
var session = scene.session

// Exceptions in setOption():
//  - If the 'option' parameter is empty.
//  - If the 'value' parameter is empty.
//  - If the 'option' parameter is invalid.
//
//  - If any of the cluster members do not support the configuration option passed in 'option'.
//  - If the value passed in 'option' is not valid for Group Replication.
//  - If the cluster has no visible quorum.
//  - If setting a Group Replication option and any of the cluster members is not ONLINE.

//@ WL#11465: ArgumentErrors of setOption
cluster.setOption();
cluster.setOption("memberWeight");
cluster.setOption("foobar", 1);

// F1.4 - The function shall not be allowed if the cluster does not have quorum or not all cluster members are ONLINE.
//@ WL#11465: Error when executing setOption on a cluster with 1 or more members not ONLINE
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.setOption("memberWeight", 25);

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum {VER(>=8.0.14)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setOption("memberWeight", 25);

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setOption("memberWeight", 25);

//@ WL#11465: Re-create the cluster
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F1.5.1.2 - The accepted values are:
//
// - exitStateAction
// - memberWeight
// - consistency
// - expelTimeout
// - clusterName

//@ WL#11465: setOption clusterName with invalid value for cluster-name
cluster.setOption("clusterName", "0_a");
cluster.setOption("clusterName", "_1234567890::_1234567890123456789012345678901");
cluster.setOption("clusterName", "::");

//@<OUT> WL#11465: setOption clusterName
cluster.setOption("clusterName", "newName");

//@<OUT> WL#11465: Verify clusterName changed correctly
print_metadata_clusters_cluster_name(session);

//@<OUT> WL#11465: setOption memberWeight {VER(>=8.0.0)}
cluster.setOption("memberWeight", 25);

//@<OUT> WL#11465: setOption memberWeight 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("memberWeight", 25);

//@<> WL#11465: Verify memberWeight changed correctly in instance 1
EXPECT_EQ(25, get_sysvar(session, "group_replication_member_weight"));

//@<> WL#11465: Verify memberWeight changed correctly in instance 2
EXPECT_EQ(25, get_sysvar(__mysql_sandbox_port2, "group_replication_member_weight"));

//@<> WL#11465: Verify memberWeight changed correctly in instance 3
EXPECT_EQ(25, get_sysvar(__mysql_sandbox_port3, "group_replication_member_weight"));

//@<ERR> WL#11465: setOption exitStateAction with invalid value
cluster.setOption("exitStateAction", "ABORT");

//@<OUT> WL#11465: setOption exitStateAction {VER(>=8.0.0)}
cluster.setOption("exitStateAction", "ABORT_SERVER");

//@<OUT> WL#11465: setOption exitStateAction 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("exitStateAction", "ABORT_SERVER");

//@<> WL#11465: Verify exitStateAction changed correctly in instance 1
EXPECT_EQ("ABORT_SERVER", get_sysvar(__mysql_sandbox_port1, "group_replication_exit_state_action"));

//@<> WL#11465: Verify exitStateAction changed correctly in instance 2
EXPECT_EQ("ABORT_SERVER", get_sysvar(__mysql_sandbox_port2, "group_replication_exit_state_action"));

//@<> WL#11465: Verify exitStateAction changed correctly in instance 3
EXPECT_EQ("ABORT_SERVER", get_sysvar(__mysql_sandbox_port3, "group_replication_exit_state_action"));

//@<OUT> WL#11465: setOption consistency {VER(>=8.0.14)}
cluster.setOption("consistency", "BEFORE_ON_PRIMARY_FAILOVER");

//@<> WL#11465: Verify consistency changed correctly in instance 1 {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port1, "group_replication_consistency"));

//@<> WL#11465: Verify consistency changed correctly in instance 2 {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port2, "group_replication_consistency"));

//@<> WL#11465: Verify consistency changed correctly in instance 3 {VER(>=8.0.14)}
EXPECT_EQ("BEFORE_ON_PRIMARY_FAILOVER", get_sysvar(__mysql_sandbox_port3, "group_replication_consistency"));

//@<OUT> WL#11465: setOption expelTimeout {VER(>=8.0.14)}
cluster.setOption("expelTimeout", 3500);

//@<> WL#11465: Verify expelTimeout changed correctly in instance 1 {VER(>=8.0.14)}
EXPECT_EQ(3500, get_sysvar(__mysql_sandbox_port1, "group_replication_member_expel_timeout"));

//@<> WL#11465: Verify expelTimeout changed correctly in instance 2 {VER(>=8.0.14)}
EXPECT_EQ(3500, get_sysvar(__mysql_sandbox_port2, "group_replication_member_expel_timeout"));

//@<> WL#11465: Verify expelTimeout changed correctly in instance 3 {VER(>=8.0.14)}
EXPECT_EQ(3500, get_sysvar(__mysql_sandbox_port3, "group_replication_member_expel_timeout"));

//@<OUT> WL#12066: TSF6_1 setOption autoRejoinTries {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", 2016);

//@ WL#12066: TSF2_4 setOption autoRejoinTries doesn't accept negative values {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", -1);

//@ WL#12066: TSF2_5 setOption autoRejoinTries doesn't accept values out of range {VER(>=8.0.16)}
cluster.setOption("autoRejoinTries", 2017);

//@<> WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 1 {VER(>=8.0.16)}
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries"));
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries", "PERSISTED"));

//@<> WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 2 {VER(>=8.0.16)}
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries"));
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries", "PERSISTED"));

//@<> WL#12066: TSF2_3 Verify autoRejoinTries changed correctly in instance 3 {VER(>=8.0.16)}
EXPECT_EQ(2016, get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries"));
EXPECT_EQ("2016", get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#13208: TS_FR2 verify disableClone cannot be set with setOption() to false in a 5.7 cluster {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("disableClone", false);

//@ WL#13208: TS_FR2 verify disableClone cannot be set with setOption() to true in a 5.7 cluster {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setOption("disableClone", true);

//@ WL#13208: TS_FR2_1 verify disableClone can be set with setOption() to false. {VER(>=8.0.17)}
cluster.setOption("disableClone", false);

//@ WL#13208: TS_FR2_2 verify disableClone is false with options(). {VER(>=8.0.17)}
cluster.options();

//@ WL#13208: TS_FR2_1 verify disableClone can be set with setOption() to true. {VER(>=8.0.17)}
cluster.setOption("disableClone", true);

//@ WL#13208: TS_FR2_2 verify disableClone is true with options(). {VER(>=8.0.17)}
cluster.options();

//@<> WL#13788: Re-create the cluster
scene.destroy();
scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
cluster = scene.cluster;
session = scene.session;

//@<> WL#13788: Argument errors with tags - TSFR1_4
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:tagname")}, "Cluster.setOption: Invalid number of arguments, expected 2 but got 1", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:invalid_symbol#", 123)}, "Cluster.setOption: 'invalid_symbol#' is not a valid tag identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:_invalid_builtin", 123)}, "Cluster.setOption: '_invalid_builtin' is not a valid built-in tag.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("unsupported_namespace:invalid_symbol#", 123)}, "Cluster.setOption: Namespace 'unsupported_namespace' not supported.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setOption(":invalid_symbol#", 123)}, "Cluster.setOption: ':invalid_symbol#' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:", 123)}, "Cluster.setOption: 'tag:' is not a valid identifier.", "ArgumentError");

//@<> WL#13788 Built-in tag values are validated and throw error if value cannot be converted to expected type - TSFR1_5
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:_hidden", "123")}, "Cluster.setOption: Built-in tag '_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:_hidden", [true])}, "Cluster.setOption: Built-in tag '_hidden' is expected to be of type Bool, but is Array", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", "True")}, "Cluster.setOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", [123])}, "Cluster.setOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is Array", "TypeError");

//@<> WL#13788 Validate cluster.options shows values about the tags set via setOption - TSFR1_7
// we are using the output of cluster.options to validate the tag was set.
EXPECT_ARRAY_NOT_CONTAINS("test_string", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_NOT_CONTAINS("test_int", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_global_tags(cluster)));

cluster.setOption("tag:test_string", "test");
cluster.setOption("tag:test_int", 123);
cluster.setOption("tag:test_bool", true);

EXPECT_ARRAY_CONTAINS("test_string", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_CONTAINS("test_int", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_global_tags(cluster)));
EXPECT_TRUE(get_global_tags(cluster)["test_string"] === "test");
EXPECT_TRUE(get_global_tags(cluster)["test_bool"] === true);
EXPECT_TRUE(get_global_tags(cluster)["test_int"] === 123);


//@<> WL#13788: SetOption must allow setting tags for instances as long as there is quorum TSFR1_6
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

// TSFR1_3 use setOption to set built-in tags and check that they are converted correctly if possible and saved with the expected type
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_NOT_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_global_tags(cluster)));

cluster.setOption("tag:_hidden", false);
cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", true);

EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_global_tags(cluster)));
EXPECT_ARRAY_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_global_tags(cluster)));
EXPECT_TRUE(get_global_tags(cluster)["_hidden"] === false);
EXPECT_TRUE(get_global_tags(cluster)["_disconnect_existing_sessions_when_hidden"] === true);

cluster.setOption("tag:_hidden", 1);
cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", 0.0);

EXPECT_TRUE(get_global_tags(cluster)["_hidden"] === true);
EXPECT_TRUE(get_global_tags(cluster)["_disconnect_existing_sessions_when_hidden"] === false);

cluster.setOption("tag:_hidden", "false");
cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", "true");

EXPECT_TRUE(get_global_tags(cluster)["_hidden"] === false);
EXPECT_TRUE(get_global_tags(cluster)["_disconnect_existing_sessions_when_hidden"] === true);

cluster.setOption("tag:_hidden", -1);
cluster.setOption("tag:_disconnect_existing_sessions_when_hidden", 0.1);

EXPECT_TRUE(get_global_tags(cluster)["_hidden"] === true);
EXPECT_TRUE(get_global_tags(cluster)["_disconnect_existing_sessions_when_hidden"] === true);

//@<> WL#13788: TSFR3_2 Setting a null value to a tag, removes the tag from the metadata
EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_global_tags(cluster)));
cluster.setOption("tag:_hidden", null);
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_global_tags(cluster)));

// Setting a non existing tag to null, throws no error
EXPECT_NO_THROWS(function(){cluster.setOption("tag:non_existing", null)});
EXPECT_ARRAY_NOT_CONTAINS("non_existing", Object.keys(get_global_tags(cluster)));

//@<> WL#13788: SetOption must not allow setting tags for instances if there is no quorum TSFR1_6
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
EXPECT_THROWS_TYPE(function(){cluster.setOption("tag:test1", "test")}, "There is no quorum to perform the operation", "MYSQLSH");

//@<> Finalization
cluster.disconnect();
session.close();
scene.destroy();
