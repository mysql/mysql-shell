//@ {VER(>=8.0.11)}

//@<> Setup + Create replicaset
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("rs", {gtidSetIsComplete:true});

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

//@<> rs.routerOptions on invalid router
EXPECT_THROWS(function(){ rs.routerOptions({"router":"invalid_router"}); }, "Router 'invalid_router' is not registered in the replicaset");

//@<> rs.routerOptions() with all defaults
rs.routerOptions();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
{
    "configuration": {
        "routing_rules": {
            "tags": {}
        }
    },
    "name": "rs",
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
`);

//@<> rs.setRoutingOption, all valid values, tag
rs.setRoutingOption(router1, "tag:test_tag", 567);
EXPECT_JSON_EQ(567, rs.routerOptions({"router": router1, "extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]["test_tag"]);
EXPECT_JSON_EQ(567, rs.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]["test_tag"]);

rs.setRoutingOption("tag:test_tag", 567);
EXPECT_JSON_EQ(567, rs.routerOptions()["configuration"]["routing_rules"]["tags"]["test_tag"]);

rs.setRoutingOption("tag:test_tag", null);
rs.setRoutingOption(router1, "tag:test_tag", null);
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);
EXPECT_JSON_EQ({}, rs.routerOptions()["configuration"]["routing_rules"]["tags"]);

rs.setRoutingOption("tag:test_tag", 567);
rs.setRoutingOption(router1, "tag:test_tag", 567);

rs.setRoutingOption("tags", null);
rs.setRoutingOption(router1, "tags", null);
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);
EXPECT_JSON_EQ({}, rs.routerOptions()["configuration"]["routing_rules"]["tags"]);

//@<> rs.setRoutingOption, all valid values
function CHECK_SET_ROUTING_OPTION(option, value, expected_value) {
  orig_options = rs.routerOptions({"extended": 2});

  router_options = rs.routerOptions({"router": router1, "extended": 2});
  global_options = rs.routerOptions({"extended": 2});

  rs.setRoutingOption(router1, option, value);
  router_options["routers"][router1]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router1]["configuration"]["routing_rules"][option] = expected_value;
  EXPECT_JSON_EQ(router_options, rs.routerOptions({"router": router1, "extended": 2}), "router check");
  EXPECT_JSON_EQ(global_options, rs.routerOptions({"extended": 2}), "router check 2");

  rs.setRoutingOption(option, value);
  global_options["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router2]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][router3]["configuration"]["routing_rules"][option] = expected_value;
  global_options["routers"][cr_router3]["configuration"]["routing_rules"][option] = expected_value;
  EXPECT_JSON_EQ(global_options, rs.routerOptions({"extended": 2}), "global check");

  // setting option to null should reset to default
  rs.setRoutingOption(option, null);
  rs.setRoutingOption(router1, option, null);
  EXPECT_JSON_EQ(orig_options, rs.routerOptions({"extended": 2}), "original check");
}

CHECK_SET_ROUTING_OPTION("stats_updates_frequency", 24, 24);

CHECK_SET_ROUTING_OPTION('tags', {}, {});
CHECK_SET_ROUTING_OPTION('tags', { "a": 123 }, { "a": 123 });

//@<> default values are not filled in when metadata is missing some option (e.g. upgrade)
var full_options = rs.routerOptions({"extended": 2});

EXPECT_EQ(full_options["configuration"]["routing_rules"].hasOwnProperty("tags"),true)

var router_options = session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]
session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options='{}'");

EXPECT_EQ(rs.routerOptions()["configuration"]["routing_rules"].hasOwnProperty("tags"),false)

EXPECT_JSON_EQ({}, rs.routerOptions()["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router2]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router3]["configuration"]["routing_rules"])
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][cr_router3]["configuration"]["routing_rules"])

session.runSql("update mysql_innodb_cluster_metadata.clusters set router_options=?", [router_options]);
EXPECT_JSON_EQ(full_options, rs.routerOptions({"extended": 2}));

//@<> reset option
rs.setRoutingOption("stats_updates_frequency", 33);
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);

rs.setRoutingOption(router1, "stats_updates_frequency", 34);
options = JSON.parse(session.runSql("select options from mysql_innodb_cluster_metadata.routers").fetchOne()[0]);
EXPECT_TRUE("stats_updates_frequency" in options);

rs.setRoutingOption("stats_updates_frequency", null);
options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);

rs.setRoutingOption(router1, "stats_updates_frequency", null);
options = JSON.parse(session.runSql("select options from mysql_innodb_cluster_metadata.routers").fetchOne()[0]);
EXPECT_FALSE("stats_updates_frequency" in options);

//@<> set individual tags
rs.setRoutingOption("tags", {"old":"oldvalue"});
rs.setRoutingOption("tag:test_tag", 1234);
rs.setRoutingOption("tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, rs.routerOptions()["configuration"]["routing_rules"]["tags"]);

rs.setRoutingOption("tags", {});
EXPECT_JSON_EQ({}, rs.routerOptions()["configuration"]["routing_rules"]["tags"]);

rs.setRoutingOption(router1, "tags", {"old":"oldvalue"});
rs.setRoutingOption(router1, "tag:test_tag", 1234);
rs.setRoutingOption(router1, "tag:bla", "test");
EXPECT_JSON_EQ({"old":"oldvalue", "test_tag":1234, "bla": "test"}, rs.routerOptions({"extended": "2"})["routers"][router1]["configuration"]["routing_rules"]["tags"]);

rs.setRoutingOption(router1, "tags", {});
EXPECT_JSON_EQ({}, rs.routerOptions({"extended": 2})["routers"][router1]["configuration"]["routing_rules"]["tags"]);

//@<> rs.setRoutingOption with stats_updates_frequency, invalid values
EXPECT_THROWS(function(){ rs.setRoutingOption(router2, "stats_updates_frequency", "asda"); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ rs.setRoutingOption(router2, "stats_updates_frequency", -1); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be a positive integer.");

EXPECT_THROWS(function(){ rs.setRoutingOption("stats_updates_frequency", "asda"); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ rs.setRoutingOption("stats_updates_frequency", -1); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be a positive integer.");

//@<> Routers that don't belong to the replicaset
EXPECT_THROWS(function(){ rs.setRoutingOption("abra", 'stats_updates_frequency', 18); }, "Router 'abra' is not part of this topology");
EXPECT_THROWS(function(){ rs.setRoutingOption("routerhost2", 'stats_updates_frequency', 18); }, "Router 'routerhost2' is not part of this topology");
EXPECT_THROWS(function(){ rs.setRoutingOption("another", 'stats_updates_frequency', 18); }, "Router 'another' is not part of this topology");
EXPECT_THROWS(function(){ rs.setRoutingOption("::system", 'stats_updates_frequency', 18); }, "Router '::system' is not part of this topology");

//@<> check types of replicaset router option values
rs.setRoutingOption("stats_updates_frequency", 46);

options = rs.routerOptions();
EXPECT_EQ("number", typeof options["configuration"]["routing_rules"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clusters").fetchOne()[0]);
EXPECT_EQ("number", typeof options["stats_updates_frequency"]);

//@<> check setting options if the MD values are cleared
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ rs.setRoutingOption("stats_updates_frequency", 10); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ rs.setRoutingOption("tag:two", 2); });

session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET router_options = '{}'");
EXPECT_NO_THROWS(function(){ rs.setRoutingOption("tags", {"one":1, "two":"2"}); });

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
