// Assumptions: smart deployment rountines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> create cluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED'});

cluster.status()

//@<OUT> Object Help
cluster.help()

//@<OUT> Add Instance
cluster.help("addInstance")

//@<OUT> Check Instance State
cluster.help("checkInstanceState")

//@<OUT> Describe
cluster.help("describe")

//@<OUT> Dissolve
cluster.help("dissolve")

//@<OUT> Force Quorum Using Partition Of
cluster.help("forceQuorumUsingPartitionOf")

//@<OUT> Get Name
cluster.help("getName")

//@<OUT> Help
cluster.help("help")

//@<OUT> Rejoin Instance
cluster.help("rejoinInstance")

//@<OUT> Remove Instance
cluster.help("removeInstance")

//@<OUT> Rescan
cluster.help("rescan")

//@<OUT> Status
cluster.help("status")

session.close();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
