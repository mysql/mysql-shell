// This module updates downgrades a cluster definition to use
// Metadata 2.0.0
// Requires:
// - primary_port: the port of an instance to be used as primary instance
// - metadata_2_0_0_file: name of the file to be created with the 2.0.0 metadata definition
var common = require("_common")
var md_utils = require("_metadata_schema_utils")

shell.connect(common.sandbox_uri(primary_port))
testutil.touch(testutil.getSandboxLogPath(primary_port));
var cluster = dba.createCluster("sample");
var cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters").fetchAll()[0][0]

var status = cluster.status({extended:2});
var topology = status.defaultReplicaSet.topology;
var instances = dir(topology);
var gr_uuid = status.defaultReplicaSet.groupName;

var uuid1 = topology[instances[0]].memberId;

var primary_uri = common.sandbox_uri(ports[0])
session1 = mysql.getSession(primary_uri);
var server_id1 = session1.runSql("SELECT @@server_id").fetchOne()[0];

session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

// connect to first instance
md_utils.prepare_2_0_0_metadata_from_template(metadata_2_0_0_file, cluster_id, gr_uuid, [[uuid1, server_id1]]);

testutil.importData(primary_uri, metadata_2_0_0_file);
