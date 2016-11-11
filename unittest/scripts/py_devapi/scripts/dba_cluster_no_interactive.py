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

#@ Cluster: add_instance
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, "root");

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
cluster.remove_instance({"host":localhost, "port":__mysql_sandbox_port2})

#@<OUT> Cluster: describe removed read only member
cluster.describe()

#@<OUT> Cluster: status removed read only member
cluster.status()

#@ Cluster: addInstance read only back
cluster.add_instance("root@localhost:%s" % __mysql_sandbox_port2, "root")

#@<OUT> Cluster: describe after adding read only instance back
cluster.describe()

#@<OUT> Cluster: status after adding read only instance back
cluster.status()

#@ Cluster: remove_instance master
uri1 = "%s:%s" % (localhost, __mysql_sandbox_port1)
uri2 = "%s:%s" % (localhost, __mysql_sandbox_port2)
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

#@ Cluster: dissolve errors
cluster.dissolve()
cluster.dissolve(1)
cluster.dissolve(1,2)
cluster.dissolve("")
cluster.dissolve({'enforce': True})
cluster.dissolve({'force': "whatever"})

#@ Cluster: dissolve
cluster.dissolve({'force': True})

#@ Cluster: describe: dissolved cluster
cluster.describe()

#@ Cluster: status: dissolved cluster
cluster.status()
