// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<> create cluster
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
\? addinstance

//@ Check Instance State
cluster.help("checkInstanceState")

//@ Check Instance State, \? [USE:Check Instance State]
\? checkInstanceState

//@ Describe
cluster.help("describe")

//@ Describe, \? [USE:Describe]
\? describe

//@ Disconnect
cluster.help("disconnect")

//@ Disconnect, \? [USE:Disconnect]
\? disconnect

//@ Dissolve
cluster.help("dissolve")

//@ Dissolve, \? [USE:Dissolve]
\? dissolve

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

//@ Rejoin Instance
cluster.help("rejoinInstance")

//@ Rejoin Instance, \? [USE:Rejoin Instance]
\? rejoinInstance

//@ Remove Instance
// WL#11862 - FR3_5
cluster.help("removeInstance");

//@ Remove Instance, \? [USE:Remove Instance]
\? removeInstance

//@ Rescan
cluster.help("rescan")

//@ Rescan, \? [USE:Rescan]
\? rescan

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


//@ Finalization
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
