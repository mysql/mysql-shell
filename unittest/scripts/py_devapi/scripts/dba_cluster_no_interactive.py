import time

# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

#@ Cluster: validating members
cluster = dba.get_cluster('devCluster')

desc = cluster.describe();
localhost = desc.defaultReplicaSet.instances[0].name.split(':')[0];


all_members = dir(cluster)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Cluster Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'admin_type')
validateMember(members, 'get_admin_type')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'rejoin_instance');
validateMember(members, 'check_instance_state');
validateMember(members, 'describe');
validateMember(members, 'status');
validateMember(members, 'help');
validateMember(members, 'dissolve');
validateMember(members, 'rescan');

#@# Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5,5)
cluster.add_instance('',5)
cluster.add_instance({"user":"sample", "weird":1},5)
cluster.add_instance({'host': 'localhost', 'schema': 'abs', 'user':"sample", 'authMethod':56})
cluster.add_instance({'port': __mysql_sandbox_port1})
cluster.add_instance({'host': 'localhost', 'port':__mysql_sandbox_port1}, 'root')

uri1 = "%s:%s" % (localhost, __mysql_sandbox_port1)
uri2 = "%s:%s" % (localhost, __mysql_sandbox_port2)
uri3 = "%s:%s" % (localhost, __mysql_sandbox_port3)

#@ Cluster: add_instance 2
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, "root");

check_slave_online(cluster, uri1, uri2);

#@ Cluster: add_instance 3
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, "root");

check_slave_online(cluster, uri1, uri3);

#@<OUT> Cluster: describe cluster with instance
cluster.describe()

#@<OUT> Cluster: status cluster with instance
cluster.status()

#@ Cluster: remove_instance errors
cluster.remove_instance();
cluster.remove_instance(1,2);
cluster.remove_instance(1);
cluster.remove_instance({"host": "localhost"});
cluster.remove_instance({"host": "localhost", "schema": 'abs', "user":"sample", "authMethod":56});
cluster.remove_instance("somehost:3306");

#@ Cluster: remove_instance read only
cluster.remove_instance({"host": "localhost", "port":__mysql_sandbox_port2})

#@<OUT> Cluster: describe removed read only member
cluster.describe()

#@<OUT> Cluster: status removed read only member
cluster.status()

#@ Cluster: addInstance read only back
cluster.add_instance("root@localhost:%s" % __mysql_sandbox_port2, "root")
check_slave_online(cluster, uri1, uri2);

#@<OUT> Cluster: describe after adding read only instance back
cluster.describe()

#@<OUT> Cluster: status after adding read only instance back
cluster.status()

#@ Cluster: remove_instance master

check_slave_online(cluster, uri1, uri2)

cluster.remove_instance(uri1)

#@ Connecting to new master
from mysqlsh import mysql
customSession = mysql.get_classic_session({"host":localhost, "port":__mysql_sandbox_port2, "user":'root', "password": 'root'})
dba.reset_session(customSession)
cluster = dba.get_cluster()

#@<OUT> Cluster: describe on new master
cluster.describe()

#@<OUT> Cluster: status on new master
cluster.status()

#@ Cluster: addInstance adding old master as read only
uri = "root@localhost:%s" % __mysql_sandbox_port1;
cluster.add_instance(uri, "root");

#@<OUT> Cluster: describe on new master with slave
cluster.describe()

#@<OUT> Cluster: status on new master with slave
cluster.status()

# Rejoin tests

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

# XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
# But won't be available for the GA release. So we need a sleep here
time.sleep(2)

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

check_slave_offline(Cluster, uri2, uri3);

#@# Cluster: rejoin_instance errors
cluster.rejoin_instance();
cluster.rejoin_instance(1,2,3);
cluster.rejoin_instance(1);
cluster.rejoin_instance({"host": "localhost"});
cluster.rejoin_instance({"host": "localhost", "schema": 'abs', "authMethod":56});
cluster.rejoin_instance("somehost:3306");

#@#: Dba: rejoin instance 3 ok
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, "root");

check_slave_online(cluster, uri2, uri3);

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()


# test the lost of quorum

#@# Dba: kill instance 1
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port1, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port1)

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

# GR needs to detect the loss of quorum
time.sleep(5)

#@# Dba: start instance 1
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port1, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port1)

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

# Verify the cluster status after loosing 2 members
#@<OUT> Cluster: status for rejoin quorum lost
cluster.status()

#@#: Dba: rejoin instance 3
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, "root");

#@ Cluster: dissolve: error quorum
cluster.dissolve()

# In order to be able to run dissolve, we must force the reconfiguration of the group using
# group_replication_force_members
customSession.run_sql("SET GLOBAL group_replication_force_members = 'localhost:1" + str(__mysql_sandbox_port2) + "'");
time.sleep(5)

# Add back the instances to make the dissolve possible

#@ Cluster: rejoinInstance 1
uri = "root@localhost:%s" % __mysql_sandbox_port1;
cluster.rejoin_instance(uri, "root");
check_slave_online(cluster, uri2, uri1)

#@ Cluster: rejoinInstance 3
uri = "root@localhost:%s" % __mysql_sandbox_port3;
cluster.rejoin_instance(uri, "root");
check_slave_online(cluster, uri2, uri3)

# Verify the cluster status after rejoining the 2 members
#@<OUT> Cluster: status for rejoin successful
cluster.status()

#@ Cluster: dissolve errors
cluster.dissolve()
cluster.dissolve(1)
cluster.dissolve(1,2)
cluster.dissolve("")
cluster.dissolve({'foobar': True})
cluster.dissolve({'force': "whatever"})

# We cannot test the output of dissolve because it will crash the rejoined instance, hitting the bug:
# BUG#24818604: MYSQLD CRASHES WHILE STARTING GROUP REPLICATION FOR A NODE IN RECOVERY PROCESS
# As soon as the bug is fixed, dissolve will work fine and we can remove the above workaround to do a clean-up

#cluster.dissolve({force: true})
#cluster.describe()
#cluster.status()

customSession.close();
