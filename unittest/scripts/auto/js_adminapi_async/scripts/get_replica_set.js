//@<> Pre-Setup for common 5.7 and 8.0 tests
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid:"11111111-1111-1111-1111-111111111111"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid:"22222222-2222-2222-2222-222222222222"});
shell.connect(__sandbox_uri1);

//@ getReplicaSet() in a standalone instance (should fail)
dba.getReplicaSet();

//@ getReplicaSet() in a standalone instance with GR MD 1.0.1 (should fail) {VER(>8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET MASTER");
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-1.0.1-cluster_1member.sql", "mysql_innodb_cluster_metadata");

dba.getReplicaSet();

//@ getReplicaSet() in a standalone instance with GR MD 2.0.0 (should fail) {VER(>8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET MASTER");
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-2.0.0-cluster_1member.sql", "mysql_innodb_cluster_metadata");

dba.getReplicaSet();

//@<> getReplicaSet() in a standalone instance with AR MD 2.0.0 in 5.7 (should fail) {VER(<8.0.4)}
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("CREATE SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET MASTER");
// import the MD schema and hack it to match the test environment
testutil.importData(__sandbox_uri1, __data_path+"/dba/md-2.0.0-replicaset_1member.sql", "mysql_innodb_cluster_metadata");

dba.getReplicaSet();

//@<> getReplicaSet() in a GR instance (prepare)
session.runSql("DROP SCHEMA IF EXISTS mysql_innodb_cluster_metadata");
session.runSql("RESET MASTER");

c = dba.createCluster("cluster");

//@ getReplicaSet() in a GR instance (should fail) {VER(<8.0.4)}
dba.getReplicaSet();

//@ getReplicaSet() in a GR instance (should fail) {VER(>8.0.4)}
dba.getReplicaSet();

//@<> getReplicaSet() in a GR instance (cleanup)
c.dissolve();
session.runSql("SET GLOBAL super_read_only=0");
session.runSql("SET GLOBAL read_only=0");
session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");
session.runSql("RESET MASTER");
session.runSql("RESET SLAVE ALL");

//@<> Setup replicaset {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);

//@ Positive case {VER(>=8.0.11)}
dba.getReplicaSet();

//@ From a secondary {VER(>=8.0.11)}
shell.connect(__sandbox_uri2);
dba.getReplicaSet();

//@ Delete metadata for the instance (should fail) {VER(>=8.0.11)}
shell.connect(__sandbox_uri1);
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances");
dba.getReplicaSet();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

