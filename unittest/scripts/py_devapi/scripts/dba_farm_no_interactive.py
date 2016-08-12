# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.get_farm({"enforce":True})
farmPassword = 'testing'
#@ Farm: validating members
farm = dba.create_farm('devFarm', farmPassword)

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
farm.add_seed_instance(farmPassword, {'host': __host, 'port':__mysql_port}, __pwd)

#@# Farm: add_instance errors
farm.add_instance()
farm.add_instance(5,6,7,1)
farm.add_instance(5,5)
farm.add_instance('',5)
farm.add_instance(farmPassword, 5)
farm.add_instance(farmPassword, {'host': __host, 'schema': 'abs', 'user':"sample", 'authMethod':56})
farm.add_instance(farmPassword, {"port": __port})
farm.add_instance(farmPassword, {'host': __host, 'port':__mysql_port}, __pwd)

# Cleanup
dba.drop_farm('devFarm', {"dropDefaultReplicaSet": True})
