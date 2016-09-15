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
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 0
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 1
||Dba.create_cluster: The Cluster name cannot be empty
||Invalid number of arguments in Dba.create_cluster, expected 2 to 3 but got 1

#@# Dba: create_cluster succeed
|<Cluster:devCluster>|

#@# Dba: create_cluster already exist
||Dba.create_cluster: Cluster is already initialized. Use getCluster() to access it.

#@# Dba: get_cluster errors
||Dba.get_cluster: Unexpected parameter received expected either the InnoDB cluster name or a Dictionary with options
||Dba.get_cluster: Argument #2 is expected to be a map
||Dba.get_cluster: The Cluster name cannot be empty
||Dba.get_cluster: Authentication failure: wrong MASTER key.

#@ Dba: get_cluster
|<Cluster:devCluster>|
