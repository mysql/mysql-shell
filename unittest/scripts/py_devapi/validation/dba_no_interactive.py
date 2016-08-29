#@ Session: validating members
|Session Members: 9|
|default_cluster: OK|
|get_default_cluster: OK|
|create_cluster: OK|
|drop_cluster: OK|
|get_cluster: OK|
|drop_metadata_schema: OK|
|reset_session: OK|
|validate_instance: OK|
|deploy_local_instance: OK|

#@# Dba: create_cluster errors
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 1
||Dba.create_cluster: The Cluster name cannot be empty
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 1
||Dba.create_cluster: There is already one Cluster initialized. Only one Cluster is supported.

#@ Dba: create_cluster
|<Cluster:devCluster>|

#@# Dba: get_cluster errors
||Invalid number of arguments in Dba.get_cluster, expected 1 but got 0
||Dba.get_cluster: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.get_cluster, expected 1 but got 2
||Dba.get_cluster: The Cluster name cannot be empty

#@ Dba: get_cluster
|<Cluster:devCluster>|

#@ Dba: add_seed_instance
||


#@# Dba: drop_cluster errors
||Invalid number of arguments in Dba.drop_cluster, expected 1 to 2 but got 0
||Dba.drop_cluster: Argument #1 is expected to be a string
||Dba.drop_cluster: The Cluster name cannot be empty
||Dba.drop_cluster: Argument #2 is expected to be a map
||Invalid number of arguments in Dba.drop_cluster, expected 1 to 2 but got 3
||Dba.drop_cluster: The cluster with the name 'sample' does not exist.
||Dba.drop_cluster: The cluster with the name 'devCluster' is not empty.

#@ Dba: drop_cluster
||
