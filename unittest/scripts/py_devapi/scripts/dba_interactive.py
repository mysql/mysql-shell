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
validateMember(members, 'delete_sandbox_instance');
validateMember(members, 'deploy_sandbox_instance');
validateMember(members, 'drop_cluster');
validateMember(members, 'drop_metadata_schema');
validateMember(members, 'get_cluster');
validateMember(members, 'help');
validateMember(members, 'kill_sandbox_instance');
validateMember(members, 'reset_session');
validateMember(members, 'start_sandbox_instance');
validateMember(members, 'check_instance_config');
validateMember(members, 'stop_sandbox_instance');
validateMember(members, 'config_local_instance');
validateMember(members, 'verbose');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(1,2,3,4)
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('')
c1 = dba.create_cluster('devCluster', {"invalid":1, "another":2})

#@<OUT> Dba: create_cluster with interaction
cluster = dba.create_cluster('devCluster')

# TODO: add multi-master unit-tests

#@ Dba: check_instance_config error
dba.check_instance_config('localhost:' + str(__mysql_sandbox_port1));

#@<OUT> Dba: check_instance_config ok 1
dba.check_instance_config('localhost:' + str(__mysql_sandbox_port2));

#@<OUT> Dba: check_instance_config ok 2
dba.check_instance_config('localhost:' + str(__mysql_sandbox_port2), {'password':'root'});

#@<OUT> Dba: check_instance_config report with errors
uri2 = 'localhost:' + str(__mysql_sandbox_port2);
res = dba.check_instance_config(uri2, {'mycnfPath':'mybad.cnf'});

#@ Dba: config_local_instance error 1
dba.config_local_instance('someotherhost:' + str(__mysql_sandbox_port1));

#@<OUT> Dba: config_local_instance error 2
dba.config_local_instance('localhost:' + str(__mysql_port));

#@<OUT> Dba: config_local_instance error 3
dba.config_local_instance('localhost:' + str(__mysql_sandbox_port1));

#@<OUT> Dba: config_local_instance updating config file
dba.config_local_instance('localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf'});

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
