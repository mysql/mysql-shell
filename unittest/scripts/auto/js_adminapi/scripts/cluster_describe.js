// <Cluster.>describe() tests

//@<> Create cluster
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var session = scene.session

// Exceptions in <Cluster.>describe():
//    - If the InnoDB Cluster topology mode does not match the current Group
//      Replication configuration.

//@<> Manually change the topology mode in the metadata
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"mm\"");
session.runSql("SET sql_log_bin = 0");
cluster = dba.getCluster();

//@ Error when executing describe on a cluster with the topology mode different than GR
cluster.describe()

//@<> Manually change back the topology mode
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set primary_mode = \"pm\"");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"newName\"");
session.runSql("SET sql_log_bin = 0");

//@ Error when executing describe on a cluster that its name is not registered in the metadata
cluster.describe()

//@<> Manually change back the the cluster name
session.runSql("SET sql_log_bin = 0");
session.runSql("update mysql_innodb_cluster_metadata.clusters set cluster_name = \"cluster\"");
session.runSql("SET sql_log_bin = 0");
var cluster = dba.getCluster();

//@<OUT> Describe cluster
cluster.describe();

//@<> Remove two instances of the cluster and add back one of them with a different label
cluster.removeInstance(__sandbox_uri2);
cluster.removeInstance(__sandbox_uri3);
cluster.addInstance(__sandbox_uri2, {label: "zzLabel"});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<OUT> Describe cluster with 2 instances having one of them a non-default label
cluster.describe();

//@<> Execute describe from a read-only member of the cluster
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");
cluster.disconnect();
session.close();
shell.connect(__sandbox_uri2);
var cluster = dba.getCluster();

//@<OUT> Describe from a read-only member
cluster.describe();

//@<> Finalization
scene.destroy();
