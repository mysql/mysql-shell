//@ Farm: validating members
|Farm Members: 8|
|name: OK|
|getName: OK|
|adminType: OK|
|getAdminType: OK|
|addSeedInstance: OK|
|addInstance: OK|
|removeInstance: OK|
|getReplicaSet: OK|

//@ Farm: addSeedInstance
||

//@# Farm: addInstance errors
||Invalid number of arguments in Farm.addInstance, expected 1 to 2 but got 0
||Invalid number of arguments in Farm.addInstance, expected 1 to 2 but got 3
||Farm.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Farm.addInstance: The connection data contains the next invalid attributes: authMethod, schema
||Farm.addInstance: Missing required attribute: host

//@# Farm: addInstance
||
