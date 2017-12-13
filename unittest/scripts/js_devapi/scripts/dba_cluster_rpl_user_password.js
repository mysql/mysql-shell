// Assumptions: smart deployment routines available

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");


shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Install validate_password plugin and configure it for the medium policy
var installed = false;
try {
  session.runSql('INSTALL PLUGIN validate_password SONAME \'' + __plugin + '\'');
  installed = true;
} catch (err) {
  // This means the plugin is already installed
}

session.runSql('SET GLOBAL validate_password_policy=\'MEDIUM\'');

//@ Create cluster
if (__have_ssl)
  var cluster = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});
else
  var cluster = dba.createCluster('devCluster', {memberSslMode: 'DISABLED'});

// configure the validate_password plugin for the strong policy
session.runSql('SET GLOBAL validate_password_policy=\'STRONG\'');

//@ Add instance 2 to cluster
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

// configure the validate_password plugin validate_password_length to maximum allowed
session.runSql('SET GLOBAL validate_password_length=32');

//@ Add instance 3 to cluster
add_instance_to_cluster(cluster, __mysql_sandbox_port3);

if (installed)
	session.runSql('UNINSTALL PLUGIN validate_password');

session.close();
cluster.disconnect();

//@ Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
