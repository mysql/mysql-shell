
//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);


// Tests assuming MD Schema 2.0.0
// ------------------------------

//@<> MD2 - Prepare metadata 2.0
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

cluster = dba.getCluster();

//@ MD2 - listRouters() - empty
cluster.listRouters();

//@<> MD2 - Insert test router instances
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r2', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r3', 'mysqlrouter', 'routerhost1', '8.0.19', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost1', '8.0.18', '2019-01-01 11:22:33', '{\"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', '{\"ROEndpoint\": 6481, \"RWEndpoint\": 6480, \"ROXEndpoint\": 6483, \"RWXEndpoint\": 6482}', ?, NULL)", [cluster_id]);
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'foobar', 'mysqlrouter', 'routerhost2', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL)", [cluster_id]);

//@ MD2 - listRouters() - full list
// NOTE: whenever new MD versions are added to the shell, the list of upgradeRequired routers will change
cluster.listRouters();

//@ MD2 - listRouters() - filtered
cluster.listRouters({onlyUpgradeRequired:1});

cluster.listRouters({onlyUpgradeRequired:0});

//@ MD2 - removeRouterMetadata() (should fail)
cluster.removeRouterMetadata("bogus");
cluster.removeRouterMetadata("bogus::bla");
cluster.removeRouterMetadata("routerhost1:r2");
cluster.removeRouterMetadata("::bla");
cluster.removeRouterMetadata("foo::bla::");
cluster.removeRouterMetadata("127.0.0.1");

//@# MD2 - removeRouterMetadata()
cluster.removeRouterMetadata("routerhost1");
cluster.removeRouterMetadata("routerhost1::r2");
cluster.removeRouterMetadata("routerhost2::system");

EXPECT_THROWS(function(){cluster.removeRouterMetadata("routerhost1");}, "Invalid router instance 'routerhost1'");

//@ MD2 - listRouters() after removed routers
cluster.listRouters();

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- BEGIN ---
//@<> Connect to SECONDARY and get replicaset.
shell.connect(__sandbox_uri2);
cluster2 = dba.getCluster();

//@ removeRouterMetadata should succeed with current session on SECONDARY (redirected to PRIMARY).
cluster2.removeRouterMetadata("routerhost2");

//@ Verify router data was removed (routerhost2).
cluster.listRouters();

//@<> remove secondary from cluster.
// NOTE: removed because direct updates to MD are done in the following testes
//       assuming there is only one instance in the cluster.
cluster.removeInstance(__sandbox_uri2);

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- END ---


// Tests assuming MD Schema 1.0.1
// ------------------------------

//@<> MD1 - Reset MD schema
shell.connect(__sandbox_uri1);
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");

//@<> MD1 - Load old metadata {!__replaying}
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/sql/router_metadata_101.sql", "mysql_innodb_cluster_metadata");

//@<> MD1 - Prepare metadata 1.0
// Inserts router version:
// - 1.0.0 (artificial test to get an upgradeRequired during MD 1.0.1)
// - 8.0.18 (upgradeRequired:false during MD 1.0.1, true during MD 2.0.0)
// - 8.0.19 (upgradeRequired:false) 

var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
session.runSql("UPDATE mysql_innodb_cluster_metadata.replicasets SET attributes = JSON_SET(attributes, '$.group_replication_group_name', ?)", [group_name]);
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = @@server_uuid");
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET instance_name = ?", ["127.0.0.1:"+__mysql_sandbox_port1]);

cluster = dba.getCluster();

//@ MD1 - listRouters() - empty
cluster.listRouters();

//@<> MD1 - Insert test router instances
session.runSql("INSERT mysql_innodb_cluster_metadata.hosts VALUES (2, 'routerhost1', '', NULL, '', NULL, NULL)");
session.runSql("INSERT mysql_innodb_cluster_metadata.hosts VALUES (3, 'routerhost2', '', NULL, '', NULL, NULL)");

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 2, NULL)");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r1', 2, '{\"version\": \"1.0.0\"}')");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r2', 2, '{\"version\": \"8.0.18\"}')");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'r3', 2, '{\"version\": \"8.0.19\"}')");

session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 3, NULL)");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 2, '{\"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}')");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 3, '{\"ROEndpoint\": 6481, \"RWEndpoint\": 6480, \"ROXEndpoint\": 6483, \"RWXEndpoint\": 6482}')");
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'foobar', 3, NULL)");

//@ MD1 - listRouters() - full list
// NOTE: whenever new MD versions are added to the shell, the list of upgradeRequired routers will change
cluster.listRouters();

//@ MD1 - listRouters() - filtered
cluster.listRouters({onlyUpgradeRequired:1});

cluster.listRouters({onlyUpgradeRequired:0});

//@ MD1 - removeRouterMetadata() (should fail)
cluster.removeRouterMetadata("bogus");
cluster.removeRouterMetadata("bogus::bla");
cluster.removeRouterMetadata("routerhost1:r2");
cluster.removeRouterMetadata("::bla");
cluster.removeRouterMetadata("foo::bla::");
cluster.removeRouterMetadata("127.0.0.1");

//@# MD1 - removeRouterMetadata()
cluster.removeRouterMetadata("routerhost1");
cluster.removeRouterMetadata("routerhost1::r2");
cluster.removeRouterMetadata("routerhost2::system");

EXPECT_THROWS(function(){cluster.removeRouterMetadata("routerhost1");}, "Invalid router instance 'routerhost1'");

//@ MD1 - listRouters() after removed routers
cluster.listRouters();

//@# MD1 - disconnected cluster object
cluster.disconnect();

cluster.listRouters();
cluster.removeRouterMetadata("");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
