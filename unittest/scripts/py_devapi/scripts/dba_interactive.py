# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

#@ Session: validating members
all_members = dir(dba)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)


print "Session Members: %d" % len(members)
validateMember(members, 'create_cluster');
validateMember(members, 'delete_local_instance');
validateMember(members, 'deploy_local_instance');
validateMember(members, 'drop_cluster');
validateMember(members, 'drop_metadata_schema');
validateMember(members, 'get_cluster');
validateMember(members, 'help');
validateMember(members, 'kill_local_instance');
validateMember(members, 'reset_session');
validateMember(members, 'start_local_instance');
validateMember(members, 'validate_instance');
validateMember(members, 'stop_local_instance');
validateMember(members, 'verbose');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(1,2,3,4)
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('')

#@<OUT> Dba: create_cluster with interaction
cluster = dba.create_cluster('devCluster')

#@# Dba: get_cluster errors
c2 = dba.get_cluster(5)
c2 = dba.get_cluster('', 5)
c2 = dba.get_cluster('')

#@<OUT> Dba: get_cluster with interaction
c2 = dba.get_cluster('devCluster')
c2

#@<OUT> Dba: get_cluster with interaction (default)
c3 = dba.get_cluster()
c3
