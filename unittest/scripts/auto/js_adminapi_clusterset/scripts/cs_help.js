//@ {VER(>=8.0.27)}

// Tests help of ClusterSet functions.

//@<> Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster('dev');
session.close();
var cs = cluster.createClusterSet("devClusterSet");

//@ Object Help
cs.help();

//@ Object Global Help [USE:Object Help]
\? ClusterSet

//@ Name
cs.help("name");

//@ Name, \? [USE:Name]
\? ClusterSet.name

//@ Disconnect
cs.help("disconnect");

//@ Disconnect \? [USE:Disconnect]
\? ClusterSet.disconnect

//@ CreateReplicaCluster
cs.help("createReplicaCluster");

//@ CreateReplicaCluster \? [USE:CreateReplicaCluster]
\? ClusterSet.CreateReplicaCluster

//@ RemoveCluster
cs.help("removeCluster");

//@ RemoveCluster \? [USE:RemoveCluster]
\? ClusterSet.removeCluster

//@ ClusterSet.setPrimaryCluster
\? ClusterSet.setPrimaryCluster

//@ ClusterSet.forcePrimaryCluster
\? ClusterSet.forcePrimaryCluster

//@ ClusterSet.rejoinCluster
\? ClusterSet.rejoinCluster

//@ Status
cs.help("status");

//@ Status \? [USE:Status]
\? ClusterSet.Status

//@ Describe
cs.help("describe");

//@ Describe \? [USE:Describe]
\? ClusterSet.Describe

//@ listRouters
cs.help("listRouters");

//@ listRouters \? [USE:listRouters]
\? ClusterSet.listRouters

//@ routingOptions
cs.help("routingOptions");

//@ routingOptions \? [USE:routingOptions]
\? ClusterSet.routingOptions

//@ setRoutingOption
cs.help("setRoutingOption");

//@ setRoutingOption \? [USE:setRoutingOption]
\? ClusterSet.setRoutingOption

//@<> Clean-up.
cs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
