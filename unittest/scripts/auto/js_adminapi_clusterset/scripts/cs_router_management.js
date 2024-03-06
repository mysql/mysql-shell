//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> INCLUDE metadata_schema_utils.inc

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});

function callMysqlsh(command_line_args) {
  testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
}


shell.options.useWizards = false;

shell.connect(__sandbox_uri1);
EXPECT_NO_THROWS(function() {cluster = dba.createCluster("cluster", {gtidSetIsComplete:1}); });

EXPECT_NO_THROWS(function() {clusterset = cluster.createClusterSet("clusterset"); });

EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri2, "replicacluster"); });

//@<> create routers
var clusterset_id = session.runSql("SELECT clusterset_id FROM mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\", \"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', NULL, NULL, ?)", [clusterset_id]);
var cm_router = "routerhost1::system";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router = "routerhost2::system";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (3, 'another', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router2 = "routerhost2::another";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (4, '', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router3 = "routerhost2::";

//@<> clusterset.routingOptions on invalid router
EXPECT_THROWS(function(){ clusterset.routingOptions("invalid_router"); }, "Router 'invalid_router' is not registered in the clusterset");

//@ clusterset.routingOptions() with all defaults
values = clusterset.routingOptions();

//@<> clusterset.setRoutingOption, all valid values, tag
clusterset.setRoutingOption(cm_router, "tag:test_tag", 567);
EXPECT_JSON_EQ(567, clusterset.routingOptions(cm_router)[cm_router]["tags"]["test_tag"]);
EXPECT_JSON_EQ(567, clusterset.routingOptions()["routers"][cm_router]["tags"]["test_tag"]);

clusterset.setRoutingOption("tag:test_tag", 567);
EXPECT_JSON_EQ(567, clusterset.routingOptions()["global"]["tags"]["test_tag"]);

clusterset.setRoutingOption("tag:test_tag", null);
clusterset.setRoutingOption(cm_router, "tag:test_tag", null);
EXPECT_JSON_EQ(null, clusterset.routingOptions()["routers"][cm_router]["tags"]["test_tag"]);
EXPECT_JSON_EQ(null, clusterset.routingOptions()["global"]["tags"]["test_tag"]);

clusterset.setRoutingOption("tags", null);
clusterset.setRoutingOption(cm_router, "tags", null);
EXPECT_JSON_EQ(undefined, clusterset.routingOptions()["routers"][cm_router]["tags"]);
EXPECT_JSON_EQ({}, clusterset.routingOptions()["global"]["tags"]);

//@<> clusterset.setRoutingOption, all valid values
function CHECK_SET_ROUTING_OPTION(option, value, expected_value) {
  orig_options = clusterset.routingOptions();

  router_options = clusterset.routingOptions(cm_router);
  global_options = clusterset.routingOptions();

  clusterset.setRoutingOption(cm_router, option, value);
  router_options[cm_router][option] = expected_value;
  global_options["routers"][cm_router][option] = expected_value;
  EXPECT_JSON_EQ(router_options, clusterset.routingOptions(cm_router), "router check");
  EXPECT_JSON_EQ(global_options, clusterset.routingOptions(), "router check 2");

  clusterset.setRoutingOption(option, value);
  global_options["global"][option] = expected_value;
  EXPECT_JSON_EQ(global_options, clusterset.routingOptions(), "global check");

  // setting option to null should reset to default
  clusterset.setRoutingOption(option, null);
  clusterset.setRoutingOption(cm_router, option, null);
  EXPECT_JSON_EQ(orig_options, clusterset.routingOptions(), "original check");
}

CHECK_SET_ROUTING_OPTION("target_cluster", "primary", "primary");

// BUG#33298735 clusterset: inconsistent case in/sensitivity in clustername
// Verify combinations of the same name using lower and uppercase to test that the command accepts it
CHECK_SET_ROUTING_OPTION("target_cluster", "cluster", "cluster");
CHECK_SET_ROUTING_OPTION("target_cluster", "Cluster", "cluster");
CHECK_SET_ROUTING_OPTION("target_cluster", "CLUSTER", "cluster");

CHECK_SET_ROUTING_OPTION('invalidated_cluster_policy', 'drop_all', 'drop_all');
CHECK_SET_ROUTING_OPTION('invalidated_cluster_policy', 'accept_ro', 'accept_ro');

CHECK_SET_ROUTING_OPTION('stats_updates_frequency', 1, 1);
CHECK_SET_ROUTING_OPTION('stats_updates_frequency', 15, 15);

CHECK_SET_ROUTING_OPTION('use_replica_primary_as_rw', false, false);
CHECK_SET_ROUTING_OPTION('use_replica_primary_as_rw', true, true);

CHECK_SET_ROUTING_OPTION('tags', {}, {});
CHECK_SET_ROUTING_OPTION('tags', { "a": 123 }, { "a": 123 });

CHECK_SET_ROUTING_OPTION('read_only_targets', 'all', 'all');
CHECK_SET_ROUTING_OPTION('read_only_targets', 'read_replicas', 'read_replicas');
CHECK_SET_ROUTING_OPTION('read_only_targets', 'secondaries', 'secondaries');

//@<> default values filled in when metadata is missing some option (e.g. upgrade)
var full_options = clusterset.routingOptions();

var router_options = session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]
session.runSql("update mysql_innodb_cluster_metadata.clustersets set router_options='{}'");

EXPECT_JSON_EQ(full_options, clusterset.routingOptions());

session.runSql("update mysql_innodb_cluster_metadata.clustersets set router_options=?", [router_options]);
EXPECT_JSON_EQ(full_options, clusterset.routingOptions());

//@<> BUG#34458017: setRoutingOption to null resets all options
// Each option should be reset individually and not all of them
clusterset.setRoutingOption("stats_updates_frequency", 42);
clusterset.setRoutingOption(cm_router, "stats_updates_frequency", 44);
var orig = clusterset.routingOptions();

// WL15601 NFR2: value must be present in the MD
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);
options = JSON.parse(session.runSql("select options from mysql_innodb_cluster_metadata.routers").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);

