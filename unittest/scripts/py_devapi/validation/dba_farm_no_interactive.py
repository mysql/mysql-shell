#@ Cluster: validating members
|Cluster Members: 8|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_seed_instance: OK|
|add_instance: OK|
|remove_instance: OK|
|get_replica_set: OK|

#@ Cluster: add_seed_instance
||

#@# Cluster: add_instance errors
||Invalid number of arguments in Cluster.add_instance, expected 2 to 3 but got 0
||Invalid number of arguments in Cluster.add_instance, expected 2 to 3 but got 4
||Cluster.add_instance: Argument #1 is expected to be a string
||Cluster.add_instance: The MASTER Cluster password cannot be empty
||Cluster.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.add_instance: The connection data contains the next invalid attributes: authMethod, schema
||Cluster.add_instance: Missing required attribute: host
||already belongs to the ReplicaSet
