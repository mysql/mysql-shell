//@ {VER(>=8.0.19)}

//@<> INCLUDE _update_sys_path.js

//@<> Helper sandboxes
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deployRawSandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deployRawSandbox(__mysql_sandbox_port3, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port1));
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port2));
testutil.touch(testutil.getSandboxLogPath(__mysql_sandbox_port3));

//@<> Variable Initialization
ports=[__mysql_sandbox_port1]
metadata_2_0_0_file = "metadata_2_0_0.sql";
primary_port = __mysql_sandbox_port1
secondary_port = __mysql_sandbox_port2
metadata_201 = false

//@<> INCLUDE BATCH _prepare_metadata_2_0_0_cluster.js

//@<> Test initialization Batch Mode
var cluster = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("It is recommended to upgrade the metadata.");

cluster.dissolve({force: true});

//@<> INCLUDE BATCH_2 _prepare_metadata_2_0_0_cluster.js

//@<> Test if answer to upgrade is honored BUG #33941759
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers(router_name, product_name, address, attributes) VALUES ('second', 'router', 'localhost', '[]')")
session.runSql("INSERT INTO mysql_innodb_cluster_metadata.routers(router_name, product_name, address, attributes) VALUES ('third', 'router', 'localhost', '[]')")
testutil.expectPrompt("Do you want to proceed with the upgrade? [y/N]: ", "n");
dba.upgradeMetadata({interactive:true});
EXPECT_STDOUT_CONTAINS("The metadata upgrade has been aborted.")
EXPECT_STDOUT_NOT_CONTAINS(`Upgrading metadata at '${hostname}:${__mysql_sandbox_port1}' from version 2.0.0 to version 2.1.0.`)

session.runSql("TRUNCATE mysql_innodb_cluster_metadata.routers;");

//@<> INCLUDE BATCH _simple_cluster_tests.js

//@<> INTERACTIVE TESTING
session.runSql("STOP group_replication");
shell.options.useWizards=true;
secondary_port = __mysql_sandbox_port3

//@<> INCLUDE INTERACTIVE _prepare_metadata_2_0_0_cluster.js

//@<> Test initialization Interactive Mode
var cluster = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("It is recommended to upgrade the metadata.");

//@<> INCLUDE INTERACTIVE _simple_cluster_tests.js

//@<> Finalization
testutil.rmfile(metadata_2_0_0_file);
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
