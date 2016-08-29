#@ Cluster: validating members
|Replica Set Members: 4|
|name: OK|
|get_name: OK|
|add_instance: OK|
|remove_instance: OK|

#@# Cluster: add_instance errors
||Invalid number of arguments in ReplicaSet.add_instance, expected 2 to 3 but got 0
||Invalid number of arguments in ReplicaSet.add_instance, expected 2 to 3 but got 4
||ReplicaSet.add_instance: Argument #1 is expected to be a string
||ReplicaSet.add_instance: The MASTER Cluster password cannot be empty
||ReplicaSet.add_instance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.add_instance: The connection data contains the next invalid attributes: authMethod, schema
||ReplicaSet.add_instance: Missing required attribute: host
||already belongs to the ReplicaSet
