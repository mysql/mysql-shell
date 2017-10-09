// Assumptions: smart deployment routines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

//@ Dba.createCluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev');

// Dba.dissolve
cluster.dissolve({force:true});

// Regression for BUG#26248116 : MYSQLPROVISION DOES NOT USE SECURE CONNECTIONS BY DEFAULT
// Test can only be performed if SSL is supported.
//@ Create cluster requiring secure connections (if supported)
if (__have_ssl) {
  var result = session.runSql("SELECT @@global.require_secure_transport");
  var row = result.fetchOne();
  var req_sec_trans = row[0];
  session.runSql("SET @@global.require_secure_transport = ON");
  var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED', clearReadOnly: true});
} else {
  var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED', clearReadOnly: true});
}

//@ Add instance requiring secure connections (if supported)
add_instance_to_cluster(cluster, __mysql_sandbox_port2);

//@ Dissolve cluster requiring secure connections (if supported)
cluster.dissolve({force:true});
if (__have_ssl) {
  session.runSql("SET @@global.require_secure_transport = '" + req_sec_trans + "'");
}

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
