//@ {VER(>=8.0)}

// Tests to check if hostnames are compared correctly

//@<> Setup sandboxes and initial cluster
hostname_caps = hostname.slice(0, hostname.length / 2).toUpperCase() + hostname.slice(hostname.length / 2).toLowerCase();

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname_caps});

//@<> create replicaset
shell.connect(__sandbox_uri1);

EXPECT_NO_THROWS(function() { rset = dba.createReplicaSet("rset", {gtidSetIsComplete:true}); });
EXPECT_NO_THROWS(function() { rset.addInstance(hostname_caps + ":" + __mysql_sandbox_port2, {recoveryMethod: "incremental"}); });

////@<> replicaset.setPrimaryInstance
EXPECT_NO_THROWS(function() { rset.setPrimaryInstance(hostname + ":" + __mysql_sandbox_port2); });
EXPECT_NO_THROWS(function() { rset.setPrimaryInstance(hostname_caps + ":" + __mysql_sandbox_port1); });

////@<> replicaset.setInstanceOption
EXPECT_NO_THROWS(function() { rset.setInstanceOption(hostname + ":" + __mysql_sandbox_port2, "tag:tagA", 321); });
EXPECT_NO_THROWS(function() { rset.setInstanceOption(hostname_caps + ":" + __mysql_sandbox_port2, "tag:tagB", 654); });
roptions = rset.options();
roptions
EXPECT_EQ(roptions["replicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`].length, 2);
EXPECT_EQ(roptions["replicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][0]["option"], "tagA");
EXPECT_EQ(roptions["replicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][0]["value"], 321);
EXPECT_EQ(roptions["replicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][1]["option"], "tagB");
EXPECT_EQ(roptions["replicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][1]["value"], 654);

////@<> replicaset.forcePrimaryInstance
testutil.stopSandbox(__mysql_sandbox_port1);

shell.connect(__sandbox_uri2);
rset = dba.getReplicaSet();
EXPECT_NO_THROWS(function() { rset.forcePrimaryInstance(hostname_caps + ":" + __mysql_sandbox_port2); });

//@<> replicaset.rejoinInstance
testutil.startSandbox(__mysql_sandbox_port1);
EXPECT_NO_THROWS(function() { rset.rejoinInstance(hostname_caps + ":" + __mysql_sandbox_port1); });

//@<> replicaset.removeInstance
EXPECT_NO_THROWS(function() { rset.setPrimaryInstance(hostname + ":" + __mysql_sandbox_port1); });
EXPECT_NO_THROWS(function() { rset.removeInstance(hostname + ":" + __mysql_sandbox_port2); });

//@<> replicaset.removeRouterMetadata
shell.connect(__sandbox_uri1);

cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'RoUterHost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

EXPECT_EQ(rset.listRouters()["routers"].hasOwnProperty("RoUterHost1::system"), true);
EXPECT_NO_THROWS(function() { rset.removeRouterMetadata("routerhost1::system"); });
EXPECT_EQ(rset.listRouters()["routers"].hasOwnProperty("RoUterHost1::system"), false);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
