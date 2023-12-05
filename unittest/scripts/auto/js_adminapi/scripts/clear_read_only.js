//@<> Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

var mysql = require('mysql');
shell.connect(__sandbox_uri1);

// create admin user without % to require read-only to be disabled to create it.
session.runSql("CREATE USER 'admin'@'localhost' IDENTIFIED BY 'adminpass'");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO 'admin'@'localhost' WITH GRANT OPTION");
session.runSql("create user bla@localhost");
session.runSql("set global super_read_only=1");
var s = mysql.getSession("bla:@localhost:" + __mysql_sandbox_port1);

EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

//@<> Dba_create_cluster.clear_read_only automatically disabled
EXPECT_NO_THROWS(function(){ dba.createCluster("dev"); });
EXPECT_OUTPUT_CONTAINS("Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.");

EXPECT_EQ(0, get_sysvar(session, "super_read_only"));

//@<> Dba_reboot_cluster.clear_read_only automatically disabled
session.runSql("set global super_read_only=1");

session.runSql("stop group_replication");
EXPECT_NO_THROWS(function(){ dba.rebootClusterFromCompleteOutage("dev"); });

EXPECT_EQ(0, get_sysvar(session, "super_read_only"));

//@<> Dba_drop_metadata.clear_read_only_unset
session.runSql("set global super_read_only=1");

EXPECT_NO_THROWS(function(){ dba.dropMetadataSchema({force:true}); });

EXPECT_EQ(0, get_sysvar(session, "super_read_only"));

//@<> Cleanup
session.close();
s.close();
testutil.destroySandbox(__mysql_sandbox_port1);
