
//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster");


// Tests assuming MD Schema 1.0.1
// ------------------------------

//@<> Load old metadata {!__replaying}
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/sql/router_metadata_101.sql", "mysql_innodb_cluster_metadata");

//@<> Prepare metadata
// Inserts router version:
// - 1.0.0 (artificial test to get an upgradeRequired during MD 1.0.1)
// - 8.0.18 (upgradeRequired:false during MD 1.0.1, true during MD 2.0.0)
// - 8.0.19 (upgradeRequired:false) 

var group_name = session.runSql("SELECT @@group_replication_group_name").fetchOne()[0];
session.runSql("UPDATE mysql_innodb_cluster_metadata.replicasets SET attributes = JSON_SET(attributes, '$.group_replication_group_name', ?)", [group_name]);
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = @@server_uuid");
session.runSql("UPDATE mysql_innodb_cluster_metadata.instances SET instance_name = ?", ["127.0.0.1:"+__mysql_sandbox_port1]);

//@ listRouters() - empty
cluster.listRouters();

//@<> Insert test router instances
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

//@ listRouters() - full list
// NOTE: whenever new MD versions are added to the shell, the list of upgradeRequired routers will change
cluster.listRouters();

//@ listRouters() - filtered
cluster.listRouters({onlyUpgradeRequired:1});

cluster.listRouters({onlyUpgradeRequired:0});

//@ removeRouterMetadata() (should fail)
cluster.removeRouterMetadata("bogus");
cluster.removeRouterMetadata("bogus::bla");
cluster.removeRouterMetadata("routerhost1:r2");
cluster.removeRouterMetadata("::bla");
cluster.removeRouterMetadata("foo::bla::");
cluster.removeRouterMetadata("127.0.0.1");

//@# removeRouterMetadata()
cluster.removeRouterMetadata("routerhost1");
cluster.removeRouterMetadata("routerhost1::r2");
cluster.removeRouterMetadata("routerhost2::system");

EXPECT_THROWS(function(){cluster.removeRouterMetadata("routerhost1");}, "Cluster.removeRouterMetadata: Invalid router instance 'routerhost1'");

//@ listRouters() after removed routers
cluster.listRouters();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
