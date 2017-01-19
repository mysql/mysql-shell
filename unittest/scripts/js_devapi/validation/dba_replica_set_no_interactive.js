//@ ReplicaSet: validating members
|Replica Set Members: 6|
|name: OK|
|getName: OK|
|addInstance: OK|
|removeInstance: OK|
|help: OK|
|rejoinInstance: OK|

//@# ReplicaSet: addInstance errors
||Invalid number of arguments in ReplicaSet.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in ReplicaSet.addInstance, expected 1 to 2 but got 4
||ReplicaSet.addInstance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.addInstance: Missing instance options: host
||ReplicaSet.addInstance: Invalid connection options, expected either a URI or a Dictionary
||ReplicaSet.addInstance: Unexpected instance options: authMethod, schema
||ReplicaSet.addInstance: Missing instance options: host, password
||is already part of this InnoDB cluster
