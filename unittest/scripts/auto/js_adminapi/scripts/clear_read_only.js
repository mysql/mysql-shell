
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);

// --- Create Cluster Tests ---
var mysql = require('mysql');
shell.connect(__sandbox_uri1);
// Create admin user without % to require read-only to be disabled to create it.
session.runSql("CREATE USER 'admin'@'localhost' IDENTIFIED BY 'adminpass'");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO 'admin'@'localhost' WITH GRANT OPTION");
session.runSql("create user bla@localhost");
session.runSql("set global super_read_only=1");
var s = mysql.getSession("bla:@localhost:" + __mysql_sandbox_port1);

EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

//@ Dba_create_cluster.clear_read_only_invalid
dba.createCluster("dev", {clearReadOnly:"NotABool"});

//@ Dba_create_cluster.clear_read_only automatically disabled and clearReadOnly deprecated
c = dba.createCluster("dev", {clearReadOnly:false});

//@ Check super read only was disabled after createCluster
EXPECT_EQ(0, get_sysvar(session, "super_read_only"));
c.dissolve({force:true});

// --- Configure Local Instance Tests ---

//@ Dba_configure_local_instance.clear_read_only_invalid
session.runSql("set global super_read_only=1");
EXPECT_EQ(1, get_sysvar(session, "super_read_only"));

dba.configureLocalInstance(__sandbox_uri1, {clearReadOnly:"NotABool"});

//@ Dba_configure_local_instance.clear_read_only_unset
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf_path, clusterAdmin: "admin", clusterAdminPassword: "pwd"});

//@ Dba_configure_local_instance.clear_read_only_false
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf_path, clusterAdmin: "admin", clusterAdminPassword: "pwd", clearReadOnly: false});

// --- Drop Metadata Schema Tests ---
session.runSql("set global super_read_only=0");

var cluster = dba.createCluster("dev");
cluster.disconnect();
session.runSql("set global super_read_only=1");

//@ Dba_drop_metadata.clear_read_only_invalid
dba.dropMetadataSchema({clearReadOnly: "NotABool"});

//@ Dba_drop_metadata.clear_read_only_unset
dba.dropMetadataSchema({force:true});

//@ Dba_drop_metadata.clear_read_only_false
dba.dropMetadataSchema({force:true, clearReadOnly: false});

// --- Reboot Cluster From Complete Outage ---
session.runSql("stop group_replication");

//@ Dba_reboot_cluster.clear_read_only automatically disabled and clearReadOnly deprecated
c = dba.rebootClusterFromCompleteOutage("dev", {clearReadOnly: false});


//@ Check super read only was disabled after rebootClusterFromCompleteOutage
EXPECT_EQ(0, get_sysvar(session, "super_read_only"));
c.dissolve({force:true});

session.close();
s.close();
testutil.destroySandbox(__mysql_sandbox_port1);
