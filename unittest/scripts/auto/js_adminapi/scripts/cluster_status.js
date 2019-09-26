// <Cluster.>status() tests

//@<> Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session;

// Exceptions in <Cluster.>status():
//    - If the InnoDB Cluster topology mode does not match the current Group
//      Replication configuration.

//@<> Manually change the topology mode in the metadata
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"mm\"");
session.runSql("SET sql_log_bin = 0");
var cluster = dba.getCluster();

//@ Error when executing status on a cluster with the topology mode different than GR
cluster.status();

//@<> Manually change back the topology mode in the metadata and change the cluster name
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"pm\"");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"newName\"");
session.runSql("SET sql_log_bin = 0");

//@ Error when executing status on a cluster that its name is not registered in the metadata
cluster.status();

//@<> Manually change back the the cluster name
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"cluster\"");
session.runSql("SET sql_log_bin = 0");
var cluster = dba.getCluster();

//@<OUT> Status cluster
var stat = cluster.status();
println(stat);

//@<> WL#13084 - TSF4_5: extended: 0 is the default (same as with no options).
var ext_0_status = cluster.status({extended: 0});
EXPECT_EQ(ext_0_status, stat);

//@<> WL#13084 - TSF4_1: extended: false is the same as extended: 0.
var ext_false_status =cluster.status({extended: false});
EXPECT_EQ(ext_false_status, ext_0_status);

//@<> WL#13084 - TSF2_1: Set super_read_only to ON on primary.
set_sysvar(session, "super_read_only", 1);

//@<> WL#13084 - TSF2_1: Set super_read_only to OFF on a secondary.
session.close();
shell.connect(__sandbox_uri2);
set_sysvar(session, "super_read_only", 0);
session.close();

//@<OUT> WL#13084 - TSF2_1: status show accurate mode based on super_read_only.
shell.connect(__sandbox_uri1);
var cluster = dba.getCluster();
cluster.status();

//@<> WL#13084 - TSF2_1: Reset super_read_only back on primary and secondary.
session.close();
shell.connect(__sandbox_uri2);
set_sysvar(session, "super_read_only", 1);
session.close();
shell.connect(__sandbox_uri1);
set_sysvar(session, "super_read_only", 0);
var cluster = dba.getCluster();

//@<> Remove two instances of the cluster and add back one of them with a different label
cluster.removeInstance(__sandbox_uri2);
cluster.removeInstance(__sandbox_uri3);
cluster.addInstance(__sandbox_uri2, {label: "zzLabel"});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Status cluster with 2 instances having one of them a non-default label
cluster.status();

//@<> Add R/O instance back to the cluster
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Status cluster after adding R/O instance back
cluster.status()

//@<> Execute status from a read-only member of the cluster
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster();

//@<OUT> Status from a read-only member
cluster.status();

//@<> Remove primary instance to force the election of a new one
cluster.removeInstance(__sandbox_uri1);
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster();

//@<OUT> Status with a new primary
cluster.status();

//@<OUT> WL#13084 - TSF5_1: queryMembers option is deprecated (true).
cluster.status({queryMembers:true});

//@<OUT> WL#13084 - TSF5_1: queryMembers option is deprecated (false).
cluster.status({queryMembers:false});

//@<> Finalization
scene.destroy();
