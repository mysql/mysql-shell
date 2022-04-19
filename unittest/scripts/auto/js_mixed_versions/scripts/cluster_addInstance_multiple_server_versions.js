var port1 = __mysql_sandbox_port1*10+1;
var port2 = __mysql_sandbox_port2*10+1;

//@<> Initialization IPv6 local_address of cluster members not supported by target instance {VER(>= 8.0.14) && DEF(MYSQLD8013_PATH)}
// Deploy 8.0 Sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});

// Deploy 8.0.13 sandbox
testutil.deploySandbox(__mysql_sandbox_port3, "root", { report_host: "127.0.0.1" }, {mysqldPath: MYSQLD8013_PATH});
shell.connect(__sandbox_uri1);
// create cluster
var cluster;
if (__version_num < 80027) {
    cluster = dba.createCluster("Cluster", {gtidSetIsComplete: true});
} else {
    cluster = dba.createCluster("Cluster", {gtidSetIsComplete: true, communicationStack: "xcom"});
}
cluster.addInstance(__sandbox_uri2);

//@ IPv6 local_address of cluster members not supported by target instance {VER(>= 8.0.14) && DEF(MYSQLD8013_PATH)}
EXPECT_THROWS(function () {
    cluster.addInstance(__sandbox_uri3);
}, "Instance does not support the following localAddress values of the cluster: '[::1]:"+ port1 + ", [::1]:" + port2 + "'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is 8.0.13.");

//@<> Cleanup IPv6 local_address of cluster members not supported by target instance {VER(>= 8.0.14) && DEF(MYSQLD8013_PATH)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);


//@<> Initialization IPv6 local_address of target instance not supported by cluster members {VER(>= 8.0.14) && DEF(MYSQLD57_PATH)}
// Deploy 5.7 sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: "127.0.0.1" }, {mysqldPath: MYSQLD57_PATH});

// Deploy 8.0 sandbox
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: "::1"});

// create cluster
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("Cluster", {gtidSetIsComplete: true});


//@<> IPv6 local_address of target instance not supported by cluster members {VER(>= 8.0.14) && DEF(MYSQLD57_PATH)}
EXPECT_THROWS(function () {
    cluster.addInstance(__sandbox_uri2);
}, "Cannot use value '[::1]:"+ port2 + "' for option localAddress because it has an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14 but the cluster contains at least one member with version");

//@<> Cleanup IPv6 local_address of target instance not supported by cluster members {VER(>= 8.0.14) && DEF(MYSQLD57_PATH)}
cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