clusterset.setRoutingOption(cm_router, "stats_updates_frequency", null);
delete orig["routers"][cm_router]["stats_updates_frequency"];
EXPECT_JSON_EQ(orig, clusterset.routingOptions());

// WL15601 NFR2: value must *not* be present in the MD
options = JSON.parse(session.runSql("select options from mysql_innodb_cluster_metadata.routers").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);

clusterset.setRoutingOption("stats_updates_frequency", null);
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);
clusterset.setRoutingOption("stats_updates_frequency", 42);

//@<> set individual tags
clusterset.setRoutingOption("tags", {"old":"oldvalue"});

clusterset.setRoutingOption("tag:test_tag", 1234);
clusterset.setRoutingOption("tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, clusterset.routingOptions()["global"]["tags"]);
clusterset.setRoutingOption("tags", {});
EXPECT_JSON_EQ({}, clusterset.routingOptions()["global"]["tags"]);

clusterset.setRoutingOption(cm_router, "tags", {"old":"oldvalue"});
clusterset.setRoutingOption(cm_router, "tags", {"test_tag":1234});
EXPECT_JSON_EQ({"test_tag":1234}, clusterset.routingOptions()["routers"][cm_router]["tags"]);
clusterset.setRoutingOption(cm_router, "tags", {});
EXPECT_JSON_EQ({}, clusterset.routingOptions()["routers"][cm_router]["tags"]);

//@<> clusterset.setRoutingOption for a router, invalid values
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", 'any_not_supported_value'); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", ''); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", ['primary']); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", {'target':'primary'}); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", 1); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 'any_not_supported_value'); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', ''); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', ['primary']); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', {'target':'primary'}); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 1); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'read_only_targets', 1); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'read_only_targets', "slow_replicas"); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");

// stats_updates_frequency
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', ''); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', 'foo'); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', ['1']); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', {'value':'1'}); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', -1); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be a positive integer.");

