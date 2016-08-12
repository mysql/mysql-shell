#@ Farm: validating members
|Farm Members: 8|
|name: OK|
|get_name: OK|
|admin_type: OK|
|get_admin_type: OK|
|add_seed_instance: OK|
|add_instance: OK|
|remove_instance: OK|
|get_replica_set: OK|

#@ Farm: add_seed_instance
||

#@# Farm: add_instance errors
||Invalid number of arguments in Farm.add_instance, expected 2 to 3 but got 0
||Invalid number of arguments in Farm.add_instance, expected 2 to 3 but got 4
||Farm.add_instance: Argument #1 is expected to be a string
||Farm.add_instance: The MASTER Farm password cannot be empty
||Farm.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Farm.add_instance: The connection data contains the next invalid attributes: authMethod, schema
||Farm.add_instance: Missing required attribute: host
||already belongs to the ReplicaSet
