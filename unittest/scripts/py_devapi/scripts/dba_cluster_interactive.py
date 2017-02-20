# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

#@<OUT> Cluster: get_cluster with interaction
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

#@ Cluster: validating members
print "Cluster Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'admin_type')
validateMember(members, 'get_admin_type')
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

#@ Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5, 5)
cluster.add_instance('', 5)
cluster.add_instance({'host': 'localhost', 'schema': 'abs', 'user': 'sample', 'authMethod': 56, "memberSslMode": "foo", "ipWhitelist": " "})
cluster.add_instance({'port': __mysql_sandbox_port1})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, "root")
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"memberSslMode": "foo", "password": "root"})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"memberSslMode": "", "password": "root"})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"ipWhitelist": " ", "password": "root"})
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"label": "", "password": "root"});
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"label": "#invalid", "password": "root"});
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"label": "invalid#char", "password": "root"});
cluster.add_instance({"dbUser": "root", "host": "localhost", "port":__mysql_sandbox_port2}, {"label": "over256chars_1234567890123456789012345678990123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123", "password": "root"});

#@ Cluster: add_instance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1
cluster.add_instance(add_instance_options, add_instance_extra_opts)

#@<OUT> Cluster: add_instance with interaction, ok
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

wait_slave_state(cluster, uri2, "ONLINE")

#@<OUT> Cluster: add_instance 3 with interaction, ok
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

wait_slave_state(cluster, uri3, "ONLINE")

#@<OUT> Cluster: describe1
cluster.describe()

#@<OUT> Cluster: status1
cluster.status()

#@ Cluster: remove_instance errors
cluster.remove_instance()
cluster.remove_instance(1,2,3)
cluster.remove_instance(1)
cluster.remove_instance({'host': 'localhost', 'port': 33060})
cluster.remove_instance({'host': 'localhost', 'port': 33060, 'schema': 'abs', 'user': 'sample', 'authMethod': 56})
cluster.remove_instance("somehost:3306")

#@ Cluster: remove_instance
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2})

#@<OUT> Cluster: describe2
cluster.describe()

#@<OUT> Cluster: status2
cluster.status()

#@<OUT> Cluster: dissolve error: not empty
cluster.dissolve()

#@ Cluster: dissolve errors
cluster.dissolve(1)
cluster.dissolve(1,2)
cluster.dissolve("")
cluster.dissolve({'enforce': True})
cluster.dissolve({'force': 'sample'})

#@ Cluster: remove_instance 3
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port3})

#@<OUT> Cluster: add_instance with interaction, ok 3
add_instance_to_cluster(cluster, __mysql_sandbox_port2, 'second_sandbox')

wait_slave_state(cluster, 'second_sandbox', "ONLINE")

#@<OUT> Cluster: add_instance with interaction, ok 4
add_instance_to_cluster(cluster, __mysql_sandbox_port3, 'third_sandbox')

wait_slave_state(cluster, 'third_sandbox', "ONLINE")

#@<OUT> Cluster: status: success
cluster.status()

# Rejoin tests

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

wait_slave_state(cluster, 'third_sandbox', ["UNREACHABLE", "OFFLINE"])

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@: Cluster: rejoin_instance errors
cluster.rejoin_instance()
cluster.rejoin_instance(1, 2, 3)
cluster.rejoin_instance(1)
cluster.rejoin_instance({"host": "localhost"})
cluster.rejoin_instance({"host": "localhost", "schema": 'abs', "authMethod":56, "memberSslMode": "foo", "ipWhitelist": " "})
cluster.rejoin_instance("somehost:3306")
cluster.rejoin_instance("somehost:3306", "root")
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port": __mysql_sandbox_port3}, {"memberSslMode": "foo", "password": "root"})
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port": __mysql_sandbox_port3}, {"memberSslMode": "", "password": "root"})
cluster.rejoin_instance({"dbUser": "root", "host": "localhost", "port": __mysql_sandbox_port3}, {"ipWhitelist": " ", "password": "root"})

#@<OUT> Cluster: rejoin_instance with interaction, ok
if __have_ssl:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, {'memberSslMode': 'AUTO', 'password': 'root'})
else:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, {'password': 'root'})

wait_slave_state(cluster, 'third_sandbox', "ONLINE")

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()


#@<OUT> Cluster: final dissolve
cluster.dissolve({'force': True})

#@ Cluster: no operations can be done on a dissolved cluster errors
cluster.name
cluster.admin_type
cluster.add_instance()
cluster.check_instance_state()
cluster.describe()
cluster.dissolve()
cluster.force_quorum_using_partition_of()
cluster.get_admin_type()
cluster.get_name()
cluster.rejoin_instance()
cluster.remove_instance()
cluster.rescan()
cluster.status()