// use_replica_primary_as_rw
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'use_replica_primary_as_rw', ''); },
  "Invalid value for routing option 'use_replica_primary_as_rw', value is expected to be a boolean.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'use_replica_primary_as_rw', 'foo'); },
  "Invalid value for routing option 'use_replica_primary_as_rw', value is expected to be a boolean.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'use_replica_primary_as_rw', ['1']); },
  "Invalid value for routing option 'use_replica_primary_as_rw', value is expected to be a boolean.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'use_replica_primary_as_rw', {'value':'1'}); },
  "Invalid value for routing option 'use_replica_primary_as_rw', value is expected to be a boolean.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'use_replica_primary_as_rw', -1); },
  "Invalid value for routing option 'use_replica_primary_as_rw', value is expected to be a boolean.");

//@<> Routers that don't belong to the clusterset
EXPECT_THROWS(function(){ clusterset.setRoutingOption("abra", 'invalidated_cluster_policy', 'drop_all'); },
  "Router 'abra' is not part of this topology");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("abra::cadabra", 'target_cluster', 'primary'); },
  "Router 'abra::cadabra' is not part of this topology");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("routerhost2", 'read_only_targets', 'all'); }, "Router 'routerhost2' is not part of this topology");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("another", 'read_only_targets', 'all'); }, "Router 'another' is not part of this topology");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("::system", 'read_only_targets', 'all'); }, "Router '::system' is not part of this topology");

