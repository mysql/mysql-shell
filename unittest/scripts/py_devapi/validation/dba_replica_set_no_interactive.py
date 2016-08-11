#@ Farm: validating members
|Replica Set Members: 4|
|name: OK|
|get_name: OK|
|add_instance: OK|
|remove_instance: OK|

#@# Farm: add_instance errors
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 to 2 but got 3
||ReplicaSet.add_instance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.add_instance: The connection data contains the next invalid attributes: authMethod, schema
||ReplicaSet.add_instance: Missing required attribute: host
||already belongs to the ReplicaSet
