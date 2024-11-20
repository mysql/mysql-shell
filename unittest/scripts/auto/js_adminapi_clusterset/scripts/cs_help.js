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

//@ Dissolve
cs.help("dissolve");

//@ Dissolve \? [USE:Dissolve]
\? ClusterSet.dissolve

//@ Execute
cs.help("execute");

//@ Execute \? [USE:Execute]
\? ClusterSet.execute

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

//@ routerOptions
cs.help("routerOptions");

//@ routerOptions \? [USE:routerOptions]
\? ClusterSet.routerOptions

//@ setRoutingOption
cs.help("setRoutingOption");

//@ setRoutingOption \? [USE:setRoutingOption]
\? ClusterSet.setRoutingOption

//@ setOption
\? ClusterSet.setOption

//@ options
\? ClusterSet.options

//@ setupAdminAccount
\? ClusterSet.setupAdminAccount

//@ setupRouterAccount
\? ClusterSet.setupRouterAccount

//@ createRoutingGuideline
cs.help("createRoutingGuideline")

//@ createRoutingGuideline, \? [USE:createRoutingGuideline]
\? ClusterSet.createRoutingGuideline

//@ createRoutingGuideline, \help [USE:createRoutingGuideline]
\help ClusterSet.createRoutingGuideline

//@ getRoutingGuideline
cs.help("getRoutingGuideline")

//@ getRoutingGuideline, \? [USE:getRoutingGuideline]
\? ClusterSet.getRoutingGuideline

//@ getRoutingGuideline, \help [USE:getRoutingGuideline]
\help ClusterSet.getRoutingGuideline

//@ removeRoutingGuideline
cs.help("removeRoutingGuideline")

//@ removeRoutingGuideline, \? [USE:removeRoutingGuideline]
\? ClusterSet.removeRoutingGuideline

//@ removeRoutingGuideline, \help [USE:removeRoutingGuideline]
\help ClusterSet.removeRoutingGuideline

//@ routingGuidelines
cs.help("routingGuidelines")

//@ routingGuidelines, \? [USE:routingGuidelines]
\? ClusterSet.routingGuidelines

//@ routingGuidelines, \help [USE:routingGuidelines]
\help ClusterSet.routingGuidelines

//@ importRoutingGuideline
cs.help("importRoutingGuideline")

//@ importRoutingGuideline, \? [USE:importRoutingGuideline]
\? ClusterSet.importRoutingGuideline

//@ importRoutingGuideline, \help [USE:importRoutingGuideline]
\help ClusterSet.importRoutingGuideline

//@<> Clean-up.
cs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);

