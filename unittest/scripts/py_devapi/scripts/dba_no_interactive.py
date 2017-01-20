# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMember and validateNotMember are defined on the setup script
dba.drop_metadata_schema({"force":True})

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
validateMember(members, 'check_instance_configuration');
validateMember(members, 'stop_sandbox_instance');
validateMember(members, 'configure_local_instance');
validateMember(members, 'verbose');
validateMember(members, 'reboot_cluster_from_complete_outage');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('', 5)
c1 = dba.create_cluster('devCluster', 'bla')
c1 = dba.create_cluster('devCluster', {"invalid":1, "another":2})
c1 = dba.create_cluster('devCluster', {"memberSslMode": "foo"})
c1 = dba.create_cluster('devCluster', {"memberSslMode": ""})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "AUTO"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "REQUIRED"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "DISABLED"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslKey": "key"})
c1 = dba.create_cluster('devCluster', {"ipWhitelist": " "})

#@# Dba: create_cluster succeed
if __have_ssl:
  c1 = dba.create_cluster('devCluster', {'memberSslMode': 'REQUIRED'})
else:
  c1 = dba.create_cluster('devCluster')

print c1

# TODO: add multi-master unit-tests

#@# Dba: create_cluster already exist
c1 = dba.create_cluster('devCluster');

#@# Dba: check_instance_configuration errors
dba.check_instance_configuration('localhost:' + str(__mysql_sandbox_port1));
dba.check_instance_configuration('sample@localhost:' + str(__mysql_sandbox_port1));
result = dba.check_instance_configuration('root:root@localhost:' + str(__mysql_sandbox_port1));

#@ Dba: check_instance_configuration ok1
uri2 = 'root:root@localhost:' + str(__mysql_sandbox_port2);
result = dba.check_instance_configuration(uri2);
print (result.status)

#@ Dba: check_instance_configuration ok2
result = dba.check_instance_configuration('root@localhost:' + str(__mysql_sandbox_port2), {'password':'root'});
print (result.status)

#@ Dba: check_instance_configuration ok3
result = dba.check_instance_configuration('root@localhost:' + str(__mysql_sandbox_port2), {'dbPassword':'root'});
print (result.status)

#@<OUT> Dba: check_instance_configuration report with errors
dba.check_instance_configuration(uri2, {'mycnfPath':'mybad.cnf'});

#@# Dba: configure_local_instance errors
dba.configure_local_instance('someotherhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('localhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('sample@localhost:' + str(__mysql_sandbox_port1));
dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port1), {'password':'root'});
dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port1), {'password':'root', 'mycnfPath':'mybad.cnf'});

#@<OUT> Dba: configure_local_instance updating config file
dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});

#@<OUT> Dba: configure_local_instance report fixed 1
result = dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});
print (result.status)

#@<OUT> Dba: configure_local_instance report fixed 2
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf', 'password':'root'});
print (result.status)

#@<OUT> Dba: configure_local_instance report fixed 3
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
