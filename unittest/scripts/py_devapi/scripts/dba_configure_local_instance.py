# Assumptions: smart deployment rountines available
#@ Initialization
import time
deployed_here = reset_or_deploy_sandboxes()

shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

#@ create cluster
if __have_ssl:
  cluster = dba.create_cluster('devCluster', {'memberSslMode':'REQUIRED'})
else:
  cluster = dba.create_cluster('devCluster')

#@ Validates the create_cluster successfully configured the grLocal member of the instance addresses
gr_local_port = __mysql_sandbox_port1 + 10000
res = session.run_sql('select json_unquote(addresses->"$.grLocal") from mysql_innodb_cluster_metadata.instances where addresses->"$.mysqlClassic" = "localhost:' + str(__mysql_sandbox_port1) + '"')
row = res.fetch_one()
print (row[0])


#@ Adding the remaining instances
add_instance_to_cluster(cluster, __mysql_sandbox_port2, 'second_sandbox')
add_instance_to_cluster(cluster, __mysql_sandbox_port3, 'third_sandbox')
wait_slave_state(cluster, 'second_sandbox', "ONLINE")
wait_slave_state(cluster, 'third_sandbox', "ONLINE")

#@<OUT> Healthy cluster status
cluster.status()

#@ Kill instance, will not auto join after start
dba.kill_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir':__sandbox_dir})
wait_slave_state(cluster, 'third_sandbox', ["UNREACHABLE", "OFFLINE"])

#@ Start instance, will not auto join the cluster
dba.start_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir': __sandbox_dir})
wait_slave_state(cluster, 'third_sandbox', ["OFFLINE", "(MISSING)"])

#@<OUT> Still missing 3rd instance
time.sleep(5)
cluster.status()

#@#: Rejoins the instance
cluster.rejoin_instance({'dbUser': "root", 'host': "localhost", 'port':__mysql_sandbox_port3}, {'memberSslMode': "AUTO", "password": "root"})
wait_slave_state(cluster, 'third_sandbox', "ONLINE")

#@<OUT> Instance is back
cluster.status()

#@<OUT> Persist the GR configuration
cnfPath = __sandbox_dir + str(__mysql_sandbox_port3) + "/my.cnf"
result = dba.configure_local_instance('root@localhost:' + str(__mysql_sandbox_port3), {'mycnfPath': cnfPath, 'dbPassword':'root'})
print (result.status)

#@ Kill instance, will auto join after start
dba.kill_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir':__sandbox_dir})
wait_slave_state(cluster, 'third_sandbox', ["UNREACHABLE", "OFFLINE"])

#@ Start instance, will auto join the cluster
dba.start_sandbox_instance(__mysql_sandbox_port3, {'sandboxDir': __sandbox_dir})
wait_slave_state(cluster, 'third_sandbox', "ONLINE")

#@ Finalization
if deployed_here:
  cleanup_sandbox(__mysql_sandbox_port1)
