# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

shell.connect({'scheme': 'mysql', 'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@<OUT> create cluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('dev')

cluster.status();

#@ Add instance 2
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

# Waiting for the second added instance to become online
wait_slave_state(cluster, uri2, "ONLINE")

#@ Add instance 3
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

# Waiting for the third added instance to become online
wait_slave_state(cluster, uri3, "ONLINE")

# Kill instance 2
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port2, {'sandboxDir':__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port2)

# Since the cluster has quorum, the instance will be kicked off the
# Cluster going OFFLINE->UNREACHABLE->(MISSING)
wait_slave_state(cluster, uri2, "(MISSING)")

# Kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir':__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

# Waiting for the third added instance to become unreachable
# Will remain unreachable since there's no quorum to kick it off
wait_slave_state(cluster, uri3, "UNREACHABLE")

# Start instance 2
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port2, {'sandboxDir':__sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port2)

# Start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir':__sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@<OUT> Cluster status
cluster.status();

#@ Cluster.force_quorum_using_partition_of errors
cluster.force_quorum_using_partition_of();
cluster.force_quorum_using_partition_of(1);
cluster.force_quorum_using_partition_of("");
cluster.force_quorum_using_partition_of(1, "");
cluster.force_quorum_using_partition_of({'host':localhost, 'port': __mysql_sandbox_port2, 'password':'root'});

#@ Cluster.force_quorum_using_partition_of success
cluster.force_quorum_using_partition_of({'host':localhost, 'port': __mysql_sandbox_port1, 'password':'root'})

#@<OUT> Cluster status after force quorum
cluster.status();

#@ Rejoin instance 2
if __have_ssl:
  cluster.rejoin_instance({'host':localhost, 'port': __mysql_sandbox_port2, 'password':'root'}, {'memberSslMode': 'REQUIRED'})
else:
  cluster.rejoin_instance({'host':localhost, 'port': __mysql_sandbox_port2, 'password':'root'})

# Waiting for the second rejoined instance to become online
wait_slave_state(cluster, uri2, "ONLINE");

#@ Rejoin instance 3
if __have_ssl:
  cluster.rejoin_instance({'host':localhost, 'port': __mysql_sandbox_port3, 'password':'root'}, {'memberSslMode': 'REQUIRED'})
else:
  cluster.rejoin_instance({'host':localhost, 'port': __mysql_sandbox_port3, 'password':'root'})

# Waiting for the third rejoined instance to become online
wait_slave_state(cluster, uri3, "ONLINE");

#@<OUT> Cluster status after rejoins
cluster.status();

session.close();

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
