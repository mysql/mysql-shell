# Assumptions: New sandboxes deployed with no server id in the config file.
# Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE

#@ Configure instance on port 1.
cnfPath1 = __sandbox_dir + str(__mysql_sandbox_port1) + "/my.cnf"
dba.configure_local_instance("root@localhost:"+str(__mysql_sandbox_port1), {'mycnfPath': cnfPath1, 'dbPassword':'root'})

#@ Configure instance on port 2.
cnfPath2 = __sandbox_dir + str(__mysql_sandbox_port2) + "/my.cnf"
dba.configure_local_instance("root@localhost:"+str(__mysql_sandbox_port2), {'mycnfPath': cnfPath2, 'dbPassword':'root'})

#@ Configure instance on port 3.
cnfPath3 = __sandbox_dir + str(__mysql_sandbox_port3) + "/my.cnf"
dba.configure_local_instance("root@localhost:"+str(__mysql_sandbox_port3), {'mycnfPath': cnfPath3, 'dbPassword':'root'})

#@ Restart instance on port 1 to apply new server id settings.
dba.stop_sandbox_instance(__mysql_sandbox_port1, {'password': 'root', 'sandboxDir': __sandbox_dir})
try_restart_sandbox(__mysql_sandbox_port1)

#@ Restart instance on port 2 to apply new server id settings.
dba.stop_sandbox_instance(__mysql_sandbox_port2, {'password': 'root', 'sandboxDir': __sandbox_dir})
try_restart_sandbox(__mysql_sandbox_port2)

#@ Restart instance on port 3 to apply new server id settings.
dba.stop_sandbox_instance(__mysql_sandbox_port3, {'password': 'root', 'sandboxDir': __sandbox_dir})
try_restart_sandbox(__mysql_sandbox_port3)

#@ Connect to instance on port 1.
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@ Create cluster with success.
if __have_ssl:
    cluster = dba.create_cluster('bug26818744', {'memberSslMode': 'REQUIRED', 'clearReadOnly': True})
else:
    cluster = dba.create_cluster('bug26818744', {'memberSslMode': 'DISABLED', 'clearReadOnly': True})

#@ Add instance on port 2 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port2)
wait_slave_state(cluster, uri2, "ONLINE")

#@ Add instance on port 3 to cluster with success.
add_instance_to_cluster(cluster, __mysql_sandbox_port3)
wait_slave_state(cluster, uri3, "ONLINE")

#@ Remove instance on port 2 from cluster with success.
cluster.remove_instance('root:root@localhost:' + str(__mysql_sandbox_port2))

#@ Remove instance on port 3 from cluster with success.
cluster.remove_instance('root:root@localhost:' + str(__mysql_sandbox_port3))

#@ Dissolve cluster with success
cluster.dissolve({'force': True})
