//@<> dba.createCluster does not error if using a single clusterAdmin account with netmask BUG#31018091
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
var ip_mask = hostname_ip.split(".").splice(0,3).join(".") + ".0";
dba.configureInstance(__sandbox_uri1, {clusterAdmin: "'admin'@'"+ ip_mask + "/255.255.255.0'", clusterAdminPassword: "pwd"});
var cluster_admin_uri= "mysql://admin:pwd@" + hostname_ip + ":" + __mysql_sandbox_port1;
shell.connect(cluster_admin_uri);
c = dba.createCluster("sample");
EXPECT_STDERR_EMPTY();
c.dissolve({interactive: false});
session.close();

//@<> check if innodb is forced for metadata schema BUG#32110085
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL super_read_only = OFF")
session.runSql("SET default_storage_engine = MyISAM");
var tmp;
EXPECT_NO_THROWS(function() { tmp = dba.createCluster("C"); });
tmp.dissolve({interactive: false});
session.runSql("SET GLOBAL super_read_only = OFF")

//@<> restore default sotrage engine
session.runSql("SET default_storage_engine = InnoDb");

//@<> dba.createCluster does not error if using instance with binlog_checksum enabled BUG#31329024 {VER(>= 8.0.21)}
session.runSql("SET GLOBAL binlog_checksum = 'CRC32';");
c = dba.createCluster("sample");
EXPECT_STDERR_EMPTY();
c.dissolve({interactive: false});

//@<> dba.createCluster error if table without pk exists or unsupported engine found BUG#29771457
session.runSql("SET GLOBAL super_read_only = OFF")
session.runSql("SET default_storage_engine = MyISAM");
session.runSql("CREATE DATABASE test");
session.runSql("CREATE TABLE test.foo (i int)");
WIPE_STDOUT()
var resp = dba.checkInstanceConfiguration();
EXPECT_EQ("error", resp["status"])
EXPECT_STDOUT_CONTAINS("ERROR: The following tables use a storage engine that are not supported by Group Replication:")
EXPECT_STDOUT_CONTAINS("ERROR: The following tables do not have a Primary Key or equivalent column:")
WIPE_STDOUT()
EXPECT_THROWS(function() { c = dba.createCluster("c"); }, "Instance check failed", "RuntimeError");
EXPECT_STDOUT_CONTAINS("ERROR: The following tables use a storage engine that are not supported by Group Replication:")
EXPECT_STDOUT_CONTAINS("ERROR: The following tables do not have a Primary Key or equivalent column:")


//@<> cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);