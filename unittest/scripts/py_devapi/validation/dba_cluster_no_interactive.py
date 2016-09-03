#@ Cluster: validating members
|Cluster Members: 11|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_instance: OK|
|remove_instance: OK|
|get_replica_set: OK|
|rejoin_instance: OK|
|describe: OK|
|status: OK|

#@ Cluster: remove_instance
||

#@ Cluster: add_instance
||

#@# Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 1 to 2 but got 4
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Missing instance options: host
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: Unexpected instance options: authMethod, schema
||Cluster.add_instance: Missing instance options: host, password
||already belongs to the ReplicaSet: 'default'

#@# Cluster: add_instance
||
