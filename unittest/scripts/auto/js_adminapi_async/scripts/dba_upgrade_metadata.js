//@ {VER(>=8.0.11)}

//@<> Configures custom user on the instance
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri1, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port1)
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
dba.configureInstance(__sandbox_uri2, {clusterAdmin: 'tst_admin', clusterAdminPassword: 'tst_pwd'});
testutil.snapshotSandboxConf(__mysql_sandbox_port2)
cluster_admin_uri= "mysql://tst_admin:tst_pwd@" + __host + ":" + __mysql_sandbox_port1;


//@<> upgradeMetadata without connection
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "An open session is required to perform this operation")

// Session to be used through all the AAPI calls
shell.connect(__sandbox_uri2)
var server_uuid2 = session.runSql("SELECT @@server_uuid").fetchOne()[0];

//@<> upgradeMetadata on a standalone instance
shell.connect(__sandbox_uri1)
var server_uuid1 = session.runSql("SELECT @@server_uuid").fetchOne()[0];
EXPECT_THROWS(function(){dba.upgradeMetadata()}, "This function is not available through a session to a standalone instance")

//@<> Creates the sample cluster
var rset = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rset.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY()

//@<> Upgrades the metadata, up to date
dba.upgradeMetadata({interactive:true})
EXPECT_OUTPUT_CONTAINS(`NOTE: Installed metadata at '${hostname}:${__mysql_sandbox_port1}' is up to date (version 2.0.0).`);


//@<> Upgrades the metadata from slave, up to date
shell.connect(__sandbox_uri2)
dba.upgradeMetadata({interactive:true})
EXPECT_OUTPUT_CONTAINS(`NOTE: Installed metadata at '${hostname}:${__mysql_sandbox_port1}' is up to date (version 2.0.0).`);

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1)
testutil.destroySandbox(__mysql_sandbox_port2)
