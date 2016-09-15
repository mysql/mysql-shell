#@ Session: validating members
|Session Members: 12|
|create_cluster: OK|
|delete_local_instance: OK|
|deploy_local_instance: OK|
|get_cluster: OK|
|help: OK|
|kill_local_instance: OK|
|reset_session: OK|
|start_local_instance: OK|
|validate_instance: OK|
|stop_local_instance: OK|
|drop_metadata_schema: OK|
|verbose: OK|

#@# Dba: create_cluster errors
||Invalid number of arguments in Dba.create_cluster, expected 1 to 3 but got 0
||Invalid number of arguments in Dba.create_cluster, expected 1 to 3 but got 4
||Dba.create_cluster: Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty

#@<OUT> Dba: create_cluster with interaction
A new InnoDB cluster will be created on instance 'root@localhost:<<<__mysql_sandbox_port1>>>'.

When setting up a new InnoDB cluster it is required to define an administrative
MASTER key for the cluster. This MASTER key needs to be re-entered when making
changes to the cluster later on, e.g.adding new MySQL instances or configuring
MySQL Routers. Losing this MASTER key will require the configuration of all
InnoDB cluster entities to be changed.

Please specify an administrative MASTER key for the cluster 'devCluster': Creating InnoDB cluster 'devCluster' on 'root@localhost:<<<__mysql_sandbox_port1>>>'...
Adding Seed Instance...

Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

#@# Dba: get_cluster errors
||ArgumentError: Dba.get_cluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.get_cluster: The Cluster name cannot be empty

#@<OUT> Dba: get_cluster with interaction
When the InnoDB cluster was setup, a MASTER key was defined in order to enable
performing administrative tasks on the cluster.

Please specify the administrative MASTER key for the cluster 'devCluster': <Cluster:devCluster>

#@<OUT> Dba: get_cluster with interaction (default)
When the InnoDB cluster was setup, a MASTER key was defined in order to enable
performing administrative tasks on the cluster.

Please specify the administrative MASTER key for the default cluster: <Cluster:devCluster>
