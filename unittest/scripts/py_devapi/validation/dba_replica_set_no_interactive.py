#@ Cluster: validating members
|Replica Set Members: 6|
|name: OK|
|get_name: OK|
|add_instance: OK|
|remove_instance: OK|
|help: OK|
|rejoin_instance: OK|

#@# Cluster: add_instance errors
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 to 2 but got 4
||ReplicaSet.add_instance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.add_instance: Missing instance options: host
||ReplicaSet.add_instance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.add_instance: Unexpected instance options: authMethod, schema
||ReplicaSet.add_instance: Missing instance options: host, password
||already belongs to the ReplicaSet: 'default'
