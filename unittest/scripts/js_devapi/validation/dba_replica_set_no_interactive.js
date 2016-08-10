//@ Farm: validating members
|Replica Set Members: 4|
|name: OK|
|getName: OK|
|addInstance: OK|
|removeInstance: OK|

//@# Farm: addInstance errors
||Invalid number of arguments in ReplicaSet.addInstance, expected 1 but got 0
||Invalid number of arguments in ReplicaSet.addInstance, expected 1 but got 2
||ReplicaSet.addInstance: Unexpected argument on connection data
||ReplicaSet.addInstance: Unexpected argument 'schema' on connection data
||ReplicaSet.addInstance: Unexpected argument 'user' on connection data
||ReplicaSet.addInstance: Unexpected argument 'password' on connection data
||ReplicaSet.addInstance: Unexpected argument 'authMethod' on connection data
||ReplicaSet.addInstance: Missing required value for hostname
||ReplicaSet.addInstance: Connection data empty

//@# Farm: addInstance
||
