function print_metadata_instance_label(session, address) {
  var res = session.runSql("select instance_name from mysql_innodb_cluster_metadata.instances where addresses->'$.mysqlClassic' = '" + address + "'");
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
//   - If the 'instance' parameter is empty.
//   - If the 'instance' definition is invalid.
//   - If the 'instance' definition is a connection dictionary but empty.
//   - If the 'option' parameter is empty.
//   - If the 'value' parameter is empty.
//   - If the 'option' parameter is invalid.
//
//   - If 'instance' does not refer to a cluster member
//   - If the cluster has no visible quorum.
//   - If 'instance' is not ONLINE.
//   - If 'instance' does not support the configuration option passed in 'option'.
//   - If the value passed in 'option' is not valid for Group Replication.


//@ WL#11465: ArgumentErrors of setInstanceOption
cluster.setInstanceOption();
cluster.setInstanceOption("", "memberWeight", 1);
cluster.setInstanceOption(1234, "memberWeight", 1);
cluster.setInstanceOption({}, "memberWeight", 1);
cluster.setInstanceOption(__sandbox_uri2);
cluster.setInstanceOption(__sandbox_uri2, "memberWeight");
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", false);
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 13.37);
cluster.setInstanceOption(__sandbox_uri2, "foobar", 1);
cluster.setInstanceOption(null, "label", "newLabel");

//@ WL#11465: F2.2.1.2 - Remove instance 2 from the cluster
cluster.removeInstance(__sandbox_uri2);

// F2.2.1.2 - The function shall not be allowed if the target 'instance' does not belong to the cluster.
//@<ERR> WL#11465: Error when executing setInstanceOption for a target instance that does not belong to the cluster
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@ WL#11465: F2.2.1.2 - Add instance 2 back to the cluster
cluster.addInstance(__sandbox_uri2);

// F2.2.1.1 - The function shall not be allowed if the cluster does not have quorum or the target 'instance' is not reachable.
//@ WL#11465: Error when executing setInstanceOption when the target instance is not reachable
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");
cluster.setInstanceOption(__sandbox_uri3, "memberWeight", 25);

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum {VER(>=8.0.14)}
testutil.startSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@<ERR> WL#11465: Error when executing setInstanceOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
testutil.startSandbox(__mysql_sandbox_port3);
cluster.rejoinInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

scene.make_no_quorum([__mysql_sandbox_port1])
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@ WL#11465: Re-create the cluster
scene.destroy();
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster

// F1.5.1.2 - The accepted values are:
//
// - label
// - exitStateAction
// - memberWeight

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 1
cluster.setInstanceOption(__sandbox_uri2, "label", "-0_a");

//@<ERR> WL#11465: setInstanceOption label with invalid value for label 2
var __invalid_label = hostname + ":" + __mysql_sandbox_port1;

cluster.setInstanceOption(__sandbox_uri2, "label", __invalid_label);

//@<OUT> WL#11465: setInstanceOption label
cluster.setInstanceOption(__sandbox_uri2, "label", "newLabel");

session.close();
shell.connect(__sandbox_uri2);
//@<OUT> WL#11465: Verify label changed correctly
var __address2 = hostname + ":" + __mysql_sandbox_port2;
print_metadata_instance_label(session, __address2);

//@<OUT> WL#11465: setInstanceOption memberWeight {VER(>=8.0.0)}
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@<OUT> WL#11465: setInstanceOption memberWeight 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setInstanceOption(__sandbox_uri2, "memberWeight", 25);

//@<OUT> WL#11465: memberWeight label changed correctly
print(get_sysvar(session, "group_replication_member_weight"));

//@<ERR> WL#11465: setInstanceOption exitStateAction with invalid value
cluster.setInstanceOption(__sandbox_uri2, "exitStateAction", "ABORT");

//@<OUT> WL#11465: setInstanceOption exitStateAction {VER(>=8.0.0)}
cluster.setInstanceOption(__sandbox_uri2, "exitStateAction", "ABORT_SERVER");

//@<OUT> WL#11465: setInstanceOption exitStateAction 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
cluster.setInstanceOption(__sandbox_uri2, "exitStateAction", "ABORT_SERVER");

