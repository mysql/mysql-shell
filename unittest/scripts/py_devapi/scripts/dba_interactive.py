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
validateMember(members, 'check_instance_configuration');
validateMember(members, 'stop_sandbox_instance');
validateMember(members, 'configure_local_instance');
validateMember(members, 'verbose');
validateMember(members, 'reboot_cluster_from_complete_outage');

#@# Dba: create_cluster errors
c1 = dba.create_cluster()
c1 = dba.create_cluster(1,2,3,4)
c1 = dba.create_cluster(5)
c1 = dba.create_cluster('')
c1 = dba.create_cluster('devCluster', {"invalid":1, "another":2})
c1 = dba.create_cluster('devCluster', {"memberSslMode": "foo"})
c1 = dba.create_cluster('devCluster', {"memberSslMode": ""})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "AUTO"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "REQUIRED"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "memberSslMode": "DISABLED"})
c1 = dba.create_cluster('devCluster', {"adoptFromGR": True, "ipWhitelist": " "})

#@<OUT> Dba: create_cluster with interaction
if __have_ssl:
  c1 = dba.create_cluster('devCluster', {'memberSslMode': 'REQUIRED'})
else:
  c1 = dba.create_cluster('devCluster')

# TODO: add multi-master unit-tests

#@ Dba: check_instance_configuration error
dba.check_instance_configuration('localhost:' + str(__mysql_sandbox_port1));

#@<OUT> Dba: check_instance_configuration ok 1
dba.check_instance_configuration('localhost:' + str(__mysql_sandbox_port2));

#@<OUT> Dba: check_instance_configuration ok 2
dba.check_instance_configuration('localhost:' + str(__mysql_sandbox_port2), {'password':'root'});

#@<OUT> Dba: check_instance_configuration report with errors
uri2 = 'localhost:' + str(__mysql_sandbox_port2);
res = dba.check_instance_configuration(uri2, {'mycnfPath':'mybad.cnf'});

#@ Dba: configure_local_instance error 1
dba.configure_local_instance('someotherhost:' + str(__mysql_sandbox_port1));

#@<OUT> Dba: configure_local_instance error 2
dba.configure_local_instance('localhost:' + str(__mysql_port));

#@<OUT> Dba: configure_local_instance error 3
dba.configure_local_instance('localhost:' + str(__mysql_sandbox_port1));

#@ Dba: Create user without all necessary privileges
# create user that has all permissions to admin a cluster but doesn't have
# the grant privileges for them, so it cannot be used to create viable accounts
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER

connect_to_sandbox([__mysql_sandbox_port2])
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("CREATE USER missingprivileges@localhost")
session.run_sql("GRANT SUPER, CREATE USER ON *.* TO missingprivileges@localhost")
session.run_sql("GRANT SELECT ON `performance_schema`.* TO missingprivileges@localhost WITH GRANT OPTION")
session.run_sql("SET SQL_LOG_BIN=1")
result = session.run_sql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'")
row = result.fetch_one()
print("Number of accounts: " + str(row[0]))
session.close()


#@ Dba: configure_local_instance not enough privileges 1
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
dba.configure_local_instance('missingprivileges@localhost:' + str(__mysql_sandbox_port2))

#@ Dba: configure_local_instance not enough privileges 2
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
dba.configure_local_instance('missingprivileges@localhost:' + str(__mysql_sandbox_port2),
                             {"clusterAdmin": "missingprivileges", "clusterAdminPassword":""})

#@ Dba: configure_local_instance not enough privileges 3
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
dba.configure_local_instance('missingprivileges@localhost:' + str(__mysql_sandbox_port2))

#@ Dba: Show list of users to make sure the user missingprivileges@% was not created
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
connect_to_sandbox([__mysql_sandbox_port2])
result = session.run_sql("select COUNT(*) from mysql.user where user='missingprivileges' and host='%'")
row = result.fetch_one()
print("Number of accounts: " + str(row[0]))

#@ Dba: Delete created user and reconnect to previous sandbox
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("DROP USER missingprivileges@localhost")
session.run_sql("SET SQL_LOG_BIN=1")
result = session.run_sql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'")
row = result.fetch_one()
print("Number of accounts: " + str(row[0]))
session.close()
connect_to_sandbox([__mysql_sandbox_port1])

#@<OUT> Dba: configure_local_instance updating config file
dba.configure_local_instance('localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf'})

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
