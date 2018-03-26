# Assumptions: smart deployment rountines available
#@ Initialization
testutil.deploy_sandbox(__mysql_sandbox_port1, "root");

shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

cluster = dba.create_cluster('dev')

#@<OUT> Object Help
cluster.help()

#@<OUT> Add Instance
cluster.help("add_instance")

#@<OUT> Check Instance State
cluster.help("check_instance_state")

#@<OUT> Describe
cluster.help("describe")

#@<OUT> Dissolve
cluster.help("dissolve")

#@<OUT> Force Quorum Using Partition Of
cluster.help("force_quorum_using_partition_of")

#@<OUT> Get Name
cluster.help("get_name")

#@<OUT> Help
cluster.help("help")

#@<OUT> Rejoin Instance
cluster.help("rejoin_instance")

#@<OUT> Remove Instance
cluster.help("remove_instance")

#@<OUT> Rescan
cluster.help("rescan")

#@<OUT> Status
cluster.help("status")

#@<> Finalization
session.close();
cluster.disconnect()
testutil.destroy_sandbox(__mysql_sandbox_port1);
