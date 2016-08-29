//@ Cluster: validating members
|Replica Set Members: 4|
|name: OK|
|getName: OK|
|addInstance: OK|
|removeInstance: OK|

//@# Cluster: addInstance errors
||Invalid number of arguments in ReplicaSet.addInstance, expected 2 to 3 but got 0
||Invalid number of arguments in ReplicaSet.addInstance, expected 2 to 3 but got 4
||ReplicaSet.addInstance: Argument #1 is expected to be a string
||ReplicaSet.addInstance: The MASTER Cluster password cannot be empty
||ReplicaSet.addInstance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.addInstance: Unexpected instance options: authMethod, schema
||ReplicaSet.addInstance: Missing required attribute: host
||already belongs to the ReplicaSet
