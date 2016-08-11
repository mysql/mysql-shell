# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.drop_metadata_schema({"enforce":True})

#@ Session: validating members
all_members = dir(dba)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Session Members: %d" % len(members)
validateMember(members, 'default_farm')
validateMember(members, 'get_default_farm')
validateMember(members, 'create_farm')
validateMember(members, 'drop_farm')
validateMember(members, 'get_farm')
validateMember(members, 'drop_metadata_schema')
validateMember(members, 'reset_session')
validateMember(members, 'validate_instance')
validateMember(members, 'deploy_local_instance')

#@# Dba: create_farm errors
farm = dba.create_farm()
farm = dba.create_farm(5)
farm = dba.create_farm('', 5)
farm = dba.create_farm('devFarm')
farm = dba.create_farm('devFarm', 'password')
farm = dba.create_farm('devFarm', 'password')

#@ Dba: create_farm
print farm

#@# Dba: get_farm errors
farm = dba.get_farm()
farm = dba.get_farm(5)
farm = dba.get_farm('', 5)
farm = dba.get_farm('')
farm = dba.get_farm('devFarm')

#@ Dba: get_farm
print farm

#@ Dba: add_seed_instance
farm.add_seed_instance({'host': __host, 'port':__mysql_port}, __pwd)

#@# Dba: drop_farm errors
farm = dba.drop_farm()
farm = dba.drop_farm(5)
farm = dba.drop_farm('')
farm = dba.drop_farm('sample', 5)
farm = dba.drop_farm('sample', {}, 5)
farm = dba.drop_farm('sample')
farm = dba.drop_farm('devFarm')

#@ Dba: drop_farm
dba.drop_farm('devFarm', {"dropDefaultReplicaSet": True})
