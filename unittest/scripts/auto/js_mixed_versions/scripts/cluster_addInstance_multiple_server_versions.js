//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.0.14) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, "==", "8.0.13")}

var port1 = __mysql_sandbox_port1*10+1;
var port2 = __mysql_sandbox_port2*10+1;

//@<> Initialization IPv6 local_address of cluster members not supported by target instance
// Deploy 8.0 Sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});

// Deploy 8.0.13 sandbox
testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: "127.0.0.1" }, {mysqldPath: MYSQLD_SECONDARY_SERVER_A.path});
shell.connect(__sandbox_uri1);
// create cluster
var cluster;
if (__version_num < 80027) {
    cluster = dba.createCluster("Cluster", {gtidSetIsComplete: true});
} else {
    cluster = dba.createCluster("Cluster", {gtidSetIsComplete: true, communicationStack: "xcom"});
}
cluster.addInstance(__sandbox_uri2);

//@<> IPv6 local_address of cluster members not supported by target instance
EXPECT_THROWS(function () {
    cluster.addInstance(__sandbox_uri3);
}, "Instance does not support the following localAddress values of the cluster: '[::1]:"+ port1 + ", [::1]:" + port2 + "'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is 8.0.13.");

EXPECT_OUTPUT_CONTAINS("ERROR: Cannot join instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' to cluster: unsupported localAddress value on the cluster.")

//@<> Cleanup IPv6 local_address of cluster members not supported by target instance
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
