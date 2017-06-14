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

#@ Dba: createCluster with ANSI_QUOTES success
# save current sql mode
result = session.run_sql("SELECT @@GLOBAL.SQL_MODE")
row = result.fetch_one()
original_sql_mode = row[0]
session.run_sql("SET @@GLOBAL.SQL_MODE = ANSI_QUOTES")
# Check that sql mode has been changed
result = session.run_sql("SELECT @@GLOBAL.SQL_MODE")
row = result.fetch_one()
print("Current sql_mode is: " + row[0] + "\n")

if __have_ssl:
    c1 = dba.create_cluster('devCluster', {'memberSslMode': 'REQUIRED'})
else:
    c1 = dba.create_cluster('devCluster')

print c1

#@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
c1.dissolve({"force": True})

# Set old_sql_mode
session.run_sql("SET @@GLOBAL.SQL_MODE = '"+ original_sql_mode+ "'")
result = session.run_sql("SELECT @@GLOBAL.SQL_MODE")
row = result.fetch_one()
restored_sql_mode = row[0]
was_restored = restored_sql_mode == original_sql_mode
print("Original SQL_MODE has been restored: " + str(was_restored) + "\n")

#@ Dba: create_cluster success
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

#@<OUT> Dba: configure_local_instance updating config file
dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});

#@ Dba: configure_local_instance report fixed 1
result = dba.configure_local_instance(uri2, {'mycnfPath':'mybad.cnf'});
print (result.status)

#@ Dba: configure_local_instance report fixed 2
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf', 'password':'root'});
print (result.status)

#@ Dba: configure_local_instance report fixed 3
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port2), {'mycnfPath':'mybad.cnf', 'dbPassword':'root'});
print (result.status)

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

#@ Dba: configure_local_instance not enough privileges
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
dba.configure_local_instance('missingprivileges:@localhost:' + str(__mysql_sandbox_port2),
                             {"clusterAdmin": "missingprivileges", "clusterAdminPassword":"",
                              "mycnfPath":__output_sandbox_dir + str(__mysql_sandbox_port2) + __path_splitter + 'my.cnf'})

#@ Dba: Show list of users to make sure the user missingprivileges@% was not created
# Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
# PERMISSIONS, CREATES A WRONG NEW USER
connect_to_sandbox([__mysql_sandbox_port2])
result = session.run_sql("select COUNT(*) from mysql.user where user='missingprivileges' and host='%'")
row = result.fetch_one()
print("Number of accounts: " + str(row[0]))
session.close()

#@ Dba: Delete created user and reconnect to previous sandbox
connect_to_sandbox([__mysql_sandbox_port2])
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("DROP USER missingprivileges@localhost")
session.run_sql("SET SQL_LOG_BIN=1")
result = session.run_sql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'")
row = result.fetch_one()
print("Number of accounts: " + str(row[0]))
session.close()

#@ Dba: create an admin user with all needed privileges
# Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
connect_to_sandbox([__mysql_sandbox_port2])
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("CREATE USER 'mydba'@'localhost' IDENTIFIED BY ''")
session.run_sql("GRANT ALL ON *.* TO 'mydba'@'localhost' WITH GRANT OPTION")
session.run_sql("SET SQL_LOG_BIN=1")
result = session.run_sql("SELECT COUNT(*) FROM mysql.user WHERE user='mydba' and host='localhost'")
row = result.fetch_one()
print("Number of 'mydba'@'localhost' accounts: "+ str(row[0]))
session.close()

#@<OUT> Dba: configureLocalInstance create different admin user
# Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configure_local_instance('mydba:@localhost:' + str(__mysql_sandbox_port2),
    {"clusterAdmin": "dba_test", "clusterAdminPassword":"",
     "mycnfPath":__output_sandbox_dir + str(__mysql_sandbox_port2) + __path_splitter + 'my.cnf'})

#@<OUT> Dba: configureLocalInstance create existing valid admin user
# Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configure_local_instance('mydba:@localhost:' + str(__mysql_sandbox_port2),
    {"clusterAdmin": "dba_test", "clusterAdminPassword":"",
     "mycnfPath":__output_sandbox_dir + str(__mysql_sandbox_port2) + __path_splitter + 'my.cnf'})

#@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
connect_to_sandbox([__mysql_sandbox_port2])
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("REVOKE REPLICATION SLAVE ON *.* FROM 'dba_test'@'%'")
session.run_sql("SET SQL_LOG_BIN=1")
session.close()

#@<OUT> Dba: configureLocalInstance create existing invalid admin user
# Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configure_local_instance('mydba:@localhost:' + str(__mysql_sandbox_port2),
    {"clusterAdmin": "dba_test", "clusterAdminPassword":"",
     "mycnfPath":__output_sandbox_dir + str(__mysql_sandbox_port2) + __path_splitter + 'my.cnf'})

#@ Dba: Delete previously create an admin user with all needed privileges
# Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
connect_to_sandbox([__mysql_sandbox_port2])
session.run_sql("SET SQL_LOG_BIN=0")
session.run_sql("DROP USER 'mydba'@'localhost'")
session.run_sql("DROP USER 'dba_test'@'%'")
session.run_sql("SET SQL_LOG_BIN=1")
result = session.run_sql("SELECT COUNT(*) FROM mysql.user WHERE user='mydba' and host='localhost'")
row = result.fetch_one()
print("Number of 'mydba'@'localhost' accounts: "+ str(row[0]))
session.close()

connect_to_sandbox([__mysql_sandbox_port1])

#@# Dba: get_cluster errors
c2 = dba.get_cluster()
c2 = dba.get_cluster(5)
c2 = dba.get_cluster('', 5)
c2 = dba.get_cluster('')
c2 = dba.get_cluster('devCluster')
c2 = dba.get_cluster('devCluster');

#@ Dba: get_cluster
print c2
