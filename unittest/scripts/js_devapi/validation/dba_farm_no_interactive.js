//@ Cluster: validating members
|Cluster Members: 8|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addSeedInstance: OK|
|addInstance: OK|
|removeInstance: OK|
|getReplicaSet: OK|

//@ Cluster: addSeedInstance
||

//@# Cluster: addInstance errors
||Invalid number of arguments in Cluster.addInstance, expected 2 to 3 but got 0
||Invalid number of arguments in Cluster.addInstance, expected 2 to 3 but got 4
||Cluster.addInstance: Argument #1 is expected to be a string
||Cluster.addInstance: The MASTER Cluster password cannot be empty
||Cluster.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Cluster.addInstance: The connection data contains the next invalid attributes: authMethod, schema
||Cluster.addInstance: Missing required attribute: host

//@# Cluster: addInstance
||
