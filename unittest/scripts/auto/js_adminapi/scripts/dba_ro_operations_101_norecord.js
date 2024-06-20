//@ {!real_host_is_loopback}

//@<> INCLUDE metadata_schema_utils.inc

//@<> Initialization
metadata_1_0_1_file = "metadata_1_0_1.sql";
var allowlist = "127.0.0.1," + hostname_ip;
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2], {ipAllowlist: allowlist, gtidSetIsComplete: true});
var status = scene.cluster.status({extended:2});
var session = scene.session;
var topology = status.defaultReplicaSet.topology;
var instances = dir(topology);
var gr_uuid = status.defaultReplicaSet.groupName;
var gr_view_change_uuid = session.runSql("SELECT NULLIF(CONCAT(''/*!80026, @@group_replication_view_change_uuid*/), '')").fetchOne()[0];
var uuid1 = topology[instances[0]].memberId;
var uuid2 = topology[instances[1]].memberId;

session1 = mysql.getSession(__sandbox_uri1);
var server_id1 = session1.runSql("SELECT @@server_id").fetchOne()[0];

session2 = mysql.getSession(__sandbox_uri2);
var server_id2 = session2.runSql("SELECT @@server_id").fetchOne()[0];

scene.make_unmanaged();

// connect to first instance
prepare_1_0_1_metadata_from_template(metadata_1_0_1_file, gr_uuid, [[uuid1, server_id1], [uuid2, server_id2]]);

shell.connect(__sandbox_uri1);

testutil.importData(__sandbox_uri1, metadata_1_0_1_file);

testutil.rmfile(metadata_1_0_1_file);

//@<> Get Cluster from master
var c;
EXPECT_NO_THROWS(function(){ c = dba.getCluster(); });
EXPECT_OUTPUT_CONTAINS("WARNING: The installed metadata version '1.0.1' is lower than the version supported by Shell, version '2.2.0'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.");

//@ Status from master
testutil.wipeAllOutput();
c.status({extended:3})

//@ Describe from master
testutil.wipeAllOutput();
c.describe()

//@ Options from master
testutil.wipeAllOutput();
normalize_cluster_options(c.options());

//@<> Get Cluster from slave
shell.connect(__sandbox_uri2);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

var c = dba.getCluster();
EXPECT_OUTPUT_CONTAINS("WARNING: The installed metadata version '1.0.1' is lower than the version supported by Shell, version '2.2.0'. It is recommended to upgrade the Metadata. See \\? dba.upgradeMetadata for additional details.")

//@ Status from slave  [USE:Status from master]
testutil.wipeAllOutput();
c.status({extended:3})

//@ Describe from slave [USE:Describe from master]
testutil.wipeAllOutput();
c.describe()

//@ Options from slave [USE:Options from master]
testutil.wipeAllOutput();
normalize_cluster_options(c.options());

session.close();

//@<> Finalization
scene.destroy()
