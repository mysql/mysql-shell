# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
import mysqlx

#@ Session: validating members
myAdmin = mysqlx.getAdminSession(__uripwd)
all_members = dir(myAdmin)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Session Members:", len(members)
validateMember(members, 'uri')
validateMember(members, 'defaultFarm')
validateMember(members, 'getUri')
validateMember(members, 'getDefaultFarm')
validateMember(members, 'createFarm')
validateMember(members, 'getFarm')
validateMember(members, 'close')

# Cleanup
myAdmin.close()