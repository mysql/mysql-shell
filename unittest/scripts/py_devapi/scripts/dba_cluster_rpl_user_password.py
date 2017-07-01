# Assumptions: smart deployment routines available

#@ Initialization
deployed_here = reset_or_deploy_sandboxes()

shell.connect({'host': localhost, 'port': __mysql_sandbox_port1, 'user': 'root', 'password': 'root'})

# Install validate_password plugin and configure it for the medium policy
installed = False
try:
  session.run_sql('INSTALL PLUGIN validate_password SONAME \'' + __plugin + '\'');
  installed = True
except:
  pass #This means the plugin is already installed

session.run_sql('SET GLOBAL validate_password_policy=\'MEDIUM\'');

#@ Create cluster
if __have_ssl:
  cluster = dba.create_cluster('pmCluster', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('pmCluster')

# configure the validate_password plugin for the strong policy
session.run_sql('SET GLOBAL validate_password_policy=\'STRONG\'');

#@ Add instance 2 to cluster
add_instance_to_cluster(cluster, __mysql_sandbox_port2)

session.run_sql('SET GLOBAL validate_password_length=33');

#@ Add instance 3 to cluster
add_instance_to_cluster(cluster, __mysql_sandbox_port3)

if installed:
	session.run_sql('UNINSTALL PLUGIN validate_password');

session.close();

#@ Finalization
if deployed_here:
    cleanup_sandboxes(deployed_here)
