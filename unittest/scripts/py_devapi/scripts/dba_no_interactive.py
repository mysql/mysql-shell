# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMember and validateNotMember are defined on the setup script
dba.drop_metadata_schema({"enforce":True})

#@ Session: validating members
all_members = dir(dba)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Session Members: %d" % len(members)
validateMember(members, 'create_cluster');
validateMember(members, 'delete_sandbox_instance');
validateMember(members, 'deploy_sandbox_instance');
validateMember(members, 'drop_metadata_schema');
validateMember(members, 'get_cluster');
validateMember(members, 'help');
validateMember(members, 'kill_sandbox_instance');
validateMember(members, 'reset_session');
validateMember(members, 'start_sandbox_instance');
validateMember(members, 'validate_instance');
validateMember(members, 'stop_sandbox_instance');
validateMember(members, 'prepare_instance');
validateMember(members, 'verbose');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('', 5)
c1 = dba.create_cluster('devCluster')

#@# Dba: create_cluster succeed
c1 = dba.create_cluster('devCluster');
print c1

#@# Dba: create_cluster already exist
c1 = dba.create_cluster('devCluster');

#@# Dba: get_cluster errors
c2 = dba.get_cluster()
c2 = dba.get_cluster(5)
c2 = dba.get_cluster('', 5)
c2 = dba.get_cluster('')
c2 = dba.get_cluster('devCluster')
c2 = dba.get_cluster('devCluster');

#@ Dba: get_cluster
print c2
