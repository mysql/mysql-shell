//@ {VER(>=8.0.11)}

function get_tags_for_instance(replicaset, uri) {
    var opts = replicaset.options();
    var res = {};
    if(typeof opts.replicaSet.tags[uri] == "undefined") {
        testutil.fail("Found no list of tags for '" + uri + "' on replicaSet.");
    }
    // Buid a dictionary of key, value pairs that we can use easily to retrieve all the keys and their values
    for (var i = 0; i < opts.replicaSet.tags[uri].length; ++i) {
        res[opts.replicaSet.tags[uri][i]["option"]] = opts.replicaSet.tags[uri][i]["value"];
    }
    return res;
}

function get_global_tags(replicaset){
    return get_tags_for_instance(replicaset,"global");
}

//@<> WL#13788 Deploy replicaset instances and create ReplicaSet
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@<> WL#13788 argument errors of ReplicaSet.setInstanceOption TSFR2_5
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:tagname")}, "ReplicaSet.setInstanceOption: Invalid number of arguments, expected 3 but got 2", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:invalid_symbol#", 123)}, "ReplicaSet.setInstanceOption: 'invalid_symbol#' is not a valid tag identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_invalid_builtin", 123)}, "ReplicaSet.setInstanceOption: '_invalid_builtin' is not a valid built-in tag.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "unsupported_namespace:invalid_symbol#", 123)}, "ReplicaSet.setInstanceOption: Namespace 'unsupported_namespace' not supported.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, ":invalid_symbol#", 123)}, "ReplicaSet.setInstanceOption: ':invalid_symbol#' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:", 123)}, "ReplicaSet.setInstanceOption: 'tag:' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "not_a_tag", 123)}, "ReplicaSet.setInstanceOption: Option 'not_a_tag' not supported.", "ArgumentError");

//@<> WL#13788 Built-in tag values are validated and throw error if value cannot be converted to expected type - TSFR2_6
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", "123")}, "ReplicaSet.setInstanceOption: Built-in tag '_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", [true])}, "ReplicaSet.setInstanceOption: Built-in tag '_hidden' is expected to be of type Bool, but is Array", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", "True")}, "ReplicaSet.setInstanceOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", [123])}, "ReplicaSet.setInstanceOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is Array", "TypeError");

//@<> WL#13788 Validate replicaSet.options shows values about the tags set via setInstanceOption - TSFR2_7
// we are using the output of replicaSet.options to validate the tag was set.
EXPECT_ARRAY_NOT_CONTAINS("test_string", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_int", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_tags_for_instance(rs, __endpoint2)));

rs.setInstanceOption(__sandbox_uri1, "tag:test_string", "test");
rs.setInstanceOption(__sandbox_uri1, "tag:test_int", 123);
rs.setInstanceOption(__sandbox_uri1, "tag:test_bool", false);
rs.setInstanceOption(__sandbox_uri2, "tag:test_bool", true);

EXPECT_ARRAY_CONTAINS("test_string", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_int", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_tags_for_instance(rs, __endpoint1)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_tags_for_instance(rs, __endpoint2)));

EXPECT_TRUE(get_tags_for_instance(rs, __endpoint1)["test_string"] === "test");
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint1)["test_int"] === 123);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint1)["test_bool"] === false);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["test_bool"] === true);

//@<> WL#13788: setInstanceOption must not allow setting tags for an instance if it is unreachable.
testutil.stopSandbox(__mysql_sandbox_port2, {wait:1});
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", false)}, "ReplicaSet.setInstanceOption: The instance '" + __sandbox2 + "' is not reachable.", "RuntimeError");

//@<> WL#13788 setInstanceOption must not allow settings tags for an instance that doesn't belong to the replicaSet.
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});
EXPECT_THROWS_TYPE(function(){rs.setInstanceOption(__sandbox_uri3, "tag:_hidden", false)}, "ReplicaSet.setInstanceOption: The instance '" + __sandbox3 + "' does not belong to the ReplicaSet.", "RuntimeError");
testutil.destroySandbox(__mysql_sandbox_port3);

//@<> WL#13788 use setOption to set built-in tags and check that they are saved correctly TSFR2_4
testutil.startSandbox(__mysql_sandbox_port2);
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));
EXPECT_ARRAY_NOT_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));

rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", false);
rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", true);

EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));
EXPECT_ARRAY_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_hidden"] === false);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_disconnect_existing_sessions_when_hidden"] === true);

// Values are converted and saved as the expected type
rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", 1);
rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", 0);

EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_hidden"] === true);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_disconnect_existing_sessions_when_hidden"] === false);

rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", "false");
rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", "true");

EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_hidden"] === false);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_disconnect_existing_sessions_when_hidden"] === true);

rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", -1);
rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", 0.1);

EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_hidden"] === true);
EXPECT_TRUE(get_tags_for_instance(rs, __endpoint2)["_disconnect_existing_sessions_when_hidden"] === true);

//@<> WL#13788: TSFR3_2 Setting a null value to a tag, removes the tag from the metadata
EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));
rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", null);
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_tags_for_instance(rs, __endpoint2)));

// Setting a non existing tag to null, throws no error
EXPECT_NO_THROWS(function(){rs.setInstanceOption(__sandbox_uri1, "tag:non_existing", null)});
EXPECT_ARRAY_NOT_CONTAINS("non_existing", Object.keys(get_tags_for_instance(rs, __endpoint1)));

//@<> Cleanup
session.close();
rs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);