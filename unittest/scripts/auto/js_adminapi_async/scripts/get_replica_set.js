//@<> Pre-Setup for common 5.7 and 8.0 tests
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"11111111-1111-1111-1111-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"22222222-2222-2222-2222-222222222222"});
shell.connect(__sandbox_uri1);

//@<> getReplicaSet() in a standalone instance (should fail)
EXPECT_THROWS(function(){
    dba.getReplicaSet();
}, "Metadata Schema not found.");
EXPECT_OUTPUT_CONTAINS("Command not available on an unmanaged standalone instance.");

//@<> getReplicaSet() in a standalone instance with GR MD 1.0.1 (should fail) {VER(>8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-1.0.1-cluster_1member.sql", "mysql_innodb_cluster_metadata");

EXPECT_THROWS(function(){
    dba.getReplicaSet();
}, "Metadata version is not compatible");
EXPECT_OUTPUT_CONTAINS("ERROR: Incompatible Metadata version. This operation is disallowed because the installed Metadata version '1.0.1' is lower than the required version, '2.0.0'. Upgrade the Metadata to remove this restriction. See \\? dba.upgradeMetadata for additional details.");

//@ getReplicaSet() in a standalone instance with GR MD 2.0.0 (should fail) {VER(>8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-2.0.0-cluster_1member.sql", "mysql_innodb_cluster_metadata");

dba.getReplicaSet();

//@<> getReplicaSet() in a standalone instance with AR MD 2.0.0 in 5.7 (should fail) {VER(<8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-2.0.0-replicaset_1member.sql", "mysql_innodb_cluster_metadata");

dba.getReplicaSet();

//@<> getReplicaSet() in a GR instance (prepare)
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("RESET " + get_reset_binary_logs_keyword());

c = dba.createCluster("cluster");

//@ getReplicaSet() in a GR instance (should fail) {VER(<8.0.4)}
dba.getReplicaSet();

//@ getReplicaSet() in a GR instance (should fail) {VER(>8.0.4)}
dba.getReplicaSet();

//@<> getReplicaSet() in a GR instance (cleanup)
c.dissolve();
session.runSql("SET GLOBAL super_read_only=0");
session.runSql("SET GLOBAL read_only=0");
session.runSql("RESET " + get_reset_binary_logs_keyword());
session.runSql("RESET " + get_replica_keyword() + " ALL");

//@<> Setup replicaset {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

//@<> Positive case {VER(>=8.0.11)}
EXPECT_NO_THROWS(function(){ dba.getReplicaSet(); });

//@<> From a secondary {VER(>=8.0.11)}
shell.connect(__sandbox_uri2);
EXPECT_NO_THROWS(function(){ dba.getReplicaSet(); });

//@<> Delete metadata for the instance (should fail) {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances");
EXPECT_THROWS(function(){
    dba.getReplicaSet();
}, "This function is not available through a session to a standalone instance (metadata exists, instance does not belong to that metadata)");

session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
EXPECT_THROWS(function(){
    dba.getReplicaSet();
}, "Metadata Schema not found.");
EXPECT_OUTPUT_CONTAINS("Command not available on an unmanaged standalone instance.");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

