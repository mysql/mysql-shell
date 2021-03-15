//@ {VER(>=8.0.19)}

//@<> INCLUDE _update_sys_path.js

//@<> Helper sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port1));

//@<> Cluster initialization
ports=[__mysql_sandbox_port1]
metadata_2_0_0_file = "metadata_2_0_0.sql";
primary_port = __mysql_sandbox_port1
secondary_port = __mysql_sandbox_port2
metadata_201 = false

//@<> INCLUDE _prepare_metadata_2_0_0_cluster.js

//@<> Additional initialization
testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port2));

// add router records before upgrade
session.runSql("create user mysql_router_321@'%'");
session.runSql("create user mysql_router1_321@'%'");

var attrs1 = '{"ROEndpoint": "6447", "RWEndpoint": "6446", "ROXEndpoint": "64470", "RWXEndpoint": "64460", "MetadataUser": "mysql_router_12345"}';
var attrs2 = '{"ROEndpoint": "6447", "RWEndpoint": "6446", "ROXEndpoint": "6448", "RWXEndpoint": "6449", "MetadataUser": "mysql_router1_12345"}';

cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters").fetchOne()[0];
session.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, '', 'mysqlrouter', 'routerhost1', '8.0.23', '2019-01-01 11:22:33', '${attrs1}', ?, NULL)`, [cluster_id]);
session.runSql(`INSERT mysql_innodb_cluster_metadata.routers VALUES (DEFAULT, 'default', 'mysqlrouter', 'routerhost2', '8.0.23', '2019-01-01 11:22:33', '${attrs2}', ?, NULL)`, [cluster_id]);


// Upgrade Metadata
dba.upgradeMetadata()
metadata_201 = true

// Get Cluster from master
var cluster = dba.getCluster();
EXPECT_OUTPUT_NOT_CONTAINS("It is recommended to upgrade the metadata.");
primary_port = __mysql_sandbox_port1
secondary_port = __mysql_sandbox_port2

//@<> INCLUDE _simple_cluster_tests.js

//@<> Finalization
testutil.rmfile(metadata_2_0_0_file);
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
