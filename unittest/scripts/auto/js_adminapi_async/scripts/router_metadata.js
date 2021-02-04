//@ {VER(>=8.0.11)}

//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip});

shell.connect(__sandbox_uri1);
rs = dba.createReplicaSet("rset", {gtidSetIsComplete:true});
rs.addInstance(__sandbox2);

// Tests assuming MD Schema 2.0.0
// ------------------------------

//@<> MD2 - Prepare metadata 2.0
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];

//@ MD2 - listRouters() - empty
rs.listRouters();

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
rs.listRouters();

//@ MD2 - listRouters() - filtered
rs.listRouters({onlyUpgradeRequired:1});

rs.listRouters({onlyUpgradeRequired:0});

//@ MD2 - removeRouterMetadata() (should fail)
rs.removeRouterMetadata("bogus");
rs.removeRouterMetadata("bogus::bla");
rs.removeRouterMetadata("routerhost1:r2");
rs.removeRouterMetadata("::bla");
rs.removeRouterMetadata("foo::bla::");
rs.removeRouterMetadata("127.0.0.1");

//@# MD2 - removeRouterMetadata()
rs.removeRouterMetadata("routerhost1");
rs.removeRouterMetadata("routerhost1::r2");
rs.removeRouterMetadata("routerhost2::system");

EXPECT_THROWS(function(){rs.removeRouterMetadata("routerhost1");}, "Invalid router instance 'routerhost1'");

//@ MD2 - listRouters() after removed routers
rs.listRouters();

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- BEGIN ---
//@<> Connect to SECONDARY and get replicaset.
rs.status();
shell.connect(__sandbox_uri2);
rs2 = dba.getReplicaSet();

//@ removeRouterMetadata should succeed with current session on SECONDARY (redirected to PRIMARY).
rs2.removeRouterMetadata("routerhost2");

//@ Verify router data was removed (routerhost2).
rs.listRouters();

// BUG#30594844 : remove_router_metadata() gets primary wrong  --- END ---

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
