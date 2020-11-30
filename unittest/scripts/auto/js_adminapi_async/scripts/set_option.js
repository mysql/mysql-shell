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

//@<> WL#13788 Argument errors of ReplicaSet.setOption TSFR_2_5
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:tagname")}, "ReplicaSet.setOption: Invalid number of arguments, expected 2 but got 1", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:invalid_symbol#", 123)}, "ReplicaSet.setOption: 'invalid_symbol#' is not a valid tag identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:_invalid_builtin", 123)}, "ReplicaSet.setOption: '_invalid_builtin' is not a valid built-in tag.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption("unsupported_namespace:invalid_symbol#", 123)}, "ReplicaSet.setOption: Namespace 'unsupported_namespace' not supported.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption(":invalid_symbol#", 123)}, "ReplicaSet.setOption: ':invalid_symbol#' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:", 123)}, "ReplicaSet.setOption: 'tag:' is not a valid identifier.", "ArgumentError");
EXPECT_THROWS_TYPE(function(){rs.setOption("not_a_tag", 123)}, "ReplicaSet.setOption: Option 'not_a_tag' not supported.", "ArgumentError");

//@<> WL#13788 Built-in tag values are validated and throw error if value cannot be converted to expected type - TSFR2_6
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:_hidden", "123")}, "ReplicaSet.setOption: Built-in tag '_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:_hidden", [true])}, "ReplicaSet.setOption: Built-in tag '_hidden' is expected to be of type Bool, but is Array", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:_disconnect_existing_sessions_when_hidden", "invalid")}, "ReplicaSet.setOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is String", "TypeError");
EXPECT_THROWS_TYPE(function(){rs.setOption("tag:_disconnect_existing_sessions_when_hidden", [123])}, "ReplicaSet.setOption: Built-in tag '_disconnect_existing_sessions_when_hidden' is expected to be of type Bool, but is Array", "TypeError");

//@<> WL#13788 Validate ReplicaSet.options shows values about the tags set via setOption - TSFR2_7
// we are using the output of ReplicaSet.options to validate the tag was set.
EXPECT_ARRAY_NOT_CONTAINS("test_string", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_NOT_CONTAINS("test_int", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_NOT_CONTAINS("test_bool", Object.keys(get_global_tags(rs)));

rs.setOption("tag:test_string", "test");
rs.setOption("tag:test_int", 123);
rs.setOption("tag:test_bool", true);

EXPECT_ARRAY_CONTAINS("test_string", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_CONTAINS("test_int", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_CONTAINS("test_bool", Object.keys(get_global_tags(rs)));
EXPECT_TRUE(get_global_tags(rs)["test_string"] === "test");
EXPECT_TRUE(get_global_tags(rs)["test_bool"] === true);
EXPECT_TRUE(get_global_tags(rs)["test_int"] === 123);

//@<> WL#13788: use setOption to set built-in tags and check that they are converted (if needed) and saved correctly TSFR2_4
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_NOT_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_global_tags(rs)));

rs.setOption("tag:_hidden", false);
rs.setOption("tag:_disconnect_existing_sessions_when_hidden", true);

EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_global_tags(rs)));
EXPECT_ARRAY_CONTAINS("_disconnect_existing_sessions_when_hidden", Object.keys(get_global_tags(rs)));
EXPECT_TRUE(get_global_tags(rs)["_hidden"] === false);
EXPECT_TRUE(get_global_tags(rs)["_disconnect_existing_sessions_when_hidden"] === true);


rs.setOption("tag:_hidden", 1);
rs.setOption("tag:_disconnect_existing_sessions_when_hidden", 0.0);

EXPECT_TRUE(get_global_tags(rs)["_hidden"] === true);
EXPECT_TRUE(get_global_tags(rs)["_disconnect_existing_sessions_when_hidden"] === false);

rs.setOption("tag:_hidden", "false");
rs.setOption("tag:_disconnect_existing_sessions_when_hidden", "true");

EXPECT_TRUE(get_global_tags(rs)["_hidden"] === false);
EXPECT_TRUE(get_global_tags(rs)["_disconnect_existing_sessions_when_hidden"] === true);

rs.setOption("tag:_hidden", -1);
rs.setOption("tag:_disconnect_existing_sessions_when_hidden", 0.1);

EXPECT_TRUE(get_global_tags(rs)["_hidden"] === true);
EXPECT_TRUE(get_global_tags(rs)["_disconnect_existing_sessions_when_hidden"] === true);

//@<> WL#13788: TSFR3_2 Setting a null value to a tag, removes the tag from the metadata
EXPECT_ARRAY_CONTAINS("_hidden", Object.keys(get_global_tags(rs)));
rs.setOption("tag:_hidden", null);
EXPECT_ARRAY_NOT_CONTAINS("_hidden", Object.keys(get_global_tags(rs)));

// Setting a non existing tag to null, throws no error
EXPECT_NO_THROWS(function(){rs.setOption("tag:non_existing", null)});
EXPECT_ARRAY_NOT_CONTAINS("non_existing", Object.keys(get_global_tags(rs)));

//@<> Cleanup
session.close();
rs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);