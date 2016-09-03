//@ Cluster: validating members
|Cluster Members: 11|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addInstance: OK|
|removeInstance: OK|
|getReplicaSet: OK|
|rejoinInstance: OK|
|describe: OK|
|status: OK|

//@ Cluster: removeInstance
||

//@ Cluster: addInstance
||

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 1 to 2 but got 4
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Missing instance options: host
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: Unexpected instance options: authMethod, schema
||Cluster.addInstance: Missing instance options: host, password
||already belongs to the ReplicaSet: 'default'

//@# Cluster: addInstance
||
