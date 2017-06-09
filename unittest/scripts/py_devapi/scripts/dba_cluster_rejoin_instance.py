# Assumptions: smart deployment rountines available
#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

# Create a new administrative account on instance 1
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.run_sql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'')
session.run_sql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION')
session.run_sql('SET sql_log_bin=1')

# Create a new administrative account on instance 2
shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.run_sql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'')
session.run_sql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION')
session.run_sql('SET sql_log_bin=1')

# Create a new administrative account on instance 3
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER IF EXISTS \'foo\'@\'%\'');
session.run_sql('CREATE USER \'foo\'@\'%\' IDENTIFIED BY \'bar\'')
session.run_sql('GRANT ALL PRIVILEGES ON *.* TO \'foo\'@\'%\' WITH GRANT OPTION')
session.run_sql('SET sql_log_bin=1')

#@ Connect to instance 1
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'foo', 'password': 'bar'})

#@ create cluster
if __have_ssl:
  cluster = dba.create_cluster('dev', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('dev')

#@ Adding instance 2 using the root account
cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port':__mysql_sandbox_port2}, {'password': 'root'})

# Waiting for the instance 2 to become online
wait_slave_state(cluster, uri2, "ONLINE");

#@ Adding instance 3
cluster.add_instance({'dbUser': 'foo', 'host': 'localhost', 'port':__mysql_sandbox_port3}, {'password': 'bar'});

# Waiting for the instance 3 to become online
wait_slave_state(cluster, uri3, "ONLINE")

# kill instance 2
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port2, {'sandboxDir':__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port2)

# Waiting for instance 2 to become missing
wait_slave_state(cluster, uri2, "(MISSING)")

# Start instance 2
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port2, {'sandboxDir':__sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port2)

#@<OUT> Cluster status
cluster.status()

#@ Rejoin instance 2
if __have_ssl:
  cluster.rejoin_instance({'dbUser': 'foo', 'host': 'localhost', 'port':__mysql_sandbox_port2}, {'memberSslMode': 'AUTO', 'password': 'bar'})
else:
  cluster.rejoin_instance({'dbUser': 'foo', 'host': 'localhost', 'port':__mysql_sandbox_port2}, {'password': 'bar'})

# Waiting for instance 2 to become back online
wait_slave_state(cluster, uri2, "ONLINE")

#@<OUT> Cluster status after rejoin
cluster.status()

# Dissolve the cluser and remove the created accounts

#@ Dissolve cluster
cluster.dissolve({'force': True})

# Delete the account on instance 1
session.close()
shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER \'foo\'@\'%\'');
session.run_sql('SET sql_log_bin=1')

# Delete the account on instance 2
shell.connect({'host': localhost, 'port': __mysql_sandbox_port2, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER \'foo\'@\'%\'');
session.run_sql('SET sql_log_bin=1')

# Delete the account on instance 3
shell.connect({'host': localhost, 'port': __mysql_sandbox_port3, 'user': 'root', 'password': 'root'})
session.run_sql('SET sql_log_bin=0')
session.run_sql('DROP USER \'foo\'@\'%\'');
session.run_sql('SET sql_log_bin=1')

#@ Finalization
# Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here):
  cleanup_sandboxes(True)
