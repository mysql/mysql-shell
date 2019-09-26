//@<> Calling dropMetadataSchema without open session
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Dba.dropMetadataSchema: An open session is required to perform this operation.")

//@<> InnoDB Cluster: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
EXPECT_STDERR_EMPTY()

//@<> InnoDB Cluster: create cluster
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("tempCluster");
cluster.addInstance(__sandbox_uri2, {recoveryMethod:'incremental'});
EXPECT_STDERR_EMPTY()

//@<> InnoDB Cluster: drop metadata on slave
shell.connect(__sandbox_uri2);
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> InnoDB Cluster: drop metadata on master, no options
shell.connect(__sandbox_uri1);
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> InnoDB Cluster: drop metadata on master, force false
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:false}) }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> InnoDB Cluster: drop metadata on read only master, force true
session.runSql("SET GLOBAL super_read_only=1");
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:true}) }, "Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode")

//@<> InnoDB Cluster: drop metadata on slave with read only master, force true
shell.connect(__sandbox_uri2);
session.runSql("SET GLOBAL super_read_only=1");
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:true}) }, "Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode")

//@<> InnoDB Cluster: drop metadata on slave with read only master: force true, clearReadOnly true
dba.dropMetadataSchema({force:true, clearReadOnly:true});
EXPECT_STDERR_EMPTY()

//@<> InnoDB Cluster: Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


//@<> Replica Set: Initialization {VER(>8.0.0)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
EXPECT_STDERR_EMPTY()

//@<> Replica Set: create cluster {VER(>8.0.0)}
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rset.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY()

//@<> Replica Set: drop metadata on slave {VER(>8.0.0)}
shell.connect(__sandbox_uri2);
rset.status()
var rset2 = dba.getReplicaSet();
rset2.status()
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> Replica Set: drop metadata on master, no options {VER(>8.0.0)}
shell.connect(__sandbox_uri1);
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> Replica Set: drop metadata on master, force false {VER(>8.0.0)}
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:false}) }, "Dba.dropMetadataSchema: No operation executed, use the 'force' option")

//@<> Replica Set: drop metadata on read only master, force true {VER(>8.0.0)}
session.runSql("SET GLOBAL super_read_only=1");
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:true}) }, "Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode")

//@<> Replica Set: drop metadata on slave with read only master, force true {VER(>8.0.0)}
shell.connect(__sandbox_uri2);
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:true}) }, "Dba.dropMetadataSchema: Server in SUPER_READ_ONLY mode")

//@<> Replica Set: drop metadata on slave with read only master: force true, clearReadOnly true {VER(>8.0.0)}
dba.dropMetadataSchema({force:true, clearReadOnly:true});
EXPECT_STDERR_EMPTY()

//@<> Replica Set: Finalization {VER(>8.0.0)}
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);