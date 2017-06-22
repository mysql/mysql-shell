// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@<OUT> create cluster
var cluster = dba.createCluster('dev');

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
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
