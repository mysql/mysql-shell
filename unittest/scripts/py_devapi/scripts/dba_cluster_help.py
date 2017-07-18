# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@<OUT> create cluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'DISABLED'})

cluster.status();

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

session.close();

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
