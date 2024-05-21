// Assumptions: smart deployment routines available
//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// create cluster
var cluster = dba.createCluster('dev');

session.close()

//@ Object Help
cluster.help()

//@ Object Global Help [USE:Object Help]
\? cluster

//@ Name
cluster.help("name")

//@ Name, \? [USE:Name]
\? cluster.name

//@ Add Instance
cluster.help("addInstance")

//@ Add Instance \? [USE:Add Instance]
\? cluster.addinstance

//@ Describe
cluster.help("describe")

//@ Describe, \? [USE:Describe]
\? cluster.describe

//@ Disconnect
cluster.help("disconnect")

//@ Disconnect, \? [USE:Disconnect]
\? cluster.disconnect

//@ Dissolve
cluster.help("dissolve")

//@ Dissolve, \? [USE:Dissolve]
\? cluster.dissolve

//@ Execute
cluster.help("execute")

//@ Execute, \? [USE:Execute]
\? cluster.execute

//@ Force Quorum Using Partition Of
cluster.help("forceQuorumUsingPartitionOf")

//@ Force Quorum Using Partition Of, \? [USE:Force Quorum Using Partition Of]
\? forceQuorumUsingPartitionOf

//@ Get Name
cluster.help("getName")

//@ Get Name, \? [USE:Get Name]
\? cluster.getName

//@ Help
cluster.help("help")

//@ Help. \? [USE:Help]
\? cluster.help

//@ listRouters
cluster.help("listRouters")

//@ listRouters. \? [USE:listRouters]
\? cluster.listRouters

//@ removeRouterMetadata
cluster.help("removeRouterMetadata")

//@ removeRouterMetadata. \? [USE:removeRouterMetadata]
\? cluster.removeRouterMetadata

//@ Options
cluster.help("options")

//@ Options. \? [USE:Options]
\? cluster.options

//@ Rejoin Instance
cluster.help("rejoinInstance")

//@ Rejoin Instance, \? [USE:Rejoin Instance]
\? cluster.rejoinInstance

//@ Remove Instance
// WL#11862 - FR3_5
cluster.help("removeInstance");

//@ Remove Instance, \? [USE:Remove Instance]
\? cluster.removeInstance

//@ SetInstanceOption
cluster.help("setInstanceOption")

//@ SetInstanceOption, \? [USE:SetInstanceOption]
\? cluster.setInstanceOption

//@ SetOption
cluster.help("setOption")

//@ SetOption, \? [USE:SetOption]
\? cluster.setOption

//@ setupAdminAccount
cluster.help("setupAdminAccount")

//@ setupAdminAccount. \? [USE:setupAdminAccount]
\? cluster.setupAdminAccount

//@ setupAdminAccount. \help [USE:setupAdminAccount]
\help cluster.setupAdminAccount

//@ setupRouterAccount
cluster.help("setupRouterAccount")

//@ setupRouterAccount. \? [USE:setupRouterAccount]
\? cluster.setupRouterAccount

//@ setupRouterAccount. \help [USE:setupRouterAccount]
\help cluster.setupRouterAccount

//@ Rescan
cluster.help("rescan")

//@ Rescan, \? [USE:Rescan]
\? cluster.rescan

//@ Status
cluster.help("status")

//@ Status, \? [USE:Status]
\? cluster.status

//@ setPrimaryInstance
cluster.help("setPrimaryInstance")

//@ setPrimaryInstance, \? [USE:setPrimaryInstance]
\? cluster.setPrimaryInstance

//@ switchToMultiPrimaryMode
cluster.help("switchToMultiPrimaryMode")

//@ switchToMultiPrimaryMode, \? [USE:switchToMultiPrimaryMode]
\? cluster.switchToMultiPrimaryMode

//@ switchToSinglePrimaryMode
cluster.help("switchToSinglePrimaryMode")

//@ switchToSinglePrimaryMode, \? [USE:switchToSinglePrimaryMode]
\? cluster.switchToSinglePrimaryMode

//@ resetRecoveryAccountsPassword
cluster.help("resetRecoveryAccountsPassword")

//@ resetRecoveryAccountsPassword, \? [USE:resetRecoveryAccountsPassword]
\? cluster.resetRecoveryAccountsPassword

//@ resetRecoveryAccountsPassword, \help [USE:resetRecoveryAccountsPassword]
\help cluster.resetRecoveryAccountsPassword

//@ createClusterSet
cluster.help("createClusterSet")

//@ createClusterSet, \? [USE:createClusterSet]
\? cluster.createClusterSet

//@ createClusterSet, \help [USE:createClusterSet]
\help cluster.createClusterSet

//@ getClusterSet
cluster.help("getClusterSet")

//@ getClusterSet, \? [USE:getClusterSet]
\? cluster.getClusterSet

//@ getClusterSet, \help [USE:getClusterSet]
\help cluster.getClusterSet

//@ fenceAllTraffic
cluster.help("fenceAllTraffic")

//@ fenceAllTraffic, \? [USE:fenceAllTraffic]
\? cluster.fenceAllTraffic

//@ fenceAllTraffic, \help [USE:fenceAllTraffic]
\help cluster.fenceAllTraffic

//@ fenceWrites
cluster.help("fenceWrites")

//@ fenceWrites, \? [USE:fenceWrites]
\? cluster.fenceWrites

//@ fenceWrites, \help [USE:fenceWrites]
\help cluster.fenceWrites

//@ unfenceWrites
cluster.help("unfenceWrites")

//@ unfenceWrites, \? [USE:unfenceWrites]
\? cluster.unfenceWrites

//@ unfenceWrites, \help [USE:unfenceWrites]
\help cluster.unfenceWrites

//@ addReplicaInstance
cluster.help("addReplicaInstance")

//@ addReplicaInstance, \? [USE:addReplicaInstance]
\? cluster.addReplicaInstance

//@ addReplicaInstance, \help [USE:addReplicaInstance]
\help cluster.addReplicaInstance

//@ routingOptions
cluster.help("routingOptions")

//@ routingOptions, \? [USE:routingOptions]
\? cluster.routingOptions

//@ routingOptions, \help [USE:routingOptions]
\help cluster.routingOptions

//@ routerOptions
cluster.help("routerOptions")

//@ routerOptions, \? [USE:routerOptions]
\? cluster.routerOptions

//@ routerOptions, \help [USE:routerOptions]
\help cluster.routerOptions

//@ setRoutingOption
cluster.help("setRoutingOption")

//@ setRoutingOption, \? [USE:setRoutingOption]
\? cluster.setRoutingOption

//@ setRoutingOption, \help [USE:setRoutingOption]
\help cluster.setRoutingOption

//@<> Finalization
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
