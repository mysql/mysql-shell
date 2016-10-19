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
validateMember(members, 'configure_local_instance');
validateMember(members, 'verbose');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('', 5)
c1 = dba.create_cluster('devCluster', 'bla')
c1 = dba.create_cluster('devCluster', {"invalid":1, "another":2})

#@# Dba: create_cluster succeed
c1 = dba.create_cluster('devCluster');
print c1

#@# Dba: create_cluster already exist
c1 = dba.create_cluster('devCluster');

#@# Dba: validate_instance errors
dba.validate_instance('localhost:' + str(__mysql_sandbox_port1));
dba.validate_instance('sample@localhost:' + str(__mysql_sandbox_port1));
result = dba.validate_instance('root:root@localhost:' + str(__mysql_sandbox_port1));

#@ Dba: validate_instance ok1
uri2 = 'root:root@localhost:' + str(__mysql_sandbox_port2);
result = dba.validate_instance(uri2);
print (result.status)

#@ Dba: validate_instance ok2
result = dba.validate_instance('root@localhost:' + str(__mysql_sandbox_port2), {'password':'root'});
print (result.status)

#@ Dba: validate_instance ok3
result = dba.validate_instance('root@localhost:' + str(__mysql_sandbox_port2), {'dbPassword':'root'});
print (result.status)

#@<OUT> Dba: validate_instance report with errors
dba.validate_instance(uri2, {'mycnfPath':'mybad.cnf'});

#@# Dba: configure_local_instance errors
dba.configure_local_instance('someotherhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('localhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('sample@localhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port1), {'password':'toor'});
dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port1), {'password':'toor', 'mycnfPath':'mybad.cnf'});

#@<OUT> Dba: configure_local_instance updating config file
dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});

#@<OUT> Dba: configure_local_instance report fixed 1
result = dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});
print (result.status)

#@<OUT> Dba: configure_local_instance report fixed 1
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf', 'password':'root'});
print (result.status)

#@<OUT> Dba: configure_local_instance report fixed 1
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf', 'dbPassword':'root'});
print (result.status)

#@# Dba: get_cluster errors
c2 = dba.get_cluster()
c2 = dba.get_cluster(5)
c2 = dba.get_cluster('', 5)
c2 = dba.get_cluster('')
c2 = dba.get_cluster('devCluster')
c2 = dba.get_cluster('devCluster');

#@ Dba: get_cluster
print c2
