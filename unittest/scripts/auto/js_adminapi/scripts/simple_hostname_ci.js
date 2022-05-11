// Simple tests to check if hostnames are compared correctly

//@<> Setup sandboxes and initial cluster
hostname_caps = hostname.slice(0, hostname.length / 2).toUpperCase() + hostname.slice(hostname.length / 2).toLowerCase();

testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname_caps});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

cluster.addInstance(__sandbox_uri2);
dba.configureLocalInstance(__sandbox_uri2);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Call cluster.setPrimaryInstance {VER(>=8.0.27)}
EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(hostname_caps + ":" + __mysql_sandbox_port2); });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname_caps + ":" + __mysql_sandbox_port2 + "' was successfully elected as primary.");

status = cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["primary"], `${hostname_caps}:${__mysql_sandbox_port2}`);

EXPECT_NO_THROWS(function() { cluster.setPrimaryInstance(hostname + ":" + __mysql_sandbox_port1); });
EXPECT_OUTPUT_CONTAINS("The instance '" + hostname + ":" + __mysql_sandbox_port1 + "' was successfully elected as primary.");

//@<> Test cluster add/remove instances
EXPECT_NO_THROWS(function() { cluster.removeInstance(hostname + ":" + __mysql_sandbox_port2); });
EXPECT_NO_THROWS(function() { cluster.addInstance(hostname_caps + ":" + __mysql_sandbox_port2); });

EXPECT_NO_THROWS(function() { cluster.removeInstance(hostname_caps + ":" + __mysql_sandbox_port2); });
EXPECT_NO_THROWS(function() { cluster.addInstance(hostname + ":" + __mysql_sandbox_port2); });

status = cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname_caps}:${__mysql_sandbox_port2}`]["status"], "ONLINE");

//@<> Test cluster.rejoinInstance
EXPECT_NO_THROWS(function() { session2 = mysql.getSession(`mysql://root:root@${hostname_caps}:${__mysql_sandbox_port2}`); });
session2.runSql('STOP GROUP_REPLICATION');
session2.close();

EXPECT_NO_THROWS(function() { cluster.rejoinInstance(hostname + ":" + __mysql_sandbox_port2); });

//@<> Set cluster.setInstanceOption
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(hostname + ":" + __mysql_sandbox_port2, "tag:tagA", 123); });
EXPECT_NO_THROWS(function() { cluster.setInstanceOption(hostname_caps + ":" + __mysql_sandbox_port2, "tag:tagB", 456); });
coptions = cluster.options();
EXPECT_EQ(coptions["defaultReplicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`].length, 2);
EXPECT_EQ(coptions["defaultReplicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][0]["option"], "tagA");
EXPECT_EQ(coptions["defaultReplicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][0]["value"], 123);
EXPECT_EQ(coptions["defaultReplicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][1]["option"], "tagB");
EXPECT_EQ(coptions["defaultReplicaSet"]["tags"][`${hostname_caps}:${__mysql_sandbox_port2}`][1]["value"], 456);

//@<> Test cluster force quorum
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE,(MISSING)");

EXPECT_NO_THROWS(function() { cluster.forceQuorumUsingPartitionOf(`mysql://root:root@${hostname_caps}:${__mysql_sandbox_port1}`); });

//@<> Test cluster.createCluster
testutil.startSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

cluster.removeInstance(__sandbox_uri2);
cluster.dissolve({interactive: false});

EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster", {gtidSetIsComplete: true}); });

//@<> Test cluster.checkInstanceState

session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);
session2.close();

EXPECT_NO_THROWS(function() { cluster.checkInstanceState(hostname + ":" + __mysql_sandbox_port2); });
EXPECT_NO_THROWS(function() { cluster.checkInstanceState(hostname_caps + ":" + __mysql_sandbox_port2); });

cluster.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

EXPECT_THROWS(function() {
    cluster.checkInstanceState(hostname + ":" + __mysql_sandbox_port2);
}, `The instance '${hostname_caps}:${__mysql_sandbox_port2}' already belongs to the cluster: 'cluster'`);

//@<> Test cluster.switchToSinglePrimaryMode {VER(>=8.0.13)}
cluster.switchToMultiPrimaryMode();
EXPECT_NO_THROWS(function() { cluster.switchToSinglePrimaryMode(hostname_caps + ":" + __mysql_sandbox_port2); });
EXPECT_NO_THROWS(function() { cluster.switchToSinglePrimaryMode(hostname_caps + ":" + __mysql_sandbox_port1); });

//@<> Test dba.rebootClusterFromCompleteOutage
disable_auto_rejoin(__mysql_sandbox_port1);
disable_auto_rejoin(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
testutil.killSandbox(__mysql_sandbox_port2);
testutil.waitMemberState(__mysql_sandbox_port2, "UNREACHABLE");
session.close();
testutil.killSandbox(__mysql_sandbox_port1);

testutil.startSandbox(__mysql_sandbox_port1);
testutil.startSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
cluster = dba.rebootClusterFromCompleteOutage("cluster", {rejoinInstances: [hostname + ":" + __mysql_sandbox_port2]});

//@<> cluster.removeRouterMetadata
cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'system', 'mysqlrouter', 'RoUterHost1', '8.0.18', '2019-01-01 11:22:33', NULL, ?, NULL, NULL)", [cluster_id]);

EXPECT_EQ(cluster.listRouters()["routers"].hasOwnProperty("RoUterHost1::system"), true);
EXPECT_NO_THROWS(function() { cluster.removeRouterMetadata("routerhost1::system"); });
EXPECT_EQ(cluster.listRouters()["routers"].hasOwnProperty("RoUterHost1::system"), false);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
