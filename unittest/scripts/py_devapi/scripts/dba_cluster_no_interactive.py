# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

import time

#@ Cluster: validating members
cluster = dba.get_cluster('devCluster')

# session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})
session.close()

desc = cluster.describe()
localhost = desc.defaultReplicaSet.instances[0].label.split(':')[0]


all_members = dir(cluster)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Cluster Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'rejoin_instance')
validateMember(members, 'check_instance_state')
validateMember(members, 'describe')
validateMember(members, 'status')
validateMember(members, 'help')
validateMember(members, 'dissolve')
validateMember(members, 'rescan')
validateMember(members, 'force_quorum_using_partition_of')

#@# Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5,5)
cluster.add_instance('',5)
cluster.add_instance({"user":"sample", "weird":1},{})
cluster.add_instance({'host': 'localhost', 'schema': 'abs', 'user':"sample", 'authMethod': 56, "memberSslMode": "foo", "ipWhitelist": " "})
cluster.add_instance({'port': __mysql_sandbox_port1})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, "root")
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"memberSslMode": "foo", "password": "root"})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"memberSslMode": "", "password": "root"})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"ipWhitelist": " ", "password": "root"})

add_instance_options['port'] = __mysql_sandbox_port1
cluster.add_instance(add_instance_options, add_instance_extra_opts)

uri1 = "%s:%s" % (localhost, __mysql_sandbox_port1)
uri2 = "%s:%s" % (localhost, __mysql_sandbox_port2)
uri3 = "%s:%s" % (localhost, __mysql_sandbox_port3)

#@ Cluster: add_instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2, 'second')

# Third instance will be added while the second is still on RECOVERY
#@ Cluster: add_instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, 'second', "ONLINE")
wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: describe cluster with instance
cluster.describe()

#@<OUT> Cluster: status cluster with instance
cluster.status()

#@ Cluster: remove_instance errors
cluster.remove_instance()
cluster.remove_instance(1,2,3)
cluster.remove_instance(1)
cluster.remove_instance({"host": "localhost"})
cluster.remove_instance({"host": "localhost", "schema": 'abs', "user": "sample", "fakeOption": 56})
cluster.remove_instance("somehost:3306")
cluster.remove_instance("second")

#@ Cluster: remove_instance read only
cluster.remove_instance({"host": "localhost", "port":__mysql_sandbox_port2})

#@<OUT> Cluster: describe removed read only member
cluster.describe()

#@<OUT> Cluster: status removed read only member
cluster.status()

#@ Cluster: addInstance read only back
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(cluster, uri2, "ONLINE")

#@<OUT> Cluster: describe after adding read only instance back
cluster.describe()

#@<OUT> Cluster: status after adding read only instance back
cluster.status()

# Make sure uri2 is selected as the new master
cluster.remove_instance(uri3)

#@ Cluster: remove_instance master
cluster.remove_instance(uri1)

#@ Connecting to new master
from mysqlsh import mysql
customSession = mysql.get_classic_session({"host": localhost, "port": __mysql_sandbox_port2, "user": 'root', "password": 'root'})
dba.reset_session(customSession)
cluster = dba.get_cluster()

# Add back uri3
add_instance_to_cluster(cluster, __mysql_sandbox_port3, 'third_sandbox')

wait_slave_state(cluster, 'third_sandbox', "ONLINE")

#@<OUT> Cluster: describe on new master
cluster.describe()

#@<OUT> Cluster: status on new master
cluster.status()

#@ Cluster: addInstance adding old master as read only
add_instance_to_cluster(cluster, __mysql_sandbox_port1, 'first_sandbox')

wait_slave_state(cluster, 'first_sandbox', "ONLINE")


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

# Since the cluster has quorum, the instance will be kicked off the
# Cluster going OFFLINE->UNREACHABLE->(MISSING)
wait_slave_state(cluster, 'third_sandbox', "(MISSING)")

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@# Cluster: rejoin_instance errors
cluster.rejoin_instance()
cluster.rejoin_instance(1,2,3)
cluster.rejoin_instance(1)
cluster.rejoin_instance({"host": "localhost"})
cluster.rejoin_instance({"host": "localhost", "schema": 'abs', "authMethod":56, "memberSslMode": "foo", "ipWhitelist": " "})
cluster.rejoin_instance("somehost:3306")
cluster.rejoin_instance("somehost:3306", "root")
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, {"memberSslMode": "foo", "password": "root"})
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, {"memberSslMode": "", "password": "root"})
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, {"ipWhitelist": " ", "password": "root"})

#@#: Dba: rejoin instance 3 ok
if __have_ssl:
  cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, {"memberSslMode": "AUTO", "password":  "root"})
else:
  cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port3}, {"password": "root"})

wait_slave_state(cluster, 'third_sandbox', "ONLINE")

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()

#@ Cluster: dissolve errors
cluster.dissolve()
cluster.dissolve(1)
cluster.dissolve(1, 2)
cluster.dissolve("")
cluster.dissolve({'foobar': True})
cluster.dissolve({'force': "whatever"})


#@ Cluster: final dissolve
cluster.dissolve({'force': True})

#@ Cluster: no operations can be done on a dissolved cluster errors
cluster.name
cluster.add_instance()
cluster.check_instance_state()
cluster.describe()
cluster.dissolve()
cluster.force_quorum_using_partition_of()
cluster.get_name()
cluster.rejoin_instance()
cluster.remove_instance()
cluster.rescan()
cluster.status()

customSession.close()

