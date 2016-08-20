# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.get_farm({"enforce":True})

farmPassword = 'testing'
#@ Farm: validating members
farm = dba.create_farm('devFarm', farmPassword)
farm.add_seed_instance(farmPassword, {'host': __host, 'port':__mysql_port}, __pwd)
rset = farm.get_replica_set()

all_members = dir(rset)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Replica Set Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')

#@# Farm: add_instance errors
rset.add_instance()
rset.add_instance(5,6,7,1)
rset.add_instance(5,5)
rset.add_instance('',5)
rset.add_instance(farmPassword, 5)
rset.add_instance(farmPassword, {'host': __host, 'schema': 'abs', 'user':"sample", 'authMethod':56})
rset.add_instance(farmPassword, {"port": __port})
rset.add_instance(farmPassword, {'host': __host, 'port':__mysql_port}, __pwd)

# Cleanup
dba.drop_farm('devFarm', {"dropDefaultReplicaSet": True})
