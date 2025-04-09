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

//@<> cluster.routerOptions on invalid routers
EXPECT_THROWS(function(){ cluster.routerOptions({"router": "invalid_router"}); }, "Router 'invalid_router' is not registered in the cluster");

EXPECT_THROWS(function(){ cluster.routerOptions({"router": "routerhost2"}); }, "Router 'routerhost2' is not registered in the cluster");

EXPECT_THROWS(function(){ cluster.routerOptions({"router": "another"}); }, "Router 'another' is not registered in the cluster");

EXPECT_THROWS(function(){ cluster.routerOptions({"router": "::system"}); }, "Router '::system' is not registered in the cluster");

//@<> cluster.routerOptions() with all defaults
cluster.routerOptions();

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "clusterName": "cluster", 
    "configuration": {
        "routing_rules": {
            "read_only_targets": "secondaries", 
            "tags": {}
        }
    }, 
    "routers": {
        "routerhost1::system": {
            "configuration": {}
        }, 
        "routerhost2::": {
            "configuration": {}
        }, 
        "routerhost2::another": {
            "configuration": {}
        }, 
        "routerhost2::system": {
            "configuration": {}
        }
    }
}
`
);

//@<> cluster.setRoutingOption, all valid values, tag
cluster.setRoutingOption(router1, "tag:test_tag", 567);
EXPECT_JSON_EQ(567, cluster.routerOptions({"router": router1, "extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]["test_tag"]);
EXPECT_JSON_EQ(567, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]["test_tag"]);

cluster.setRoutingOption("tag:test_tag", 567);
EXPECT_JSON_EQ(567, cluster.routerOptions()["configuration"]["routing_rules"]["tags"]["test_tag"]);

cluster.setRoutingOption("tag:test_tag", null);
cluster.setRoutingOption(router1, "tag:test_tag", null);
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);
EXPECT_JSON_EQ({}, cluster.routerOptions()["configuration"]["routing_rules"]["tags"]);

cluster.setRoutingOption(router1, "tag:test_tag", 567);
cluster.setRoutingOption("tag:test_tag", 567);

cluster.setRoutingOption("tags", null);
cluster.setRoutingOption(router1, "tags", null);
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);
EXPECT_JSON_EQ({}, cluster.routerOptions()["configuration"]["routing_rules"]["tags"]);

//@<> cluster.setRoutingOption, all valid values
function CHECK_SET_ROUTING_OPTION(option, value, expected_value) {
  orig_options = cluster.routerOptions({"extended": 2});

  router_options = cluster.routerOptions({"router": router1, "extended": 2});
  global_options = cluster.routerOptions({"extended": 2});

  cluster.setRoutingOption(router1, option, value);
  router_options["routers"][router1]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router1]["configuration"]["routing_rules"][option] = expected_value;
  EXPECT_JSON_EQ(router_options, cluster.routerOptions({"router": router1, "extended": 2}), "router check");
  EXPECT_JSON_EQ(global_options, cluster.routerOptions({"extended": 2}), "router check 2");

  cluster.setRoutingOption(option, value);
  global_options["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router2]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router3]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][cr_router3]["configuration"]["routing_rules"][option] = expected_value;
  EXPECT_JSON_EQ(global_options, cluster.routerOptions({"extended": 2}), "global check");

  // setting option to null should reset to default
  cluster.setRoutingOption(option, null);
  cluster.setRoutingOption(router1, option, null);
  EXPECT_JSON_EQ(orig_options, cluster.routerOptions({"extended": 2}), "original check");
}

CHECK_SET_ROUTING_OPTION("read_only_targets", "all", "all");
CHECK_SET_ROUTING_OPTION("read_only_targets", "read_replicas", "read_replicas");
CHECK_SET_ROUTING_OPTION("read_only_targets", "secondaries", "secondaries");

CHECK_SET_ROUTING_OPTION('tags', {}, {});
CHECK_SET_ROUTING_OPTION('tags', { "a": 123 }, { "a": 123 });

//@<> default values are not filled in when metadata is missing some option (e.g. upgrade)
var full_options = cluster.routerOptions({"extended": 2});

EXPECT_EQ(full_options["configuration"]["routing_rules"].hasOwnProperty("read_only_targets"),true)
EXPECT_EQ(full_options["configuration"]["routing_rules"].hasOwnProperty("tags"),true)

var router_options = session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]
session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options='{}'");

EXPECT_EQ(cluster.routerOptions({"extended": 2})["configuration"]["routing_rules"].hasOwnProperty("read_only_targets"),false)
EXPECT_EQ(cluster.routerOptions({"extended": 2})["configuration"]["routing_rules"].hasOwnProperty("tags"),false)

EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router2]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router3]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][cr_router3]["configuration"]["routing_rules"])

session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options=?", [router_options]);
EXPECT_JSON_EQ(full_options, cluster.routerOptions({"extended": 2}));

//@<> reset option
cluster.setRoutingOption("read_only_targets", "read_replicas");
cluster.setRoutingOption(router1, "read_only_targets", "all");
var orig = cluster.routerOptions();

cluster.setRoutingOption(router1, "read_only_targets", null);
delete orig["routers"][router1]["read_only_targets"];
EXPECT_JSON_EQ(orig, cluster.routerOptions());

//@<> set individual tags
cluster.setRoutingOption("tags", {"old":"oldvalue"});
cluster.setRoutingOption("tag:test_tag", 1234);
cluster.setRoutingOption("tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, cluster.routerOptions()["configuration"]["routing_rules"]["tags"]);

cluster.setRoutingOption("tags", {});
EXPECT_JSON_EQ({}, cluster.routerOptions()["configuration"]["routing_rules"]["tags"]);

cluster.setRoutingOption(router1, "tags", {"old":"oldvalue"});
cluster.setRoutingOption(router1, "tag:test_tag", 1234);
cluster.setRoutingOption(router1, "tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);

cluster.setRoutingOption(router1, "tags", {});
EXPECT_JSON_EQ({}, cluster.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);

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

//@<> Routers that don't belong to the cluster
EXPECT_THROWS(function(){ cluster.setRoutingOption("abra", 'read_only_targets', 'all'); }, "Router 'abra' is not part of this topology");
EXPECT_THROWS(function(){ cluster.setRoutingOption("routerhost2", 'read_only_targets', 'all'); }, "Router 'routerhost2' is not part of this topology");
EXPECT_THROWS(function(){ cluster.setRoutingOption("another", 'read_only_targets', 'all'); }, "Router 'another' is not part of this topology");
EXPECT_THROWS(function(){ cluster.setRoutingOption("::system", 'read_only_targets', 'all'); }, "Router '::system' is not part of this topology");

//@<> check types of cluster router option values
options = cluster.routerOptions();
EXPECT_EQ("string", typeof options["configuration"]["routing_rules"]["read_only_targets"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_EQ("string", typeof options["read_only_targets"]);

//@<> WL15601 FR1 stats_updates_frequency support

EXPECT_THROWS(function(){ cluster.setRoutingOption("stats_updates_frequency", "asda"); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ cluster.setRoutingOption("stats_updates_frequency", -1); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be a positive integer.");

CHECK_SET_ROUTING_OPTION("stats_updates_frequency", 22, 22);

cluster.setRoutingOption("stats_updates_frequency", 23);
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);

cluster.setRoutingOption("stats_updates_frequency", null);
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);

// check setting options if the MD values are cleared
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("stats_updates_frequency", 10); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("tag:two", 2); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("tags", {"one":1, "two":"2"}); });

//@<> WL15842 unreachable_quorum_allowed_traffic support

EXPECT_THROWS(function(){ cluster.setRoutingOption("unreachable_quorum_allowed_traffic", "asda"); },
  "Invalid value for routing option 'unreachable_quorum_allowed_traffic', value is expected to be either 'read', 'all' or 'none'.");
EXPECT_THROWS(function(){ cluster.setRoutingOption("unreachable_quorum_allowed_traffic", -1); },
  "Invalid value for routing option 'unreachable_quorum_allowed_traffic', value is expected to be either 'read', 'all' or 'none'.");

CHECK_SET_ROUTING_OPTION("unreachable_quorum_allowed_traffic", "read", "read");

WIPE_OUTPUT();

cluster.setRoutingOption("unreachable_quorum_allowed_traffic", "all");
EXPECT_OUTPUT_CONTAINS("Setting the 'unreachable_quorum_allowed_traffic' option to 'all' may have unwanted consequences: the consistency guarantees provided by InnoDB Cluster are broken since the data read can be stale; different Routers may be accessing different partitions, thus return different data; and different Routers may also have different behavior (i.e. some provide only read traffic while others read and write traffic). Note that writes on a partition with no quorum will block until quorum is restored.");
EXPECT_OUTPUT_CONTAINS("This option has no practical effect if the server variable group_replication_unreachable_majority_timeout is set to a positive number and group_replication_exit_state_action is set to either OFFLINE_MODE or ABORT_SERVER.");

options = JSON.parse(session.runSql("SELECT router_options FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_TRUE("unreachable_quorum_allowed_traffic" in options);

cluster.setRoutingOption("unreachable_quorum_allowed_traffic", null);
options = JSON.parse(session.runSql("SELECT router_options FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_FALSE("unreachable_quorum_allowed_traffic" in options);

// check setting options if the MD values are cleared
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ cluster.setRoutingOption("unreachable_quorum_allowed_traffic", "none"); });

//@<> Error when cluster has no quorum
scene.make_no_quorum([__mysql_sandbox_port1]);

EXPECT_THROWS(function(){ cluster.setRoutingOption("read_only_targets", 1); }, "There is no quorum to perform the operation");

cluster.forceQuorumUsingPartitionOf(__sandbox_uri1);
cluster.removeInstance(__endpoint2, {force: true})

//@<> Error when Cluster belongs to ClusterSet {VER(>=8.0.27)}
cs = cluster.createClusterSet("cs");

EXPECT_THROWS(function(){ cluster.setRoutingOption("read_only_targets", 1); },
  "Option not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'read_only_targets'");

EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "read_only_targets", 1); },
  "Option not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'read_only_targets'");

// WL15601 FR1.1
EXPECT_THROWS(function(){ cluster.setRoutingOption("stats_updates_frequency", 1); },
  "Option not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'stats_updates_frequency'");

EXPECT_THROWS(function(){ cluster.setRoutingOption(router1, "stats_updates_frequency", 1); },
  "Option not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'cs', use <ClusterSet>.setRoutingOption() to change the option 'stats_updates_frequency'");

//@<> Cleanup
scene.destroy();