//@<> check types of clusterset router option values
// Bug#34604612
options = clusterset.routingOptions();
EXPECT_EQ("string", typeof options["global"]["target_cluster"]);
EXPECT_EQ("string", typeof options["global"]["invalidated_cluster_policy"]);
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ("boolean", typeof options["global"]["use_replica_primary_as_rw"]);
EXPECT_EQ("string", typeof options["global"]["read_only_targets"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_EQ("string", typeof options["target_cluster"]);
EXPECT_EQ("string", typeof options["invalidated_cluster_policy"]);
EXPECT_EQ("number", typeof options["stats_updates_frequency"]);
EXPECT_EQ("boolean", typeof options["use_replica_primary_as_rw"]);
EXPECT_EQ("string", typeof options["read_only_targets"]);


//@<> clusterset.setRoutingOption invalid values
EXPECT_THROWS(function(){ clusterset.setRoutingOption("target_cluster", 'any_not_supported_value'); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("invalidated_cluster_policy", 'any_not_supported_value'); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");

//@ clusterset.listRouters (CLI)
clusterset.setRoutingOption(cm_router, "target_cluster", cluster.getName());
clusterset.setRoutingOption(cr_router, "target_cluster", replicacluster.getName());
callMysqlsh([`${__sandbox_uri1}`, "--", "clusterset", "list-routers"])

//@ clusterset.listRouters (CLI-1)
callMysqlsh([`${__sandbox_uri1}`, "--", "clusterset", "list-routers", `${cm_router}`])

//@ clusterset.listRouters (CLI-2)
callMysqlsh([`${__sandbox_uri1}`, "--", "clusterset", "list-routers", `${cr_router}`])

//@<> clusterset.listRouters on invalid router
EXPECT_THROWS(function(){ clusterset.listRouters("invalid_router"); }, "Router 'invalid_router' is not registered in the ClusterSet");

//@<> cluster.listRouters not available for clusterset members
EXPECT_THROWS(function(){ cluster.listRouters(); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'clusterset', use <ClusterSet>.listRouters() to list the ClusterSet Router instances");

//@<> Primary cluster of the clusterset not available
session.runSql("STOP group_replication");
shell.connect(__sandbox_uri2);
clusterset = dba.getClusterSet();

EXPECT_THROWS(function(){
    clusterset.setRoutingOption(cr_router, 'invalidated_cluster_policy', 'drop_all');
}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: All members of Primary Cluster are OFFLINE.");
EXPECT_THROWS(function(){
    clusterset.setRoutingOption('target_cluster', 'primary');
}, "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY instance isn't available. Operation is not possible when in that state: All members of Primary Cluster are OFFLINE.");

shell.connect(__sandbox_uri1);
dba.rebootClusterFromCompleteOutage("cluster");
cluster = dba.getCluster();
clusterset = dba.getClusterSet();

//@<> special cases with router names
clusterset.setRoutingOption(cr_router2, "target_cluster", "replicacluster");
clusterset.setRoutingOption(cr_router3, "target_cluster", "cluster");

session.runSql("select * from mysql_innodb_cluster_metadata.routers");

EXPECT_EQ(replicacluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=3").fetchOne()[0]);
EXPECT_EQ(cluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=4").fetchOne()[0]);

clusterset.setRoutingOption(cr_router3, "target_cluster", "replicacluster");
clusterset.setRoutingOption(cr_router2, "target_cluster", "cluster");

EXPECT_EQ(replicacluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=4").fetchOne()[0]);
EXPECT_EQ(cluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=3").fetchOne()[0]);

//@<> setRoutingOption() after changing the primary instance of a clusterset cluster
cluster.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
cluster.setPrimaryInstance(__sandbox_uri3);

EXPECT_NO_THROWS(function() { clusterset.setRoutingOption(cr_router3, "target_cluster", "primary"); });

//@<> setRoutingOption() when Shell's active session is not established a primary member
shell.connect(__sandbox_uri1);
cluster = dba.getCluster()
clusterset = dba.getClusterSet()

EXPECT_NO_THROWS(function() { clusterset.setRoutingOption(cr_router3, "target_cluster", "replicacluster"); });

//BUG#33250212 Warn about Routers requiring a re-bootstrap in ClusterSets

//<> listRouters() must print a warning for routers needing a re-bootstrap
shell.connect(__sandbox_uri3);
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'cluster') WHERE router_id=2");
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'cluster') WHERE router_id=3");

// 'routerhost2::' and 'routerhost2::another' must be displayed in the warning msg

//@<> clusterset.listRouters() warning re-bootstrap
clusterset.listRouters();
EXPECT_OUTPUT_CONTAINS("WARNING: The following Routers were bootstrapped before the ClusterSet was created: [routerhost2::system, routerhost2::another]. Please re-bootstrap the Routers to ensure the ClusterSet is recognized and the configurations are updated. Otherwise, Routers will operate as if the Clusters were standalone.");

//@<> check setting options if the MD values are cleared
session.runSql("UPDATE mysql_innodb_cluster_metadata.clustersets SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", 10); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clustersets SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("tag:two", 2); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clustersets SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("tags", {"one":1, "two":"2"}); });

//@<> cluster.routingOptions(), for global options that also exist in clusterset.routingOptions(), should show the values (except tags) stored on the ClusterSet
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

EXPECT_NO_THROWS(function() {cluster = dba.createCluster("cluster", {gtidSetIsComplete:1}); });

EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("tag:five", 5); });
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("stats_updates_frequency", 52); });

options = cluster.routingOptions();
EXPECT_EQ("number", typeof options["global"]["tags"]["five"]);
EXPECT_EQ(5, options["global"]["tags"]["five"]);
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(52, options["global"]["stats_updates_frequency"]);

EXPECT_NO_THROWS(function() {clusterset = cluster.createClusterSet("clusterset"); });
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("tag:six", 6); });
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", 66); });

options = cluster.routingOptions();
EXPECT_EQ("number", typeof options["global"]["tags"]["five"]);
EXPECT_EQ(5, options["global"]["tags"]["five"]);
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(66, options["global"]["stats_updates_frequency"]);

//@<> check if "stats_updates_frequency", in ClusterSets, defaults to 0
EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", 48); });

options = clusterset.routingOptions();
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(48, options["global"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);
EXPECT_EQ(48, options["stats_updates_frequency"]);

EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", null); });

options = clusterset.routingOptions();
EXPECT_EQ("object", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(null, options["global"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);

set_metadata_version(2, 1, 0);

EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", 47); });

options = clusterset.routingOptions();
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(47, options["global"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);
EXPECT_EQ(47, options["stats_updates_frequency"]);

EXPECT_NO_THROWS(function(){ clusterset.setRoutingOption("stats_updates_frequency", null); });

options = clusterset.routingOptions();
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);
EXPECT_EQ(0, options["global"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);
EXPECT_EQ(0, options["stats_updates_frequency"]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