//@<OUT> WL#11465: exitStateAction label changed correctly
print(get_sysvar(session, "group_replication_exit_state_action"));
session.close();

//@<OUT> WL#12066: TSF6_1 setInstanceOption autoRejoinTries {VER(>=8.0.16)}
cluster.setInstanceOption(__sandbox_uri1, "autoRejoinTries", 2016);
cluster.setInstanceOption(__sandbox_uri2, "autoRejoinTries", 20);
cluster.setInstanceOption(__sandbox_uri3, "autoRejoinTries", 0);

//@ WL#12066: TSF3_4 setInstanceOption autoRejoinTries doesn't accept negative values {VER(>=8.0.16)}
cluster.setInstanceOption(__sandbox_uri1, "autoRejoinTries", -1);

//@ WL#12066: TSF3_5 setInstanceOption autoRejoinTries doesn't accept values out of range {VER(>=8.0.16)}
cluster.setInstanceOption(__sandbox_uri1, "autoRejoinTries", 2017);

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 1 {VER(>=8.0.16)}
print(get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries"));
print(get_sysvar(__mysql_sandbox_port1, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 2 {VER(>=8.0.16)}
print(get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries"));
print(get_sysvar(__mysql_sandbox_port2, "group_replication_autorejoin_tries", "PERSISTED"));

//@ WL#12066: TSF3_3 Verify autoRejoinTries changed correctly in instance 3 {VER(>=8.0.16)}
print(get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries"));
print(get_sysvar(__mysql_sandbox_port3, "group_replication_autorejoin_tries", "PERSISTED"));

//@<> Check that setInstanceOption works if you have quorum and the instance reachable but not online
// keep instance 3 in RECOVERING state by setting a wrong recovery user.
session3 = shell.connect(__sandbox_uri3);
session3.runSql("CHANGE MASTER TO MASTER_USER = 'not_exist', MASTER_PASSWORD = '' FOR CHANNEL 'group_replication_recovery'");
session3.runSql("STOP GROUP_REPLICATION");
session3.runSql("START GROUP_REPLICATION");
testutil.waitMemberState(__mysql_sandbox_port3, "RECOVERING");
cluster.setInstanceOption(__sandbox_uri3, "memberWeight", 28);
EXPECT_EQ(get_sysvar(session3, "group_replication_member_weight"), 28);

//@<> WL#13788 Check that setInstanceOption allows setting a tag if you have quorum, the and instance is reachable even if it is not online
// Verify it is still in recovering
testutil.waitMemberState(__mysql_sandbox_port3, "RECOVERING");
EXPECT_ARRAY_NOT_CONTAINS("recovering_tag", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
cluster.setInstanceOption(__sandbox_uri3, "tag:recovering_tag", true);
EXPECT_ARRAY_CONTAINS("recovering_tag", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["recovering_tag"] === true);

//@<> WL#13788: Re-create the cluster
session3.close();
scene.destroy();
scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
cluster = scene.cluster;
session = scene.session;

//@<> WL#13788: Argument errors with tags - TSFR1_4
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:tagname")}, "Cluster.setInstanceOption: Invalid number of arguments, expected 3 but got 2", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:invalid_symbol#", 123)}, "Cluster.setInstanceOption: 'invalid_symbol#' is not a valid tag identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_invalid_builtin", 123)}, "Cluster.setInstanceOption: '_invalid_builtin' is not a valid built-in tag.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "unsupported_namespace:invalid_symbol#", 123)}, "Cluster.setInstanceOption: Namespace 'unsupported_namespace' not supported.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, ":invalid_symbol#", 123)}, "Cluster.setInstanceOption: ':invalid_symbol#' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:", 123)}, "Cluster.setInstanceOption: 'tag:' is not a valid identifier.", "ArgumentError");

//@<> WL#13788 Built-in tag values are validated and throw error if value cannot be converted to expected type - TSFR1_5
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_hidden", "123")}, "Cluster.setInstanceOption: Built-in tag '_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_hidden", [true])}, "Cluster.setInstanceOption: Built-in tag '_hidden' is expected to be of type Bool, but is Array", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", "True")}, "Cluster.setInstanceOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", [123])}, "Cluster.setInstanceOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is Array", "TypeError");

//@<> WL#13788 Validate cluster.options shows values about the tags set via setInstanceOption - TSFR1_7
// we are using the output of cluster.options to validate the tag was set.
EXPECT_ARRAY_NOT_CONTAINS("test_string", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_int", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_tags_for_instance(cluster, __endpoint2)));

cluster.setInstanceOption(__sandbox_uri1, "tag:test_string", "test");
cluster.setInstanceOption(__sandbox_uri1, "tag:test_int", 123);
cluster.setInstanceOption(__sandbox_uri1, "tag:test_bool", false);
cluster.setInstanceOption(__sandbox_uri2, "tag:test_bool", true);

EXPECT_ARRAY_CONTAINS("test_string", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_int", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_tags_for_instance(cluster, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_tags_for_instance(cluster, __endpoint2)));

EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint1)["test_string"] === "test");
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint1)["test_int"] === 123);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint1)["test_bool"] === false);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint2)["test_bool"] === true);


