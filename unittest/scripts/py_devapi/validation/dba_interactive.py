#@ Session: validating members
|Session Members: 13|
|create_cluster: OK|
|delete_sandbox_instance: OK|
|deploy_sandbox_instance: OK|
|get_cluster: OK|
|help: OK|
|kill_sandbox_instance: OK|
|reset_session: OK|
|start_sandbox_instance: OK|
|validate_instance: OK|
|stop_sandbox_instance: OK|
|drop_metadata_schema: OK|
|prepare_instance: OK|
|verbose: OK|

#@# Dba: create_cluster errors
||Invalid number of arguments in Dba.create_cluster, expected 1 to 2 but got 0
||Invalid number of arguments in Dba.create_cluster, expected 1 to 2 but got 4
||Dba.create_cluster: Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty
||Dba.create_cluster: Invalid values in the options: another, invalid

#@<OUT> Dba: create_cluster with interaction
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@# Dba: get_cluster errors
||ArgumentError: Dba.get_cluster: Invalid cluster name: Argument #1 is expected to be a string
||Dba.get_cluster: The Cluster name cannot be empty

#@<OUT> Dba: get_cluster with interaction
<Cluster:devCluster>

#@<OUT> Dba: get_cluster with interaction (default)
<Cluster:devCluster>
