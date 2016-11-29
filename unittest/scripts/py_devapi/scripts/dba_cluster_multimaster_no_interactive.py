# Assumptions: check_slave_online_multimaster and check_slave_offline_multimaster
# are defined

#@ Dba: create_cluster multiMaster, ok
if __have_ssl:
  dba.create_cluster('devCluster', {'multiMaster': True, 'force': True});
else:
  dba.create_cluster('devCluster', {'multiMaster': True, 'force': True, 'ssl': False});

cluster = dba.get_cluster('devCluster');

uri1 = "%s:%s" % ("localhost", __mysql_sandbox_port1)
uri2 = "%s:%s" % ("localhost", __mysql_sandbox_port2)
uri3 = "%s:%s" % ("localhost", __mysql_sandbox_port3)

#@ Cluster: add_instance 2
cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2}, 'root');

check_slave_online_multimaster(Cluster, uri2);

#@ Cluster: add_instance 3
cluster.add_instance({'dbUser': "root", 'host': "localhost", 'port': __mysql_sandbox_port3}, 'root');

check_slave_online_multimaster(cluster, uri3);

#@<OUT> Cluster: describe cluster with instance
cluster.describe()

#@<OUT> Cluster: status cluster with instance
cluster.status()

#@ Cluster: remove_instance 2
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2})

#@<OUT> Cluster: describe removed member
cluster.describe()

#@<OUT> Cluster: status removed member
cluster.status()

#@ Cluster: remove_instance 3
cluster.remove_instance(uri3)

#@ Cluster: remove_instance 1
cluster.remove_instance(uri1)

#@<OUT> Cluster: describe
cluster.describe()

#@<OUT> Cluster: status
cluster.status()

#@ Cluster: add_instance 1
cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port' :__mysql_sandbox_port1}, 'root');

#@ Cluster: add_instance 2
cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2}, 'root');

check_slave_online_multimaster(cluster, uri2);

#@ Cluster: add_instance 3
cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, 'root');

check_slave_online_multimaster(cluster, uri3);

#@<OUT> Cluster: status: success
cluster.status()

# Rejoin tests

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir': __sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

# XCOM needs time to kick out the member of the group. The GR team has a patch to fix this
# But won't be available for the GA release. So we need to wait until the instance is reported
# as offline
check_slave_offline_multimaster(cluster, uri3);

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir': __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@ Cluster: rejoin_instance errors
cluster.rejoin_instance();
cluster.rejoin_instance(1,2,3);
cluster.rejoin_instance(1);
cluster.rejoin_instance({'host': 'localhost'});
cluster.rejoin_instance({'host': 'localhost', 'schema': 'abs', 'authMethod': 56});
cluster.rejoin_instance("somehost:3306");

#@#: Dba: rejoin instance 3 ok
cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3}, 'root');

check_slave_online_multimaster(cluster, uri3);

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()
