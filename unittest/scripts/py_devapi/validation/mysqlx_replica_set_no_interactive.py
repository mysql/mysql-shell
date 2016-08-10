#@ Farm: validating members
|Replica Set Members: 4|
|name: OK|
|get_name: OK|
|add_instance: OK|
|remove_instance: OK|

#@# Farm: add_instance errors
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 but got 0
||Invalid number of arguments in ReplicaSet.add_instance, expected 1 but got 2
||ReplicaSet.add_instance: Unexpected argument on connection data
||ReplicaSet.add_instance: Unexpected argument 'schema' on connection data
||ReplicaSet.add_instance: Unexpected argument 'user' on connection data
||ReplicaSet.add_instance: Unexpected argument 'password' on connection data
||ReplicaSet.add_instance: Unexpected argument 'authMethod' on connection data
||ReplicaSet.add_instance: Missing required value for hostname
||ReplicaSet.add_instance: Connection data empty

#@# Farm: add_instance
||
