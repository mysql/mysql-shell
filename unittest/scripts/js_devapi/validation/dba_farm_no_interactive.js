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
||Invalid number of arguments in Farm.addInstance, expected 2 to 3 but got 0
||Invalid number of arguments in Farm.addInstance, expected 2 to 3 but got 4
||Farm.addInstance: Argument #1 is expected to be a string
||Farm.addInstance: The MASTER Farm password cannot be empty
||Farm.addInstance: Invalid connection options, expected either a URI or a Dictionary
||Farm.addInstance: The connection data contains the next invalid attributes: authMethod, schema
||Farm.addInstance: Missing required attribute: host

//@# Farm: addInstance
||
