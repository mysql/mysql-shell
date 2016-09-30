#@ Session: validating members
|Session Members: 12|
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
||Dba.get_cluster: Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.getCluster, expected 0 to 1 but got 2
||Dba.get_cluster: The Cluster name cannot be empty

#@ Dba: get_cluster
|<Cluster:devCluster>|
