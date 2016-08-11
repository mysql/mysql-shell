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
||Invalid number of arguments in Farm.add_instance, expected 1 to 2 but got 0
||Invalid number of arguments in Farm.add_instance, expected 1 to 2 but got 3
||Farm.add_instance: Invalid connection options, expected either a URI or a Dictionary
||Farm.add_instance: The connection data contains the next invalid attributes: authMethod, schema
||Farm.add_instance: Missing required attribute: host
||already belongs to the ReplicaSet
