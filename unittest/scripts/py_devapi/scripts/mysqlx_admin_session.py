# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
import mysqlx

myAdmin = mysqlx.get_admin_session(__uripwd)

#@ Session: validating members
all_members = dir(myAdmin)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Session Members:", len(members)
validateMember(members, 'uri')
validateMember(members, 'default_farm')
validateMember(members, 'get_uri')
validateMember(members, 'get_default_farm')
validateMember(members, 'is_open')
validateMember(members, 'create_farm')
validateMember(members, 'drop_farm')
validateMember(members, 'get_farm')
validateMember(members, 'close')

#@# AdminSession: create_farm errors
farm = myAdmin.create_farm();
farm = myAdmin.create_farm(5);
farm = myAdmin.create_farm('', 5);
farm = myAdmin.create_farm('');
farm = myAdmin.create_farm('devFarm');
farm = myAdmin.create_farm('devFarm');

#@ AdminSession: create_farm
print farm

#@# AdminSession: get_farm errors
farm = myAdmin.get_farm()
farm = myAdmin.get_farm(5)
farm = myAdmin.get_farm('', 5)
farm = myAdmin.get_farm('')
farm = myAdmin.get_farm('devFarm')

#@ AdminSession: get_farm
print farm

#@# AdminSession: drop_farm errors
# Need a node to reproduce the not empty error
farm.add_seed_instance('192.168.1.1:33060')
farm = myAdmin.drop_farm()
farm = myAdmin.drop_farm(5)
farm = myAdmin.drop_farm('')
farm = myAdmin.drop_farm('sample', 5)
farm = myAdmin.drop_farm('sample', {}, 5)
farm = myAdmin.drop_farm('sample')
farm = myAdmin.drop_farm('devFarm')

#@ AdminSession: drop_farm
myAdmin.drop_farm('devFarm', {'dropDefaultReplicaSet': True})

# Cleanup
myAdmin.close()
