// Requires:
// - primary_port: the port of an instance to be used as primary instance
// - metadata_2_1_0_file: name of the file to be created with the 2.1.0 metadata definition

sandbox_uri = function(port) {
    switch(port){
        case __mysql_sandbox_port1:
            return __sandbox_uri1;
        case __mysql_sandbox_port2:
            return __sandbox_uri2;
        case __mysql_sandbox_port3:
            return __sandbox_uri3;
        case __mysql_sandbox_port4:
            return __sandbox_uri4;
        case __mysql_sandbox_port5:
            return __sandbox_uri5;
        case __mysql_sandbox_port6:
            return __sandbox_uri6;
    }
};

shell.connect(sandbox_uri(primary_port))
testutil.touch(testutil.getSandboxLogPath(primary_port));
var rs = dba.createReplicaSet("sample");
var cluster_id = session.runSql("select cluster_id from mysql_innodb_cluster_metadata.clusters").fetchAll()[0][0]

var status = rs.status({extended:2});
var topology = status.replicaSet.topology;
var instances = dir(topology);

var uuid1 = topology[instances[0]].serverUuid;

var primary_uri = sandbox_uri(ports[0])
session1 = mysql.getSession(primary_uri);
var server_id1 = session1.runSql("SELECT @@server_id").fetchOne()[0];

session.runSql("DROP SCHEMA mysql_innodb_cluster_metadata");

// connect to first instance
prepare_2_1_0_metadata_from_template(metadata_2_1_0_file, cluster_id, [[uuid1, server_id1]]);

testutil.importData(primary_uri, metadata_2_1_0_file);
