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
||ArgumentError: Invalid number of arguments in Dba.create_cluster, expected 1 to 2 but got 0
||Argument #1 is expected to be a string
||Dba.create_cluster: The Cluster name cannot be empty
||Argument #2 is expected to be a map
||Invalid values in the options: another, invalid

#@# Dba: create_cluster succeed
|<Cluster:devCluster>|

#@# Dba: create_cluster already exist
||Dba.create_cluster: Cluster is already initialized. Use getCluster() to access it.

#@# Dba: get_cluster errors
||Invalid cluster name: Argument #1 is expected to be a string
||Invalid number of arguments in Dba.get_cluster, expected 0 to 1 but got 2
||Dba.get_cluster: The Cluster name cannot be empty

#@ Dba: get_cluster
|<Cluster:devCluster>|