//@<> WL#13788: SetInstanceOption must not allow setting tags for an instance if it is unreachable.
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "(MISSING),UNREACHABLE");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:_hidden", false)}, "Cluster.setInstanceOption: The instance '" + __sandbox2 + "' is not reachable.", "RuntimeError");

//@<> WL#13788 setInstanceOption must not allow settings tags for an instance that doesn't belong to the cluster.
testutil.deploySandbox(__mysql_sandbox_port4, "root", {report_host: hostname});
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri4, "tag:_hidden", false)}, "Cluster.setInstanceOption: The instance '" + __sandbox4 + "' does not belong to the Cluster.", "RuntimeError");
testutil.destroySandbox(__mysql_sandbox_port4);

//@<> WL#13788 use setInstanceOption to set built-in tags and check that they are saved correctly as long as there is quorum TSFR1_3
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
EXPECT_ARRAY_NOT_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));

cluster.setInstanceOption(__sandbox_uri3, "tag:_hidden", false);
cluster.setInstanceOption(__sandbox_uri3, "tag:_disconnect_existing_sessions_when_hidden", true);

EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
EXPECT_ARRAY_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_hidden"] === false);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_disconnect_existing_sessions_when_hidden"] === true);

// Values are converted and saved as the expected type
cluster.setInstanceOption(__sandbox_uri3, "tag:_hidden", 1);
cluster.setInstanceOption(__sandbox_uri3, "tag:_disconnect_existing_sessions_when_hidden", 0);

EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_hidden"] === true);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_disconnect_existing_sessions_when_hidden"] === false);

cluster.setInstanceOption(__sandbox_uri3, "tag:_hidden", "false");
cluster.setInstanceOption(__sandbox_uri3, "tag:_disconnect_existing_sessions_when_hidden", "true");

EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_hidden"] === false);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_disconnect_existing_sessions_when_hidden"] === true);

cluster.setInstanceOption(__sandbox_uri3, "tag:_hidden", -1);
cluster.setInstanceOption(__sandbox_uri3, "tag:_disconnect_existing_sessions_when_hidden", 0.1);

EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_hidden"] === true);
EXPECT_TRUE(get_tags_for_instance(cluster, __endpoint3)["_disconnect_existing_sessions_when_hidden"] === true);

//@<> WL#13788: TSFR3_2 Setting a null value to a tag, removes the tag from the metadata
EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));
cluster.setInstanceOption(__sandbox_uri3, "tag:_hidden", null);
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_tags_for_instance(cluster, __endpoint3)));

// Setting a non existing tag to null, throws no error
EXPECT_NO_THROWS(function(){cluster.setInstanceOption(__sandbox_uri3, "tag:non_existing", null)});
EXPECT_ARRAY_NOT_CONTAINS("non_existing", Object.keys(get_tags_for_instance(cluster, __endpoint3)));

//@<> WL#13788: SetInstanceOption must not allow setting tags for instances if there is no quorum TSFR1_6
testutil.killSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING),UNREACHABLE");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri1, "tag:test1", "test")}, "There is no quorum to perform the operation", "RuntimeError");
EXPECT_THROWS_TYPE(function(){cluster.setInstanceOption(__sandbox_uri2, "tag:test1", "test")}, "There is no quorum to perform the operation", "RuntimeError");

//@<>Finalization
cluster.disconnect();
session.close();
scene.destroy();
