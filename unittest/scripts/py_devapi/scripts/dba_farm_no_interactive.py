# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.get_farm({"enforce":True})

#@ Farm: validating members
farm = dba.create_farm('devFarm', 'testing')

all_members = dir(farm)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Farm Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'admin_type')
validateMember(members, 'get_admin_type')
validateMember(members, 'add_seed_instance')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'get_replica_set')

#@ Farm: add_seed_instance
# Added this to enable add_instance, full testing of add_seed_instance is needed
farm.add_seed_instance({"host": '192.168.1.1'})

#@# Farm: add_instance errors
farm.add_instance()
farm.add_instance(5,6)
farm.add_instance(5)
farm.add_instance({"host": '192.168.1.1', "schema": 'abs'})
farm.add_instance({"host": '192.168.1.1', "user": 'abs'})
farm.add_instance({"host": '192.168.1.1', "password": 'abs'})
farm.add_instance({"host": '192.168.1.1', "authMethod": 'abs'})
farm.add_instance({"port": 33060})
farm.add_instance('')

#@# Farm: add_instance
farm.add_instance('192.168.1.1:33060')
farm.add_instance({"host": '192.168.1.1', "port": 1234})

# Cleanup
dba.drop_farm('devFarm', {"dropDefaultReplicaSet": True})
