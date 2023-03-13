//@<> Setup + Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
var session = scene.session
var cluster = scene.cluster

//@<> create routers
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

var router1 = "routerhost1::system";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.1.0', '2023-04-26 11:22:33', '{\"bootstrapTargetType\": \"cluster\", \"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', ?, NULL, NULL)", [cluster_id]);

var router2 = "routerhost2::system";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.1.0', '2023-04-26 11:22:33', '{\"bootstrapTargetType\": \"cluster\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', ?, NULL, NULL)", [cluster_id]);

var router3 = "routerhost2::another";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (3, 'another', 'mysqlrouter', 'routerhost2', '8.1.0', '2023-04-26 11:22:33', '{\"bootstrapTargetType\": \"cluster\"}', ?, NULL, NULL)", [cluster_id]);

var cr_router3 = "routerhost2::";

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (4, '', 'mysqlrouter', 'routerhost2', '8.1.0', '2023-04-26 11:22:33', '{\"bootstrapTargetType\": \"cluster\"}', ?, NULL, NULL)", [cluster_id]);

//@<> cluster.routingOptions on invalid router
EXPECT_THROWS(function(){ cluster.routingOptions("invalid_router"); }, "Router 'invalid_router' is not registered in the cluster");

//@<> cluster.routingOptions() with all defaults
cluster.routingOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
  "clusterName": "cluster",
  "global": {
      "read_only_targets": "secondaries",
      "tags": {}
  },
  "routers": {
      "routerhost1::system": {},
      "routerhost2::": {},
      "routerhost2::another": {},
      "routerhost2::system": {}
  }
}
`
);

//@<> cluster.setRoutingOption, all valid values
function CHECK_SET_ROUTING_OPTION(option, value, expected_value) {
  orig_options = cluster.routingOptions();

  router_options = cluster.routingOptions(router1);
  global_options = cluster.routingOptions();

  cluster.setRoutingOption(router1, option, value);
  router_options[router1][option] = expected_value;
  global_options["routers"][router1][option] = expected_value;
  EXPECT_JSON_EQ(router_options, cluster.routingOptions(router1), "router check");
  EXPECT_JSON_EQ(global_options, cluster.routingOptions(), "router check 2");

  cluster.setRoutingOption(option, value);
  global_options["global"][option] = expected_value;
  EXPECT_JSON_EQ(global_options, cluster.routingOptions(), "global check");

  // setting option to null should reset to default
  cluster.setRoutingOption(option, null);
  cluster.setRoutingOption(router1, option, null);
  EXPECT_JSON_EQ(orig_options, cluster.routingOptions(), "original check");
}

CHECK_SET_ROUTING_OPTION("read_only_targets", "all", "all");
CHECK_SET_ROUTING_OPTION("read_only_targets", "read_replicas", "read_replicas");
CHECK_SET_ROUTING_OPTION("read_only_targets", "secondaries", "secondaries");

CHECK_SET_ROUTING_OPTION('tags', {}, {});
CHECK_SET_ROUTING_OPTION('tags', { "a": 123 }, { "a": 123 });

//@<> default values filled in when metadata is missing some option (e.g. upgrade)
var full_options = cluster.routingOptions();

var router_options = session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]
session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options='{}'");

EXPECT_JSON_EQ(full_options, cluster.routingOptions());

session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options=?", [router_options]);
EXPECT_JSON_EQ(full_options, cluster.routingOptions());

//@<> reset option
cluster.setRoutingOption("read_only_targets", "read_replicas");
cluster.setRoutingOption(router1, "read_only_targets", "all");
var orig = cluster.routingOptions();

cluster.setRoutingOption(router1, "read_only_targets", null);
delete orig["routers"][router1]["read_only_targets"];
EXPECT_JSON_EQ(orig, cluster.routingOptions());

//@<> set individual tags
cluster.setRoutingOption("tags", {"old":"oldvalue"});
cluster.setRoutingOption("tag:test_tag", 1234);
cluster.setRoutingOption("tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, cluster.routingOptions()["global"]["tags"]);

cluster.setRoutingOption("tags", {});
EXPECT_JSON_EQ({}, cluster.routingOptions()["global"]["tags"]);

cluster.setRoutingOption(router1, "tags", {"old":"oldvalue"});
cluster.setRoutingOption(router1, "tag:test_tag", 1234);
cluster.setRoutingOption(router1, "tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, cluster.routingOptions()["routers"][router1]["tags"]);

cluster.setRoutingOption(router1, "tags", {});
EXPECT_JSON_EQ({}, cluster.routingOptions()["routers"][router1]["tags"]);

//@<> cluster.setRoutingOption for a router, invalid values
EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", 'any_not_supported_value'); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");
EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", ''); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");
EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", ['primary']); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");
EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", {'target':'primary'}); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");
EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", 1); },
  "Invalid value for routing option 'read_only_targets', accepted values: 'all', 'read_replicas', 'secondaries'");

//@<> Router does not belong to the cluster
EXPECT_THROWS(function(){ cluster.setRoutingOption("abra", 'read_only_targets', 'all'); }, "Router 'abra' is not part of this topology");

//@<> check types of cluster router option values
options = cluster.routingOptions();
EXPECT_EQ("string", typeof options["global"]["read_only_targets"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_EQ("string", typeof options["read_only_targets"]);

//@<> Error when cluster has no quorum
scene.make_no_quorum([__mysql_sandbox_port1]);

EXPECT_THROWS(function(){ cluster.setRoutingOption("read_only_targets", 1); }, "There is no quorum to perform the operation");

cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
cluster.removeInstance(__endpoint2, {force: true})

//@<> Error when Cluster belongs to ClusterSet {VER(>=8.0.27)}
cs = cluster.createClusterSet("cs");

EXPECT_THROWS(function(){ cluster.setRoutingOption("read_only_targets", 1); },
  "Option not available for ClusterSet member");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'read_only_targets'");

EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", 1); },
  "Option not available for ClusterSet member");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'read_only_targets'");

//@<> Cleanup
scene.destroy();
