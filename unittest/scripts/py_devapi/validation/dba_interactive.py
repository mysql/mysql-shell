#@ Initialization
|Are you sure you want to remove the Metadata? [y/N]:|*

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
||Invalid number of arguments in Dba.create_cluster, expected 1 to 3 but got 0
||Dba.create_cluster: Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty

#@# Dba: create_cluster with interaction
|Please enter an administrative MASTER password to be used for the Cluster|
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
