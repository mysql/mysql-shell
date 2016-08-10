# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

#@ Initialization
dba.drop_metadata_schema()

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

#@# Dba: create_farm errors
farm = dba.create_farm()
farm = dba.create_farm(5)
farm = dba.create_farm('')

#@# Dba: create_farm with interaction
farm = dba.create_farm('devFarm')
print farm

#@# Dba: get_farm errors
farm = dba.get_farm()
farm = dba.get_farm(5)
farm = dba.get_farm('', 5)
farm = dba.get_farm('')
farm = dba.get_farm('devFarm')

#@ Dba: get_farm
print farm

#@# Dba: drop_farm errors
# Need a node to reproduce the not empty error
farm.add_seed_instance('192.168.1.1:33060')
farm = dba.drop_farm()
farm = dba.drop_farm(5)
farm = dba.drop_farm('')
farm = dba.drop_farm('sample', 5)
farm = dba.drop_farm('sample', {}, 5)

#@ Dba: drop_farm interaction no options, cancel
farm = dba.drop_farm('sample')

#@ Dba: drop_farm interaction missing option, ok error
farm = dba.drop_farm('sample', {})

#@ Dba: drop_farm interaction no options, ok success
farm = dba.drop_farm('devFarm')
