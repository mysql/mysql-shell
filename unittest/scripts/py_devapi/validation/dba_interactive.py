#@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

#@ Session: validating members
|Session Members: 11|
|create_cluster: OK|
|delete_local_instance: OK|
|deploy_local_instance: OK|
|drop_cluster: OK|
|get_cluster: OK|
|help: OK|
|kill_local_instance: OK|
|reset_session: OK|
|start_local_instance: OK|
|validate_instance: OK|

#@# Dba: create_cluster errors
||Invalid number of arguments in Dba.create_cluster, expected 1 to 3 but got 0
||Dba.create_cluster: Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty

#@# Dba: create_cluster with interaction
|A new InnoDB cluster will be created on instance|
|When setting up a new InnoDB cluster it is required to define an administrative|
|MASTER key for the cluster.This MASTER key needs to be re - entered when making|
|changes to the cluster later on, e.g.adding new MySQL instances or configuring|
|MySQL Routers.Losing this MASTER key will require the configuration of all|
|InnoDB cluster entities to be changed.|
|Please specify an administrative MASTER key for the cluster 'devCluster':|
|Creating InnoDB cluster 'devCluster' on|
|Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.|
|At least 3 instances are needed for the cluster to be able to withstand up to|
|one server failure.|

|<Cluster:devCluster>|

#@# Dba: get_cluster errors
||ArgumentError: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||ArgumentError: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.get_cluster: The Cluster name cannot be empty

#@ Dba: get_cluster
|<Cluster:devCluster>|

#@ Dba: add_instance
||already belongs to the ReplicaSet: 'default'.||

#@ Dba: remove_instance
||

#@# Dba: drop_cluster errors
||Invalid number of arguments in Dba.drop_cluster, expected 1 to 2 but got 0
||Dba.drop_cluster: Argument #1 is expected to be a string
||Dba.drop_cluster: The Cluster name cannot be empty
||Dba.drop_cluster: Argument #2 is expected to be a map
||Invalid number of arguments in Dba.drop_cluster, expected 1 to 2 but got 3

#@ Dba: drop_cluster interaction no options, cancel
|To remove the Cluster 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|

#@ Dba: drop_cluster interaction missing option, ok error
|To remove the Cluster 'sample' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
||||Dba.drop_cluster: The cluster with the name 'sample' does not exist.

#@ Dba: drop_cluster interaction no options, ok success
|To remove the Cluster 'devCluster' the default replica set needs to be removed.|
|Do you want to remove the default replica set? [y/n]:|
